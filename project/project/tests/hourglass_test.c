#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "display.h"
#include "hourglass.h"
#include "motion.h"

static bool valid;
static Gravity2D gravity;
static uint8_t shake;
static uint32_t now;
static bool frame[16][8];
static const uint8_t open_rows[16] = {
    0, 126, 126, 126, 126, 60, 24, 24, 24, 24, 60, 126, 126, 126, 126, 0
};

bool Motion_IsValid(void) { return valid; }
Gravity2D Motion_GetDisplayGravity(void) { return gravity; }
uint8_t Motion_GetShakeStrength(void) { return shake; }
void Display_Clear(void) { memset(frame, 0, sizeof(frame)); }
void Display_SetPixel(uint8_t x, uint8_t y, bool on)
{
    assert(x < 16U && y < 8U && on);
    assert(!frame[x][y]);
    frame[x][y] = on;
}

static void check(void)
{
    HourglassState state = Hourglass_GetState();
    unsigned count_a = 0U;
    unsigned count_b = 0U;
    Hourglass_Render();
    for (unsigned x = 0U; x < 16U; x++)
        for (unsigned y = 0U; y < 8U; y++)
            if (frame[x][y] && (open_rows[x] & (1U << y)) != 0U)
            {
                if (x < 8U) count_a++;
                else count_b++;
            }
    assert(count_a == state.particles_a && count_b == state.particles_b);
    if (state.initialized)
    {
        assert(count_a + count_b == HOURGLASS_PARTICLE_COUNT);
        assert(state.sand_a <= HOURGLASS_SAND_TOTAL);
        assert(state.sand_b <= HOURGLASS_SAND_TOTAL);
        assert(state.sand_a + state.sand_b == HOURGLASS_SAND_TOTAL);
    }
    else
    {
        assert(count_a + count_b == 0U);
        assert(!state.complete);
    }
}

static void tick(unsigned dt)
{
    now += dt;
    Hourglass_Update(now);
    check();
}

static void run(unsigned milliseconds)
{
    assert(milliseconds % 20U == 0U);
    for (unsigned elapsed = 0U; elapsed < milliseconds; elapsed += 20U)
        tick(20U);
}

static void begin(uint32_t start, float gx, float gy)
{
    now = start;
    valid = true;
    gravity = (Gravity2D){gx, gy};
    shake = 0U;
    Hourglass_Init(now);
    check();
    tick(20U);
    assert(!Hourglass_TakeCompletionEvent()); /* Initial empty chamber is silent. */
}

static void check_complete(bool at_b)
{
    HourglassState state = Hourglass_GetState();
    /* Physical movement may take a few frames after the final volume threshold. */
    for (unsigned i = 0; i < 50U && !state.complete; i++)
    {
        tick(20U);
        state = Hourglass_GetState();
    }
    assert(state.complete);
    assert(state.sand_a == (at_b ? 0U : HOURGLASS_SAND_TOTAL));
    assert(state.particles_a == (at_b ? 0U : HOURGLASS_PARTICLE_COUNT));
    assert(Hourglass_TakeCompletionEvent());
    assert(!Hourglass_TakeCompletionEvent());
}

int main(void)
{
    const unsigned duration = HOURGLASS_DURATION_MS;
    const unsigned interval = HOURGLASS_GRAIN_INTERVAL_MS;
    const unsigned quarter = duration / 4U;
    assert(interval <= 10000U);
    begin(0U, 0.0f, 1.0f);
    run(1000U);
    assert(!Hourglass_GetState().initialized);
    gravity.x = 1.0f;
    gravity.y = 0.0f;
    tick(20U);
    assert(Hourglass_GetState().particles_a == HOURGLASS_PARTICLE_COUNT);

    /* The visible throat is closed until one whole particle's volume is owed. */
    run(interval - 20U);
    assert(Hourglass_GetState().particles_a == HOURGLASS_PARTICLE_COUNT);
    assert(!Hourglass_TakeCompletionEvent());
    /* Allow one physics tick for integer-volume quantization at the threshold. */
    run(40U);
    assert(Hourglass_GetState().particles_a == HOURGLASS_PARTICLE_COUNT - 1U);
    run(duration - interval - 40U);
    assert(Hourglass_GetState().sand_a > 0U);
    assert(!Hourglass_TakeCompletionEvent());
    tick(20U);
    assert(Hourglass_GetState().sand_a == 0U);
    check_complete(true);
    run(10000U);
    assert(!Hourglass_TakeCompletionEvent());

    /* No visible grain crossed during this small wobble: no new emptying event. */
    gravity.x = -1.0f;
    run(100U);
    gravity.x = 1.0f;
    run(100U);
    assert(Hourglass_GetState().sand_a == 0U);
    assert(!Hourglass_TakeCompletionEvent());
    gravity.x = -1.0f;
    run(duration);
    check_complete(false);
    gravity.x = 1.0f;
    run(duration);
    check_complete(true);

    /* Starting upside-down fills the physically upper chamber B. */
    begin(0U, -1.0f, 0.0f);
    assert(Hourglass_GetState().sand_a == 0U);
    assert(Hourglass_GetState().particles_b == HOURGLASS_PARTICLE_COUNT);
    run(duration);
    check_complete(false);

    /* Half axial projection takes twice as long, independent of animation rate. */
    begin(0U, 0.5f, 0.0f);
    run(duration);
    assert(Hourglass_GetState().sand_a == HOURGLASS_SAND_TOTAL / 2U);
    assert(!Hourglass_TakeCompletionEvent());
    run(duration);
    assert(Hourglass_GetState().sand_a == 0U);
    check_complete(true);

    begin(0U, 1.0f, 0.0f);
    run(quarter);
    HourglassState paused = Hourglass_GetState();
    assert(paused.sand_a == 750000U);
    gravity = (Gravity2D){0.1f, 1.0f};
    shake = 255U;
    run(20000U);
    assert(Hourglass_GetState().sand_a == paused.sand_a);
    assert(Hourglass_GetState().particles_a == paused.particles_a);
    valid = false;
    run(20000U);
    assert(Hourglass_GetState().sand_a == paused.sand_a);
    valid = true;
    shake = 0U;
    gravity = (Gravity2D){1.0f, 0.0f};
    now += 60000U;
    Hourglass_Resume(now);
    Hourglass_Update(now);
    assert(Hourglass_GetState().sand_a == paused.sand_a);
    run(quarter);
    assert(Hourglass_GetState().sand_a == 500000U);
    paused = Hourglass_GetState();
    tick(2000U); /* A stalled loop is not interpreted as continuous flow. */
    assert(Hourglass_GetState().sand_a == paused.sand_a);

    /* Returning partially transferred sand to the original chamber also sounds. */
    gravity.x = -1.0f;
    run(quarter);
    assert(Hourglass_GetState().sand_a == 750000U);
    run(quarter);
    assert(Hourglass_GetState().sand_a == HOURGLASS_SAND_TOTAL);
    check_complete(false);
    run(1000U);
    assert(!Hourglass_TakeCompletionEvent());

    /* In either starting pose, even a one-grain excursion and return empties
     * a real nonempty chamber and must produce a new one-shot event.
     */
    for (int direction = -1; direction <= 1; direction += 2)
    {
        begin(0U, (float)direction, 0.0f);
        for (unsigned cycle = 0U; cycle < 2U; cycle++)
        {
            gravity.x = (float)direction;
            run(interval + 40U);
            HourglassState split = Hourglass_GetState();
            assert(split.particles_a > 0U && split.particles_b > 0U);
            assert(!split.complete && !Hourglass_TakeCompletionEvent());
            gravity.x = (float)-direction;
            run(interval + 40U);
            check_complete(direction < 0);
            run(1000U);
            assert(!Hourglass_TakeCompletionEvent());
        }
    }

    /* Irregular short frame intervals must integrate real elapsed time. */
    begin(0U, 1.0f, 0.0f);
    for (unsigned i = 0; i < 2000U; i++)
    {
        tick(20U);
        tick(30U);
    }
    assert(Hourglass_GetState().sand_a == HOURGLASS_SAND_TOTAL -
           (uint32_t)((uint64_t)HOURGLASS_SAND_TOTAL * 100000U / duration));

    begin(UINT32_MAX - 1000U, 1.0f, 0.0f);
    run(duration);
    check_complete(true);

    /* Runtime rate changes preserve both the continuous volume and the frame. */
    begin(0U, 1.0f, 0.0f);
    run(60000U);
    paused = Hourglass_GetState();
    bool previous_frame[16][8];
    memcpy(previous_frame, frame, sizeof(frame));
    assert(!Hourglass_SetGrainIntervalMs(0U));
    assert(!Hourglass_SetGrainIntervalMs(1249U));
    assert(!Hourglass_SetGrainIntervalMs(10001U));
    assert(Hourglass_GetGrainIntervalMs() == 10000U);
    assert(Hourglass_SetGrainIntervalMs(1250U));
    check();
    assert(Hourglass_GetState().sand_a == paused.sand_a);
    assert(Hourglass_GetState().particles_a == paused.particles_a);
    assert(memcmp(previous_frame, frame, sizeof(frame)) == 0);
    run(22500U); /* Remaining 3/4 of the new 30-second full duration. */
    assert(Hourglass_GetState().sand_a == 0U);
    check_complete(true);
    assert(Hourglass_SetGrainIntervalMs(5000U));
    assert(!Hourglass_TakeCompletionEvent());
    gravity.x = -1.0f;
    run(120000U);
    check_complete(false);

    /* All selectable rates must finish, including the fastest neck cadence. */
    for (unsigned seconds = 30U; seconds <= 240U; seconds += 30U)
    {
        begin(0U, 1.0f, 0.0f);
        assert(Hourglass_SetGrainIntervalMs((uint16_t)(seconds * 1000U / HOURGLASS_PARTICLE_COUNT)));
        run(seconds * 1000U - 20U);
        assert(Hourglass_GetState().sand_a > 0U);
        assert(!Hourglass_TakeCompletionEvent());
        tick(20U);
        assert(Hourglass_GetState().sand_a == 0U);
        check_complete(true);
    }
    puts("PASS: all 30..240-second durations, live changes, tilt, gating, conservation, pause, bidirectional empty-chamber events, startup silence, tick wrap");
    return 0;
}
