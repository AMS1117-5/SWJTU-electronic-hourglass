#include "app.h"

#include <stdbool.h>

#include "app_config.h"
#include "buttons.h"
#include "buzzer.h"
#include "display.h"
#include "hourglass.h"
#include "menu.h"
#include "mpu6050.h"
#include "ocean.h"
#include "settings.h"
#include "ui.h"

static AppMode mode;
static uint32_t last_display_at;

static void enter_mode(AppMode next, uint32_t now)
{
    if (next == APP_MENU)
    {
        if (mode == APP_HOURGLASS)
        {
            Menu_Select(MENU_HOURGLASS);
        }
        else if (mode == APP_OCEAN)
        {
            Menu_Select(MENU_OCEAN);
        }
        /* Settings return to their existing selection, not the first item. */
    }
    else if (next == APP_HOURGLASS)
    {
        Hourglass_Resume(now);
    }
    else if (next == APP_OCEAN)
    {
        Ocean_Resume(now);
    }
    mode = next;
}

static uint8_t sensor_error(void)
{
    switch (Mpu6050_GetStatus())
    {
        case MPU6050_BUS_ERROR: return 1U;
        case MPU6050_ID_ERROR: return 2U;
        case MPU6050_DATA_ERROR: return 3U;
        default: return 0U;
    }
}

static void render(uint32_t now)
{
    uint8_t error = sensor_error();
    if ((mode == APP_HOURGLASS || mode == APP_OCEAN) && error != 0U)
    {
        Ui_RenderError(error);
    }
    else
    {
        switch (mode)
        {
            case APP_HOURGLASS: Hourglass_Render(); break;
            case APP_OCEAN: Ocean_Render(); break;
            case APP_MENU: Menu_Render(now); break;
            case APP_SETTINGS: Ui_RenderTime(Settings_GetDurationSec()); break;
            case APP_BRIGHTNESS: Ui_RenderBrightness(Settings_GetBrightness()); break;
        }
    }
    Display_Update();
    last_display_at = now;
}

void App_Init(uint32_t now)
{
    Settings_Init();
    Display_Init();
    Display_SetBrightness(Settings_GetBrightness());
    Hourglass_Init(now);
    (void)Hourglass_SetGrainIntervalMs(Settings_GetGrainIntervalMs());
    Ocean_Init(now);
    Menu_Select(MENU_HOURGLASS);
    mode = APP_HOURGLASS;
    last_display_at = now;
    render(now);
}

void App_Update(uint32_t now)
{
    uint8_t events = Buttons_TakeEvents();
    bool changed = false;

    switch (mode)
    {
        case APP_HOURGLASS:
        case APP_OCEAN:
            if ((events & BUTTON_EVENT_K1_LONG) != 0U)
            {
                enter_mode(APP_MENU, now);
                changed = true;
            }
            else if (mode == APP_HOURGLASS)
            {
                Hourglass_Update(now);
                if (Hourglass_TakeCompletionEvent())
                {
                    Buzzer_StartAlarm(now);
                }
            }
            else
            {
                Ocean_Update(now);
            }
            break;

        case APP_MENU:
            if (Menu_HandleEvents(events))
            {
                switch (Menu_GetSelection())
                {
                    case MENU_HOURGLASS: enter_mode(APP_HOURGLASS, now); break;
                    case MENU_OCEAN: enter_mode(APP_OCEAN, now); break;
                    case MENU_TIME: enter_mode(APP_SETTINGS, now); break;
                    case MENU_BRIGHTNESS: enter_mode(APP_BRIGHTNESS, now); break;
                    default: break;
                }
            }
            changed = events != 0U;
            break;

        case APP_SETTINGS:
        case APP_BRIGHTNESS:
            if ((events & BUTTON_EVENT_K1_LONG) != 0U)
            {
                enter_mode(APP_MENU, now);
                changed = true;
            }
            else if ((events & (BUTTON_EVENT_K1_SHORT | BUTTON_EVENT_K2_SHORT)) != 0U)
            {
                bool increase = (events & BUTTON_EVENT_K1_SHORT) != 0U;
                if (mode == APP_SETTINGS)
                {
                    Settings_AdjustTime(increase);
                    (void)Hourglass_SetGrainIntervalMs(Settings_GetGrainIntervalMs());
                }
                else
                {
                    Settings_AdjustBrightness(increase);
                    Display_SetBrightness(Settings_GetBrightness());
                }
                changed = true;
            }
            break;
    }

    /* Menu/settings modes never call physics updates: both effects keep state. */
    if (changed || (uint32_t)(now - last_display_at) >= DISPLAY_REFRESH_PERIOD_MS)
    {
        render(now);
    }
}

AppMode App_GetMode(void) { return mode; }
