#include <Arduino.h>

// Arduino networking and time libraries
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <NTPClient.h>
#include <mbed_mktime.h>
#include "time.h"

// Opta persistent storage API
#include "KVStore.h"
#include "kvstore_global_api.h"

// BACnet-Arduino Arduino port
#include "BACnet-Arduino.h"

// ----------------------------------------------------------------------------- 
// Network configuration 
// -----------------------------------------------------------------------------
constexpr uint8_t deviceIp[] = { 192, 168, 0, 150 };
constexpr uint16_t bacnetUdpPort = 47808; // BAC0

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

// ----------------------------------------------------------------------------- 
// BACnet configuration 
// -----------------------------------------------------------------------------
#define TIME_UTC_OFF_ESTERN_ADVANCE -4
#define DCC_PASSWORD "1234"
#define REINIT_PASSWORD "4321"

// Object instance numbers used by this example
#define AV_0 0
#define AI_0 0
#define BI_0 0
#define BV_0 0
#define AO_0 0
#define BO_0 0

#define MSI_FAN_SPEED 0
#define MSO_FAN_MODE 0
#define MSV_FAN_STATUS 0

// -----------------------------------------------------------------------------
// Global objects
// -----------------------------------------------------------------------------
EthernetUDP Udp;

// NTP is used only to initialize/synchronize the board clock. 
// BACnet subsequently uses the board clock through bacnet_get_time().
NTPClient timeClient(Udp, "pool.ntp.org", TIME_UTC_OFF_ESTERN_ADVANCE * 3600, 0);

BACnetSchedule mainSchedule(0);

// ----------------------------------------------------------------------------- 
// Forward declarations
// -----------------------------------------------------------------------------
void setupNetwork();

void setupBacnet();

void reinitColdStartCb();
void setupSchedule();
bool restoreSchedule(uint32_t inst, BACnetSchedule::SchedulePersistentData *data);
void saveSchedule(uint32_t inst, const BACnetSchedule::SchedulePersistentData &data);

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


// ----------------------------------------------------------------------------- 
// BACnet initialization 
// -----------------------------------------------------------------------------

// Configure the BACnet device, callbacks and example objects.
void setupBacnet()
{
    bacnet_init(&deviceInfos);
    setTimeCallback_set(set_time);

    // Configure BACnet device control passwords.
    handler_dcc_password_set(DCC_PASSWORD);
    Device_Reinitialize_Password_Set(REINIT_PASSWORD);

    // The actual reset mechanism is platform-specific.
    bacnet_reinitialize_callback_set(BACNET_REINIT_COLDSTART, reinitColdStartCb);

    // ------------------------------------------------------------------------- 
    // Example BACnet objects 
    // -------------------------------------------------------------------------

    createAnalogValue( AV_0, "Temperature", "Temperature in Celsius", UNITS_DEGREES_CELSIUS, 42.0f);
    createBinaryValue(BV_0, "BV_0", "Binary Value 0", BINARY_INACTIVE);
    createAnalogInput(AI_0, "AI_0", "Analog Input 0", UNITS_MILLIAMPERES);
    createBinaryInput(BI_0, "BI_0", "Binary Input 0", BINARY_ACTIVE);
    createAnalogOutput(AO_0, "AO_0", "Analog Output 0", UNITS_PERCENT, 50.0f);
    createBinaryOutput(BO_0, "BO_0", "Binary Output 0", BINARY_ACTIVE);

    // Multi-state objects use a list of null-terminated strings.
    static const char fanStateList[] = {"OFF\0" "LOW\0" "MEDIUM\0" "HIGH\0" "AUTO\0"};
    createMultiStateInput(MSI_FAN_SPEED, "FAN_SPEED", "Fan Speed", fanStateList, 1);

    static const char fanModeList[] = {"OFF\0" "ON\0" "AUTO\0"}; 
    createMultiStateOutput(MSO_FAN_MODE, "FAN_MODE", "Fan Mode", fanModeList, 1);

    Multistate_Output_Present_Value_Set(MSO_FAN_MODE, 1, BACNET_PRIORITY_NORMAL);

    static const char fanStatusList[] = {"OFF\0" "MANUAL\0" "AUTO\0" "ALARM\0"};
    createMultiStateValue(MSV_FAN_STATUS, "FAN_STATUS", "Fan Status", fanStatusList, 3);

    // Schedule persistence callbacks must be registered BEFORE begin(). 
    // BACnetSchedule::begin() restores the saved schedule during initialization.
    BACnetSchedule::setRestoreCallback(restoreSchedule);
    BACnetSchedule::setSaveCallback(saveSchedule);
    
    BACnetSchedule::begin();
 

    Serial.print("Temperature");
    Serial.println(Analog_Value_Present_Value(AV_0));
}


// ----------------------------------------------------------------------------- 
// Arduino setup 
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(9600);
    analogReadResolution(16);

    pinMode(BTN_USER, INPUT_PULLUP);
    // Give the serial monitor time to connect before initialization messages.
    delay(4000);
    setupNetwork();
    setupBacnet();

    // Uncomment to create/modify the test schedule. 
    // setupSchedule();

}

// ----------------------------------------------------------------------------- 
// Main loop 
// -----------------------------------------------------------------------------
void loop() {
    // Device_Timer() expects the elapsed time since its previous call.
    static uint32_t last_ms = 0;
    uint32_t now = millis();

    Device_Timer(now - last_ms);  // elapsed since last call
    last_ms = now;

    // Run periodic one-second tasks without blocking the BACnet loop.
    static uint32_t lastTimer = 0;

    if (millis() - lastTimer >= 1000) {
        lastTimer += 1000;

        // Maintain BACnet DCC timing.
        dcc_timer_seconds(1);
        
        // --------------------------------------------------------------------- 
        // COV test 
        // --------------------------------------------------------------------- 
        // Change the Analog Value periodically so clients subscribed to its 
        // COV can observe value changes.
        float val = Analog_Value_Present_Value(AV_0);
        val+= 1.5;

        Analog_Value_Present_Value_Set(AV_0, val, BACNET_PRIORITY_NORMAL);

        // Update the Analog Input from the Opta analog input.
        Analog_Input_Present_Value_Set(AI_0, analogRead(A0));
    }

    // Process BACnet network traffic and protocol timers.
    bacnet_Poll();

    // Update the Binary Input from the physical user button.
    Binary_Input_Present_Value_Set(BI_0, digitalRead(BTN_USER) == LOW ? BINARY_ACTIVE : BINARY_INACTIVE);
}


// ----------------------------------------------------------------------------- 
// Platform-specific BACnet reinitialization 
// ----------------------------------------------------------------------------- 
// Called by the BACnet stack when a cold-start reinitialization is requested. 
// The actual reset mechanism is platform-specific.
void reinitColdStartCb()
{
    Serial.println("Cold reinit!!!");
    //each board should have it own reinit method
    //STM32 (Opta, GIGA, H7, F7, etc.)
    NVIC_SystemReset();

}


// ----------------------------------------------------------------------------- 
// Schedule example 
// ----------------------------------------------------------------------------- 
// Create a test weekly schedule. 
// 
// The schedule is only created when setupSchedule() is explicitly called. 
// Normally the schedule is restored from persistent storage by begin().
void setupSchedule()
{
    mainSchedule.setEffectivePeriod(2024, 1, 1, 2099, 12, 31);
    mainSchedule.setDefaultEnum(1);
    mainSchedule.set_Object_Property_Reference(OBJECT_MULTI_STATE_OUTPUT, 0);
    mainSchedule.setSchedule(dowFriday, 0, 21, 37, 2); // Friday at 21:37 set to state 2 (ON)
    mainSchedule.setSchedule(dowSunday, 1, 11, 14, 3); // Sunday at 11:14 set to state 3 (AUTO)
    mainSchedule.setSchedule(dowThursday, 2, 18, 59, 1); // Thursday at 18:59 set to state 1 (OFF)
}


// ----------------------------------------------------------------------------- 
// Schedule persistence 
// ----------------------------------------------------------------------------- 
/** * @brief Restore a schedule from Opta persistent storage. 
 *
 * BACnetSchedule calls this callback during begin() for each schedule 
 * instance. The callback is responsible for loading the serialized schedule 
 * data from the board-specific persistent storage. 
 * 
 * @param[in] inst Schedule instance number. 
 * @param[out] data Buffer to receive the saved schedule. 
 * 
 * @return true when valid schedule data was restored. 
 */
bool restoreSchedule(uint32_t inst, BACnetSchedule::SchedulePersistentData *data)
{
    if (!data) return false;
    
    char key[20];
    snprintf(key, sizeof(key), "/kv/schedule%lu", (unsigned long)inst);

    size_t actual_size;
    int rc = kv_get(
        key,
        data,
        sizeof(*data),
        &actual_size);

    return (rc == MBED_SUCCESS && actual_size == sizeof(*data));
}


/**
 * @brief Save a schedule to Opta persistent storage. 
 * 
 * BACnetSchedule calls this callback whenever schedule data is modified. 
 * The callback is responsible for storing the complete SchedulePersistentData 
 * structure using the board's persistent storage API. 
 * 
 * @param[in] inst Schedule instance number. 
 * @param[in] data Complete schedule data to store. 
 */
void saveSchedule(uint32_t inst, const BACnetSchedule::SchedulePersistentData &data)
{
    char key[20];
    snprintf(key, sizeof(key), "/kv/schedule%lu", (unsigned long)inst);

    kv_set(key, &data, sizeof(data), 0);
}

