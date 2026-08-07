#ifndef DATETIME_PORTS_H
#define DATETIME_PORTS_H

#include "bacnet/datetime.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register the callback used to obtain the current date and time.
 *
 * The callback is invoked by the BACnet stack whenever the current
 * date, time, UTC offset, or daylight saving status is required.
 *
 * @param callback Function that supplies the current time information.
 *                 The callback should return true if valid time data is
 *                 available, or false otherwise.
 *
 * @note The application is responsible for maintaining the system time
 *       (RTC, NTP, GPS, etc.).
 */
void bacnet_time_callback_set(bool (*callback)(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active));

#ifdef __cplusplus
}
#endif

#endif // DATETIME_PORTS_H