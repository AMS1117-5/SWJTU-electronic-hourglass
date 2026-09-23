#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "display.h"
#include "motion.h"
#include "ocean.h"

static uint16_t frame[8];
static bool motion_valid;
static Gravity2D gravity;
static uint8_t shake;

bool Motion_IsValid(void) { return motion_valid; }
Gravity2D Motion_GetDisplayGravity(void) { return gravity; }
uint8_t Motion_GetShakeStrength(void) { return shake; }
void Display_Clear(void) { memset(frame, 0, sizeof(frame)); }
void Display_SetPixel(uint8_t x, uint8_t y, bool on)
{
    assert(x < 16U && y < 8U && on);
    assert((frame[y] & (1U << x)) == 0U);
    frame[y] |= (uint16_t)(1U << x);
}

static unsigned check_frame(void)
{
    unsigned count = 0;
    unsigned sum_x = 0;
    Ocean_Render();
    for (unsigned y = 0; y < 8U; y++)
        for (unsigned x = 0; x < 16U; x++)
            if ((frame[y] & (1U << x)) != 0U)
            {
                count++;
                sum_x += x;
            }
    assert(count == 32U);
    return sum_x;
}

int main(void)
{
    uint16_t snapshot[8];
    motion_valid = true;
    gravity = (Gravity2D){1.0f, 0.0f};
    shake = 0U;
    Ocean_Init(100U);
    check_frame();
    memcpy(snapshot, frame, sizeof(frame));
    Ocean_Update(119U);
    check_frame();
    assert(memcmp(snapshot, frame, sizeof(frame)) == 0);
    Ocean_Update(120U);
    check_frame();
    assert(memcmp(snapshot, frame, sizeof(frame)) != 0);

    memcpy(snapshot, frame, sizeof(frame));
    motion_valid = false;
    Ocean_Update(1000U);
    check_frame();
    assert(memcmp(snapshot, frame, sizeof(frame)) == 0);
    motion_valid = true;
    Ocean_Resume(2000U);
    Ocean_Update(2019U);
    check_frame();
    assert(memcmp(snapshot, frame, sizeof(frame)) == 0);

    /* A long stall must not replay thousands of steps or teleport water. */
    unsigned old_sum = check_frame();
    Ocean_Update(100000U);
    unsigned new_sum = check_frame();
    assert(new_sum > old_sum && new_sum - old_sum <= 32U);

    Ocean_Init(0U);
    gravity = (Gravity2D){0.0f, 0.0f};
    check_frame();
    memcpy(snapshot, frame, sizeof(frame));
    for (unsigned now = 20U; now <= 1000U; now += 20U)
        Ocean_Update(now);
    check_frame();
    assert(memcmp(snapshot, frame, sizeof(frame)) == 0);
    shake = 255U;
    for (unsigned now = 1020U; now <= 1200U; now += 20U)
    {
        Ocean_Update(now);
        check_frame();
    }
    assert(memcmp(snapshot, frame, sizeof(frame)) != 0);

    Ocean_Init(UINT32_MAX - 9U);
    gravity = (Gravity2D){1.0f, 0.0f};
    shake = 0U;
    check_frame();
    memcpy(snapshot, frame, sizeof(frame));
    Ocean_Update(9U);
    check_frame();
    assert(memcmp(snapshot, frame, sizeof(frame)) == 0);
    Ocean_Update(10U);
    check_frame();
    assert(memcmp(snapshot, frame, sizeof(frame)) != 0);
    puts("PASS: 32-pixel rendering, scheduler, stale sensor freeze, resume, flat rest, shake, tick wrap");
    return 0;
}
