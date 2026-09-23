#include <assert.h>
#include <stdio.h>

#include "buttons.h"
#include "display.h"
#include "max7219.h"
#include "menu.h"
#include "settings.h"
#include "ui.h"

void Max7219_Init(uint8_t intensity) { (void)intensity; }
void Max7219_SetIntensity(uint8_t intensity) { (void)intensity; }
void Max7219_WriteRegister(uint8_t reg, const uint8_t values[2]) { (void)reg; (void)values; }

static void check_digit(const uint8_t glyph[5], unsigned row, unsigned column)
{
    for (unsigned r = 0; r < 5U; r++)
        for (unsigned c = 0; c < 3U; c++)
            assert(Display_GetPixel(row + r, column + c) ==
                   ((glyph[r] & (1U << (2U - c))) != 0U));
}

int main(void)
{
    const uint8_t zero[5] = {7, 5, 5, 5, 7};
    const uint8_t two[5] = {7, 1, 7, 4, 7};
    const uint8_t three[5] = {7, 1, 7, 1, 7};
    const uint8_t four[5] = {5, 5, 7, 1, 1};
    Settings_Init();
    assert(Settings_GetGrainIntervalMs() == 10000U && Settings_GetDurationSec() == 240U);
    assert(Settings_GetBrightness() == 0U);
    Settings_AdjustTime(true);
    assert(Settings_GetGrainIntervalMs() == 10000U);
    for (unsigned seconds = 240U; seconds >= 30U; seconds -= 30U)
    {
        assert(Settings_GetGrainIntervalMs() == seconds * 1000U / 24U);
        assert(Settings_GetDurationSec() == seconds);
        Settings_AdjustTime(false);
    }
    assert(Settings_GetGrainIntervalMs() == 1250U);
    assert(!Settings_SetGrainIntervalMs(0U));
    assert(!Settings_SetGrainIntervalMs(1249U));
    assert(!Settings_SetGrainIntervalMs(1500U));
    assert(!Settings_SetGrainIntervalMs(10001U));
    assert(Settings_GetGrainIntervalMs() == 1250U);
    assert(Settings_SetGrainIntervalMs(3750U));
    assert(Settings_GetDurationSec() == 90U);
    for (unsigned i = 0; i < 20U; i++) Settings_AdjustTime(true);
    assert(Settings_GetGrainIntervalMs() == 10000U);
    for (unsigned i = 0; i < 10U; i++) Settings_AdjustBrightness(true);
    assert(Settings_GetBrightness() == 2U);
    assert(!Settings_SetBrightness(3U));
    for (unsigned i = 0; i < 10U; i++) Settings_AdjustBrightness(false);
    assert(Settings_GetBrightness() == 0U);

    Ui_RenderTime(240U);
    check_digit(zero, 1U, 0U);
    check_digit(four, 1U, 4U);
    check_digit(zero, 9U, 0U);
    check_digit(zero, 9U, 4U);
    Ui_RenderTime(210U);
    check_digit(zero, 1U, 0U);
    check_digit(three, 1U, 4U);
    check_digit(three, 9U, 0U);
    check_digit(zero, 9U, 4U);
    Ui_RenderTime(30U);
    check_digit(zero, 1U, 0U);
    check_digit(zero, 1U, 4U);
    check_digit(three, 9U, 0U);
    check_digit(zero, 9U, 4U);
    Ui_RenderBrightness(2U);
    check_digit(two, 10U, 2U);

    Menu_Select(MENU_HOURGLASS);
    assert(!Menu_HandleEvents(BUTTON_EVENT_K1_SHORT));
    assert(Menu_GetSelection() == MENU_BRIGHTNESS);
    assert(!Menu_HandleEvents(BUTTON_EVENT_K2_SHORT));
    assert(Menu_GetSelection() == MENU_HOURGLASS);
    for (unsigned i = 0; i < 3U; i++) Menu_HandleEvents(BUTTON_EVENT_K2_SHORT);
    assert(Menu_GetSelection() == MENU_BRIGHTNESS);
    assert(Menu_HandleEvents(BUTTON_EVENT_K1_LONG | BUTTON_EVENT_K2_SHORT));
    assert(Menu_GetSelection() == MENU_BRIGHTNESS);
    assert(!Menu_HandleEvents(BUTTON_EVENT_K2_LONG));
    assert(Menu_GetSelection() == MENU_BRIGHTNESS);
    Menu_Render(0U);
    check_digit(four, 10U, 2U);
    assert(Display_GetPixel(15U, 6U));
    Menu_Render(400U);
    assert(!Display_GetPixel(15U, 6U));
    Settings_SetBrightness(2U);
    Settings_SetGrainIntervalMs(1250U);
    Settings_Init();
    assert(Settings_GetBrightness() == 0U && Settings_GetGrainIntervalMs() == 10000U);
    puts("PASS: settings limits/steps/defaults, 3x5 MM/SS, brightness page, four-item cyclic menu and confirmation priority");
    return 0;
}
