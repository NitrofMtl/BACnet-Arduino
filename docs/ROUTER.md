# BACnet Router

`BacnetRouterClass` provides BACnet network-layer routing between the configured BACnet datalinks.

The router is independent of the physical transport. The same router can be configured with multiple supported datalinks, for example:

* BACnet/IP
* BACnet MS/TP

Additional datalinks supported by the underlying BACnet stack can be added when supported by the Arduino port.

The router can also optionally host a local BACnet Device.

---

## Configuration

The required datalinks must be enabled in:

```text
ports/arduino/arduino_configs.h
```

For BACnet/IP:

```cpp
// Ethernet configuration
//#define BACDL_BIP  // <-- Uncomment
```

BACnet/IP is enabled by default when no other BACnet datalink is enabled. When using BACnet/IP together with another datalink, `BACDL_BIP` must be explicitly enabled.

For BACnet MS/TP:

```cpp
// RS-485 / MS/TP configuration
//#define BACDL_MSTP  // <-- Uncomment
```

To enable the router:

```cpp
//#define BACNET_ROUTER  // <-- Uncomment
```

If the router also provides a local BACnet Device, enable:

```cpp
//#define ROUTER_DEVICE  // <-- Uncomment
```

`ROUTER_DEVICE` is optional. A router can operate only as a network router without hosting a local BACnet Device.

---

## Router Networks

Each physical datalink used by the router is assigned a BACnet network number.

For example:

```cpp
constexpr uint16_t ROUTER_NETWORK_BIP = 100;
constexpr uint16_t ROUTER_NETWORK_MSTP = 200;
```

The network numbers must be unique within the BACnet internetwork.

The router is then configured with the required networks:

```cpp
BacnetRouterClass router;

router.begin(
    RouterNetwork{BacnetDatalink::BIP, ROUTER_NETWORK_BIP},
    RouterNetwork{BacnetDatalink::MSTP, ROUTER_NETWORK_MSTP}
);
```

The router uses the configured network numbers to determine which datalink must be used when forwarding a routed NPDU.

---

## Datalink Initialization

The router does not replace the normal datalink initialization.

Each datalink must be configured and initialized as described in its corresponding documentation.

For example, BACnet MS/TP requires the normal MS/TP callbacks:

```cpp
mstp_callback_read_set(rs485_read);
mstp_callback_send_set(rs485_send);
mstp_callback_transmitting_set(rs485_transmitting);

mstp_baud_rate_set(BACNET_MSTP_BAUD);
init_bacnet_mstp(UNIT_MAC);
mstp_nmax_master_set(2);
```

BACnet/IP must likewise be initialized and provided with its UDP transport:

```cpp
Ethernet.begin(deviceIp);
Udp.begin(bacnetUdpPort);
bacnet_set_udp(&Udp, bacnetUdpPort);
```

The router uses these configured datalinks to receive and transmit BACnet packets.

---

## Polling

When using the router, the individual BIP and MS/TP `poll()` functions must not be called.

Instead, call:

```cpp
router.poll();
```

from the Arduino `loop()`.

The router polls the configured datalinks and routes received BACnet packets as required.

For example:

```cpp
void loop()
{
    Device_Timer(...);

    // Other periodic BACnet tasks
    dcc_timer_seconds(...);

    router.poll();
}
```

When `ROUTER_DEVICE` is enabled, `router.poll()` also performs the periodic processing required by the local BACnet Device.


The application therefore does not need to call these functions separately.

---

## Local Router Device

A router can optionally contain a local BACnet Device.

First initialize the device normally:

```cpp
bacnet_init(&deviceInfos);
```

Then attach the device to one of the router's configured networks:

```cpp
if (!router.attachRouterDevice(BacnetDatalink::MSTP)) {
    Serial.println("Failed to attach MSTP as router device");
}
```

The selected datalink determines the network on which the local Router Device belongs.

The local device can still receive requests arriving through another routed network. The router handles the request locally and sends the response back through the network on which the request was received.

For example, if the Router Device belongs to the MS/TP network but receives a request through BACnet/IP, the response is returned through BACnet/IP.

---

## Device Configuration

The Router Device uses the normal BACnet-Arduino device configuration:

```cpp
const deviceInfos_t deviceInfos {
    2000,
    "My Opta",
    "Basic BACnet device running on Arduino",
    "Mechanical room",
    "Arduino",
    "Opta Bacnet",
    "1.0",
    "0.1"
};
```

The device instance is independent of the BACnet network number.

A project may define its own device-instance convention. For example:

```cpp
// Project convention: device instance = network * 1000 + device MAC.
const deviceInfos_t deviceInfos {
    ROUTER_NETWORK_MSTP * 1000u + UNIT_MAC,
    ...
};
```

This is only a project convention; BACnet does not require this particular device-instance scheme.

---

## Time Synchronization

Time synchronization is configured in the same way as for a normal BACnet device.

The application must provide the BACnet time callback:

```cpp
bacnet_time_callback_set(bacnet_get_time);
```

If the device should accept BACnet Time Synchronization requests:

```cpp
handler_timesync_set_callback_set(bacnet_sync_time);
```

The callbacks must update/read the same system clock used by the BACnet stack.

The router does not require a separate time implementation.

---

## Router and Device Timers

The router does not replace the normal BACnet application timers.

The application must continue to call:

```cpp
Device_Timer(elapsed_ms);
```

and maintain the DCC timer:

```cpp
dcc_timer_seconds(1);
```

For example:

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

    router.poll();
}
```

`router.poll()` handles datalink polling and, when `ROUTER_DEVICE` is enabled, the periodic local-device processing.

---

## Complete Example

A minimal BIP ↔ MS/TP router setup has the following general structure:

```cpp
#include <Arduino.h>

#include "BACnet-Arduino.h"
#include "BACnet-Arduino_MSTP.h"
#include "BACnet-Arduino_router.h"

BacnetRouterClass router;

void setup()
{
    // Configure and initialize BACnet/IP
    setupNetwork();

    // Configure and initialize BACnet MS/TP
    setupMSTP();

    // Configure the local BACnet Device if ROUTER_DEVICE is enabled
    setupDevice();

    router.begin(
        RouterNetwork{BacnetDatalink::BIP, 100},
        RouterNetwork{BacnetDatalink::MSTP, 200}
    );

#if defined(ROUTER_DEVICE)
    router.attachRouterDevice(BacnetDatalink::MSTP);
#endif
}

void loop()
{
    // Application/BACnet timers
    Device_Timer(...);
    dcc_timer_seconds(...);

    // Poll all configured router datalinks
    router.poll();
}
```

See the router example included with the library for a complete Arduino implementation including Ethernet, NTP, RS-485, MS/TP callbacks, BACnet time synchronization, and local device configuration.

---

## Important Notes

* Configure and initialize each datalink before starting the router.
* Each configured router network must have a unique BACnet network number.
* Do not call the individual BIP or MS/TP `poll()` functions when using `BacnetRouterClass`.
* Use `router.poll()` as the application's main BACnet datalink polling function.
* `ROUTER_DEVICE` is optional.
* When a local Router Device is enabled, `router.poll()` also services the local device.
* The router does not require changes to the underlying BACnet application handlers.
* The router preserves the original source address when delivering routed requests to the local device, allowing responses to be returned through the network on which the request arrived.
