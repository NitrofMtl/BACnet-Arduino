// bacnet_debug.h
#ifndef BACNET_DEBUG_H
#define BACNET_DEBUG_H

#include <Arduino.h>


#ifdef __cplusplus
extern "C" {
#endif

void bacnet_debug(const char *msg);

void pinWrite(uint8_t pin, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* BACNET_DEBUG_H */