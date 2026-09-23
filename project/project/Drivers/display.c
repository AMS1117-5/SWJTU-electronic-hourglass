#include "display.h"

#include "app_config.h"
#include "max7219.h"

#if (DISPLAY_LEFT_DEVICE > 1U) || (DISPLAY_RIGHT_DEVICE > 1U) || \
    (DISPLAY_LEFT_DEVICE == DISPLAY_RIGHT_DEVICE)
#error "Display device mapping must assign each matrix to a different MAX7219"
#endif

static uint16_t framebuffer[DISPLAY_HEIGHT];
static uint8_t brightness;
_Static_assert(DISPLAY_INTENSITY <= DISPLAY_BRIGHTNESS_MAX && DISPLAY_BRIGHTNESS_MAX <= 15U,
               "Display brightness must fit the MAX7219 intensity register");
static const uint8_t segment_map[MAX7219_DEVICE_COUNT][8] = {
    DISPLAY_DEVICE0_SEGMENT_MAP,
    DISPLAY_DEVICE1_SEGMENT_MAP
};
static const uint8_t digit_map[MAX7219_DEVICE_COUNT][8] = {
    DISPLAY_DEVICE0_DIGIT_MAP,
    DISPLAY_DEVICE1_DIGIT_MAP
};

static void map_pixel(uint8_t matrix, uint8_t x, uint8_t y,
                      uint8_t *device, uint8_t *physical_x, uint8_t *physical_y)
{
    uint8_t rotation;
    uint8_t flip_x;
    uint8_t flip_y;

    if (matrix == 0U)
    {
        *device = DISPLAY_LEFT_DEVICE;
        rotation = DISPLAY_LEFT_ROTATION;
        flip_x = DISPLAY_LEFT_FLIP_X;
        flip_y = DISPLAY_LEFT_FLIP_Y;
    }
    else
    {
        *device = DISPLAY_RIGHT_DEVICE;
        rotation = DISPLAY_RIGHT_ROTATION;
        flip_x = DISPLAY_RIGHT_FLIP_X;
        flip_y = DISPLAY_RIGHT_FLIP_Y;
    }

    switch (rotation)
    {
        case DISPLAY_ROTATION_90:
            *physical_x = (uint8_t)(7U - y);
            *physical_y = x;
            break;
        case DISPLAY_ROTATION_180:
            *physical_x = (uint8_t)(7U - x);
            *physical_y = (uint8_t)(7U - y);
            break;
        case DISPLAY_ROTATION_270:
            *physical_x = y;
            *physical_y = (uint8_t)(7U - x);
            break;
        default:
            *physical_x = x;
            *physical_y = y;
            break;
    }

    if (flip_x != 0U)
    {
        *physical_x = (uint8_t)(7U - *physical_x);
    }
    if (flip_y != 0U)
    {
        *physical_y = (uint8_t)(7U - *physical_y);
    }
}

void Display_Init(void)
{
    brightness = DISPLAY_INTENSITY;
    Max7219_Init(DISPLAY_INTENSITY);
    Display_Clear();
    Display_Update();
}

void Display_SetBrightness(uint8_t level)
{
    if (level > DISPLAY_BRIGHTNESS_MAX)
    {
        level = DISPLAY_BRIGHTNESS_MAX;
    }
    if (level != brightness)
    {
        Max7219_SetIntensity(level);
        brightness = level;
    }
}

uint8_t Display_GetBrightness(void)
{
    return brightness;
}

void Display_Clear(void)
{
    for (uint8_t y = 0U; y < DISPLAY_HEIGHT; y++)
    {
        framebuffer[y] = 0U;
    }
}

void Display_Fill(void)
{
    for (uint8_t y = 0U; y < DISPLAY_HEIGHT; y++)
    {
        framebuffer[y] = 0xFFFFU;
    }
}

void Display_SetPixel(uint8_t x, uint8_t y, bool state)
{
    if ((x >= DISPLAY_WIDTH) || (y >= DISPLAY_HEIGHT))
    {
        return;
    }

    if (state)
    {
        framebuffer[y] |= (uint16_t)(1U << x);
    }
    else
    {
        framebuffer[y] &= (uint16_t)~(1U << x);
    }
}

bool Display_GetPixel(uint8_t x, uint8_t y)
{
    if ((x >= DISPLAY_WIDTH) || (y >= DISPLAY_HEIGHT))
    {
        return false;
    }
    return (framebuffer[y] & (uint16_t)(1U << x)) != 0U;
}

void Display_Update(void)
{
    uint8_t rows[MAX7219_DEVICE_COUNT][8] = {{0U}};

    for (uint8_t y = 0U; y < DISPLAY_HEIGHT; y++)
    {
        for (uint8_t x = 0U; x < DISPLAY_WIDTH; x++)
        {
            uint8_t device;
            uint8_t physical_x;
            uint8_t physical_y;
            uint8_t matrix;
            uint8_t local_x;

            if (!Display_GetPixel(x, y))
            {
                continue;
            }

            matrix = (uint8_t)(x / 8U);
            local_x = (uint8_t)(x % 8U);
            map_pixel(matrix, local_x, y, &device, &physical_x, &physical_y);
            rows[device][digit_map[device][physical_y]] |=
                (uint8_t)(1U << segment_map[device][physical_x]);
        }
    }

    for (uint8_t row = 0U; row < 8U; row++)
    {
        uint8_t values[MAX7219_DEVICE_COUNT] = {rows[0][row], rows[1][row]};
        Max7219_WriteRegister((uint8_t)(row + 1U), values);
    }
}
