#ifndef UI_H
#define UI_H

#include <stdint.h>

typedef enum
{
    UI_ICON_HOURGLASS,
    UI_ICON_OCEAN,
    UI_ICON_TIME,
    UI_ICON_BRIGHTNESS,
    UI_ICON_ERROR,
    UI_ICON_COUNT
} UiIcon;

/* Upright UI in the calibrated virtual frame: x=row, y=column. */
void Ui_DrawIcon(UiIcon icon, uint8_t row);
void Ui_DrawDigit(uint8_t digit, uint8_t row, uint8_t column);
void Ui_RenderTime(uint16_t duration_sec);
void Ui_RenderBrightness(uint8_t level);
void Ui_RenderError(uint8_t code);

#endif
