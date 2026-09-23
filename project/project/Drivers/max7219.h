#ifndef MAX7219_H
#define MAX7219_H

#include <stdint.h>

#define MAX7219_DEVICE_COUNT 2U

void Max7219_Init(uint8_t intensity);
void Max7219_SetIntensity(uint8_t intensity);
void Max7219_WriteRegister(uint8_t reg, const uint8_t values[MAX7219_DEVICE_COUNT]);

#endif
