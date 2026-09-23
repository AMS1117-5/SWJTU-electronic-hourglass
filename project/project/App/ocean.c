#include "ocean.h"

#include <stddef.h>

#include "app_config.h"
#include "display.h"
#include "motion.h"
#include "particles.h"

_Static_assert(OCEAN_PARTICLE_COUNT <= MAX_PARTICLES,
               "Ocean particle count exceeds static storage");

static ParticleSystem water;
static uint32_t last_step_at;

void Ocean_Init(uint32_t now)
{
    Particles_Init(&water, NULL, now ^ 0x4F434541U);
    for (uint8_t i = 0U; i < OCEAN_PARTICLE_COUNT; i++)
    {
        /* 37 is coprime with 128: spread particles without duplicate cells. */
        uint8_t cell = (uint8_t)((i * 37U + 11U) % (DISPLAY_WIDTH * DISPLAY_HEIGHT));
        (void)Particles_Add(&water, cell % DISPLAY_WIDTH, cell / DISPLAY_WIDTH);
    }
    last_step_at = now;
}

void Ocean_Resume(uint32_t now)
{
    last_step_at = now;
}

void Ocean_Update(uint32_t now)
{
    if ((uint32_t)(now - last_step_at) < PHYSICS_PERIOD_MS)
    {
        return;
    }
    last_step_at = now;
    if (!Motion_IsValid())
    {
        return;
    }
    /* At most one step per call; never replay time spent in another test page. */
    Particles_Step(&water, Motion_GetDisplayGravity(), Motion_GetShakeStrength());
}

void Ocean_Render(void)
{
    Display_Clear();
    for (uint8_t i = 0U; i < water.count; i++)
    {
        Display_SetPixel((uint8_t)water.particles[i].x,
                         (uint8_t)water.particles[i].y, true);
    }
}
