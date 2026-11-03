#!/usr/bin/env python3
"""Compare and selectively align BACnet-Arduino with upstream bacnet-stack.

Commands:
  compare: Generate comparison report and review statuses.
  merge:   Apply reviewed decisions and copy upstream versions of
           remaining modified matching files.

Only files already present in both src/bacnet trees are considered.
"""

from __future__ import annotations

import argparse
import difflib
import hashlib
import html
import json
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
from typing import Any

STATUSES = ("pending", "restore", "keep", "port-required", "investigate")
MANIFEST_VERSION = 2


def now() -> str:
    return datetime.now(timezone.utc).isoformat()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git_output(repo: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", "-C", str(repo), *args],
        check=True, capture_output=True, text=True
    )
    return result.stdout.strip()


def git_commit(repo: Path) -> str:
    try:
        return git_output(repo, "rev-parse", "HEAD")
    except (OSError, subprocess.CalledProcessError):
        raise SystemExit(f"Cannot read Git HEAD in Arduino repository: {repo}")


def require_clean_git(repo: Path) -> str:
    commit = git_commit(repo)

    try:
        status = git_output(
            repo, "status", "--porcelain", "--untracked-files=all"
        )
    except (OSError, subprocess.CalledProcessError):
        raise SystemExit(
            "Could not check Git working-tree status; refusing to update."
        )

    if status:
        print("Git working tree is not clean. Commit or resolve these files:")
        print(status)
        raise SystemExit("Update cancelled; no source files were changed.")

    return commit


def safe_relative(value: str) -> str:
    value = value.replace("\\", "/").strip("/")
    p = PurePosixPath(value)

    if not value or p.is_absolute() or ".." in p.parts:
        raise argparse.ArgumentTypeError(
            "Path must be relative to src/bacnet"
        )

    return p.as_posix()


def parse_status(value: str) -> tuple[str, str]:
    if "=" not in value:
        raise argparse.ArgumentTypeError(
            "Use PATH=STATUS, e.g. basic/object/device.c=keep"
        )

    path, status = value.rsplit("=", 1)
    path = safe_relative(path)

    if status not in STATUSES:
        raise argparse.ArgumentTypeError(
            f"Choose a status from: {', '.join(STATUSES)}"
        )

    return path, status


def read_text(path: Path) -> list[str]:
    return path.read_text(
        encoding="utf-8", errors="replace"
    ).splitlines(keepends=True)


def make_diff(upstream: Path, arduino: Path, relative: str) -> str:
    return "".join(difflib.unified_diff(
        read_text(upstream),
        read_text(arduino),
        fromfile=f"upstream/src/bacnet/{relative}",
        tofile=f"BACnet-Arduino/src/bacnet/{relative}"
    ))


def load_manifest(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {"version": MANIFEST_VERSION, "files": {}}

    try:
        data = json.loads(path.read_text(encoding="utf-8"))

        if not isinstance(data, dict) or not isinstance(
            data.get("files"), dict
        ):
            raise ValueError("Unexpected manifest structure")

        return data

    except (OSError, json.JSONDecodeError, ValueError) as exc:
        raise SystemExit(f"Cannot read manifest {path}: {exc}")


def save_json(path: Path, data: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_suffix(path.suffix + ".tmp")
    temp.write_text(json.dumps(data, indent=2), encoding="utf-8")
    temp.replace(path)


def open_vscode_diff(upstream: Path, arduino: Path) -> None:
    import os

    code = shutil.which("code")

    if not code:
        raise SystemExit("Could not find VS Code 'code' command in PATH.")

    command = (
        ["cmd", "/c", code, "--diff", str(upstream), str(arduino)]
        if os.name == "nt"
        else [code, "--diff", str(upstream), str(arduino)]
    )

    subprocess.Popen(command)


def roots(args: argparse.Namespace):
    baseline = Path(args.baseline).expanduser().resolve()
    arduino = Path(args.arduino).expanduser().resolve()

    upstream_dir = baseline / "src" / "bacnet"
    arduino_dir = arduino / "src" / "bacnet"

    report = (
        Path(args.report_dir).expanduser().resolve()
        if args.report_dir
        else arduino / "upstream-review"
    )

    if not upstream_dir.is_dir():
        raise SystemExit(f"Upstream folder not found: {upstream_dir}")

    if not arduino_dir.is_dir():
        raise SystemExit(f"Arduino folder not found: {arduino_dir}")

    return baseline, arduino, upstream_dir, arduino_dir, report


def write_html(
    report: Path,
    manifest: dict[str, Any],
    modified: list[str],
    counts: dict[str, int]
) -> None:
    rows = []

    for rel in modified:
        entry = manifest["files"][rel]

        rows.append(
            f'<tr><td><code>{html.escape(rel)}</code></td>'
            f'<td>{html.escape(entry["status"])}</td>'
            f'<td><a href="diffs/{html.escape(rel)}.diff">Diff</a></td></tr>'
        )

    doc = f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>BACnet upstream comparison</title>
<style>
body {{font:14px system-ui,sans-serif;margin:2rem}}
table {{border-collapse:collapse;width:100%}}
th,td {{text-align:left;border-bottom:1px solid #ccc;padding:.55rem}}
code {{overflow-wrap:anywhere}}
</style>
</head>
<body>
<h1>BACnet upstream comparison</h1>
<p>Generated: {html.escape(manifest['generated_at'])}</p>
<ul>
<li>Modified: {counts['modified']}</li>
<li>Unchanged: {counts['unchanged']}</li>
<li>Arduino-only: {counts['ignored_no_upstream']}</li>
</ul>
<p>Review statuses are stored in <code>comparison.json</code>.</p>
<table>
<thead><tr><th>File</th><th>Status</th><th>Diff</th></tr></thead>
<tbody>{''.join(rows)}</tbody>
</table>
</body>
</html>"""

    report.mkdir(parents=True, exist_ok=True)
    (report / "index.html").write_text(doc, encoding="utf-8")


def do_compare(args: argparse.Namespace) -> int:
    baseline, arduino, upstream_dir, arduino_dir, report = roots(args)

    manifest_path = report / "comparison.json"
    manifest = load_manifest(manifest_path)
    previous = manifest["files"]

    entries = {}
    modified = []

    counts = {
        "modified": 0,
        "unchanged": 0,
        "ignored_no_upstream": 0
    }

    for local in sorted(p for p in arduino_dir.rglob("*") if p.is_file()):
        rel = local.relative_to(arduino_dir).as_posix()
        upstream = upstream_dir / rel

        if not upstream.is_file():
            counts["ignored_no_upstream"] += 1
            continue

        upstream_hash = sha256_file(upstream)
        arduino_hash = sha256_file(local)
        changed = upstream_hash != arduino_hash

        old = previous.get(rel, {})
        status = old.get("status", "pending")

        entry = {
            "state": "modified" if changed else "unchanged",
            "status": status if status in STATUSES else "pending",
            "upstream_sha256": upstream_hash,
            "arduino_sha256": arduino_hash
        }

        if old.get("reviewed_at"):
            entry["reviewed_at"] = old["reviewed_at"]

        entries[rel] = entry

        if changed:
            counts["modified"] += 1
            modified.append(rel)

            diff_path = report / "diffs" / (rel + ".diff")
            diff_path.parent.mkdir(parents=True, exist_ok=True)

            diff_path.write_text(
                make_diff(upstream, local, rel),
                encoding="utf-8",
                newline=""
            )
        else:
            counts["unchanged"] += 1

    merged = dict(previous)

    for rel, old in merged.items():
        if rel not in entries:
            old["state"] = "not-in-current-match-set"

    merged.update(entries)

    commit = git_commit(arduino)
    try:
        upstream_commit = git_commit(baseline)
    except SystemExit:
        upstream_commit = "unknown (no Git metadata)"

    manifest.update({
        "version": MANIFEST_VERSION,
        "generated_at": now(),
        "baseline_repository": str(baseline),
        "baseline_git_commit": upstream_commit,
        "arduino_repository": str(arduino),
        "arduino_git_commit": commit,
        "scope": "Existing Arduino src/bacnet files with matching upstream paths only",
        "counts": counts,
        "files": merged
    })

    save_json(manifest_path, manifest)
    write_html(report, manifest, modified, counts)

    print(
        f"Compared {counts['modified'] + counts['unchanged']} matching files; "
        f"{counts['modified']} modified, "
        f"{counts['unchanged']} unchanged."
    )
    print(f"Ignored (no upstream counterpart): {counts['ignored_no_upstream']}")
    print(f"Report: {report / 'index.html'}")
    print(f"Manifest: {manifest_path}")

    if args.set_status:
        rel, status = args.set_status

        if (
            rel not in manifest["files"]
            or manifest["files"][rel].get("state") != "modified"
        ):
            raise SystemExit(
                f"{rel} is not a modified matching file in this comparison."
            )

        manifest["files"][rel]["status"] = status
        manifest["files"][rel]["reviewed_at"] = now()

        save_json(manifest_path, manifest)
        print(f"Set {rel}: {status}")

    if args.file:
        rel = safe_relative(args.file)

        if rel not in modified:
            raise SystemExit(f"{rel} is not modified in this comparison.")

        open_vscode_diff(upstream_dir / rel, arduino_dir / rel)

    elif args.open_tracked:
        for rel in modified:
            if entries[rel]["status"] != "pending":
                open_vscode_diff(upstream_dir / rel, arduino_dir / rel)

    elif args.open:
        for rel in modified:
            open_vscode_diff(upstream_dir / rel, arduino_dir / rel)

    return 0


def do_merge(args: argparse.Namespace) -> int:
    baseline, arduino, upstream_dir, arduino_dir, report = roots(args)

    manifest_path = report / "comparison.json"

    if not manifest_path.is_file():
        raise SystemExit(
            "No comparison manifest found. Run compare first."
        )

    manifest = load_manifest(manifest_path)

    if Path(manifest.get("baseline_repository", "")).resolve() != baseline:
        raise SystemExit(
            "Manifest uses a different upstream baseline. Compare again."
        )

    if Path(manifest.get("arduino_repository", "")).resolve() != arduino:
        raise SystemExit("Manifest belongs to a different Arduino repository.")

    commit = require_clean_git(arduino)

    if commit != manifest.get("arduino_git_commit"):
        raise SystemExit(
            "Arduino Git HEAD differs from comparison commit. "
            "Commit and run compare again."
        )

    files = manifest["files"]
    protected = set(args.protect or [])
    actions = []

    # Every protected file must be modified and explicitly reviewed.
    for rel in protected:
        entry = files.get(rel)

        if not entry or entry.get("state") != "modified":
            raise SystemExit(
                f"Protected file {rel} is not modified in this comparison."
            )

        if entry.get("status", "pending") == "pending":
            raise SystemExit(
                f"Protected file {rel} has not been reviewed. "
                "Set its status first."
            )

    for rel, entry in files.items():
        if entry.get("state") != "modified":
            continue

        status = entry.get("status", "pending")

        if status == "investigate":
            raise SystemExit(
                f"{rel} is marked investigate; resolve it before merging."
            )

        if status in ("keep", "port-required"):
            actions.append((rel, "keep-local"))
        else:
            actions.append((rel, "copy-upstream"))

    # Validate all files before modifying the first one.
    for rel, action in actions:
        entry = files[rel]
        src = upstream_dir / rel
        dst = arduino_dir / rel

        if not src.is_file() or not dst.is_file():
            raise SystemExit(f"File disappeared since comparison: {rel}")

        if sha256_file(src) != entry.get("upstream_sha256"):
            raise SystemExit(
                f"Upstream file changed since comparison: {rel}. "
                "Run compare again."
            )

        if sha256_file(dst) != entry.get("arduino_sha256"):
            raise SystemExit(
                f"Arduino file changed since comparison: {rel}. "
                "Commit and run compare again."
            )

    copied = []
    kept = []

    for rel, action in actions:
        if action == "copy-upstream":
            shutil.copy2(upstream_dir / rel, arduino_dir / rel)
            copied.append(rel)
        else:
            kept.append(rel)

    manifest["last_update"] = {
        "timestamp": now(),
        "starting_git_commit": commit,
        "upstream_repository": str(baseline),
        "copied_from_upstream": copied,
        "kept_local": kept,
        "note": (
            "Only matching files already present in both trees were "
            "considered. Upstream-only files were not added."
        )
    }

    save_json(manifest_path, manifest)

    print(
        f"Merge complete. Copied {len(copied)} files from upstream; "
        f"kept {len(kept)} local files."
    )
    print("Upstream-only files were not added.")
    print("Run compare again to verify the resulting differences.")

    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Compare and align BACnet-Arduino with upstream."
    )

    sub = parser.add_subparsers(dest="command", required=True)

    for name in ("compare", "merge"):
        p = sub.add_parser(name)

        p.add_argument("--baseline", required=True)
        p.add_argument("--arduino", required=True)
        p.add_argument("--report-dir", default=None)

        if name == "compare":
            p.add_argument("--open", action="store_true")
            p.add_argument("--file")
            p.add_argument("--set-status", type=parse_status)
            p.add_argument(
                "--open-tracked",
                action="store_true",
                help="Open only files with a previously reviewed status."
)

        else:
            p.add_argument(
                "--protect",
                action="append",
                type=safe_relative,
                default=[],
                help="Require explicit review of this file; repeat as needed."
            )

    return parser


def main() -> int:
    args = build_parser().parse_args()

    if args.command == "compare":
        return do_compare(args)

    return do_merge(args)


if __name__ == "__main__":
    sys.exit(main())