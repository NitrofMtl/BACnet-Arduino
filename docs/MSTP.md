# BACnet MS/TP

BACnet-Arduino supports BACnet MS/TP communication over an RS-485 interface.

MS/TP (Master-Slave/Token-Passing) uses the BACnet MS/TP datalink and does not require an Ethernet or UDP connection.

The BACnet MS/TP port is designed to be **RS-485/transceiver agnostic**. It does not directly control a UART, RS-485 transceiver, or driver-enable pin. Instead, the application provides callbacks for reading, transmitting, and determining the current transmission state.

This allows BACnet-Arduino to be used with different Arduino RS-485 implementations and hardware.

For functionality common to BACnet/IP and BACnet MS/TP, see the main [README](../README.md).

---
# Enabling datalink

Before using BACnet MS/TP, uncomment the MS/TP datalink definition in 'ports/arduino/arduino_configs.h' to enable the datalink.

```cpp
//MSTP configuration
//#define BACDL_MSTP  //<--- uncomment
```


# MS/TP Transport Callbacks

The MS/TP port uses three callbacks to communicate with the underlying serial/RS-485 implementation.

## Read Callback

The read callback provides received bytes to the MS/TP stack.

```cpp
static bool rs485_read(uint8_t *b)
{
    if (RS485.available()) {
        if (b == nullptr) {
            return true;
        }

        uint8_t byte = RS485.read();
        *b = byte;
        return true;
    }

    if (b != nullptr) {
        *b = 0;
    }

    return false;
}
```

Register it with:

```cpp
mstp_callback_read_set(rs485_read);
```

When `b` is `nullptr`, the callback should only report whether a byte is available.

When `b` is not `nullptr`, the callback should read the next available byte and store it in `*b`.

The callback should return:

* `true` when a byte is available/read successfully
* `false` when no byte is available

---

## Send Callback

The send callback is responsible for transmitting a complete MS/TP frame through the underlying RS-485 interface.

For example, using `ArduinoRS485`:

```cpp
static void rs485_send(const uint8_t *p, uint16_t len)
{
    RS485.noReceive();
    RS485.beginTransmission();

    RS485.write(p, len);

    mstp_silence_reset();

    RS485.endTransmission();
    RS485.receive();
}
```

Register it with:

```cpp
mstp_callback_send_set(rs485_send);
```

The callback is responsible for the physical transmission operation.

BACnet-Arduino does not assume how the underlying UART or RS-485 transceiver is controlled.

---

## Transmitting-State Callback

The MS/TP port also needs to know whether the underlying RS-485 interface is currently transmitting.

Register a callback with:

```cpp
mstp_callback_transmitting_set(rs485_transmitting);
```

For an `ArduinoRS485` implementation where the DE pin reflects the current transmission state:

```cpp
bool rs485_transmitting()
{
    return digitalRead(DE_PIN) == HIGH;
}
```

For example:

```cpp
#define DE_PIN RS485_DEFAULT_DE_PIN
```

The `DE_PIN` value is used here to **determine the transmission state**.

BACnet-Arduino does not use this pin to control the RS-485 transceiver.

The actual transmit/receive direction is handled by the underlying RS-485 implementation.

For a different RS-485 implementation, the transmitting callback can use whatever mechanism is appropriate for determining whether the transmitter is active.

---

# Using ArduinoRS485

The following is an example of implementing the MS/TP callbacks using the Arduino `ArduinoRS485` library.

```cpp
#include <Arduino.h>
#include "ArduinoRS485.h"

#define DE_PIN RS485_DEFAULT_DE_PIN
```

The ArduinoRS485 library handles the RS-485 transceiver direction.

The application uses the MS/TP callbacks to connect the ArduinoRS485 implementation to BACnet-Arduino.

This separation allows the MS/TP implementation to remain independent of the particular RS-485 library or transceiver hardware.

---

# RS-485 Timing

BACnet MS/TP is a timing-sensitive protocol.

The MS/TP port handles BACnet protocol timing internally, while the underlying RS-485 implementation is responsible for the physical serial interface and its transmit/receive behavior.

The most important hardware-dependent timing parameter for the Arduino RS-485 implementation is the **post-transmit delay**.

## Post-Transmit Delay

The post-transmit delay is the delay applied by the RS-485 interface after a transmission before the interface returns to normal receive operation.

The default post-transmit delay may be too short for some Arduino RS-485 hardware.

A value of approximately **1.5 character times** is a good starting point and has proven reliable across different baud rates and hardware configurations.

With a normal 10-bit UART character:

```text
1 character = 10 bit times
1.5 characters ≈ 15 bit times
```

For example:

```cpp
#define BACNET_MSTP_BAUD 115200

#define RS485_POSTDELAY (15UL * 1000000UL / BACNET_MSTP_BAUD)
```

With `ArduinoRS485`, the delay can be configured with:

```cpp
RS485.setDelays(RS485_PREDELAY, RS485_POSTDELAY);
```

For example:

```cpp
#define RS485_PREDELAY 25
#define RS485_POSTDELAY (15UL * 1000000UL / BACNET_MSTP_BAUD)

RS485.begin(BACNET_MSTP_BAUD);
RS485.setDelays(RS485_PREDELAY, RS485_POSTDELAY);
RS485.receive();
```

The exact delay required can depend on the board, UART, and RS-485 transceiver.

The recommended starting point is approximately **15 bit times (1.5 character times)**.

If communication is unreliable, especially when frames are visible on the RS-485 bus but responses are not consistently recognized by the other device, the post-transmit delay should be one of the first parameters to check.

Do not add arbitrary delays around `bacnet_Poll_mstp()`. Configure the delay through the underlying RS-485 implementation.

---

# Baud Rate

All devices on an MS/TP network must use the same baud rate.

For example:

```cpp
#define BACNET_MSTP_BAUD 115200
```

The baud rate is provided to the MS/TP port:

```cpp
mstp_baud_rate_set(BACNET_MSTP_BAUD);
```

The underlying serial interface must use the same baud rate:

```cpp
RS485.begin(BACNET_MSTP_BAUD);
```

Common MS/TP baud rates include:

```text
9600
19200
38400
76800
115200
```

Changing the baud rate also changes the duration of character and bit times.

---

# MS/TP MAC Address

Every MS/TP device must have a unique MAC address on the MS/TP network.

For example:

```cpp
#define UNIT_MAC 1
```

The MAC address is passed when initializing the MS/TP port:

```cpp
init_bacnet_mstp(UNIT_MAC);
```

The MS/TP MAC address is independent of the BACnet Device Instance.

For example:

```text
BACnet Device Instance: 4321
MS/TP MAC Address:       1
```

These values do not need to be the same.

Each device on the same MS/TP network must use a different MAC address.

---

# MS/TP Masters

BACnet MS/TP uses token passing between master devices.

The MS/TP network therefore has a maximum master address.

For example, a network using MAC addresses 0 through 5 as possible masters can use:

```text
mstp_nmax_master_set(5);
```

The value should be large enough to include all master devices that may participate in token passing.

For small networks, keeping `Nmax_master` close to the highest master MAC address can reduce unnecessary token-passing searches.

The exact master configuration is handled by the BACnet MS/TP port.

---

# MS/TP Initialization

A typical MS/TP application performs the following steps:

1. Initialize the underlying RS-485 interface.
2. Configure its baud rate and physical-layer delays.
3. Configure the MS/TP callbacks.
4. Configure the MS/TP baud rate.
5. Initialize the MS/TP port with the local MAC address.
6. Initialize the BACnet device and application.

For example:

```cpp
void setupMSTP()
{
    bacnet_init(&deviceInfos);
    mstp_nmax_master_set(5);

    bacnet_time_callback_set(bacnet_get_time);
    handler_timesync_set_callback_set(bacnet_sync_time);

    mstp_callback_read_set(rs485_read);
    mstp_callback_send_set(rs485_send);
    mstp_callback_transmitting_set(rs485_transmitting);

    mstp_baud_rate_set(BACNET_MSTP_BAUD);

    init_bacnet_mstp(UNIT_MAC);
}
```

The underlying RS-485 interface is initialized separately:

```cpp
void setup()
{
    Serial.begin(9600);

    RS485.begin(BACNET_MSTP_BAUD);

    RS485.setDelays(
        RS485_PREDELAY,
        RS485_POSTDELAY);

    RS485.receive();

    setupMSTP();
}
```

The exact RS-485 initialization depends on the hardware and transport library being used.

---

# Main Loop

BACnet MS/TP processing must be performed regularly from the Arduino main loop.

The application should maintain the BACnet device timer and call:

```cpp
bacnet_Poll_mstp();
```

For example:

```cpp
void loop()
{
    static uint32_t last_ms = millis();
    uint32_t now = millis();

    Device_Timer(now - last_ms);
    last_ms = now;

    bacnet_Poll_mstp();
}
```

For DCC functionality, maintain the DCC timer once per second:

```cpp
static uint32_t lastTimer = 0;

if (millis() - lastTimer >= 1000) {
    lastTimer += 1000;
    dcc_timer_seconds(1);
}
```

A complete loop can therefore look like:

```cpp
void loop()
{
    static uint32_t last_ms = millis();
    uint32_t now = millis();

    Device_Timer(now - last_ms);
    last_ms = now;

    static uint32_t lastTimer = 0;

    if (millis() - lastTimer >= 1000) {
        lastTimer += 1000;
        dcc_timer_seconds(1);
    }

    bacnet_Poll_mstp();
}
```

Avoid long blocking operations in the main loop.

---

# Complete ArduinoRS485 Example

The following example shows the complete connection between the ArduinoRS485 library and BACnet-Arduino MS/TP.

```cpp
#ifdef MSTP_EXAMPLE

#include <Arduino.h>
#include "ArduinoRS485.h"

// Opta-specific time library
#include <mbed_rtc_time.h>

// BACnet MS/TP
#include "BACnet-Arduino_MSTP.h"

#define DE_PIN RS485_DEFAULT_DE_PIN

#define UNIT_MAC 1
#define BACNET_MSTP_BAUD 115200

#define RS485_PREDELAY 25
#define RS485_POSTDELAY \
    (15UL * 1000000UL / BACNET_MSTP_BAUD)

const deviceInfos_t deviceInfos {
    2,
    "My Opta",
    "Basic BACnet device running on Arduino",
    "Mechanical room",
    "Arduino",
    "Opta Bacnet",
    "1.0"
};

static void rs485_send(const uint8_t *p, uint16_t len)
{
    RS485.noReceive();
    RS485.beginTransmission();

    RS485.write(p, len);

    mstp_silence_reset();

    RS485.endTransmission();
    RS485.receive();
}

static bool rs485_read(uint8_t *b)
{
    if (RS485.available()) {
        if (b == nullptr) {
            return true;
        }

        uint8_t byte = RS485.read();
        *b = byte;

        return true;
    }

    if (b != nullptr) {
        *b = 0;
    }

    return false;
}

bool rs485_transmitting()
{
    return digitalRead(DE_PIN) == HIGH;
}

void bacnet_sync_time(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    bool UTC)
{
    // This example uses local time and therefore ignores the UTC flag.
    // A real application should convert UTC when UTC == true.
    (void)UTC;

    tmElements_t tm_info =
        BACnetSchedule::toTimeElements(*bdate, *btime);

    time_t epoch_time = makeTime(tm_info);

    set_time(epoch_time);
}

bool bacnet_get_time(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    time_t epoch = now();

    tmElements_t tm_info = {};

    breakTime(epoch, tm_info);

    *bdate = BACnetSchedule::toBACnetDate(tm_info);
    *btime = BACnetSchedule::toBACnetTime(tm_info);

    *dst_active = false;

    return true;
}

void setupMSTP()
{
    bacnet_init(&deviceInfos);
    mstp_nmax_master_set(5);

    // BACnet reads the current board time through this callback.
    bacnet_time_callback_set(bacnet_get_time);

    // Allow BACnet Time Synchronization requests
    // to update the board clock.
    handler_timesync_set_callback_set(bacnet_sync_time);

    // Connect the MS/TP stack to the underlying RS-485 implementation.
    mstp_callback_read_set(rs485_read);
    mstp_callback_send_set(rs485_send);
    mstp_callback_transmitting_set(rs485_transmitting);

    // Configure the MS/TP baud rate.
    mstp_baud_rate_set(BACNET_MSTP_BAUD);

    // Initialize the MS/TP port.
    init_bacnet_mstp(UNIT_MAC);
}

void setup()
{
    Serial.begin(9600);

    RS485.begin(BACNET_MSTP_BAUD);

    RS485.setDelays(
        RS485_PREDELAY,
        RS485_POSTDELAY);

    RS485.receive();

    setupMSTP();
}

void loop()
{
    // Device_Timer() expects the elapsed time
    // since its previous call.
    static uint32_t last_ms = millis();

    uint32_t now = millis();

    Device_Timer(now - last_ms);
    last_ms = now;

    // Run periodic one-second tasks without blocking
    // the BACnet loop.
    static uint32_t lastTimer = 0;

    if (millis() - lastTimer >= 1000) {
        lastTimer += 1000;

        // Maintain BACnet DCC timing.
        dcc_timer_seconds(1);
    }

    bacnet_Poll_mstp();
}

#endif
```

The example uses `ArduinoRS485`, but the BACnet MS/TP implementation itself does not depend on it.

A different RS-485/UART implementation can be used by providing equivalent callbacks.

---

# RS-485 Wiring

The physical RS-485 connection depends on the board and transceiver being used.

The network normally consists of:

```text
Arduino / BACnet device
        |
        | RS-485
        |
       A/B
        |
        +---------------- Device
        |
        +---------------- Device
        |
        +---------------- Device
```

The A/B polarity must be consistent across the network.

If the network does not communicate, verify the A and B connections first.

RS-485 networks should also follow the electrical requirements of the transceiver and BACnet MS/TP installation, including appropriate termination and biasing where required.

---

# Interoperability

BACnet MS/TP devices from different manufacturers can communicate on the same RS-485 network when they use compatible:

* baud rate
* MAC address configuration
* MS/TP master configuration
* RS-485 wiring
* BACnet configuration

A BACnet MS/TP analyzer or BACnet client such as YABE can be used to verify communication.

When troubleshooting, first verify that valid MS/TP frames are visible on the RS-485 bus before investigating BACnet object-level behavior.

---

# Troubleshooting

## No frames are visible on the RS-485 bus

Check:

1. RS-485 wiring.
2. A/B polarity.
3. RS-485 baud rate.
4. RS-485 transceiver power.
5. The RS-485 implementation's transmit/receive handling.
6. The MS/TP read and send callbacks.
7. The MS/TP MAC address.
8. That the MS/TP datalink is initialized.
9. That `bacnet_Poll_mstp()` is being called regularly.

---

## Frames are visible but the device does not respond

Check:

1. Destination MAC address.
2. Local MS/TP MAC address.
3. `Nmax_master`.
4. Configured baud rate.
5. MS/TP timing.
6. Transmit-state callback.
7. RS-485 post-transmit delay.
8. That received frames are being passed through the read callback.
9. That the BACnet application is processing received frames.

A protocol analyzer or serial sniffer can help determine whether the device is receiving valid MS/TP frames and participating in token passing.

---

## Communication works at one baud rate but not another

MS/TP timing depends on baud rate.

If communication works at one baud rate but fails at another, check:

* UART baud-rate configuration
* MS/TP baud-rate configuration
* character timing
* RS-485 post-transmit delay
* transmit-state detection
* receive frame timing

The recommended starting point for the RS-485 post-transmit delay is approximately **1.5 character times (15 bit times)**.

---

# BACnet/IP vs BACnet MS/TP

The application-level BACnet functionality is largely the same regardless of the datalink.

The main difference is the transport initialization:

|                      | BACnet/IP        | BACnet MS/TP  |
| -------------------- | ---------------- | ------------- |
| Physical transport   | Ethernet         | RS-485        |
| BACnet datalink      | BACnet/IP        | BACnet MS/TP  |
| Transport            | UDP              | Serial RS-485 |
| Addressing           | IP address / UDP | MS/TP MAC     |
| Token passing        | No               | Yes           |
| Baud rate            | N/A              | Required      |
| `bacnet_set_udp()`   | Required         | Not used      |
| `bacnet_Poll()`      | Used             | Not used      |
| `bacnet_Poll_mstp()` | Not used         | Used          |

The same BACnet device objects, callbacks, Schedule functionality, and application logic can generally be used with either transport.

---

# Example

A complete working MS/TP example is included with the library.

The example demonstrates:

* Arduino RS-485 initialization
* BACnet MS/TP initialization
* MS/TP transport callbacks
* MS/TP MAC configuration
* baud-rate configuration
* RS-485 pre-delay and post-delay configuration
* BACnet device configuration
* BACnet object creation
* time callbacks
* BACnet time synchronization
* periodic BACnet timers
* object Present Value updates
* Schedule configuration
* Schedule persistence where supported

The example uses `ArduinoRS485` on the Arduino Opta, but the MS/TP port can be connected to other RS-485 implementations through the transport callbacks.
