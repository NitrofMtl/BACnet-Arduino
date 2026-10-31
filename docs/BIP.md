# BACnet/IP

BACnet-Arduino provides BACnet/IP communication through a templated peripheral. The supplied network and UDP classes must be compatible with the Arduino `Ethernet` and `EthernetUDP` APIs.

This document describes the BACnet/IP-specific configuration and initialization.

For functionality common to BACnet/IP and BACnet MS/TP, see the main [README](../README.md).

---

## Enable BACnet/IP

BACnet/IP is enabled by default when no other BACnet data link is enabled. If multiple data links are used, uncomment the BACnet/IP data link definition in `ports/arduino/arduino_configs.h` to enable it explicitly.

```cpp
// Ethernet configuration
//#define BACDL_BIP
```

---

## BACnet/IP Setup

The BACnet/IP peripheral uses templated network and UDP classes. The supplied instances must be created before the peripheral is initialized.

```cpp
EthernetUDP Udp;

BIP_Peripheral<EthernetClass, EthernetUDP>
    BIP_Periph(&Ethernet, &Udp, bacnetUdpPort);
```
The supplied network and UDP classes must be compatible with the Arduino Ethernet and EthernetUDP APIs.

The network and UDP instances must be initialized according to their respective library APIs:

```cpp
Ethernet.begin(deviceIp);

Udp.begin(bacnetUdpPort);

BIP_Periph.begin();
```

The BACnet UDP port must be the same in both the peripheral constructor and the UDP initialization.

The default BACnet/IP UDP port is `47808` (`0xBAC0`).

A different port can be used if the same value is supplied to both.

The network and UDP objects must remain valid for as long as BACnet/IP is running. The BACnet/IP peripheral does not configure the physical Ethernet interface itself. Network configuration and initialization depend on the Arduino networking library and hardware being used.

---

## Troubleshooting

### BACnet device does not respond

Check the following:

1. The network interface is initialized correctly.
2. The UDP instance is initialized on the configured BACnet port.
3. The `BIP_Peripheral` is properly initialized.
4. `bacnet_Poll()` is called regularly.
5. The board has a valid IP configuration.
6. The PC and Arduino have a network path that permits BACnet/IP UDP traffic.

A packet capture tool such as Wireshark can help determine whether BACnet/IP packets are reaching the Arduino.