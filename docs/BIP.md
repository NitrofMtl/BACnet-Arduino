# BACnet/IP

BACnet-Arduino provides BACnet/IP communication through the Arduino `EthernetUDP` interface.

This document describes the BACnet/IP-specific configuration and initialization.

For functionality common to BACnet/IP and BACnet MS/TP, see the main [README](../README.md).

---

# BACnet/IP Enable datalink

BACnet/IP is enabled by default when no other BACnet datalink is enabled. If multiple datalinks are used, uncomment the BACnet/IP datalink definition in ports/arduino/arduino_configs.h to enable it explicitly.

```cpp
//Ethernet configuration
//#define BACDL_BIP //<--- Uncomment
```

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

```text
47808
```

which corresponds to hexadecimal:

```text
0xBAC0
```

The UDP object must remain valid for as long as BACnet/IP is running.

---

# Initialization

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

Do not use long blocking delays in the main loop.

BACnet processing and timers depend on `loop()` being called regularly.

---

# Network Configuration

The Arduino Ethernet interface must be initialized before the BACnet UDP transport is configured.

For example:

```cpp
Ethernet.begin(deviceIp);
Udp.begin(47808);
bacnet_set_udp(&Udp, 47808);
```

The exact Ethernet initialization depends on the Arduino Ethernet library and hardware being used.

The BACnet port does not configure the physical Ethernet interface itself.

---

# BACnet/IP Port

BACnet/IP uses UDP port `47808` by default.

```cpp
#define BACNET_IP_PORT 47808

Udp.begin(BACNET_IP_PORT);
bacnet_set_udp(&Udp, BACNET_IP_PORT);
```

BACnet/IP devices on the same IP network can communicate using the standard BACnet UDP port.

If a different port is required by the application, the same port number must be used consistently by the UDP transport and BACnet port configuration.

---

# Complete BACnet/IP Example

A typical BACnet/IP application looks like:

```cpp
#include "BACnet-Arduino.h"

EthernetUDP Udp;

void setup()
{
    Serial.begin(9600);

    // Initialize the Ethernet interface.
    setupNetwork();

    // Configure BACnet/IP.
    Udp.begin(47808);
    bacnet_set_udp(&Udp, 47808);

    // Initialize the BACnet device.
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

void loop()
{
    // Maintain BACnet timers.
    static uint32_t last_ms = 0;

    uint32_t now = millis();

    Device_Timer(now - last_ms);
    last_ms = now;

    static uint32_t lastTimer = 0;

    if (millis() - lastTimer >= 1000) {
        lastTimer += 1000;
        dcc_timer_seconds(1);
    }

    // Process BACnet/IP traffic.
    bacnet_Poll();

    // Application I/O.
    ...
}
```

The exact network initialization in `setupNetwork()` depends on the Arduino Ethernet hardware being used.

---

# BACnet/IP Application Requirements

A BACnet/IP application needs to provide:

### 1. Ethernet/network transport

The Arduino Ethernet interface must be initialized before BACnet/IP starts.

### 2. UDP transport

```cpp
bacnet_set_udp(&Udp, 47808);
```

### 3. Device information

```cpp
bacnet_init(&deviceInfos);
```

### 4. A valid board clock callback

```cpp
bacnet_time_callback_set(bacnet_get_time);
```

### 5. Regular BACnet processing

```cpp
bacnet_Poll();
```

### 6. Regular BACnet timer processing

```cpp
Device_Timer(elapsed);
```

Feature-dependent functionality such as Time Synchronization, DCC, device reinitialization, and Schedule persistence is described in the main README.

---

# Troubleshooting

## BACnet device does not respond

Check:

1. The Ethernet interface is initialized.
2. The UDP socket has been started with port `47808`.
3. `bacnet_set_udp()` is called with the same UDP object.
4. `bacnet_init()` is called.
5. `bacnet_Poll()` is called regularly.
6. `Device_Timer()` is maintained.
7. The device has a valid BACnet Device instance.
8. The board has a valid IP configuration.
9. The PC and Arduino are on a network path that permits BACnet/IP UDP traffic.

A packet capture tool such as Wireshark can be useful for determining whether BACnet/IP packets are reaching the Arduino.

---

# BACnet MS/TP

BACnet-Arduino also supports BACnet MS/TP over RS-485.

MS/TP does not use the UDP transport described in this document.

For RS-485 hardware configuration, baud rate, MAC address, MS/TP timing, and MS/TP examples, see [BACnet MS/TP documentation](MSTP.md).
