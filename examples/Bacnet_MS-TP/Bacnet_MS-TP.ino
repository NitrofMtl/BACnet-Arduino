/*
## BACnet MS/TP Example

**Board:** Arduino Opta (STM32H747)

This example demonstrates a BACnet MS/TP device using the BACnet-Arduino library.

It configures the RS-485 interface and BACnet MS/TP data link, then initializes a BACnet device.
The device communicates over the MS/TP network using its configured MAC address and baud rate.

**Dependencies / Notes:**
- Requires an RS-485 interface supported by the ArduinoRS485 library.
- Configure the MS/TP MAC address and baud rate to match the network.
- Enable `BACDL_MSTP` in `arduino_configs.h`.
- This example is intended for standalone MS/TP operation, not router mode.
*/

#include <Arduino.h>
#include "ArduinoRS485.h"

//Opta specific lib
#include <mbed_rtc_time.h>

//BACnet MS/TP
#include "BACnet-Arduino_MSTP.h"

// Verify that the shared configuration is compatible with this example.
#if !defined(BACDL_MSTP)
#error "This example requires MSTP datalink to be enable in arduino_configs.h"
#endif
#if defined(BACDL_MULTIPLE)
#error "This example requires only MSTP as datalink to be enable in arduino_configs.h"
#endif
#if defined(BACNET_ROUTER)
#error "This example requires BACNET_ROUTER to be disabled in arduino_configs.h"
#endif

#define DE_PIN RS485_DEFAULT_DE_PIN
#define UNIT_MAC 1
#define BACNET_MSTP_BAUD 115200
#define RS485_PREDELAY 25
#define RS485_POSTDELAY 15UL * 1000000UL /BACNET_MSTP_BAUD


const deviceInfos_t deviceInfos {
    2,  //Instance_number
    "My Opta", //Name
    "Basic BACnet device running on Arduino",  //description
    "Mechanical room", //Location
    "Arduino", //Vendor name
    "Opta Bacnet", //Model name
    "1.0", //Firmware revision
    "0.1" //Application software version
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
    if (b != nullptr) *b = 0;
    return false;
}


bool rs485_transmitting()
{
    if (digitalRead(DE_PIN) == HIGH) return true;
    return false;
}


void setupMSTP()
{
    bacnet_init(&deviceInfos);
    setTimeCallback_set(set_time);

    mstp_callback_read_set(rs485_read);
    mstp_callback_send_set(rs485_send);
    mstp_callback_transmitting_set(rs485_transmitting);
    mstp_baud_rate_set(BACNET_MSTP_BAUD);

    init_bacnet_mstp(UNIT_MAC);
    mstp_nmax_master_set(2);
}


void setup() {
    Serial.begin(9600);

    RS485.begin(BACNET_MSTP_BAUD);
    RS485.setDelays(RS485_PREDELAY, RS485_POSTDELAY);
    RS485.receive();

    delay(1000);

    setupMSTP();
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
    
    bacnet_Poll_mstp();
}

