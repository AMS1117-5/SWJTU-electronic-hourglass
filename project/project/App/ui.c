#include "ui.h"

#include "display.h"

static const uint8_t digits[10][5] = {
    {7U, 5U, 5U, 5U, 7U}, {2U, 6U, 2U, 2U, 7U},
    {7U, 1U, 7U, 4U, 7U}, {7U, 1U, 7U, 1U, 7U},
    {5U, 5U, 7U, 1U, 1U}, {7U, 4U, 7U, 1U, 7U},
    {7U, 4U, 7U, 5U, 7U}, {7U, 1U, 2U, 2U, 2U},
    {7U, 5U, 7U, 5U, 7U}, {7U, 5U, 7U, 1U, 7U}
};

static const uint8_t icons[UI_ICON_COUNT][7] = {
    {0x7FU, 0x22U, 0x14U, 0x08U, 0x14U, 0x22U, 0x7FU}, /* Hourglass */
    {0x08U, 0x08U, 0x14U, 0x14U, 0x22U, 0x22U, 0x1CU}, /* Water drop */
    {0x1CU, 0x22U, 0x49U, 0x4DU, 0x41U, 0x22U, 0x1CU}, /* Clock */
    {0x49U, 0x2AU, 0x1CU, 0x7FU, 0x1CU, 0x2AU, 0x49U}, /* Sun */
    {0x7FU, 0x40U, 0x40U, 0x7CU, 0x40U, 0x40U, 0x7FU}  /* E */
};

void Ui_DrawDigit(uint8_t digit, uint8_t row, uint8_t column)
{
    if (digit > 9U)
    {
        return;
    }
    for (uint8_t r = 0U; r < 5U; r++)
    {
        for (uint8_t c = 0U; c < 3U; c++)
        {
            Display_SetPixel((uint8_t)(row + r), (uint8_t)(column + c),
                             (digits[digit][r] & (1U << (2U - c))) != 0U);
        }
    }
}

void Ui_DrawIcon(UiIcon icon, uint8_t row)
{
    if ((unsigned)icon >= UI_ICON_COUNT)
    {
        return;
    }
    for (uint8_t r = 0U; r < 7U; r++)
    {
        for (uint8_t c = 0U; c < 7U; c++)
        {
            Display_SetPixel((uint8_t)(row + r), c,
                             (icons[icon][r] & (1U << (6U - c))) != 0U);
        }
    }
}

static void draw_pair(uint8_t value, uint8_t row)
{
    Ui_DrawDigit((uint8_t)(value / 10U), row, 0U);
    Ui_DrawDigit((uint8_t)(value % 10U), row, 4U);
}

void Ui_RenderTime(uint16_t duration_sec)
{
    Display_Clear();
    draw_pair((uint8_t)(duration_sec / 60U), 1U);
    draw_pair((uint8_t)(duration_sec % 60U), 9U);
    /* Separator between upper minutes and lower seconds. */
    Display_SetPixel(7U, 3U, true);
    Display_SetPixel(8U, 3U, true);
}

void Ui_RenderBrightness(uint8_t level)
{
    Display_Clear();
    Ui_DrawIcon(UI_ICON_BRIGHTNESS, 0U);
    Ui_DrawDigit(level, 10U, 2U);
}

void Ui_RenderError(uint8_t code)
{
    Display_Clear();
    Ui_DrawIcon(UI_ICON_ERROR, 0U);
    Ui_DrawDigit(code, 10U, 2U);
}
