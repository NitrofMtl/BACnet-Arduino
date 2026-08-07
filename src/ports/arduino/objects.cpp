#include "ports/arduino/objects.h"

#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bv.h"
#include "bacnet/basic/object/ao.h"
#include "bacnet/basic/object/bo.h"

#include "bacnet/basic/object/ms-input.h"
#include "bacnet/basic/object/mso.h"
#include "bacnet/basic/object/msv.h"


void createAnalogValue(
    uint32_t instance,
    const char *name,
    const char *description,
    BACNET_ENGINEERING_UNITS units,
    float initialValue)
{
    Analog_Value_Create(instance);

    Analog_Value_Name_Set(instance, name);

    Analog_Value_Description_Set(instance, description);

    Analog_Value_Units_Set(instance, units);

    Analog_Value_Present_Value_Set(
        instance,
        initialValue,
        16
    );
}


void createAnalogInput(
    uint32_t instance,
    const char *name,
    const char *description,
    BACNET_ENGINEERING_UNITS units)
{
    Analog_Input_Create(instance);

    Analog_Input_Name_Set(instance, name);

    Analog_Input_Description_Set(instance, description);

    Analog_Input_Units_Set(instance, units);
}


void createBinaryInput(
    uint32_t instance,
    const char *name,
    const char *description,
    BACnetBinaryPV initialValue)
{
    Binary_Input_Create(instance);

    Binary_Input_Name_Set(instance, name);

    Binary_Input_Description_Set(instance, description);

    Binary_Input_Present_Value_Set(instance, initialValue);
}


void createBinaryValue(
    uint32_t instance,
    const char *name,
    const char *description,
    BACnetBinaryPV initialValue)
{
    Binary_Value_Create(instance);

    Binary_Value_Name_Set(instance, name);

    Binary_Value_Description_Set(instance, description);

    Binary_Value_Present_Value_Set(instance, initialValue);
}


void createAnalogOutput(
    uint32_t instance,
    const char *name,
    const char *description,
    BACNET_ENGINEERING_UNITS units,
    float initialValue)
{
    Analog_Output_Create(instance);

    Analog_Output_Name_Set(instance, name);

    Analog_Output_Description_Set(instance, description);

    Analog_Output_Units_Set(instance, units);

    Analog_Output_Present_Value_Set(
        instance,
        initialValue,
        16
    );
}


void createBinaryOutput(
    uint32_t instance,
    const char *name,
    const char *description,
    BACnetBinaryPV initialValue)
{
    Binary_Output_Create(instance);

    Binary_Output_Name_Set(instance, name);

    Binary_Output_Description_Set(instance, description);

    Binary_Output_Present_Value_Set(instance, initialValue, 16);
}


void createMultiStateInput(
    uint32_t instance,
    const char *name,
    const char *description,
    const char states[],
    uint8_t initialValue)
{
    Multistate_Input_Create(instance);

    Multistate_Input_Name_Set(instance, name);

    Multistate_Input_Description_Set(instance, description);

    Multistate_Input_State_Text_List_Set(instance, states);

    Multistate_Input_Present_Value_Set(instance, initialValue);
}


void createMultiStateOutput(
    uint32_t instance,
    const char *name,
    const char *description,
    const char states[],
    uint8_t initialValue)
{
    Multistate_Output_Create(instance);

    Multistate_Output_Name_Set(instance, name);

    Multistate_Output_Description_Set(instance, description);

    Multistate_Output_State_Text_List_Set(instance, states);

    Multistate_Output_Present_Value_Set(instance, initialValue, 16);
}


void createMultiStateValue(
    uint32_t instance,
    const char *name,
    const char *description,
    const char states[],
    uint8_t initialValue)
{
    Multistate_Value_Create(instance);

    Multistate_Value_Name_Set(instance, name);

    Multistate_Value_Description_Set(instance, description);

    Multistate_Value_State_Text_List_Set(instance, states);

    Multistate_Value_Present_Value_Set(instance, initialValue);
}