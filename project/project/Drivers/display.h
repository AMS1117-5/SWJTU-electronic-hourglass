#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

void Display_Init(void);
void Display_SetBrightness(uint8_t level);
uint8_t Display_GetBrightness(void);
void Display_Clear(void);
void Display_Fill(void);
void Display_SetPixel(uint8_t x, uint8_t y, bool state);
bool Display_GetPixel(uint8_t x, uint8_t y);
void Display_Update(void);

#endif
