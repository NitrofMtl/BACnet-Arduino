/*
BACnet Router Example

This example configure 2 BIP and one MS/TP port.

Board: LilyGO TTGO T-Display (ESP32)

Configuration:
Edit arduino_configs.h:

  Enable:
    -#define BACDL_BIP
    -#define BACDL_MSTP
    -#define BACNET_ROUTER
    -#define ROUTER_DEVICE

  Set:
    -#define BACNET_ROUTER_BIP_PORTS_COUNT  2
    -#define BACNET_ROUTER_MSTP_PORTS_COUNT 1

Dependencies / Notes:

The ESP32 requires a modification to the ArduinoRS485 library
for this example to work correctly. The library was modified
locally; see the project's documentation for details.

Ensure that the configured router port counts match the number
of ports used by the example.
*/


#include <Arduino.h>
#include "WiFi.h"

#include <SPI.h>
#include "Ethernet.h"
#include <NTPClient.h>
#include "ArduinoRS485.h"

#include "BACnet-Arduino_router.h"



// -----------------------------------------------------------------------------
// MSTP definition
// -----------------------------------------------------------------------------
//MSTP1
#define RS485_UART_INSTANCE 1
#define RS485_RX 32
#define RS485_TX 33

#define MSTP1_DE_PIN 17
#define BACNET_MSTP_BAUD 115200
#define RS485_PREDELAY 25
#define RS485_POSTDELAY 15UL * 1000000UL / BACNET_MSTP_BAUD

#define UNIT_MAC 0
#define ROUTER_NETWORK_MSTP1 200
HardwareSerial RS485Serial(RS485_UART_INSTANCE);
RS485Class RS485(RS485Serial, RS485_TX, MSTP1_DE_PIN, -1);
MSTP_port<RS485Class> MSTP1 = {BacnetDatalink::MSTP, ROUTER_NETWORK_MSTP1, &RS485};


//MSTP2 for future test
#define RS485_1_UART_INSTANCE 2
#define RS485_1_RX 2
#define RS485_1_TX 12
#define RS485_1_DE 27


// -----------------------------------------------------------------------------
//Bip Ethernet configs
// -----------------------------------------------------------------------------

//18, 19, 23
#define SPI_MOSI_PIN 21
#define SPI_MISO_PIN 22
#define SPI_CLK_PIN 13
#define ETH_CS_PIN 15

#define ROUTER_NETWORK_BIP_ETH 100

byte EthMac[] = { 0x90, 0xA2, 0xDA, 0x10, 0xCD, 0xA3 };
IPAddress EthIp = {192,168,0,151};
constexpr uint16_t bacnetEthUdpPort = 47808; // BAC0
EthernetUDP EthUdp;

#define BACNET_ETH_UDP_PORT 0xBAC0

BIP_Peripheral<EthernetClass, EthernetUDP> EthPort(&Ethernet, &EthUdp, bacnetEthUdpPort);
BIP_Port BIP1(BacnetDatalink::BIP, ROUTER_NETWORK_BIP_ETH, &EthPort);


// -----------------------------------------------------------------------------
//BIP WIFI configs
// -----------------------------------------------------------------------------
#define ROUTER_NETWORK_BIP_WIFI 300
IPAddress WiFiIp = {192,168,0,152};
IPAddress WiFiGateway = {192,168,0,1};
IPAddress WiFiSubnet = {255,255,255,0};
WiFiServer WiFI(WiFiIp);
const char* ssid     = "Nitrofnet"; // Change this to your WiFi SSID
const char* WiFiPassword = "3011198030111980"; // Change this to your WiFi password
constexpr uint16_t bacnetWiFiUdpPort = 47809; // BAC1
WiFiUDP WiFiUdp;

#define BACNET_WIFI_UDP_PORT 0xBAC1

BIP_Peripheral<WiFiClass, WiFiUDP> WiFiPort(&WiFi, &WiFiUdp, bacnetWiFiUdpPort);
BIP_Port BIP2(BacnetDatalink::BIP, ROUTER_NETWORK_BIP_WIFI, &WiFiPort);



// ----------------------------------------------------------------------------- 
//Device Configurations
// ----------------------------------------------------------------------------- 
// Device instance convention used by this example:
// network * 1000 + device MAC(if mstp)
const deviceInfos_t deviceInfos {
  ROUTER_NETWORK_MSTP1 * 1000u + UNIT_MAC,  //Instance_number
  "TTGO_Router", //Name
  "Basic BACnet device running on Arduino",  //description
  "Mechanical room", //Location
  "Arduino", //Vendor name
  "TTGO Bacnet", //Model name
  "1.0", //Firmware revision
  "0.1" //Application software version
};


// ----------------------------------------------------------------------------- 
//Router Configurations
// ----------------------------------------------------------------------------- 
BacnetRouterClass router;

// ----------------------------------------------------------------------------- 


void setupEthernetPort()
{
  SPI.end();//reset the SPI to use custom pins
  SPI.begin(SPI_CLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, -1);
  Ethernet.init(ETH_CS_PIN);
  Ethernet.begin(EthMac, EthIp);

  EthUdp.begin(BACNET_ETH_UDP_PORT);
}


void setupWifiPort()
{
  if (!WiFi.config(WiFiIp, WiFiGateway, WiFiSubnet)) {
        Serial.println("WiFi static IP configuration failed");
  }
  WiFi.begin(ssid, WiFiPassword);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi IP: ");
  Serial.println(WiFi.localIP());

  WiFiUdp.begin(BACNET_WIFI_UDP_PORT);
}


static void rs485_send(const uint8_t *p, uint16_t len)
{
  MSTP1.active().noReceive();
  MSTP1.active().beginTransmission();
  MSTP1.active().write(p, len);
  mstp_silence_reset();
  MSTP1.active().endTransmission();
  MSTP1.active().receive();   
}


static bool rs485_read(uint8_t *b)
{
  if (MSTP1.active().available()) {
      if (b == nullptr) {
          return true;
      }
      
      uint8_t byte = MSTP1.active().read();
      *b = byte;
      return true;
  }
  if (b != nullptr) *b = 0;
  return false;
}



bool rs485_transmitting()
{
  if (MSTP1.isActive()) {
    if (digitalRead(MSTP1_DE_PIN) == HIGH) return true;
  }
  return false;
}


void setupMSTP()
{
  RS485Serial.begin(BACNET_MSTP_BAUD, SERIAL_8N1, RS485_RX, RS485_TX);
  RS485.begin(BACNET_MSTP_BAUD);
  RS485.setDelays(RS485_PREDELAY, RS485_POSTDELAY);

  RS485.receive();

  mstp_callback_read_set(rs485_read);
  mstp_callback_send_set(rs485_send);
  mstp_callback_transmitting_set(rs485_transmitting);

  //isReady() validate that the object is correctly set
  if (MSTP1.isReady()) {
    uint8_t MSTP1_id = MSTP1.instanceIndexGet();
    mstp_baud_rate_set(BACNET_MSTP_BAUD, MSTP1_id);
    init_bacnet_mstp(UNIT_MAC, MSTP1_id);
    mstp_nmax_master_set(2, MSTP1_id);    
  } else {
    Serial.println("Fail to configure MSTP Post!!!");
  }
}


void setupRouter()
{
  if(!router.begin(&BIP1, &BIP2, &MSTP1)){
    Serial.println("[ERROR] Failed to add router ports");
  }
}


void setup() {
  Serial.begin(9600);
  delay(3000);

  setupRouter();

  setupEthernetPort();
  setupWifiPort();
  setupMSTP();

  bacnet_init(&deviceInfos);
  delay(1000);
  Serial.println("Setup Complete !!!");

}

void loop() {
      // Device_Timer() expects the elapsed time since its previous call.
    static uint32_t last_ms = millis();
    uint32_t now = millis();

    Device_Timer(now - last_ms);  // elapsed since last call
    last_ms = now;

    // Run periodic one-second tasks without blocking the BACnet loop.
    static uint32_t lastTimer = 0;

    if (millis() - lastTimer >= 1000) {
        lastTimer += 1000;

        // Maintain BACnet DCC timing.
        dcc_timer_seconds(1);
    }
    router.poll();
}
