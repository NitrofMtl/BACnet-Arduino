#include "ports/arduino/bacnet.h"

//mstp
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/mstp.h"
#include "bacnet/datalink/dlmstp.h"

#include "bacnet/basic/object/device.h"
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

#include "ports/arduino/device.h"
#include "ports/arduino/bacnet_core.h"

#define MSTP_INPUT_BUF_SIZE 512 //could be 600
#define MSTP_OUTPUT_BUF_SIZE 512 //could be 600

#define MSTP_DEFAULT_NMAX_MASTER 127

void Sprintf(const char *format, ...)
{
    char buffer[256];

    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);
    Serial.print(buffer);
}


struct MSTP_Instance {
    uint8_t inputBuffer[MSTP_INPUT_BUF_SIZE] {};
    uint8_t outputBuffer[MSTP_OUTPUT_BUF_SIZE] {};

    dlmstp_rs485_driver mstp_driver = {};
    dlmstp_user_data_t user_data = {};
    mstp_port_struct_t Mstp_port = {};

    uint32_t baudRate = 0;
    uint32_t silence_reference = 0;
};
static MSTP_Instance mstpInstances[BACNET_ROUTER_MSTP_PORTS_COUNT];


void mstp_callback_send_set(void (*cb)(const uint8_t *p, uint16_t len), uint8_t index)
{
    mstpInstances[index].mstp_driver.send = cb;
}


void mstp_callback_read_set(bool (*cb)(uint8_t *b), uint8_t index)
{
    mstpInstances[index].mstp_driver.read = cb;
}


void mstp_callback_transmitting_set(bool (*cb)(), uint8_t index)
{
    mstpInstances[index].mstp_driver.transmitting = cb;
}


uint8_t mstp_ActiveInstanceIndexGet()
{
    // get the active interface
    mstp_port_struct_t *active_port = (struct mstp_port_struct_t *)dlmstp_get_interface(); 

    for (uint8_t i = 0; i < BACNET_ROUTER_MSTP_PORTS_COUNT; i++) {
        if (&mstpInstances[i].Mstp_port == active_port) {
            return i;
        }
    }
    return 0; // default to first instance if not found
}


void mstp_ActiveInstanceIndexSet(uint8_t index)
{
    if (index >= BACNET_ROUTER_MSTP_PORTS_COUNT) {
        return;
    }

    dlmstp_set_interface(
        reinterpret_cast<const char *>(&mstpInstances[index].Mstp_port));
}


static uint32_t mstp_silence_timer()
{
    uint8_t index = mstp_ActiveInstanceIndexGet();
    return millis() - mstpInstances[index].silence_reference;
}


void mstp_silence_reset()
{
    uint8_t index = mstp_ActiveInstanceIndexGet();
    mstpInstances[index].silence_reference = millis();
}


static uint32_t mstp_baud_rate()
{
    uint8_t index = mstp_ActiveInstanceIndexGet();
    return mstpInstances[index].baudRate;
}

bool mstp_baud_rate_set(uint32_t baud)
{
    uint8_t index = mstp_ActiveInstanceIndexGet();
    mstpInstances[index].baudRate = baud;
    return true;
}


bool mstp_baud_rate_set(uint32_t baud, uint8_t index)
{
    mstpInstances[index].baudRate = baud;
    return true;
}


static void mstp_init(){}


void init_bacnet_mstp(int8_t mac, uint8_t index)
{
    mstp_port_struct_t *mstp_port = &mstpInstances[index].Mstp_port;
    mstp_port->This_Station = mac;

    mstp_port->InputBuffer = mstpInstances[index].inputBuffer;
    mstp_port->InputBufferSize = MSTP_INPUT_BUF_SIZE;

    mstp_port->OutputBuffer = mstpInstances[index].outputBuffer;
    mstp_port->OutputBufferSize = MSTP_OUTPUT_BUF_SIZE;

    mstp_port->Nmax_master = MSTP_DEFAULT_NMAX_MASTER;
    mstp_port->Nmax_info_frames = 1;

    mstp_port->SlaveNodeEnabled = false;

    dlmstp_rs485_driver *driver = &mstpInstances[index].mstp_driver;
    driver->init = mstp_init;
    driver->silence_milliseconds = mstp_silence_timer;
    driver->silence_reset = mstp_silence_reset;
    driver->baud_rate = mstp_baud_rate;
    driver->baud_rate_set =  mstp_baud_rate_set;
    
    dlmstp_user_data_t *user_data = &mstpInstances[index].user_data;
    user_data->RS485_Driver = driver;
    mstp_port->UserData = user_data;

    mstp_port->ZeroConfigEnabled = true;
    /* use the libc srand() and rand() generated random number*/
    MSTP_Zero_Config_UUID_Init(mstp_port);
   
    dlmstp_set_interface(reinterpret_cast<const char *>(mstp_port));
    dlmstp_user_data_t *u = (dlmstp_user_data_t *)mstp_port->UserData;
    u->Initialized = false;
    
    if (!dlmstp_init(reinterpret_cast<char *>(mstp_port))) {
        Serial.println("[ERROR] Failed to initialize MSTP port");
    }

    uint32_t turnaround_ms = (40UL * 1000UL + mstp_baud_rate() - 1) / mstp_baud_rate();
    mstp_port->Tturnaround_timeout = turnaround_ms;
}


void mstp_nmax_master_set(uint8_t nmax_master, uint8_t index)
{
    if (nmax_master > 127) {
        nmax_master = 127;
    }

    mstpInstances[index].Mstp_port.Nmax_master = nmax_master;
}


void bacnet_Poll_mstp()
{
    BACNET_ADDRESS src;
    static uint8_t pdu[512];
    uint16_t len;

    len = dlmstp_receive(&src, pdu, sizeof(pdu), 0);

    if (len > 0) {
        BACNET_NPDU_DATA npdu_data;

        int apdu_offset = bacnet_npdu_decode(&pdu[0], len, nullptr, &src, &npdu_data);
        if (apdu_offset > 0 && len > apdu_offset) {
            apdu_handler(&src, &pdu[apdu_offset], len - apdu_offset);
        }
    }

    bacnet_handle_reinitialize();
    BACnetSchedule::handler();
}

