# BACnet-Arduino

Arduino BACnet library based on the open-source bacnet-stack project.

BACnet-Arduino provides an Arduino-oriented interface to the BACnet stack, including BACnet object creation and access, time integration, device reinitialization, and BACnet Schedule support. The Arduino port can use the BACnet datalinks supported by the underlying stack; currently, examples are provided for BACnet/IP and BACnet MS/TP.

The library keeps the BACnet protocol stack largely unchanged while providing an Arduino-specific port layer. It also provides BACnet/IP ↔ MS/TP routing through the BacnetRouterClass, including support for multiple ports and networks.

> **Status:** 0.7.0

---

## Features

* BACnet/IP support
* BACnet MS/TP support
* Multi-port BACnet routing (BACnet/IP and MS/TP)
* Generic BIP peripheral integration (template-based network and UDP interfaces)
* Arduino RS-485 / MS/TP transport integration
* BACnet device configuration
* BACnet object creation and Present Value access
* BACnet time synchronization
* Device reinitialization callbacks
* DCC support
* BACnet Schedule support
* Schedule persistence through user-provided callbacks
* Arduino-friendly C++ Schedule API
* Based on the open-source `bacnet-stack` project

---

## Resource Usage

BACnet-Arduino is based on the full `bacnet-stack` protocol implementation, but unused BACnet features can be disabled to reduce the final application size.

The actual RAM and flash usage depends on the selected BACnet features, objects, configuration, Arduino core, and application.

As a reference, the current BACnet-Arduino BIP example on an Arduino Opta uses approximately:

```text
RAM:   19.5% (102144 bytes / 523624 bytes)
Flash: 36.8% (289304 bytes / 786432 bytes)
```

The memory footprint can vary significantly depending on the application and the BACnet features and objects enabled.

BACnet-Arduino is primarily intended for **32-bit Arduino boards with sufficient RAM and flash** rather than small 8-bit Arduino boards.

Applications should still verify memory usage on their target board before deployment.

---

## Basic Architecture

BACnet-Arduino provides independent interfaces for BACnet datalinks and routing. Applications can use a datalink directly, combine multiple datalinks through BacnetRouterClass, or use the router with an optional local BACnet Device.

```text
                Arduino Sketch
                        |
        +---------------+----------------+
        |               |                |
        v               v                v
    BIP API          MS/TP API       Router API
        |               |                |
        v               v                v
    BACnet/IP       BACnet MS/TP    BIP <-> MS/TP <-> ...
        |               |                |
        +---------------+----------------+
                        |
                        v
                bacnet-stack
```
The router currently supports BACnet/IP and BACnet MS/TP; additional datalinks supported by the underlying BACnet stack can be added to the router.

The Arduino port provides the connection between the BACnet stack and the board-specific Arduino APIs.

Board-specific functionality such as:

* networking
* RS-485 hardware
* system time
* hardware reset
* persistent storage

is provided through callbacks or port functions.

This keeps the BACnet core independent from the hardware.

---

# Getting Started

BACnet-Arduino supports two BACnet datalinks:

* **BACnet/IP** over UDP
* **BACnet MS/TP** over RS-485

The configuration and initialization are different for each datalink.

For BACnet/IP, see [BACnet/IP Setup](docs/BIP.md).

For BACnet MS/TP, see [BACnet MS/TP Setup](docs/MSTP.md).

For BACnet Router, see [BACnet Router Setup](docs/ROUTER.md)

The sections below describe functionality common to both BACnet/IP, BACnet MS/TP and Router applications.

---

# BACnet Device Information

The BACnet device is configured through `deviceInfos_t`:

```cpp
const deviceInfos_t deviceInfos {
    4321,
    "My Opta",
    "Basic BACnet device running on Arduino",
    "Mechanical room",
    "Arduino",
    "Opta Bacnet",
    "1.0"
};
```

The fields are:

| Field                          | Description                  |
| ------------------------------ | ---------------------------- |
| `instance_number`              | BACnet Device instance       |
| `name`                         | Device name                  |
| `description`                  | Device description           |
| `location`                     | Physical/device location     |
| `vendor_name`                  | Vendor name                  |
| `model_name`                   | Device model                 |
| `firmware_revision`            | Firmware revision            |
| `application_software_version` | Application software version |

Pass this structure to:

```cpp
bacnet_init(&deviceInfos);
```

---

# Callbacks

Callbacks are an important part of BACnet-Arduino.

They allow the library to use board-specific functionality without making the BACnet stack depend on a particular Arduino board.

There are several callbacks, but **not all of them are required**.

## Callback Summary

| Callback                               | Required?     | Purpose                                   |
| -------------------------------------- | ------------- | ----------------------------------------- |
| `bacnet_time_callback_set()`           | **Yes**       | Allows BACnet to read the board clock     |
| `handler_timesync_set_callback_set()`  | Optional      | Allows BACnet to modify the board clock   |
| `bacnet_reinitialize_callback_set()`   | Optional      | Allows BACnet to request a board reset    |
| `BACnetSchedule::setRestoreCallback()` | Schedule only | Restore schedules from persistent storage |
| `BACnetSchedule::setSaveCallback()`    | Schedule only | Save schedules to persistent storage      |

---

# Time API

BACnet-Arduino uses a **TimeLib-compatible API** as its internal time interface.

The `WeeklyAlarm` library provides a compatibility layer between this interface and the POSIX-style time functions used by the BACnet stack.

On non-POSIX platforms, **TimeLib is included as a library dependency** to provide the TimeLib-compatible functions. On POSIX platforms, the corresponding system time functions are used instead.

The current time is obtained through:

```cpp
now()
```

Only the operation used to **set** the clock is application-dependent. Register a callback:

```cpp
setTimeCallback_set(bacnet_set_time);
```

with the following signature:

```cpp
void bacnet_set_time(time_t epoch);
```

The callback should update the clock used by `now()`.

For example:

```cpp
void bacnet_set_time(time_t epoch)
{
    // Update the board's clock.
}
```

The time-reading and POSIX compatibility functions are provided by the library.


---

# Device Reinitialization

BACnet can request a device reinitialization.

The actual reset mechanism is hardware-specific, so BACnet-Arduino exposes a callback.

For example:

```cpp
bacnet_reinitialize_callback_set(
    BACNET_REINIT_COLDSTART,
    reinitColdStartCb);
```

Example for an STM32 board:

```cpp
void reinitColdStartCb()
{
    NVIC_SystemReset();
}
```

The callback should use the reset mechanism appropriate for the target board.

The BACnet library does not assume how the board should reset itself.

---

# Main Loop

BACnet processing must be performed regularly:

```cpp
bacnet_Poll();
```

The application should also maintain the BACnet device timer:

```cpp
static uint32_t last_ms = 0;

uint32_t now = millis();

Device_Timer(now - last_ms);

last_ms = now;
```

The elapsed time is passed to `Device_Timer()`.

For DCC functionality, the DCC timer must also be maintained:

```cpp
dcc_timer_seconds(1);
```

This should be called once per second.

A typical loop therefore looks like:

```cpp
void loop()
{
    static uint32_t last_ms = 0;
    uint32_t now = millis();

    Device_Timer(now - last_ms);
    last_ms = now;

    static uint32_t lastTimer = 0;

    if (millis() - lastTimer >= 1000) {
        lastTimer += 1000;
        dcc_timer_seconds(1);
    }

    bacnet_Poll();
}
```

The application can perform its normal I/O processing in the same loop.

---

# BACnet Objects

BACnet-Arduino provides helper functions for creating and accessing common BACnet objects.

The general pattern is:

```text
create object
     |
     v
read Present_Value
     |
     v
modify Present_Value
     |
     v
application hardware
```

Objects should normally be created during initialization.

---

## Analog Value

Create an Analog Value:

```cpp
createAnalogValue(
    0,
    "Temperature",
    "Temperature in Celsius",
    UNITS_DEGREES_CELSIUS,
    42.0f);
```

Read its Present Value:

```cpp
float value = Analog_Value_Present_Value(0);
```

Change its Present Value:

```cpp
Analog_Value_Present_Value_Set(
    0,
    25.5f,
    BACNET_PRIORITY_NORMAL);
```

The priority argument allows the application to write the value through the BACnet priority mechanism.

---

## Analog Input

Create an Analog Input:

```cpp
createAnalogInput(
    0,
    "AI_0",
    "Analog Input 0",
    UNITS_MILLIAMPERES);
```

Update its Present Value:

```cpp
Analog_Input_Present_Value_Set(0, analogRead(A0));
```

An Analog Input is normally updated by the application from a physical input or sensor.

---

## Analog Output

Create an Analog Output:

```cpp
createAnalogOutput(
    0,
    "AO_0",
    "Analog Output 0",
    UNITS_PERCENT,
    50.0f);
```

The Present Value can then be accessed through the corresponding object API.

Outputs are normally used as the BACnet-controlled side of the application.

---

## Binary Value

Create a Binary Value:

```cpp
createBinaryValue(
    0,
    "BV_0",
    "Binary Value 0",
    BINARY_INACTIVE);
```

Binary Values are application-controlled BACnet values and are useful for internal states, commands, or configuration.

---

## Binary Input

Create a Binary Input:

```cpp
createBinaryInput(
    0,
    "BI_0",
    "Binary Input 0",
    BINARY_ACTIVE);
```

Update it from hardware:

```cpp
Binary_Input_Present_Value_Set(
    0,
    digitalRead(BTN_USER) == LOW ?
        BINARY_ACTIVE :
        BINARY_INACTIVE);
```

---

## Binary Output

Create a Binary Output:

```cpp
createBinaryOutput(
    0,
    "BO_0",
    "Binary Output 0",
    BINARY_ACTIVE);
```

A Binary Output is normally used when the BACnet device controls a physical output.

The application should connect the BACnet Present Value to the corresponding hardware action.

---

# Multi-State Objects

Multi-state objects use a list of state strings.

For example:

```cpp
static const char fanModeList[] = {
    "OFF\0" "ON\0" "AUTO\0"
};
```

Create a Multi-State Output:

```cpp
createMultiStateOutput(
    0,
    "FAN_MODE",
    "Fan Mode",
    fanModeList,
    1);
```

The state numbers are one-based:

```text
1 = OFF
2 = ON
3 = AUTO
```

Set the Present Value:

```cpp
Multistate_Output_Present_Value_Set(
    0,
    2,
    BACNET_PRIORITY_NORMAL);
```

This is useful when the BACnet value represents an application state such as:

```text
OFF
ON
AUTO
```

or:

```text
STOPPED
RUNNING
ALARM
```

---

# Object API Pattern

For normal application development, the most useful functions are generally the object creation functions and the Present Value access functions.

The port API follows the object type:

```text
Analog Value
    createAnalogValue()
    Analog_Value_Present_Value()
    Analog_Value_Present_Value_Set()

Analog Input
    createAnalogInput()
    Analog_Input_Present_Value_Set()

Analog Output
    createAnalogOutput()
    ...

Binary Value
    createBinaryValue()
    ...

Binary Input
    createBinaryInput()
    ...

Binary Output
    createBinaryOutput()
    ...

Multi-State Input
    createMultiStateInput()
    ...

Multi-State Output
    createMultiStateOutput()
    Multistate_Output_Present_Value_Set()

Multi-State Value
    createMultiStateValue()
    ...
```

The complete low-level bacnet-stack object API remains available through the stack headers when more advanced functionality is required.

For normal Arduino applications, prefer the Arduino port helpers where available.

---

# BACnet Schedule

BACnet-Arduino provides a C++ `BACnetSchedule` wrapper around the BACnet Schedule object.

Create a schedule:

```cpp
BACnetSchedule mainSchedule(0);
```

Configure its effective period:

```cpp
mainSchedule.setEffectivePeriod(
    2024, 1, 1,
    2099, 12, 31);
```

Set the default value:

```cpp
mainSchedule.setDefaultEnum(1);
```

Configure the object controlled by the schedule:

```cpp
mainSchedule.set_Object_Property_Reference(
    OBJECT_MULTI_STATE_OUTPUT,
    0);
```

Add weekly events:

```cpp
mainSchedule.setSchedule(
    dowFriday,
    0,
    21,
    37,
    2);
```

The schedule automatically recalculates its Present Value and applies changes to the referenced BACnet object.

Priority is handled by the Schedule object:

```cpp
mainSchedule.setWritePriority(10);
```

and can be read with:

```cpp
uint8_t priority = mainSchedule.getWritePriority();
```

---

# Schedule Persistence

Schedule persistence is optional, but if schedules must survive a reboot, two callbacks should be registered.

**These callbacks must be registered before `BACnetSchedule::begin()`.**

```cpp
BACnetSchedule::setRestoreCallback(restoreSchedule);
BACnetSchedule::setSaveCallback(saveSchedule);

BACnetSchedule::begin();
```



## Restore Callback

The restore callback is used to provide previously saved schedule data to the Schedule API.

The restore callback receives:

* the Schedule instance
* a pointer to a `SchedulePersistentData` structure to populate

```cpp
bool restoreSchedule(
    uint32_t inst,
    BACnetSchedule::SchedulePersistentData *data);
```

Return:

```cpp
true
```

when persistent data was available and successfully copied into `data`.

Return:

```cpp
false
```

when no valid saved schedule exists.

The storage implementation is intentionally board/application-specific.

For example, the Opta example uses `KVStore`.

---

## Save Callback

The save callback receives:

* the Schedule instance
* the complete schedule data

```cpp
void saveSchedule(
    uint32_t inst,
    const BACnetSchedule::SchedulePersistentData &data);
```

The application decides how and where the data is stored.

This allows different boards to use different persistence mechanisms without putting storage dependencies inside `BACnetSchedule`.

---

# Schedule Initialization

The correct initialization sequence is:

```cpp
bacnet_init(&deviceInfos);

BACnetSchedule::setRestoreCallback(restoreSchedule);
BACnetSchedule::setSaveCallback(saveSchedule);

BACnetSchedule::begin();
```

`begin()` initializes the schedule instances and restores persistent data when a restore callback is available.

If the schedule does not need persistence, the callbacks can be omitted.

---

# Upstream BACnet Stack

BACnet-Arduino is based on the open-source `bacnet-stack` project.

The original BACnet stack remains the protocol implementation, while this project provides the Arduino-specific integration and C++ helpers.

Some upstream files are intentionally excluded or replaced in the Arduino port. In particular, the original Schedule implementation is replaced by the Arduino-specific Schedule implementation.

The project attempts to keep changes to the upstream core minimal so that future upstream updates remain manageable.

---

# License

BACnet-Arduino is released under the MIT License.

The project includes code derived from `bacnet-stack`; the original upstream license and copyright notices are retained where applicable.

---

# Examples

Complete working Arduino examples are provided with the library.

The examples demonstrate functionality including:

* BACnet/IP
* BACnet MS/TP
* BACnet router
* device configuration
* BACnet object creation
* time callbacks
* BACnet time synchronization
* device reinitialization
* periodic BACnet timers
* object Present Value updates
* Schedule configuration
* Schedule persistence using the Opta `KVStore`

For BACnet/IP configuration, see [docs/BIP.md](docs/BIP.md).

For BACnet MS/TP configuration, see [docs/MSTP.md](docs/MSTP.md).
