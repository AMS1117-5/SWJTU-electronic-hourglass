#ifndef MENU_H
#define MENU_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    MENU_HOURGLASS = 0,
    MENU_OCEAN,
    MENU_TIME,
    MENU_BRIGHTNESS,
    MENU_COUNT
} MenuItem;

void Menu_Select(MenuItem item);
MenuItem Menu_GetSelection(void);
/* K1/K2 short: previous/next. Returns true on K1 long confirmation. */
bool Menu_HandleEvents(uint8_t events);
void Menu_Render(uint32_t now);

#endif
