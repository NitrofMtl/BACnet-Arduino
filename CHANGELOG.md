
# Changelog

## [0.7.0]

### Add
- Added multi-port routing support for BACnet/IP and MS/TP, enabling a single router instance to connect multiple BACnet networks.
- Added compile time configs validation in each examples.

### Changed
- Refactored BACnet/IP peripheral integration to use template-based network and UDP interfaces, removing the dependency on a specific Arduino UDP implementation.
- Update examples to use the new API.
`MS/TP`
- Removed the unused `over_init` variable from `bacnet_mstp_init()`.
- Restored the baud-rate-dependent calculation of `turnaround_ms` instead of the hardcoded value.

- Remove fold expresssion from `constexpr_max`


## [0.6.0]

### Add
- Added **experimental** STM32RS485DMA support and an example.


### Changed
- restore schedule restoring to default 1 day scope
- Make time set/get handling internal




### Fixed
- Fix BACnet router configuration for single and multiple datalink builds.
- Fix router buffer sizing and datalink selection when optional datalinks are disabled.

---

## [0.5.0]

### Add
- Datalink BACnetRouter with local device support


### Changed
- Remove mbed_mktime.h inclusion from bacnet.h
- Add macro for Schedule size to arduino-configs
- Add to readme <#define> parameter instructions

### Fixed
- remove brace initializaqtion of ScheduleCallback for better compatibility
- Guard devices infos setter
- bip receive port indexing
- BacnetMstp apdu handler arguments ordering

---

## [0.4.0]

### Changed
- Add MS/TP support
- Memory optimization optimizable via ports config file


---

## [0.3.0]

### Changed
- Add new header BACnet-Arduino.h

---

## [0.2.0]

### Changed
- Exclude upstream Schedule implementation for Arduino compatibility

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
