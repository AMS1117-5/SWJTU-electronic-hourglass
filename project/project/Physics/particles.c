#include "particles.h"

#include <stddef.h>

_Static_assert(DISPLAY_WIDTH == 16U && DISPLAY_HEIGHT == 8U,
               "Particle grid uses the project's 16x8 virtual coordinates");

static uint32_t random_next(ParticleSystem *system)
{
    uint32_t value = system->random_state;
    value ^= value << 13U;
    value ^= value >> 17U;
    value ^= value << 5U;
    system->random_state = value;
    return value;
}

static bool inside(int x, int y)
{
    return x >= 0 && x < (int)DISPLAY_WIDTH && y >= 0 && y < (int)DISPLAY_HEIGHT;
}

static bool solid(const ParticleSystem *system, int x, int y)
{
    return !inside(x, y) || (system->walls[y] & (uint16_t)(1U << x)) != 0U;
}

static bool try_move(ParticleSystem *system, Particle *particle, int dx, int dy)
{
    int x = particle->x + dx;
    int y = particle->y + dy;
    if (solid(system, x, y) || Particles_GetPixel(system, x, y))
    {
        return false;
    }
    if (dx != 0 && dy != 0 &&
        (solid(system, particle->x + dx, particle->y) &&
         solid(system, particle->x, particle->y + dy)))
    {
        /* Round a slope via a free orthogonal neighbor, but never squeeze
         * diagonally between two solid cells into a disconnected chamber.
         */
        return false;
    }
    if (system->move_gate != NULL &&
        !system->move_gate(system->move_context, particle->x, particle->y, x, y))
    {
        return false;
    }
    system->occupied[(uint8_t)particle->y] &= (uint16_t)~(1U << particle->x);
    system->occupied[y] |= (uint16_t)(1U << x);
    particle->x = (int8_t)x;
    particle->y = (int8_t)y;
    return true;
}

static int gravity_q8(float value)
{
    if (value > 1.0f)
    {
        value = 1.0f;
    }
    if (value < -1.0f)
    {
        value = -1.0f;
    }
    int result = (int)(value * 256.0f);
    /* About 0.08 g: a flat board should not drift from sensor noise. */
    return result > -20 && result < 20 ? 0 : result;
}

void Particles_Init(ParticleSystem *system, const uint16_t *walls, uint32_t seed)
{
    for (uint8_t y = 0U; y < DISPLAY_HEIGHT; y++)
    {
        system->occupied[y] = 0U;
        system->walls[y] = walls != NULL ? walls[y] : 0U;
    }
    system->count = 0U;
    system->random_state = seed != 0U ? seed : 0x6D2B79F5U;
    system->move_gate = NULL;
    system->move_context = NULL;
}

void Particles_SetMoveGate(ParticleSystem *system, ParticleMoveGate gate, void *context)
{
    system->move_gate = gate;
    system->move_context = context;
}

bool Particles_Add(ParticleSystem *system, int x, int y)
{
    if (system->count >= MAX_PARTICLES || solid(system, x, y) ||
        Particles_GetPixel(system, x, y))
    {
        return false;
    }
    Particle *particle = &system->particles[system->count++];
    particle->x = (int8_t)x;
    particle->y = (int8_t)y;
    /* Stagger small-tilt motion instead of moving every pixel in lockstep. */
    particle->move_credit = (uint16_t)(random_next(system) & 255U);
    system->occupied[y] |= (uint16_t)(1U << x);
    return true;
}

bool Particles_GetPixel(const ParticleSystem *system, int x, int y)
{
    return inside(x, y) && (system->occupied[y] & (uint16_t)(1U << x)) != 0U;
}

void Particles_Step(ParticleSystem *system, Gravity2D gravity, uint8_t shake)
{
    static const int8_t kick_x[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    static const int8_t kick_y[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    uint8_t order[MAX_PARTICLES];
    int gx = gravity_q8(gravity.x);
    int gy = gravity_q8(gravity.y);
    int ax = gx < 0 ? -gx : gx;
    int ay = gy < 0 ? -gy : gy;
    int major = ax >= ay ? ax : ay;
    int minor = ax >= ay ? ay : ax;

    for (uint8_t i = 0U; i < system->count; i++)
    {
        order[i] = i;
    }
    for (uint8_t n = system->count; n > 1U; n--)
    {
        uint8_t j = (uint8_t)(random_next(system) % n);
        uint8_t saved = order[n - 1U];
        order[n - 1U] = order[j];
        order[j] = saved;
    }

    for (uint8_t i = 0U; i < system->count; i++)
    {
        Particle *particle = &system->particles[order[i]];
        if (shake != 0U && (random_next(system) & 255U) < shake / 2U)
        {
            uint8_t direction = (uint8_t)(random_next(system) & 7U);
            if (try_move(system, particle, kick_x[direction], kick_y[direction]))
            {
                continue;
            }
        }
        if (major == 0)
        {
            continue;
        }
        particle->move_credit = (uint16_t)(particle->move_credit + major);
        if (particle->move_credit < 256U)
        {
            continue;
        }
        particle->move_credit -= 256U;

        int sx = gx < 0 ? -1 : 1;
        int sy = gy < 0 ? -1 : 1;
        if (minor == 0)
        {
            int side = (random_next(system) & 1U) != 0U ? 1 : -1;
            if (ax >= ay)
            {
                sy = side;
            }
            else
            {
                sx = side;
            }
        }
        int dx[3] = {ax >= ay ? sx : 0, sx, ax >= ay ? sx : -sx};
        int dy[3] = {ax >= ay ? 0 : sy, sy, ax >= ay ? -sy : sy};

        /* A small transverse component produces occasional diagonal steps;
         * it must not turn every nonzero tilt into a fixed 45-degree fall.
         */
        if (minor != 0 && random_next(system) % (unsigned)major < (unsigned)minor)
        {
            int saved = dx[0]; dx[0] = dx[1]; dx[1] = saved;
            saved = dy[0]; dy[0] = dy[1]; dy[1] = saved;
        }
        bool moved = false;
        for (uint8_t candidate = 0U; candidate < 3U; candidate++)
        {
            if (dx[candidate] * gx + dy[candidate] * gy > 0 &&
                try_move(system, particle, dx[candidate], dy[candidate]))
            {
                moved = true;
                break;
            }
        }
        if (!moved && minor != 0)
        {
            /* Slide along a wall on the secondary downhill axis when the
             * dominant direction is blocked (important for tilted chambers).
             */
            moved = try_move(system, particle, ax >= ay ? 0 : sx, ax >= ay ? sy : 0);
        }
        if (!moved && minor == 0 && (random_next(system) & 3U) == 0U)
        {
            /* Occasional level spreading helps a liquid surface relax.
             * It never climbs against gravity and uses the same wall checks.
             */
            (void)try_move(system, particle, ax >= ay ? 0 : sx, ax >= ay ? sy : 0);
        }
    }
}
