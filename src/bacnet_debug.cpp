// bacnet_debug.cpp
#include <Arduino.h>
#include "bacnet_debug.h"

extern "C" void bacnet_debug(const char *msg)
{
    Serial.println(msg);
    
}

extern "C" void pinWrite(uint8_t pin, uint8_t value)
{
    digitalWrite(pin, value);
}