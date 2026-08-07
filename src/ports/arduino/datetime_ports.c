// datetime_ports.c
// Minimal implementation of platform-specific BACnet datetime functions for Opta
// Place in your ports folder

#include <Arduino.h>
#include "bacnet/datetime.h"


/**
 * @brief Callback used to retrieve current date and time
 *
 * Platform-specific function used by the BACnet date/time services
 * to obtain the current local date and time.
 *
 * The callback should populate all output parameters and return
 * true when valid time information is available.
 *
 * @note The callback typically reads from the board RTC or system clock.
 * @note Network time sources (NTP, BACnet TimeSync, etc.) should update
 *       the RTC rather than be accessed directly by the callback.
 */
static bool (*bacnet_time_callback)(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active) = NULL;


/**
 * @brief Register a date/time provider callback
 *
 * Registers the platform-specific function used to retrieve the
 * current date and time.
 *
 * @param[in] callback Function used to obtain current date/time
 *
 * @note Passing NULL disables callback-based time retrieval.
 * @note The callback must remain valid for the lifetime of the application.
 */
void bacnet_time_callback_set(bool (*callback)(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active))
    {
    bacnet_time_callback = callback;
    }


// Initialize datetime system
void datetime_init(void) {
    // Nothing needed for now; must set time in each board sketch
}


/**
 * @brief Retrieve current local date and time
 *
 * Provides the current local date and time to the BACnet stack.
 * This function is used by BACnet services and properties requiring
 * access to the device clock (Local Date, Local Time, TimeSync, etc.).
 *
 * The implementation typically forwards the request to a
 * platform-specific time provider callback.
 *
 * @param[out] bdate Pointer to BACNET_DATE structure to populate
 * @param[out] btime Pointer to BACNET_TIME structure to populate
 * @param[out] utc_offset_minutes Local UTC offset in minutes
 * @param[out] dst_active Daylight Saving Time status
 *
 * @return true if valid date/time information was obtained
 * @return false if no valid time source is available
 *
 * @note The returned time should originate from the device's
 *       authoritative clock source (typically RTC or system time).
 *
 * @note External time services such as NTP should synchronize
 *       the device clock rather than be queried directly here.
 */
bool datetime_local(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    if (!bdate || !btime || !utc_offset_minutes || !dst_active)
        return false;
    if (bacnet_time_callback) {
        return bacnet_time_callback(bdate, btime, utc_offset_minutes, dst_active);
    }
    return false;
}
