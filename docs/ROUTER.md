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



---

---

---


---



# BACnet Router

`BacnetRouterClass` provides BACnet network-layer routing between configured BACnet networks.

A router can connect multiple BACnet networks using supported datalinks, such as:

- BACnet/IP
- BACnet MS/TP

Multiple ports of the same datalink can also be used. For example, a router can connect two BACnet/IP networks and one BACnet MS/TP network.

The router is independent of the physical transport implementation. Each port is configured with its own datalink, network number, and transport peripheral.

A router can operate without hosting a local BACnet Device. An optional local device can also be configured when required.

For information about configuring individual datalinks, see the [BACnet/IP](BIP.md) and [BACnet MS/TP](mstp.md) documentation.

---

## Configuration

Router support and the required datalinks must be enabled in:

`ports/arduino/arduino_configs.h`

For example, to enable BACnet/IP, BACnet MS/TP, and router support:

```cpp
#define BACDL_BIP
#define BACDL_MSTP
#define BACNET_ROUTER
```

The number of ports configured for each datalink must also be specified:

```cpp
#define BACNET_ROUTER_BIP_PORTS_COUNT  2
#define BACNET_ROUTER_MSTP_PORTS_COUNT 1
```

These values must match the number of ports of each datalink used by the application.

If the router will also host a local BACnet Device, enable:

```cpp
#define ROUTER_DEVICE
```

A local BACnet Device is optional. It is not required for routing traffic between networks.

---

## Router Networks

Each router port belongs to a BACnet network and must be assigned a network number.

For example:

```cpp
#define ROUTER_NETWORK_BIP_ETH  100
#define ROUTER_NETWORK_BIP_WIFI 300
#define ROUTER_NETWORK_MSTP1    200
```

Each network number must be unique within the BACnet internetwork.

The network number is assigned when the port is created.

## Creating the BIP peripheral

The BIP peripheral provides BACnet/IP communication using an Arduino-compatible network interface and UDP class. It is templated on both classes, allowing different network and UDP implementations to be used.

Create a peripheral instance by passing pointers to the network interface and UDP objects, along with the BACnet UDP port:

```cpp

BIP_Peripheral<EthernetClass, EthernetUDP> EthPort(
    &Ethernet, &EthUdp, bacnetEthUdpPort);

```
The network interface and UDP objects must be configured separately. The UDP port must match the port used when initializing the UDP object with begin().

Once the peripheral is created, it can be used to create a router port associated with a BACnet network number.

```cpp
BIP_Port BIP1(
    BacnetDatalink::BIP, ROUTER_NETWORK_BIP_ETH, &EthPort);
```

The same approach applies to other compatible network and UDP classes, such as Wi-Fi.

```cpp
BIP_Peripheral<WiFiClass, WiFiUDP> WiFiPort(
    &WiFi, &WiFiUdp, bacnetWiFiUdpPort);
BIP_Port BIP2(
    BacnetDatalink::BIP, ROUTER_NETWORK_BIP_WIFI, &WiFiPort);
```

## Creating the MS/TP peripheral

```cpp
MSTP_port<RS485Class> MSTP1 = {
    BacnetDatalink::MSTP,
    ROUTER_NETWORK_MSTP1,
    &RS485
};
```

The transport peripheral is supplied by the application. Its configuration and initialization follow the corresponding datalink documentation.

---

## Router Initialization

Create the router instance:

```cpp
BacnetRouterClass router;
```

Add the configured ports by passing them to `router.begin()`:

```cpp
void setupRouter()
{
    if (!router.begin(&BIP1, &BIP2, &MSTP1)) {
        Serial.println("Failed to add router ports");
    }
}
```

Call `router.begin()` before initializing the individual ports.

After the router has been configured, initialize each physical transport and its corresponding BACnet datalink.

For example:

```cpp
void setup()
{
    Serial.begin(9600);

    setupRouter();

    setupEthernetPort();
    setupWifiPort();
    setupMSTP();

    // Optional local BACnet Device initialization
}
```

The initialization of Ethernet, Wi-Fi, and RS-485 depends on the hardware and libraries being used. Refer to the individual datalink documentation for the corresponding setup requirements.

---

## Polling

When using `BacnetRouterClass`, call:

```cpp
router.poll();
```

regularly from the Arduino `loop()`.

The router handles polling of its configured datalinks and forwarding of received BACnet traffic.

Do not call the individual BIP or MS/TP polling functions separately when using the router.

For example:

```cpp
void loop()
{
    router.poll();
}
```

If the application also hosts a local BACnet Device, its normal application timers must still be maintained. See the [Router Device](#router-device) section.

---

## Complete Router Example

A complete router example is included with the library.

The example uses a LilyGO TTGO T-Display (ESP32) and demonstrates a configuration with:

- Two BACnet/IP ports:
  - Ethernet
  - Wi-Fi
- One BACnet MS/TP port:
  - RS-485

The example includes the physical interface configuration, transport callbacks, router initialization, and main loop.

The example's configuration requires the following definitions in `ports/arduino/arduino_configs.h`:

```cpp
#define BACDL_BIP
#define BACDL_MSTP
#define BACNET_ROUTER
#define ROUTER_DEVICE

#define BACNET_ROUTER_BIP_PORTS_COUNT  2
#define BACNET_ROUTER_MSTP_PORTS_COUNT 1

#define MAX_TSM_TRANSACTIONS 15
```

The reduced `MAX_TSM_TRANSACTIONS` value is used by this ESP32 example to accommodate its available RAM.

**ESP32 dependency:** The example requires a modification to the ArduinoRS485 library for the RS-485 implementation to work correctly. See the project's documentation for details.

The configured router port counts must match the ports instantiated by the example.

---

## Router Device

A router can optionally host a local BACnet Device in addition to forwarding traffic between networks.

This is a separate function from network-layer routing. The router can operate without a local device; enable `ROUTER_DEVICE` only when the application requires one.

### Device Configuration

When using a local Router Device, enable:

```cpp
#define ROUTER_DEVICE
```

Provide the normal BACnet device information:

```cpp
const deviceInfos_t deviceInfos {
    2000,
    "TTGO_Router",
    "Basic BACnet device running on Arduino",
    "Mechanical room",
    "Arduino",
    "TTGO Bacnet",
    "1.0",
    "0.1"
};
```

Initialize the BACnet device:

```cpp
bacnet_init(&deviceInfos);
```

The Device Instance is a BACnet device identifier. It is separate from the network number and the MS/TP MAC address.

For example, a project may use the following convention:

```cpp
// Project convention: network number * 1000 + MS/TP MAC.
const deviceInfos_t deviceInfos {
    ROUTER_NETWORK_MSTP1 * 1000u + UNIT_MAC,
    // Remaining device information...
};
```

This is only an application-defined convention. BACnet does not require the Device Instance to be calculated from the network number or MAC address.

### Device Application Requirements

The local device uses the normal BACnet-Arduino application functionality.

If required by the application, configure the BACnet time callbacks:

```cpp
setTimeCallback_set(set_time);
```

The application must also maintain the normal BACnet device and DCC timers.

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

The router handles the configured datalinks through `router.poll()`. The application remains responsible for the normal device timers and any other application-specific processing.