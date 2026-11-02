#!/usr/bin/env python3
"""
Compare BACnet-Arduino's src/bacnet files against an upstream bacnet-stack
checkout. The Arduino tree defines the scope: upstream-only files are ignored.

Examples (PowerShell):
  python .\compare_stack.py `
    --baseline "D:\Dropbox\bacnet-stack" `
    --arduino "D:\Projects\BACnet-Arduino"

  python .\compare_stack.py --baseline "D:\Dropbox\bacnet-stack" `
    --arduino "D:\Projects\BACnet-Arduino" --open

  python .\compare_stack.py --baseline "D:\Dropbox\bacnet-stack" `
    --arduino "D:\Projects\BACnet-Arduino" --file "basic/object/device.c"

  python .\compare_stack.py --baseline "D:\Dropbox\bacnet-stack" `
    --arduino "D:\Projects\BACnet-Arduino" `
    --set-status "basic/object/device.c=keep"
"""

from __future__ import annotations

import argparse
import difflib
import hashlib
import html
import json
import os
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

STATUSES = ("pending", "restore", "keep", "port-required", "investigate")
MANIFEST_VERSION = 1


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git_commit(repo: Path) -> str | None:
    if not (repo / ".git").exists():
        return None
    try:
        result = subprocess.run(
            ["git", "-C", str(repo), "rev-parse", "HEAD"],
            check=True, capture_output=True, text=True,
        )
        return result.stdout.strip() or None
    except (OSError, subprocess.CalledProcessError):
        return None


def read_text_for_diff(path: Path) -> list[str]:
    # Preserve line endings as far as possible; replacement avoids failing on
    # an unusual byte sequence in a source file.
    return path.read_text(encoding="utf-8", errors="replace").splitlines(keepends=True)


def make_diff(upstream: Path, arduino: Path, relative: str) -> str:
    return "".join(difflib.unified_diff(
        read_text_for_diff(upstream),
        read_text_for_diff(arduino),
        fromfile=f"upstream/src/bacnet/{relative}",
        tofile=f"BACnet-Arduino/src/bacnet/{relative}",
    ))


def load_manifest(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {"version": MANIFEST_VERSION, "files": {}}
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
        if not isinstance(data, dict) or not isinstance(data.get("files"), dict):
            raise ValueError("Unexpected manifest structure")
        return data
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        raise SystemExit(
            f"Cannot read existing review manifest:\n  {path}\n{exc}\n"
            "Move it aside if you intentionally want to start a new review."
        )


def save_json(path: Path, data: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(data, indent=2), encoding="utf-8")
    temporary.replace(path)


def parse_status(value: str) -> tuple[str, str]:
    if "=" not in value:
        raise argparse.ArgumentTypeError(
            "Use PATH=STATUS, for example basic/object/device.c=keep"
        )
    relative, status = value.rsplit("=", 1)
    relative = relative.replace("\\", "/").strip("/")
    if status not in STATUSES:
        raise argparse.ArgumentTypeError(
            f"Unknown status {status!r}. Choose: {', '.join(STATUSES)}"
        )
    if not relative or relative.startswith("../") or "/../" in relative:
        raise argparse.ArgumentTypeError("Path must be relative to src/bacnet")
    return relative, status


def open_vscode_diff(upstream: Path, arduino: Path) -> None:
    import shutil

    code_command = shutil.which("code")
    if not code_command:
        raise SystemExit("Could not find VS Code in PATH.")

    try:
        subprocess.Popen(
            ["cmd", "/c", code_command, "--diff",
             str(upstream), str(arduino)]
        )
    except OSError as exc:
        raise SystemExit(f"Could not launch VS Code: {exc}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Review BACnet-Arduino src/bacnet files against upstream."
    )
    parser.add_argument("--baseline", required=True,
                        help="Full upstream repository folder (contains src/bacnet)")
    parser.add_argument("--arduino", required=True,
                        help="BACnet-Arduino repository root")
    parser.add_argument("--report-dir", default=None,
                        help="Output folder (default: <arduino>/upstream-review)")
    parser.add_argument("--open", action="store_true",
                        help="Open every modified file in VS Code diff tabs")
    parser.add_argument("--file", default=None,
                        help="Open one modified file; path relative to src/bacnet")
    parser.add_argument("--set-status", type=parse_status, default=None,
                        metavar="PATH=STATUS",
                        help=f"Set review status: {', '.join(STATUSES)}")
    args = parser.parse_args()

    baseline_root = Path(args.baseline).expanduser().resolve()
    arduino_root = Path(args.arduino).expanduser().resolve()
    upstream_dir = baseline_root / "src" / "bacnet"
    arduino_dir = arduino_root / "src" / "bacnet"
    report_dir = (Path(args.report_dir).expanduser().resolve()
                  if args.report_dir else arduino_root / "upstream-review")

    if not upstream_dir.is_dir():
        parser.error(f"Upstream folder not found: {upstream_dir}")
    if not arduino_dir.is_dir():
        parser.error(f"Arduino folder not found: {arduino_dir}")

    manifest_path = report_dir / "comparison.json"
    manifest = load_manifest(manifest_path)
    manifest.setdefault("files", {})

    if args.set_status:
        relative, status = args.set_status
        entry = manifest["files"].get(relative)
        if not entry:
            raise SystemExit(
                f"No comparison entry for {relative}. Run a comparison first."
            )
        entry["status"] = status
        entry["reviewed_at"] = datetime.now(timezone.utc).isoformat()
        save_json(manifest_path, manifest)
        print(f"Set {relative}: {status}")
        return 0

    # The Arduino tree is authoritative for the file list. Files with no
    # upstream counterpart are intentionally ignored.
    arduino_files = sorted(p for p in arduino_dir.rglob("*") if p.is_file())
    current_entries: dict[str, dict[str, Any]] = {}
    modified: list[str] = []
    counts = {"modified": 0, "unchanged": 0, "ignored_no_upstream": 0}

    diff_root = report_dir / "diffs"
    for arduino_file in arduino_files:
        relative = arduino_file.relative_to(arduino_dir).as_posix()
        upstream_file = upstream_dir / Path(relative)

        if not upstream_file.is_file():
            counts["ignored_no_upstream"] += 1
            continue

        upstream_hash = sha256_file(upstream_file)
        arduino_hash = sha256_file(arduino_file)
        is_modified = upstream_hash != arduino_hash
        previous = manifest["files"].get(relative, {})
        status = previous.get("status", "pending")

        entry = {
            "state": "modified" if is_modified else "unchanged",
            "status": status if status in STATUSES else "pending",
            "upstream_sha256": upstream_hash,
            "arduino_sha256": arduino_hash,
        }
        if previous.get("reviewed_at"):
            entry["reviewed_at"] = previous["reviewed_at"]
        current_entries[relative] = entry

        if is_modified:
            counts["modified"] += 1
            modified.append(relative)
            diff_text = make_diff(upstream_file, arduino_file, relative)
            diff_path = diff_root / (relative + ".diff")
            diff_path.parent.mkdir(parents=True, exist_ok=True)
            diff_path.write_text(diff_text, encoding="utf-8", newline="")
        else:
            counts["unchanged"] += 1

    # Keep historical review decisions for files not currently found, but
    # mark them as absent from this scan rather than silently deleting them.
    for relative, old_entry in manifest["files"].items():
        if relative not in current_entries:
            old_entry["state"] = "not-in-current-match-set"

    manifest.update({
        "version": MANIFEST_VERSION,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "baseline_repository": str(baseline_root),
        "baseline_git_commit": git_commit(baseline_root),
        "arduino_repository": str(arduino_root),
        "scope": "Arduino src/bacnet files with matching upstream paths only",
        "files": {**manifest["files"], **current_entries},
        "counts": counts,
    })
    save_json(manifest_path, manifest)

    # Human-readable HTML index.
    rows = []
    for relative in modified:
        entry = current_entries[relative]
        rows.append(
            "<tr>"
            f"<td><code>{html.escape(relative)}</code></td>"
            f"<td>{html.escape(entry['status'])}</td>"
            f"<td><a href=\"diffs/{html.escape(relative)}.diff\">Unified diff</a></td>"
            "</tr>"
        )
    html_doc = f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<title>BACnet upstream comparison</title>
<style>
body {{ font: 14px system-ui, sans-serif; margin: 2rem; }}
table {{ border-collapse: collapse; width: 100%; }}
th, td {{ text-align: left; border-bottom: 1px solid #ccc; padding: .55rem; }}
code {{ overflow-wrap: anywhere; }}
</style></head><body>
<h1>BACnet upstream comparison</h1>
<p>Generated: {html.escape(manifest['generated_at'])}</p>
<ul>
<li>Modified: {counts['modified']}</li>
<li>Unchanged: {counts['unchanged']}</li>
<li>Arduino files without upstream counterpart (ignored): {counts['ignored_no_upstream']}</li>
</ul>
<p>Review statuses are stored in <code>comparison.json</code>.
Use the script's <code>--set-status</code> option to update them.</p>
<table><thead><tr><th>File</th><th>Review status</th><th>Diff</th></tr></thead>
<tbody>{''.join(rows)}</tbody></table>
</body></html>"""
    report_dir.mkdir(parents=True, exist_ok=True)
    (report_dir / "index.html").write_text(html_doc, encoding="utf-8")

    print(f"Compared {counts['modified'] + counts['unchanged']} matching files.")
    print(f"Modified: {counts['modified']}")
    print(f"Unchanged: {counts['unchanged']}")
    print(f"Ignored (no upstream counterpart): {counts['ignored_no_upstream']}")
    print(f"Report: {report_dir / 'index.html'}")
    print(f"Manifest: {manifest_path}")

    if args.file:
        relative = args.file.replace("\\", "/").strip("/")
        if relative not in modified:
            raise SystemExit(f"{relative} is not a modified matching file.")
        open_vscode_diff(upstream_dir / Path(relative), arduino_dir / Path(relative))
    elif args.open:
        for relative in modified:
            open_vscode_diff(upstream_dir / Path(relative), arduino_dir / Path(relative))

    return 0


if __name__ == "__main__":
    sys.exit(main())
