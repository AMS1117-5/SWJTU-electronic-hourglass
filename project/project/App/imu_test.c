#include "imu_test.h"

#include "display.h"
#include "mpu6050.h"
#include "motion.h"
#include "app_config.h"

static const uint8_t axis_glyphs[3][5] = {
    {17U, 10U, 4U, 10U, 17U}, /* X */
    {17U, 10U, 4U, 4U, 4U},   /* Y */
    {31U, 2U, 4U, 8U, 31U}    /* Z */
};
static const uint8_t error_glyph[5] = {31U, 16U, 30U, 16U, 31U};
static const uint8_t error_digits[3][5] = {
    {2U, 6U, 2U, 2U, 7U},
    {7U, 1U, 7U, 4U, 7U},
    {7U, 1U, 7U, 1U, 7U}
};

/* Page 0 = mapped gravity; pages 1..3 = raw sensor AX/AY/AZ. */
static uint8_t selected_page;
static const uint8_t gravity_glyph[5] = {14U, 16U, 23U, 17U, 14U};
static int32_t filtered_accel[3];
static uint32_t last_sequence;
static uint32_t sample_time;
static bool filter_ready;
static Mpu6050Status sensor_status;

/* Upright test UI: virtual x is physical row; virtual y is physical column.
 * Raw-axis pages show sensor values; the gravity page uses Motion exclusively.
 */
static void draw_glyph(uint8_t row, uint8_t column, const uint8_t glyph[5],
                       uint8_t width)
{
    for (uint8_t r = 0U; r < 5U; r++)
    {
        for (uint8_t c = 0U; c < width; c++)
        {
            Display_SetPixel((uint8_t)(row + r), (uint8_t)(column + c),
                             (glyph[r] & (1U << (width - 1U - c))) != 0U);
        }
    }
}

void ImuTest_Init(void)
{
    selected_page = 0U;
    for (uint8_t axis = 0U; axis < 3U; axis++)
    {
        filtered_accel[axis] = 0;
    }
    last_sequence = 0U;
    sample_time = 0U;
    filter_ready = false;
    sensor_status = MPU6050_STARTING;
}

void ImuTest_Update(uint32_t now)
{
    Mpu6050Sample sample;
    sensor_status = Mpu6050_GetStatus();
    if (!Mpu6050_GetSample(&sample, now))
    {
        filter_ready = false;
        if (sensor_status == MPU6050_READY)
        {
            sensor_status = MPU6050_DATA_ERROR;
        }
        return;
    }

    if (!filter_ready || sample.sequence != last_sequence)
    {
        for (uint8_t axis = 0U; axis < 3U; axis++)
        {
            if (!filter_ready)
            {
                filtered_accel[axis] = sample.accel[axis];
            }
            else
            {
                /* Light smoothing for the diagnostic bars only. */
                filtered_accel[axis] +=
                    ((int32_t)sample.accel[axis] - filtered_accel[axis]) / 8;
            }
        }
        filter_ready = true;
        last_sequence = sample.sequence;
        sample_time = sample.timestamp_ms;
    }
}

void ImuTest_SelectPage(bool next)
{
    selected_page = (uint8_t)((selected_page + (next ? 1U : 3U)) % 4U);
}

static uint8_t gravity_pixel(float component, uint8_t size)
{
    if (component < -1.0f)
    {
        component = -1.0f;
    }
    if (component > 1.0f)
    {
        component = 1.0f;
    }
    return (uint8_t)((component + 1.0f) * (float)(size - 1U) * 0.5f + 0.5f);
}

void ImuTest_Render(void)
{
    Display_Clear();
    if (sensor_status == MPU6050_STARTING)
    {
        for (uint8_t col = 2U; col < 6U; col++)
        {
            Display_SetPixel(3U, col, true);
            Display_SetPixel(11U, col, true);
        }
        return;
    }

    if (sensor_status != MPU6050_READY || !filter_ready)
    {
        uint8_t error = sensor_status == MPU6050_BUS_ERROR ? 0U :
                        sensor_status == MPU6050_ID_ERROR ? 1U : 2U;
        draw_glyph(1U, 1U, error_glyph, 5U);
        draw_glyph(9U, 2U, error_digits[error], 3U);
        return;
    }

    if (selected_page == 0U)
    {
        if (!Motion_IsValid())
        {
            draw_glyph(1U, 1U, gravity_glyph, 5U);
            for (uint8_t col = 2U; col < 6U; col++)
            {
                Display_SetPixel(11U, col, true);
            }
            return;
        }
        Gravity2D gravity = Motion_GetDisplayGravity();
        Display_SetPixel(gravity_pixel(gravity.x, DISPLAY_WIDTH),
                         gravity_pixel(gravity.y, DISPLAY_HEIGHT), true);
        return;
    }

    uint8_t selected_axis = (uint8_t)(selected_page - 1U);
    draw_glyph(1U, 1U, axis_glyphs[selected_axis], 5U);
    /* Heartbeat uses actual sample timestamps, so frozen data cannot animate it. */
    Display_SetPixel(7U, 7U, ((sample_time / 500U) & 1U) != 0U);

    int32_t value = filtered_accel[selected_axis];
    int32_t magnitude = value < 0 ? -value : value;
    uint8_t bars = (uint8_t)((magnitude * 8 + MPU6050_ACCEL_LSB_PER_G / 2) /
                             MPU6050_ACCEL_LSB_PER_G);
    if (bars > 8U)
    {
        bars = 8U;
    }

    /* +/- symbol; near zero (<1/16 g) shows a single dot instead. */
    Display_SetPixel(10U, 3U, true);
    if (magnitude >= MPU6050_ACCEL_LSB_PER_G / 16)
    {
        Display_SetPixel(10U, 2U, true);
        Display_SetPixel(10U, 4U, true);
        if (value > 0)
        {
            Display_SetPixel(9U, 3U, true);
            Display_SetPixel(11U, 3U, true);
        }
    }
    for (uint8_t col = 0U; col < bars; col++)
    {
        Display_SetPixel(14U, col, true);
        Display_SetPixel(15U, col, true);
    }
}
