#pragma once

#include "Arduino.h"
#include "bacnet_peripheral.h"

class BIP_Port;
#if !defined(BACNET_ROUTER)
extern BIP_Port BIP;
#endif


class BIP_PeripheralBase
{
public:
    explicit BIP_PeripheralBase(uint16_t port) : _udpPort(port) {}
    virtual ~BIP_PeripheralBase() = default;
    
    virtual IPAddress localIP() = 0;
    virtual int read(unsigned char *buffer, size_t len) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) = 0;
    virtual IPAddress remoteIP() = 0;
    virtual uint16_t remotePort() =0;
    virtual int parsePacket() = 0;
    uint16_t getUdpPort() const { return _udpPort; }
    virtual int beginPacket(IPAddress ip, uint16_t port);
    virtual int endPacket();
    protected:
    uint16_t _udpPort = 0;
};

template<class Teth, class Tudp>
class BIP_Peripheral : public BIP_PeripheralBase
{
    public:
    BIP_Peripheral(Teth *ethernet, Tudp *udp, uint16_t port)
    : BIP_PeripheralBase(port), _ethernet(ethernet), _udp(udp)
    {}
#if defined (BACDL_BIP) && !defined(BACNET_ROUTER)
    void begin() { BIP = BIP_Port(BacnetDatalink::BIP, 0, this); }
#endif
    Teth& ethernet() { return *_ethernet; }
    Tudp& udp() { return *_udp; }

    IPAddress localIP() override {
        return _ethernet->localIP();
    }
    int read(unsigned char *buffer, size_t len) override {
        return _udp->read(buffer, len);
    }

    size_t write(const uint8_t *buffer, size_t size) override {
        return _udp->write(buffer, size);
    }

    IPAddress remoteIP() override {
        return _udp->remoteIP();
    }

    uint16_t remotePort() {
        return _udp->remotePort();
    }

    int parsePacket() override {
        return _udp->parsePacket();
    }

    int beginPacket(IPAddress ip, uint16_t port) override {
        return _udp->beginPacket(ip, port);
    }

    int endPacket() override {
        return _udp->endPacket();
    }

    private:
    Teth *_ethernet;
    Tudp *_udp;
};


class BIP_Port : public RouterPort
{
public:
    BIP_Port(BacnetDatalink portType, uint16_t net, BIP_PeripheralBase* peripheral);
    static BIP_PeripheralBase* active();
private:
    void activePeripheralSet() override;
    static BIP_PeripheralBase* _activePeripheral;
    BIP_PeripheralBase* _peripheral;
};

#if !defined(BACNET_ROUTER)
extern BIP_Port BIP;
#endif