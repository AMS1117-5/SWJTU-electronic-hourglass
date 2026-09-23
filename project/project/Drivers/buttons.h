#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BUTTON_K1 = 0,
    BUTTON_K2,
    BUTTON_COUNT
} ButtonId;

typedef enum
{
    BUTTON_EVENT_NONE     = 0U,
    BUTTON_EVENT_K1_SHORT = 1U << 0,
    BUTTON_EVENT_K1_LONG  = 1U << 1,
    BUTTON_EVENT_K2_SHORT = 1U << 2,
    BUTTON_EVENT_K2_LONG  = 1U << 3
} ButtonEvent;

void Buttons_Init(uint32_t now);
void Buttons_Update(uint32_t now);
uint8_t Buttons_TakeEvents(void);
bool Buttons_IsPressed(ButtonId button);

#endif
