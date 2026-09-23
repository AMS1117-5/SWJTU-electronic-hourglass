#include "menu.h"

#include "buttons.h"
#include "display.h"
#include "ui.h"

static MenuItem selected;

void Menu_Select(MenuItem item)
{
    if ((unsigned)item < MENU_COUNT)
    {
        selected = item;
    }
}

MenuItem Menu_GetSelection(void) { return selected; }

bool Menu_HandleEvents(uint8_t events)
{
    if ((events & BUTTON_EVENT_K1_LONG) != 0U)
    {
        return true;
    }
    if ((events & BUTTON_EVENT_K1_SHORT) != 0U)
    {
        selected = (MenuItem)((selected + MENU_COUNT - 1U) % MENU_COUNT);
    }
    else if ((events & BUTTON_EVENT_K2_SHORT) != 0U)
    {
        selected = (MenuItem)((selected + 1U) % MENU_COUNT);
    }
    return false;
}

void Menu_Render(uint32_t now)
{
    static const UiIcon icons[MENU_COUNT] = {
        UI_ICON_HOURGLASS, UI_ICON_OCEAN, UI_ICON_TIME, UI_ICON_BRIGHTNESS
    };
    Display_Clear();
    Ui_DrawIcon(icons[selected], 0U);
    Ui_DrawDigit((uint8_t)(selected + 1U), 10U, 2U);
    if (((now / 400U) & 1U) == 0U)
    {
        Display_SetPixel(15U, (uint8_t)(selected * 2U), true);
    }
}
