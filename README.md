# BACnet-Arduino

Arduino BACnet library based on the open-source [bacnet-stack](https://github.com/bacnet-stack/bacnet-stack) project.

BACnet-Arduino provides an Arduino-oriented interface to the BACnet stack, including BACnet/IP networking, BACnet object creation and access, time integration, device reinitialization, and BACnet Schedule support.

The library keeps the BACnet protocol stack largely unchanged while providing an Arduino-specific port layer.

> **Status:** 0.3.0

---

## Features

* BACnet/IP support
* Arduino UDP transport integration
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

## ⚠️Resource Usage

BACnet-Arduino is based on the full `bacnet-stack` protocol implementation and therefore has a **significant memory footprint compared with typical Arduino libraries**.

The actual RAM and flash usage depends on the selected BACnet features, objects, configuration, Arduino core, and application.

As a reference, the initial BACnet-Arduino configuration uses a substantial portion of the available RAM on an Arduino Opta. Applications should therefore verify memory usage on their target board before deployment.

This library is primarily intended for **32-bit Arduino boards with sufficient RAM and flash**, rather than small 8-bit Arduino boards.

> **Note:** The memory footprint is not a fixed library requirement. Disabling unused BACnet features and reducing object/configuration usage can significantly reduce the final application size.

---

## Basic Architecture

BACnet-Arduino is divided into three main parts:

```text
Application / Arduino Sketch
          |
          v
    Arduino port API
          |
          v
     bacnet-stack
          |
          v
     UDP / network
```

The Arduino port provides the connection between the BACnet stack and the board-specific Arduino APIs.

Board-specific functionality such as:

* networking
* system time
* hardware reset
* persistent storage

is provided through callbacks or port functions.

This keeps the BACnet core independent from the hardware.

---

# Getting Started

A BACnet/IP application normally needs the following initialization:

```cpp
#include "BACnet-Arduino.h"

EthernetUDP Udp;

void setup()
{
    Ethernet.begin(deviceIp);

    Udp.begin(47808);
    bacnet_set_udp(&Udp, 47808);

    bacnet_init(&deviceInfos);

    bacnet_time_callback_set(bacnet_get_time);
}
```

The BACnet processing function must then be called continuously:

```cpp
void loop()
{
    bacnet_Poll();
}
```

Do not use long blocking delays in the main loop. BACnet processing and timers depend on `loop()` being called regularly.

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

# BACnet/IP Setup

The Arduino port uses an `EthernetUDP` object for BACnet/IP.

```cpp
EthernetUDP Udp;

Ethernet.begin(deviceIp);

Udp.begin(47808);
bacnet_set_udp(&Udp, 47808);
```

`bacnet_set_udp()` gives the BACnet port access to the UDP transport.

The default BACnet/IP port is:

```cpp
47808
```

which corresponds to hexadecimal `0xBAC0`.

The UDP object must remain valid for as long as BACnet/IP is running.

---

# Callbacks

Callbacks are an important part of BACnet-Arduino.

They allow the library to use board-specific functionality without making the BACnet stack depend on a particular Arduino board.

There are several callbacks, but **not all of them are required**.

## Callback summary

| Callback                               | Required?     | Purpose                                   |
| -------------------------------------- | ------------- | ----------------------------------------- |
| `bacnet_time_callback_set()`           | **Yes**       | Allows BACnet to read the board clock     |
| `handler_timesync_set_callback_set()`  | Optional      | Allows BACnet to modify the board clock   |
| `bacnet_reinitialize_callback_set()`   | Optional      | Allows BACnet to request a board reset    |
| `BACnetSchedule::setRestoreCallback()` | Schedule only | Restore schedules from persistent storage |
| `BACnetSchedule::setSaveCallback()`    | Schedule only | Save schedules to persistent storage      |

---

# Time Callback

## `bacnet_time_callback_set()`

BACnet needs access to the board's current time.

Register a callback:

```cpp
bacnet_time_callback_set(bacnet_get_time);
```

The callback has this signature:

```cpp
bool bacnet_get_time(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active);
```

The callback must:

1. Read the board's current authoritative clock.
2. Convert it to BACnet date/time.
3. Set the UTC offset.
4. Set the daylight-saving state.
5. Return `true` when the time was successfully provided.

Example:

```cpp
bool bacnet_get_time(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    time_t epoch = time(nullptr);

    tmElements_t tm_info = {};
    breakTime(epoch, tm_info);

    *bdate = BACnetSchedule::toBACnetDate(tm_info);
    *btime = BACnetSchedule::toBACnetTime(tm_info);

    *utc_offset_minutes = -4;
    *dst_active = false;

    return true;
}
```

### Important requirement

The callback should **not obtain time from NTP or another network service every time it is called**.

Use an authoritative local time source such as:

* RTC
* system clock
* board time maintained by the application

NTP can be used to initially synchronize that clock.

---

# BACnet Time Synchronization

## `handler_timesync_set_callback_set()`

If the application should allow another BACnet device to synchronize its clock, register a write callback:

```cpp
handler_timesync_set_callback_set(bacnet_sync_time);
```

The callback is:

```cpp
void bacnet_sync_time(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    bool UTC);
```

The callback must update the **same clock used by `bacnet_get_time()`**.

For example:

```cpp
void bacnet_sync_time(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    bool UTC)
{
    // Handle UTC conversion if required by the application.
    (void)UTC;

    tmElements_t tm_info =
        BACnetSchedule::toTimeElements(*bdate, *btime);

    time_t epoch_time = makeTime(tm_info);

    set_time(epoch_time);
}
```

### Important requirement

The read and write paths must use the same clock:

```text
BACnet
  |
  +---- bacnet_get_time() ----> board clock
  |
  +---- bacnet_sync_time() ---> board clock
```

Otherwise BACnet may read a different time than the one that was synchronized.

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
createAnalogValue(0, "Temperature", "Temperature in Celsius", UNITS_DEGREES_CELSIUS, 42.0f);
```

Read its Present Value:

```cpp
float value = Analog_Value_Present_Value(0);
```

Change its Present Value:

```cpp
Analog_Value_Present_Value_Set(0, 25.5f, BACNET_PRIORITY_NORMAL);
```

The priority argument allows the application to write the value through the BACnet priority mechanism.

---

## Analog Input

Create an Analog Input:

```cpp
createAnalogInput(0, "AI_0", "Analog Input 0", UNITS_MILLIAMPERES);
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
createAnalogOutput(0, "AO_0", "Analog Output 0", UNITS_PERCENT, 50.0f);
```

The Present Value can then be accessed through the corresponding object API.

Outputs are normally used as the BACnet-controlled side of the application.

---

## Binary Value

Create a Binary Value:

```cpp
createBinaryValue(0, "BV_0", "Binary Value 0", BINARY_INACTIVE);
```

Binary Values are application-controlled BACnet values and are useful for internal states, commands, or configuration.

---

## Binary Input

Create a Binary Input:

```cpp
createBinaryInput(0, "BI_0", "Binary Input 0", BINARY_ACTIVE);
```

Update it from hardware:

```cpp
Binary_Input_Present_Value_Set(0,
    digitalRead(BTN_USER) == LOW ? BINARY_ACTIVE : BINARY_INACTIVE
);
```

---

## Binary Output

Create a Binary Output:

```cpp
createBinaryOutput(0, "BO_0", "Binary Output 0", BINARY_ACTIVE);
```

A Binary Output is normally used when the BACnet device controls a physical output.

The application should connect the BACnet Present Value to the corresponding hardware action.

---

# Multi-State Objects

Multi-state objects use a list of state strings.

For example:

```cpp
static const char fanModeList[] = {"OFF\0" "ON\0" "AUTO\0"};
```

Create a Multi-State Output:

```cpp
createMultiStateOutput(0, "FAN_MODE", "Fan Mode", fanModeList, 1);
```

The state numbers are one-based:

```text
1 = OFF
2 = ON
3 = AUTO
```

Set the Present Value:

```cpp
Multistate_Output_Present_Value_Set(0, 2, BACNET_PRIORITY_NORMAL);
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
    2099, 12, 31
);
```

Set the default value:

```cpp
mainSchedule.setDefaultEnum(1);
```

Configure the object controlled by the schedule:

```cpp
mainSchedule.set_Object_Property_Reference(OBJECT_MULTI_STATE_OUTPUT, 0);
```

Add weekly events:

```cpp
mainSchedule.setSchedule(dowFriday, 0, 21, 37, 2);
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

This order is important.

## Restore callback

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

When persistent data was available and successfully copied into data.

Return:

```cpp
false
```

when no valid saved schedule exists.

The storage implementation is intentionally board/application-specific.

For example, the Opta example uses `KVStore`.

---

## Save callback

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

# Complete Initialization Example

A typical BACnet/IP application therefore looks like:

```cpp
void setup()
{
    Serial.begin(9600);

    setupNetwork();

    bacnet_init(&deviceInfos);

    // Required: BACnet must be able to read the board clock.
    bacnet_time_callback_set(bacnet_get_time);

    // Optional: allow BACnet Time Synchronization.
    handler_timesync_set_callback_set(bacnet_sync_time);

    // Optional: allow BACnet device reinitialization.
    bacnet_reinitialize_callback_set(
        BACNET_REINIT_COLDSTART,
        reinitColdStartCb);

    // Create application objects.
    setupBacnetObjects();

    // Optional: Schedule persistence.
    BACnetSchedule::setRestoreCallback(restoreSchedule);
    BACnetSchedule::setSaveCallback(saveSchedule);

    BACnetSchedule::begin();
}
```

And:

```cpp
void loop()
{
    // Maintain BACnet timers.
    ...

    // Process BACnet traffic.
    bacnet_Poll();

    // Application I/O.
    ...
}
```

---

# What the Application Must Provide

At minimum, a BACnet/IP Arduino application needs to provide:

### 1. UDP transport

```cpp
bacnet_set_udp(&Udp, 47808);
```

### 2. Device information

```cpp
bacnet_init(&deviceInfos);
```

### 3. A valid board clock callback

```cpp
bacnet_time_callback_set(bacnet_get_time);
```

### 4. Regular BACnet processing

```cpp
bacnet_Poll();
```

### 5. Regular BACnet timer processing

```cpp
Device_Timer(elapsed);
```

The following are feature-dependent:

| Feature                     | Required application code                      |
| --------------------------- | ---------------------------------------------- |
| BACnet Time Synchronization | `handler_timesync_set_callback_set()`          |
| Device Reinitialize         | `bacnet_reinitialize_callback_set()`           |
| DCC                         | `dcc_timer_seconds()`                          |
| Schedule persistence        | `setRestoreCallback()` + `setSaveCallback()`   |
| BACnet objects              | Appropriate `create*()` and Present Value APIs |
| Physical I/O                | Application-specific hardware code             |

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

# Example

A complete working Arduino example is provided with the library and demonstrates:

* Ethernet initialization
* BACnet/IP setup
* device configuration
* BACnet object creation
* time callbacks
* BACnet time synchronization
* device reinitialization
* periodic BACnet timers
* object Present Value updates
* Schedule configuration
* Schedule persistence using the Opta `KVStore`
