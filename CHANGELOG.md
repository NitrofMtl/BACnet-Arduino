
# Changelog

## [0.1.0] - Initial release

First public release of BACnet-Arduino.

### Added

- Arduino port of the bacnet-stack BACnet implementation.
- Arduino-compatible BACnet/IP integration.
- BACnet Schedule C++ wrapper with:
  - weekly scheduling support
  - automatic Present_Value application
  - BACnet priority handling
  - persistence callbacks

### Changes from upstream bacnet-stack

- Added Arduino-specific port integration.
- Replaced the default object table configuration with an Arduino port object table.
- Disabled unused BACnet application data structures:
  - `BACAPP_WEEKLY_SCHEDULE`
  - `BACAPP_BDT_ENTRY`
- Removed the original `basic/object/schedule.c` from compilation.
- Added an Arduino-specific Schedule implementation in:
  - `ports/arduino`

### Notes

This release keeps the original bacnet-stack core structure while adding Arduino-specific integration layers and C++ helpers.
