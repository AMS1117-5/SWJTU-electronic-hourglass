#include <assert.h>
#include <stdio.h>

#include "display.h"
#include "max7219.h"

static uint8_t rows[2][8];
static uint8_t intensity;
static unsigned intensity_writes;
void Max7219_Init(uint8_t value) { intensity = value; }
void Max7219_SetIntensity(uint8_t value) { intensity = value; intensity_writes++; }
void Max7219_WriteRegister(uint8_t reg, const uint8_t values[2])
{
    assert(reg >= 1U && reg <= 8U);
    rows[0][reg - 1U] = values[0];
    rows[1][reg - 1U] = values[1];
}

int main(void)
{
    static const unsigned bit_to_row[8] = {7, 6, 5, 4, 3, 2, 1, 8};
    unsigned visited[2][8][8] = {{{0}}};
    Display_Init();
    assert(Display_GetBrightness() == 0U && intensity == 0U);
    Display_SetPixel(3U, 4U, true);
    Display_SetBrightness(1U);
    assert(Display_GetPixel(3U, 4U) && intensity == 1U && intensity_writes == 1U);
    Display_SetBrightness(2U);
    Display_SetBrightness(2U);
    Display_SetBrightness(255U);
    assert(Display_GetPixel(3U, 4U) && intensity == 2U && intensity_writes == 2U);
    Display_SetBrightness(0U);
    assert(Display_GetBrightness() == 0U && intensity == 0U);

    for (unsigned x = 0; x < 16U; x++)
        for (unsigned y = 0; y < 8U; y++)
        {
            unsigned lit = 0U;
            Display_Clear();
            Display_SetPixel(x, y, true);
            Display_Update();
            for (unsigned d = 0; d < 2U; d++)
                for (unsigned reg = 0; reg < 8U; reg++)
                    for (unsigned bit = 0; bit < 8U; bit++)
                        if ((rows[d][reg] & (1U << bit)) != 0U)
                        {
                            lit++;
                            assert(d == x / 8U && bit_to_row[bit] == x % 8U + 1U);
                            assert(8U - reg == y + 1U);
                            assert(visited[d][reg][bit]++ == 0U);
                        }
            assert(lit == 1U);
        }
    Display_Clear();
    Display_SetPixel(255U, 255U, true);
    assert(!Display_GetPixel(255U, 255U));
    Display_Update();
    for (unsigned d = 0; d < 2U; d++)
        for (unsigned row = 0; row < 8U; row++)
            assert(rows[d][row] == 0U);
    Display_Fill();
    Display_Update();
    for (unsigned d = 0; d < 2U; d++)
        for (unsigned row = 0; row < 8U; row++)
            assert(rows[d][row] == 255U);
    puts("PASS: calibrated 128-pixel mapping, bounds, clear/fill, brightness range and framebuffer preservation");
    return 0;
}
