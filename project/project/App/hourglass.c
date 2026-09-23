#include "hourglass.h"

#include "app_config.h"
#include "display.h"
#include "motion.h"
#include "particles.h"

_Static_assert(HOURGLASS_PARTICLE_COUNT > 0U && HOURGLASS_PARTICLE_COUNT <= 32U &&
               HOURGLASS_PARTICLE_COUNT <= MAX_PARTICLES,
               "Each chamber has room for 32 particles");
_Static_assert(HOURGLASS_GRAIN_INTERVAL_MS >= HOURGLASS_MIN_GRAIN_INTERVAL_MS &&
               HOURGLASS_GRAIN_INTERVAL_MS <= HOURGLASS_MAX_GRAIN_INTERVAL_MS,
               "Upright grain interval must be positive and no more than 10 seconds");

/* Virtual x is the hourglass axis. A: x<8, B: x>=8.
 * Each byte describes traversable y positions in that physical row.
 * Both chambers have capacity 32; neck crosses x=7/8 at y=3/4 only.
 */
static const uint8_t open_rows[16] = {
    0x00U, 0x7EU, 0x7EU, 0x7EU, 0x7EU, 0x3CU, 0x18U, 0x18U,
    0x18U, 0x18U, 0x3CU, 0x7EU, 0x7EU, 0x7EU, 0x7EU, 0x00U
};
static uint16_t hourglass_wall_mask[DISPLAY_HEIGHT];
static ParticleSystem sand;
static HourglassState state;
static uint32_t last_step_at;
static uint32_t flow_remainder;
static int last_flow_direction;
static int flow_direction;
static uint8_t target_particles_a;
static bool completion_pending;
static uint16_t grain_interval_ms = HOURGLASS_GRAIN_INTERVAL_MS;

static bool open_cell(int x, int y)
{
    return x >= 0 && x < 16 && y >= 0 && y < 8 &&
           (open_rows[x] & (1U << y)) != 0U;
}

static bool neck_gate(void *context, int from_x, int from_y, int to_x, int to_y)
{
    HourglassState *current = context;
    if ((from_x < 8) == (to_x < 8))
    {
        return true;
    }
    if (from_y < 3 || from_y > 4 || to_y < 3 || to_y > 4)
    {
        return false;
    }
    if (from_x == 7 && to_x == 8 && flow_direction > 0 &&
        current->particles_a > target_particles_a)
    {
        current->particles_a--;
        current->particles_b++;
        return true;
    }
    if (from_x == 8 && to_x == 7 && flow_direction < 0 &&
        current->particles_a < target_particles_a)
    {
        current->particles_a++;
        current->particles_b--;
        return true;
    }
    return false;
}

static void fill_upper_chamber(bool a_is_upper)
{
    int first = a_is_upper ? 1 : 8;
    int end = a_is_upper ? 8 : 15;
    for (int x = first; x < end && sand.count < HOURGLASS_PARTICLE_COUNT; x++)
    {
        for (int y = 0; y < 8 && sand.count < HOURGLASS_PARTICLE_COUNT; y++)
        {
            if (open_cell(x, y))
            {
                (void)Particles_Add(&sand, x, y);
            }
        }
    }
    state.sand_a = a_is_upper ? HOURGLASS_SAND_TOTAL : 0U;
    state.sand_b = HOURGLASS_SAND_TOTAL - state.sand_a;
    state.particles_a = a_is_upper ? sand.count : 0U;
    state.particles_b = (uint8_t)(sand.count - state.particles_a);
    state.initialized = true;
}

static void integrate_flow(uint32_t elapsed, unsigned strength)
{
    if (flow_direction == 0)
    {
        return;
    }
    uint32_t denominator = (uint32_t)grain_interval_ms * HOURGLASS_PARTICLE_COUNT * 1024U;
    uint64_t numerator = (uint64_t)HOURGLASS_SAND_TOTAL * strength * elapsed +
                         flow_remainder;
    uint32_t transfer = (uint32_t)(numerator / denominator);
    flow_remainder = (uint32_t)(numerator % denominator);
    uint32_t source = flow_direction > 0 ? state.sand_a : state.sand_b;
    if (transfer >= source)
    {
        transfer = source;
        flow_remainder = 0U;
    }
    if (flow_direction > 0)
    {
        state.sand_a -= transfer;
    }
    else
    {
        state.sand_a += transfer;
    }
    state.sand_b = HOURGLASS_SAND_TOTAL - state.sand_a;
}

void Hourglass_Init(uint32_t now)
{
    for (int y = 0; y < 8; y++)
    {
        hourglass_wall_mask[y] = 0U;
        for (int x = 0; x < 16; x++)
        {
            if (!open_cell(x, y))
            {
                hourglass_wall_mask[y] |= (uint16_t)(1U << x);
            }
        }
    }
    Particles_Init(&sand, hourglass_wall_mask, now ^ 0x53414E44U);
    Particles_SetMoveGate(&sand, neck_gate, &state);
    state = (HourglassState){0};
    last_step_at = now;
    flow_remainder = 0U;
    flow_direction = 0;
    last_flow_direction = 0;
    target_particles_a = 0U;
    completion_pending = false;
    grain_interval_ms = HOURGLASS_GRAIN_INTERVAL_MS;
}

void Hourglass_Resume(uint32_t now)
{
    last_step_at = now;
}

bool Hourglass_SetGrainIntervalMs(uint16_t interval_ms)
{
    if (interval_ms < HOURGLASS_MIN_GRAIN_INTERVAL_MS ||
        interval_ms > HOURGLASS_MAX_GRAIN_INTERVAL_MS)
    {
        return false;
    }
    /* Remainder represents a fraction of one sand unit. Preserve that fraction
     * when changing the rate denominator, without creating/destroying sand.
     */
    flow_remainder = (uint32_t)((uint64_t)flow_remainder * interval_ms / grain_interval_ms);
    grain_interval_ms = interval_ms;
    return true;
}

uint16_t Hourglass_GetGrainIntervalMs(void)
{
    return grain_interval_ms;
}

void Hourglass_Update(uint32_t now)
{
    if (!Motion_IsValid())
    {
        last_step_at = now;
        return;
    }
    uint32_t elapsed = (uint32_t)(now - last_step_at);
    if (elapsed < PHYSICS_PERIOD_MS)
    {
        return;
    }
    last_step_at = now;
    Gravity2D gravity = Motion_GetDisplayGravity();
    float projection = gravity.x;
    float magnitude = projection < 0.0f ? -projection : projection;
    if (magnitude > 1.0f)
    {
        magnitude = 1.0f;
    }
    flow_direction = magnitude < HOURGLASS_FLOW_DEADZONE ? 0 :
                     projection > 0.0f ? 1 : -1;

    if (!state.initialized)
    {
        if (flow_direction != 0)
        {
            fill_upper_chamber(flow_direction > 0);
        }
        return;
    }
    if (flow_direction != last_flow_direction)
    {
        /* A fractional micro-unit of credit cannot leak across direction changes. */
        flow_remainder = 0U;
        last_flow_direction = flow_direction;
    }
    if (elapsed <= 250U)
    {
        integrate_flow(elapsed, (unsigned)(magnitude * 1024.0f + 0.5f));
    }
    /* A long debugger/main-loop stall is treated as a pause, not elapsed flow. */

    uint32_t scaled = state.sand_a * HOURGLASS_PARTICLE_COUNT;
    if (flow_direction > 0)
    {
        target_particles_a = (uint8_t)((scaled + HOURGLASS_SAND_TOTAL - 1U) /
                                       HOURGLASS_SAND_TOTAL);
    }
    else
    {
        target_particles_a = (uint8_t)(scaled / HOURGLASS_SAND_TOTAL);
    }
    uint8_t previous_a = state.particles_a;
    uint8_t previous_b = state.particles_b;
    Particles_Step(&sand, gravity, Motion_GetShakeStrength());

    /* Completion follows visible sand, regardless of the starting chamber or
     * elapsed time: sound once when either nonempty chamber becomes empty.
     * Initialization and a chamber that stays empty cannot create an edge.
     */
    bool emptied_a = previous_a > 0U && state.particles_a == 0U;
    bool emptied_b = previous_b > 0U && state.particles_b == 0U;
    if (emptied_a || emptied_b)
    {
        state.complete = true;
        completion_pending = true;
    }
    else if (state.particles_a > 0U && state.particles_b > 0U)
    {
        state.complete = false;
    }
}

void Hourglass_Render(void)
{
    Display_Clear();
    /* Only draw the inner outline, not every solid cell outside the chambers. */
    for (int x = 0; x < 16; x++)
    {
        for (int y = 0; y < 8; y++)
        {
            bool outline = !open_cell(x, y) &&
                           (open_cell(x - 1, y) || open_cell(x + 1, y) ||
                            open_cell(x, y - 1) || open_cell(x, y + 1));
            if (outline || Particles_GetPixel(&sand, x, y))
            {
                Display_SetPixel((uint8_t)x, (uint8_t)y, true);
            }
        }
    }
}

bool Hourglass_TakeCompletionEvent(void)
{
    bool pending = completion_pending;
    completion_pending = false;
    return pending;
}

HourglassState Hourglass_GetState(void)
{
    return state;
}
