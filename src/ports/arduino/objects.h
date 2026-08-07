#ifndef PORTS_OBJECTS_H
#define PORTS_OBJECTS_H


#include <Arduino.h>

#include "bacnet/bacdef.h"

/**
 * @brief Create a BACnet Analog Value object
 *
 * Creates an Analog Value object with the specified instance and initial value.
 *
 * @param instance Object instance number
 * @param name Object name (must remain valid or be copied internally)
 * @param description Object description
 * @param units Engineering units (BACnet standard units)
 * @param initialValue Initial present value
 */
void createAnalogValue(
    uint32_t instance,
    const char *name,
    const char *description,
    BACNET_ENGINEERING_UNITS units,
    float initialValue);


    /**
 * @brief Create a BACnet Analog Input object
 *
 * Represents a read-only analog input value from the field.
 *
 * @param instance Object instance number
 * @param name Object name
 * @param description Object description
 * @param units Engineering units (BACnet standard units)
 */
void createAnalogInput(
    uint32_t instance,
    const char *name,
    const char *description,
    BACNET_ENGINEERING_UNITS units);


/**
 * @brief Create a BACnet Binary Input object
 *
 * Represents a read-only binary input value (active/inactive).
 *
 * @param instance Object instance number
 * @param name Object name
 * @param description Object description
 * @param initialValue Initial state (active/inactive)
 */
void createBinaryInput(
    uint32_t instance,
    const char *name,
    const char *description,
    BACnetBinaryPV initialValue);


/**
 * @brief Create a BACnet Binary Value object
 *
 * Represents an internal writable binary value.
 *
 * @param instance Object instance number
 * @param name Object name
 * @param description Object description
 * @param initialValue Initial state (active/inactive)
 */ 
void createBinaryValue(
    uint32_t instance,
    const char *name,
    const char *description,
    BACnetBinaryPV initialValue);


/**
 * @brief Create a BACnet Analog Output object
 *
 * Represents a writable analog output with priority array support.
 *
 * @param instance Object instance number
 * @param name Object name
 * @param description Object description
 * @param units Engineering units (BACnet standard units)
 * @param initialValue Initial present value
 */
void createAnalogOutput(
    uint32_t instance,
    const char *name,
    const char *description,
    BACNET_ENGINEERING_UNITS units,
    float initialValue);


    /**
 * @brief Create a BACnet Binary Output object
 *
 * Represents a writable binary output with priority array support.
 *
 * @param instance Object instance number
 * @param name Object name
 * @param description Object description
 * @param initialValue Initial state (active/inactive)
 */
void createBinaryOutput(
    uint32_t instance,
    const char *name,
    const char *description,
    BACnetBinaryPV initialValue);


/**
 * @brief Create a BACnet Multi-State Input object
 *
 * Represents a read-only multi-state input value.
 *
 * @param instance Object instance number
 * @param name Object name
 * @param description Object description
 * @param states List of state names (null-separated string list)
 * @param initialValue Initial state index (1-based BACnet index)
 */
void createMultiStateInput(
    uint32_t instance,
    const char *name,
    const char *description,
    const char states[],
    uint8_t initialValue);


/**
 * @brief Create a BACnet Multi-State Output object
 *
 * Represents a writable multi-state output value.
 *
 * @param instance Object instance number
 * @param name Object name
 * @param description Object description
 * @param states List of state names (null-separated string list)
 * @param initialValue Initial state index (1-based BACnet index)
 */
void createMultiStateOutput(
    uint32_t instance,
    const char *name,
    const char *description,
    const char states[],
    uint8_t initialValue);


/**
 * @brief Create a BACnet Multi-State Value object
 *
 * Represents an internal writable multi-state value.
 *
 * @param instance Object instance number
 * @param name Object name
 * @param description Object description
 * @param states List of state names (null-separated string list)
 * @param initialValue Initial state index (1-based BACnet index)
 */
void createMultiStateValue(
    uint32_t instance, 
    const char *name,
    const char *description,
    const char states[],
    uint8_t initialValue);


#endif // PORTS_OBJECTS_H
