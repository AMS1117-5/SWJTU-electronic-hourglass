#include "bringup.h"

#include <stdbool.h>

#include "app_config.h"
#include "buttons.h"
#include "display.h"
#include "imu_test.h"
#include "ocean.h"
#include "hourglass.h"
#include "buzzer.h"

typedef enum
{
    BRINGUP_SINGLE_PIXEL = 0,
    BRINGUP_ALL_ON,
    BRINGUP_CHECKERBOARD,
    BRINGUP_LEFT_MATRIX,
    BRINGUP_RIGHT_MATRIX,
    BRINGUP_BUTTON_MONITOR,
    BRINGUP_MODE_COUNT
} BringupMode;

static BringupMode current_mode;
static uint8_t scan_position;
static uint32_t last_scan_at;
static uint32_t last_display_at;
static uint32_t feedback_at;
static uint8_t feedback_keys;

/* Round 3: verify calibrated physical corners with charging port down.
 * Upper TL -> TR -> BR -> BL, then lower TL -> TR -> BR -> BL.
 */
static const uint8_t calibration_points[][2] = {
    {0U, 0U}, {0U, 7U}, {7U, 7U}, {7U, 0U},
    {8U, 0U}, {8U, 7U}, {15U, 7U}, {15U, 0U}
};
#define CALIBRATION_POINT_COUNT \
    (sizeof(calibration_points) / sizeof(calibration_points[0]))
static bool calibration_active;
static uint8_t calibration_index;
static bool sensor_test_active;
static bool ocean_active;
static bool hourglass_active;

/* Show the same key number on both matrices, independent of panel order. */
static void draw_key_feedback(void)
{
    static const uint8_t digits[2][5] = {
        {2U, 6U, 2U, 2U, 7U},
        {7U, 1U, 7U, 4U, 7U}
    };

    for (uint8_t matrix = 0U; matrix < 2U; matrix++)
    {
        for (uint8_t key = 0U; key < 2U; key++)
        {
            if ((feedback_keys & (1U << key)) == 0U)
            {
                continue;
            }
            uint8_t origin = (uint8_t)(matrix * 8U +
                                      (feedback_keys == 3U ? key * 4U : 2U));
            for (uint8_t y = 0U; y < 5U; y++)
            {
                for (uint8_t x = 0U; x < 3U; x++)
                {
                    Display_SetPixel((uint8_t)(origin + x), (uint8_t)(y + 1U),
                                     (digits[key][y] & (1U << (2U - x))) != 0U);
                }
            }
        }
    }
}

static void draw_matrix_border(uint8_t x_offset)
{
    for (uint8_t i = 0U; i < 8U; i++)
    {
        Display_SetPixel((uint8_t)(x_offset + i), 0U, true);
        Display_SetPixel((uint8_t)(x_offset + i), 7U, true);
        Display_SetPixel(x_offset, i, true);
        Display_SetPixel((uint8_t)(x_offset + 7U), i, true);
    }
}

static void draw_button(uint8_t x_offset, bool pressed)
{
    draw_matrix_border(x_offset);
    if (!pressed)
    {
        return;
    }

    for (uint8_t y = 1U; y < 7U; y++)
    {
        for (uint8_t x = 1U; x < 7U; x++)
        {
            Display_SetPixel((uint8_t)(x_offset + x), y, true);
        }
    }
}

static void render(void)
{
    Display_Clear();

    if (hourglass_active)
    {
        Hourglass_Render();
        Display_Update();
        return;
    }

    if (ocean_active)
    {
        Ocean_Render();
        Display_Update();
        return;
    }

    if (sensor_test_active)
    {
        ImuTest_Render();
        Display_Update();
        return;
    }

    if (calibration_active)
    {
        Display_SetPixel(calibration_points[calibration_index][0],
                         calibration_points[calibration_index][1], true);
        Display_Update();
        return;
    }

    if (feedback_keys != 0U)
    {
        draw_key_feedback();
        Display_Update();
        return;
    }

    switch (current_mode)
    {
        case BRINGUP_SINGLE_PIXEL:
            Display_SetPixel((uint8_t)(scan_position % DISPLAY_WIDTH),
                             (uint8_t)(scan_position / DISPLAY_WIDTH), true);
            break;
        case BRINGUP_ALL_ON:
            Display_Fill();
            break;
        case BRINGUP_CHECKERBOARD:
            for (uint8_t y = 0U; y < DISPLAY_HEIGHT; y++)
            {
                for (uint8_t x = 0U; x < DISPLAY_WIDTH; x++)
                {
                    Display_SetPixel(x, y, ((x + y) & 1U) == 0U);
                }
            }
            break;
        case BRINGUP_LEFT_MATRIX:
            for (uint8_t y = 0U; y < DISPLAY_HEIGHT; y++)
            {
                for (uint8_t x = 0U; x < 8U; x++)
                {
                    Display_SetPixel(x, y, true);
                }
            }
            break;
        case BRINGUP_RIGHT_MATRIX:
            for (uint8_t y = 0U; y < DISPLAY_HEIGHT; y++)
            {
                for (uint8_t x = 8U; x < DISPLAY_WIDTH; x++)
                {
                    Display_SetPixel(x, y, true);
                }
            }
            break;
        case BRINGUP_BUTTON_MONITOR:
            draw_button(0U, Buttons_IsPressed(BUTTON_K1));
            draw_button(8U, Buttons_IsPressed(BUTTON_K2));
            break;
        default:
            break;
    }

    Display_Update();
}

void Bringup_Init(uint32_t now)
{
    hourglass_active = true;
    Hourglass_Init(now);
    ocean_active = false;
    Ocean_Init(now);
    sensor_test_active = false;
    ImuTest_Init();
    calibration_active = false;
    calibration_index = 0U;
    current_mode = BRINGUP_SINGLE_PIXEL;
    scan_position = 0U;
    last_scan_at = now;
    last_display_at = now;
    feedback_at = now;
    feedback_keys = 0U;
    Display_Init();
    render();
}

void Bringup_Update(uint32_t now)
{
    uint8_t events = Buttons_TakeEvents();
    ImuTest_Update(now);

    if (hourglass_active)
    {
        if ((events & BUTTON_EVENT_K1_LONG) != 0U)
        {
            hourglass_active = false;
            ocean_active = true;
            Ocean_Resume(now);
        }
        else
        {
            Hourglass_Update(now);
            if (Hourglass_TakeCompletionEvent() ||
                (events & BUTTON_EVENT_K2_SHORT) != 0U)
            {
                Buzzer_StartAlarm(now);
            }
        }
        if ((uint32_t)(now - last_display_at) >= DISPLAY_REFRESH_PERIOD_MS)
        {
            last_display_at = now;
            render();
        }
        return;
    }

    if (ocean_active)
    {
        if ((events & BUTTON_EVENT_K1_LONG) != 0U)
        {
            ocean_active = false;
            sensor_test_active = true;
        }
        else if ((events & BUTTON_EVENT_K2_SHORT) != 0U)
        {
            Ocean_Init(now);
        }
        else
        {
            Ocean_Update(now);
        }

        if ((uint32_t)(now - last_display_at) >= DISPLAY_REFRESH_PERIOD_MS)
        {
            last_display_at = now;
            render();
        }
        return;
    }

    if (sensor_test_active)
    {
        if ((events & BUTTON_EVENT_K1_LONG) != 0U)
        {
            sensor_test_active = false;
            calibration_active = true;
            calibration_index = 0U;
            feedback_keys = 0U;
        }
        else if ((events & BUTTON_EVENT_K2_SHORT) != 0U)
        {
            ImuTest_SelectPage(true);
        }
        else if ((events & BUTTON_EVENT_K1_SHORT) != 0U)
        {
            ImuTest_SelectPage(false);
        }

        if ((uint32_t)(now - last_display_at) >= DISPLAY_REFRESH_PERIOD_MS)
        {
            last_display_at = now;
            render();
        }
        return;
    }

    if (calibration_active)
    {
        /* Keep a single static point even while a key is held. */
        if ((events & BUTTON_EVENT_K1_LONG) != 0U)
        {
            calibration_active = false;
            current_mode = BRINGUP_SINGLE_PIXEL;
            scan_position = 0U;
            last_scan_at = now;
        }
        else if ((events & BUTTON_EVENT_K2_SHORT) != 0U)
        {
            calibration_index = (uint8_t)((calibration_index + 1U) %
                                           CALIBRATION_POINT_COUNT);
        }
        else if ((events & BUTTON_EVENT_K1_SHORT) != 0U)
        {
            calibration_index = calibration_index == 0U
                                    ? (uint8_t)(CALIBRATION_POINT_COUNT - 1U)
                                    : (uint8_t)(calibration_index - 1U);
        }

        if ((uint32_t)(now - last_display_at) >= DISPLAY_REFRESH_PERIOD_MS)
        {
            last_display_at = now;
            render();
        }
        return;
    }

    if ((events & BUTTON_EVENT_K1_LONG) != 0U)
    {
        hourglass_active = true;
        Hourglass_Resume(now);
        feedback_keys = 0U;
        last_display_at = now;
        render();
        return;
    }

    uint8_t pressed_keys = (Buttons_IsPressed(BUTTON_K1) ? 1U : 0U) |
                           (Buttons_IsPressed(BUTTON_K2) ? 2U : 0U);
    if (pressed_keys != 0U)
    {
        feedback_keys = pressed_keys;
        feedback_at = now;
    }
    else if ((uint32_t)(now - feedback_at) >= 400U)
    {
        feedback_keys = 0U;
    }

    if ((events & BUTTON_EVENT_K1_SHORT) != 0U)
    {
        current_mode = current_mode == BRINGUP_SINGLE_PIXEL
                           ? (BringupMode)(BRINGUP_MODE_COUNT - 1)
                           : (BringupMode)(current_mode - 1);
    }
    if ((events & (BUTTON_EVENT_K2_SHORT | BUTTON_EVENT_K2_LONG)) != 0U)
    {
        current_mode = current_mode == BRINGUP_BUTTON_MONITOR
                           ? BRINGUP_SINGLE_PIXEL
                           : (BringupMode)(current_mode + 1);
    }

    if (current_mode == BRINGUP_SINGLE_PIXEL &&
        ((uint32_t)(now - last_scan_at) >= BRINGUP_SCAN_PERIOD_MS))
    {
        last_scan_at = now;
        scan_position = (uint8_t)((scan_position + 1U) %
                                  (DISPLAY_WIDTH * DISPLAY_HEIGHT));
    }

    if ((uint32_t)(now - last_display_at) >= DISPLAY_REFRESH_PERIOD_MS)
    {
        last_display_at = now;
        render();
    }
}
