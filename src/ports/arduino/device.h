#include <Arduino.h>

#ifndef DEVICE_PORT_H
#define DEVICE_PORT_H

#include "bacnet/basic/object/device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the port-specific BACnet object table.
 *
 * Returns the table of BACnet object handlers registered by the
 * current platform port. This table is used internally by the
 * BACnet stack during initialization and object dispatch.
 *
 * @return Pointer to the port object table.
 *
 * @note This function is intended for internal use by the BACnet
 *       library. Applications normally do not need to call it.
 */
object_functions_t* get_Port_Object_Table(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

#endif