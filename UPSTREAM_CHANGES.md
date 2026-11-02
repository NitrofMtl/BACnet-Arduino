# Upstream Changes

**BACnet-Arduino version:** 0.7.0  
**Upstream baseline:** bacnet-stack snapshot from April 2026  
**Comparison date:** November 2026

This document records intentional differences between the upstream bacnet-stack and the BACnet-Arduino port.

The comparison is based on the upstream source available from April 2026. The exact upstream commit or version is unknown.

## Modified files

### 1. `device.c`

**Change:** Added a compile-time option to disable the default `My_Object_Table` (`object_functions_t`).

**Reason:** Allows the Arduino port to provide a custom object table from the `ports` folder instead of using the default implementation.

**Classification:** Port adaptation.

### 2. `debug.c`

**Changes:**
- Added `bacnet_debug.h` to provide Arduino-compatible debug output using `print()`.
- Added `bacnet_debug` and `debug_printf` macros for Arduino debug compatibility.

**Reason:** Adapt the stack's debug output to the Arduino environment and its available print interface.

**Classification:** Port adaptation.

### 3. `config.h`

**Changes:**
- Added inclusion of `ports/arduino/arduino_configs.h`.
- Reduced `MAX_APDU` from 1476 to 512 bytes.
- Enabled `PRINT_ENABLED`.
- Disabled `BACAPP_WEEKLY_SCHEDULE`.
- Disabled `BACAPP_BDT_ENTRY`.

**Reasons:**
- `arduino_configs.h` provides configuration specific to the Arduino port.
- The reduced `MAX_APDU` limits memory usage on resource-constrained Arduino boards.
- `PRINT_ENABLED` enables the debug/print functionality used by the port.
- The built-in weekly schedule support is disabled because the Arduino port provides its own schedule handling through `WeeklyAlarm`.
- The built-in BDT entry support is disabled; the reason for this configuration is not yet documented.

**Classification:** Port adaptation / memory optimization.

### 4. `hostnport.h`

**Change:** Reworked the minimal `BACnetHostNPort` representation using separate C data types.

The original implementation was replaced with:
- `BACNET_HOST_OCTET_STRING`
- `BACNET_HOST_CHARACTER_STRING`
- `BACNET_HOST_ADDRESS_MINIMAL`
- `BACNET_HOST_N_PORT_MINIMAL`

The resulting structure retains the tag, host address/name union, and port while using explicitly sized C data types.

**Reason:** Provide a minimal representation intended to reduce RAM footprint.

**Origin:** Unknown. This implementation may have been adapted from an AVR port example, but this has not been confirmed.

**Classification:** Port adaptation / memory optimization.

---

## Notes

- Only intentional differences from the April 2026 upstream baseline should remain in this list.
- The generated comparison report contains the actual source diffs.
- Changes whose original purpose or source is uncertain are explicitly identified rather than assumed to be Arduino-specific requirements.
