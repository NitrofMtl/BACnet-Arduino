#include "ports/arduino/bacnet.h"

//mstp
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/mstp.h"
#include "bacnet/datalink/dlmstp.h"

#include "bacnet/basic/object/device.h"
//#include "ports/arduino/bacnet_udp_port.h"
//#include "bacnet/datalink/bip.h"
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

static uint8_t mstpInputBuffer[MSTP_INPUT_BUF_SIZE];
static uint8_t mstpOutputBuffer[MSTP_OUTPUT_BUF_SIZE];

static dlmstp_rs485_driver mstp_driver = {};
static dlmstp_user_data_t user_data = {};
static mstp_port_struct_t Mstp_port = {};


void Sprintf(const char *format, ...)
{
    char buffer[256];

    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);
    Serial.print(buffer);
}


void mstp_callback_send_set(void (*cb)(const uint8_t *p, uint16_t len))
{
    mstp_driver.send = cb;
}


void mstp_callback_read_set(bool (*cb)(uint8_t *b))
{
    mstp_driver.read = cb;
}


void mstp_callback_transmitting_set(bool (*cb)())
{
    mstp_driver.transmitting = cb;
}

static uint32_t silence_reference = 0;

static uint32_t mstp_silence_timer()
{
    return millis() - silence_reference;
}


void mstp_silence_reset()
{
    silence_reference = millis();
}


static uint32_t mstp_baud = 0;


static uint32_t mstp_baud_rate()
{
    return mstp_baud; // your fixed baud
}

bool mstp_baud_rate_set(uint32_t baud)
{
    mstp_baud = baud;
    return true;
}

static void mstp_init(){}

void init_bacnet_mstp(int8_t mac)
{
    //need to select datalink only in BACDL_MULTIPLE
    //datalink_set((char*)"mstp");  //Select MS/TP as datalink

    mstp_port_struct_t *mstp_port = &Mstp_port;
    mstp_port->This_Station = mac;

    mstp_port->InputBuffer = mstpInputBuffer;
    mstp_port->InputBufferSize = MSTP_INPUT_BUF_SIZE;

    mstp_port->OutputBuffer = mstpOutputBuffer;
    mstp_port->OutputBufferSize = MSTP_OUTPUT_BUF_SIZE;

    mstp_port->Nmax_master = 1; //default
    mstp_port->Nmax_info_frames = 1;

    mstp_port->SlaveNodeEnabled = false;

    mstp_driver.init = mstp_init;
    mstp_driver.silence_milliseconds = mstp_silence_timer;
    mstp_driver.silence_reset = mstp_silence_reset;
    mstp_driver.baud_rate = mstp_baud_rate;
    mstp_driver.baud_rate_set =  mstp_baud_rate_set;
    
    user_data.RS485_Driver = &mstp_driver;
    mstp_port->UserData = &user_data;
    user_data.Initialized = true;

    mstp_port->ZeroConfigEnabled = true;
    /* use the libc srand() and rand() generated random number*/
    MSTP_Zero_Config_UUID_Init(mstp_port);
   
    dlmstp_set_interface((const char *)mstp_port);
    dlmstp_user_data_t *u = (dlmstp_user_data_t *)mstp_port->UserData;
    u->Initialized = false;
    
    dlmstp_init((char*)mstp_port);

    uint32_t turnaround_ms = 2;//(40UL * 1000UL + mstp_baud_rate() - 1) / mstp_baud_rate();
    mstp_port->Tturnaround_timeout = turnaround_ms;
}


void mstp_nmax_master_set(uint8_t nmax_master)
{
    Mstp_port.Nmax_master = nmax_master;
}


void bacnet_Poll_mstp()
{
    BACNET_ADDRESS src;
    static uint8_t pdu[512];
    uint16_t len;

    len = dlmstp_receive(&src, pdu, sizeof(pdu), 0);

    if (len > 0) {
        BACNET_NPDU_DATA npdu_data;

        int apdu_offset = bacnet_npdu_decode(&pdu[0], len, &src, NULL, &npdu_data);
        if (apdu_offset > 0 && len > apdu_offset) {
            apdu_handler(&src, &pdu[apdu_offset], len - apdu_offset);
        }
    }

    bacnet_handle_reinitialize();
    BACnetSchedule::handler();
}
