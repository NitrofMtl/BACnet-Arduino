#include "ports/arduino/bacnet_core.h"

#include "bacnet/basic/object/device.h"
#include "ports/arduino/bacnet_udp_port.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/basic/npdu/h_npdu.h"
#include "bacnet/basic/service/h_apdu.h"
#include "bacnet/basic/service/h_whois.h"
#include "bacnet/basic/service/h_iam.h"
#include "bacnet/basic/service/h_rp.h"
#include "bacnet/bacapp.h"
#include "bacnet/basic/service/h_wp.h"
#include "bacnet/basic/service/h_cov.h"
#include "bacnet/basic/service/h_rpm.h"
#include "bacnet/basic/service/h_dcc.h"
#include "bacnet/basic/service/h_ts.h"
#include "bacnet/basic/service/h_rd.h"
#include "bacnet/basic/object/schedule.h"

#include "ports/arduino/device.h"


void bacnet_init(const deviceInfos_t *deviceInfos)
{
    Device_Init(get_Port_Object_Table()); // initialize objects with our custom object table
    Device_Infos_Set(deviceInfos); // set device information from provided structure

    // This function should be called in the setup() function of your Arduino sketch
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_bind);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, handler_timesync);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, handler_timesync_utc);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);
    //apdu_set_confirmed_handler(SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY, NO HANDLER);
    //apdu_set_confirmed_handler(SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY_MULTIPLE, NO HANDLER);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL, handler_device_communication_control);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);
    Schedule_Init();
}


static BACNET_CHARACTER_STRING Device_Name;


void Device_Infos_Set(const deviceInfos_t *infos)
{
    if (infos->instance_number) {
        Device_Set_Object_Instance_Number(infos->instance_number);
    }
    if (infos->name) {
        characterstring_init_ansi(&Device_Name, infos->name);
        Device_Set_Object_Name(&Device_Name);
    }
    if (infos->vendor_name) {
        Device_Set_Vendor_Name(infos->vendor_name, strlen(infos->vendor_name));
    }
    if (infos->model_name) {
        Device_Set_Model_Name(infos->model_name, strlen(infos->model_name));
    }
    if (infos->firmware_revision) {
        Device_Set_Firmware_Revision(infos->firmware_revision, strlen(infos->firmware_revision));
    }
    if (infos->application_software_version) {
        Device_Set_Application_Software_Version(infos->application_software_version, strlen(infos->application_software_version));
    }
    if (infos->description) {
        Device_Set_Description(infos->description, strlen(infos->description));
    }
    if (infos->location) {
        Device_Set_Location(infos->location, strlen(infos->location));
    }
}


/* User callbacks executed when a BACnet Reinitialize Device request
   is received and processed by the stack. */
static void (*reinitialize_coldstart_callback)() = nullptr;
static void (*reinitialize_warmstart_callback)() = nullptr;
static void (*reinitialize_activate_change_callback)() = nullptr;


void bacnet_reinitialize_callback_set(BACNET_REINITIALIZED_STATE state, void(*callback)())
{
   switch (state) {
        case BACNET_REINIT_COLDSTART:
            reinitialize_coldstart_callback = callback;
            break;
        case BACNET_REINIT_WARMSTART:
            reinitialize_warmstart_callback = callback;
            break;
        case BACNET_REINIT_ACTIVATE_CHANGES:
            reinitialize_activate_change_callback = callback;
            break;
        case BACNET_REINIT_IDLE:
            // Nothing to do
            break;
        default:
            break;
   }
}


void bacnet_handle_reinitialize()
{
    BACNET_REINITIALIZED_STATE state = Device_Reinitialized_State();
    switch (state) {
        case BACNET_REINIT_COLDSTART:
            Device_Reinitialize_State_Set(BACNET_REINIT_IDLE);
            if (reinitialize_coldstart_callback) {
                reinitialize_coldstart_callback();
            }
            break;
        case BACNET_REINIT_WARMSTART:
            Device_Reinitialize_State_Set(BACNET_REINIT_IDLE);
            if (reinitialize_warmstart_callback) {
                reinitialize_warmstart_callback();
            }
            break;
        case BACNET_REINIT_ACTIVATE_CHANGES:
            Device_Reinitialize_State_Set(BACNET_REINIT_IDLE);
            if (reinitialize_activate_change_callback) {
                reinitialize_activate_change_callback();
            }
            break;
        case BACNET_REINIT_IDLE:
            // Nothing to do
            break;
        default:
            break;
    }
}


