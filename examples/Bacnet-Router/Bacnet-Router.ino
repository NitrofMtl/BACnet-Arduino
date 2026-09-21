
/*
 * BACnet BIP <-> MS/TP Router
 *
 * 
 * To adapt this example:
 * 
 * Some configuration macros need to be uncommented in
 * ports/arduino/arduino_configs.h.
 * 
 * 2. Enable BACNET_ROUTER. 
 * 3. If the router has a local BACnet Device, enable ROUTER_DEVICE.
 * 
 * Configure BACnet/IP and MS/TP as in their respective examples,
 * including all required callbacks and time synchronization.
 *
 * Do not call the BIP or MS/TP poll functions. The router poll()
 * handles both datalinks and the local Router Device when enabled.
 *
 * Then configure the router networks and, if required, attach
 * the local Router Device.
 */


#include <Arduino.h>
#include "ArduinoRS485.h"


// Arduino networking and time libraries
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <NTPClient.h>

//Opta specific lib
#include <mbed_rtc_time.h>

//BACnet BIP
#include "BACnet-Arduino.h"
//BACnet MS/TP
#include "BACnet-Arduino_MSTP.h"
//BACnet Router
#include "BACnet-Arduino_router.h"



// ----------------------------------------------------------------------------- 
// Network configuration 
// -----------------------------------------------------------------------------
constexpr uint8_t deviceIp[] = { 192, 168, 0, 150 };
constexpr uint16_t bacnetUdpPort = 47808; // BAC0

// -----------------------------------------------------------------------------
// Global objects
// -----------------------------------------------------------------------------
EthernetUDP Udp;
#define TIME_UTC_OFF_ESTERN_ADVANCE -4
// NTP is used only to initialize/synchronize the board clock. 
// BACnet subsequently uses the board clock through bacnet_get_time().
NTPClient timeClient(Udp, "pool.ntp.org", TIME_UTC_OFF_ESTERN_ADVANCE * 3600, 0);

// ----------------------------------------------------------------------------- 
//MS/TP Configurations
// ----------------------------------------------------------------------------- 
#define DE_PIN RS485_DEFAULT_DE_PIN
#define UNIT_MAC 0
#define BACNET_MSTP_BAUD 115200
#define RS485_PREDELAY 25
#define RS485_POSTDELAY 15UL * 1000000UL /BACNET_MSTP_BAUD

// ----------------------------------------------------------------------------- 
//Router Configurations
// ----------------------------------------------------------------------------- 
#define ROUTER_NETWORK_BIP 100
#define ROUTER_NETWORK_MSTP 200
BacnetRouterClass router;

// ----------------------------------------------------------------------------- 
//Device Configurations
// ----------------------------------------------------------------------------- 
// Device instance convention used by this example:
// network * 1000 + device MAC
const deviceInfos_t deviceInfos {
    ROUTER_NETWORK_MSTP*100u+UNIT_MAC,  //Instance_number
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


/**
 * @brief Synchronize board time from BACnet time service
 *
 * Updates the board's authoritative clock using a BACnet Time Synchronization
 * request (Local or UTC depending on implementation).
 *
 * This function MUST update the SAME time source used by bacnet_get_time().
 * It defines the WRITE path of the BACnet time subsystem.
 *
 * @param[in] bdate Pointer to BACNET_DATE received from BACnet stack
 * @param[in] btime Pointer to BACNET_TIME received from BACnet stack
 *
 * @return true if time was successfully applied
 * @return false if time update failed (hardware or conversion error)
 *
 * @note This function MUST NOT interact with NTP or external time clients.
 *       Only the board's internal time system (RTC/system time) may be updated.
 *
 * @warning Repeated calls must be safe and should not introduce drift.
 */
void bacnet_sync_time(BACNET_DATE *bdate, BACNET_TIME *btime, bool UTC)
{
    // This example uses local time and therefore ignores the UTC flag. 
    // A real application should convert UTC when UTC == true.
    (void)UTC;

    tmElements_t tm_info = BACnetSchedule::toTimeElements(*bdate, *btime);
    time_t epoch_time = makeTime(tm_info);
    set_time(epoch_time);
}


/**
 * @brief Get current BACnet system time
 *
 * Retrieves the current time from the board's authoritative clock source
 * (RTC or system time) and converts it into BACnet DATE/TIME format.
 *
 * This function MUST be implemented by each platform port.
 * It defines the READ path of the BACnet time subsystem.
 *
 * @param[out] bdate Pointer to BACnet_DATE structure to fill (must not be NULL)
 * @param[out] btime Pointer to BACnet_TIME structure to fill (must not be NULL)
 *
 * @return true if time was successfully retrieved and populated
 * @return false only in case of critical failure (no valid clock source)
 *
 * @note This function MUST NOT use NTP or external network time sources.
 *       It must rely on the board's internal time base (RTC/system clock).
 */
bool bacnet_get_time(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    time_t epoch = now();//time(nullptr);
    tmElements_t tm_info = {};
    breakTime(epoch, tm_info);

    *bdate = BACnetSchedule::toBACnetDate(tm_info);
    *btime = BACnetSchedule::toBACnetTime(tm_info);

    *dst_active = false; // NTPClient doesn't provide DST info, so we hardcode it here. You could implement your own DST logic if needed.
    return true;
}


void setupDevice()
{
    bacnet_init(&deviceInfos);
    
    // BACnet reads the current board time through this callback.
    bacnet_time_callback_set(bacnet_get_time);//set time from ntp

    // Allow BACnet Time Synchronization requests to update the board clock.
    handler_timesync_set_callback_set(bacnet_sync_time);//allow bacnet time sync requests to set time
}


// ----------------------------------------------------------------------------- 
// Network initialization 
// -----------------------------------------------------------------------------
void setupNetwork()
{
    Ethernet.begin(deviceIp);
    Udp.begin(bacnetUdpPort);
    bacnet_set_udp(&Udp, bacnetUdpPort);
    timeClient.begin();
    timeClient.update();
    const unsigned long epoch = timeClient.getEpochTime();
    set_time(epoch);
}


void setupMSTP()
{
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

    delay(3000);
    setupNetwork();
    setupDevice();
    setupMSTP();
    router.begin(
        RouterNetwork{BacnetDatalink::BIP, ROUTER_NETWORK_BIP},
        RouterNetwork{BacnetDatalink::MSTP, ROUTER_NETWORK_MSTP}
    );
    //Needed only if you have local Router Device
    if (!router.attachRouterDevice(BacnetDatalink::MSTP)) {
        Serial.println("Failed to attach MSTP as router device");
    }
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


