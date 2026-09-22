/**
 * @file bacnet_core.h
 * @brief High-level initialization and configuration interface for the
 *        BACnet stack.
 *
 * This module provides helper functions for configuring the BACnet
 * Device object, initializing the BACnet stack, and handling
 * ReinitializeDevice requests.
 */

#ifndef BACNET_CORE_H
#define BACNET_CORE_H

#include <Arduino.h>

#include "bacnet/bacdef.h"

// BACnet Stack API 
#include "bacnet/apdu.h"//
#include "bacnet/bacdcode.h"//
#include "bacnet/bactext.h"//
#include "bacnet/dcc.h"
#include "bacnet/getevent.h"
#include "bacnet/iam.h"//
#include "bacnet/npdu.h"//
#include "bacnet/version.h"//
//oject and service handlers
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/service/h_apdu.h"
#include "bacnet/basic/service/h_whois.h"
#include "bacnet/basic/service/h_iam.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/service/h_wp.h"
#include "bacnet/basic/service/h_ts.h"


//handlers
#include "bacnet/basic/service/h_rp.h"
#include "bacnet/basic/npdu/h_npdu.h"
#include "bacnet/basic/service/h_rp_a.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/abort.h"
#include "bacnet/bacerror.h"
#include "bacnet/datetime.h"

#include "bacnet/basic/service/h_cov.h"
#include "bacnet/basic/service/h_rpm.h"
#include "bacnet/basic/service/h_dcc.h"

//objects
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bv.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/ao.h"
#include "bacnet/basic/object/bo.h"
#include "bacnet/basic/object/ms-input.h"
#include "bacnet/basic/object/mso.h"
#include "bacnet/basic/object/msv.h"

//port libs
//#include "ports/arduino/bacnet_udp_port.h"
#include "ports/arduino/device.h"
//#include "ports/arduino/bacnet.h"
#include "ports/arduino/objects.h"
#include "ports/arduino/datetime_ports.h"
#include "ports/arduino/Schedule.h"

/**
 * @brief BACnet device information.
 *
 * Defines the properties used to configure the BACnet Device object
 * during initialization.
 */
typedef struct 
{
    const uint32_t instance_number;
    const char* name;
    const char* description;
    const char* location;
    const char* vendor_name;
    const char* model_name;
    const char* firmware_revision;
    const char* application_software_version;
}deviceInfos_t;

/**
 * @brief Update the BACnet Device object information.
 *
 * Copies the specified device information into the BACnet Device object.
 *
 * @param infos Pointer to the device information structure.
 */
void Device_Infos_Set(const deviceInfos_t *infos);

/**
 * @brief Register a board-specific ReinitializeDevice callback.
 *
 * Registers the function that will be called when a BACnet
 * ReinitializeDevice request is received for the specified state.
 *
 * This allows each platform to implement the appropriate restart
 * behavior (for example, a hardware reset on STM32 devices).
 *
 * @param state BACnet reinitialize state.
 * @param callback Function implementing the requested reinitialization.
 *
 * @note The library does not perform the restart itself. The application
 * must provide the platform-specific implementation.
 */
void bacnet_reinitialize_callback_set(BACNET_REINITIALIZED_STATE state, void(*callback)());

/**
 * @brief Process pending ReinitializeDevice requests.
 *
 * @note Called internally by the BACnet stack. Applications should not
 * call this function directly.
 */
void bacnet_handle_reinitialize();

/**
 * @brief Initialize the BACnet stack.
 *
 * Initializes the BACnet Device object and all supported BACnet services.
 *
 * @param deviceInfos Pointer to the BACnet device information.
 */
void bacnet_init(const deviceInfos_t *deviceInfos);

void setTimeCallback_set(void (*cb)(time_t));
void bacnet_sync_time(BACNET_DATE *bdate, BACNET_TIME *btime, bool UTC);
bool bacnet_get_time(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active);


#endif