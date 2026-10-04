#include <Arduino.h>

#include "STM32RS485DMA.h"

//Opta specific lib
#include <mbed_rtc_time.h>

//BACnet MS/TP
#include "BACnet-Arduino_MSTP.h"


#define DE_PIN RS485_DEFAULT_DE_PIN
#define UNIT_MAC 0
#define BACNET_MSTP_BAUD 115200
#define RS485_PREDELAY 25
#define RS485_POSTDELAY 15UL * 1000000UL / BACNET_MSTP_BAUD
#define RS485_TURNAROUND 20UL * 1000000UL / BACNET_MSTP_BAUD

#define DMA_RX_BUFFER_SIZE 256


const deviceInfos_t deviceInfos {
    0,
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
    //mstp_silence_reset();
    RS485.endTransmission();
    RS485.receive(); 
}


struct RS485Read_Buffer {
    uint8_t buffer[DMA_RX_BUFFER_SIZE] = {};
    uint16_t pos = 0;
    uint16_t len = 0;
};
RS485Read_Buffer RxBuffer;


static bool rs485_read(uint8_t *b)
{
    //refresh from DMA
    if (RxBuffer.len == 0) {
        RxBuffer.len = RS485.readFrame(RxBuffer.buffer, DMA_RX_BUFFER_SIZE);
        if ( RxBuffer.len > 250 ) {
            Serial.print("[RS485] Read frame: ");
            Serial.println(RxBuffer.len);
        }
        RxBuffer.pos = 0;
    }
    //available
    if (b == nullptr) {
        return RxBuffer.len > 0;
    }
    if (RxBuffer.pos < RxBuffer.len) {
        *b = RxBuffer.buffer[RxBuffer.pos++];
        //reset buffer when fully consumed
        if (RxBuffer.pos == RxBuffer.len) {
            RxBuffer.len = 0;
            RxBuffer.pos = 0;
        }
        return true;
    }
    
    return false;
}

void txCompleteCb()
{
    mstp_silence_reset();
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
    RS485.setOnTxCompleteCallback(txCompleteCb);
    
    // BACnet reads the current board time through this callback.
    bacnet_time_callback_set(bacnet_get_time);//set time from ntp

    // Allow BACnet Time Synchronization requests to update the board clock.
    handler_timesync_set_callback_set(bacnet_sync_time);//allow bacnet time sync requests to set time

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
    RS485.setRxIdleTime(RS485_TURNAROUND);

    RS485.receive();
  
    delay(3000);
    Serial.println("[System setup]");
    if (!RS485.hasValidConfig()){
      Serial.println("[ERROR] CHECK RS485 CONFIGURATIONS !!!");
    }

    setupMSTP();
    while (RS485.available()) RS485.read();
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
