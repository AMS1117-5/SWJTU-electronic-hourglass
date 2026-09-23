#ifndef APP_H
#define APP_H

#include <stdint.h>

typedef enum
{
    APP_HOURGLASS,
    APP_OCEAN,
    APP_MENU,
    APP_SETTINGS,
    APP_BRIGHTNESS
} AppMode;

void App_Init(uint32_t now);
void App_Update(uint32_t now);
AppMode App_GetMode(void);

#endif
