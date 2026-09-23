#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "particles.h"

static void check(const ParticleSystem *system, unsigned expected_count)
{
    uint16_t observed[DISPLAY_HEIGHT] = {0};
    assert(system->count == expected_count);
    for (unsigned i = 0; i < system->count; i++)
    {
        const Particle *particle = &system->particles[i];
        assert(particle->x >= 0 && particle->x < (int)DISPLAY_WIDTH);
        assert(particle->y >= 0 && particle->y < (int)DISPLAY_HEIGHT);
        uint16_t bit = (uint16_t)(1U << particle->x);
        assert((observed[(unsigned)particle->y] & bit) == 0U);
        assert((system->walls[(unsigned)particle->y] & bit) == 0U);
        assert(particle->move_credit < 256U);
        observed[(unsigned)particle->y] |= bit;
    }
    assert(memcmp(observed, system->occupied, sizeof(observed)) == 0);
}

static void seed_water(ParticleSystem *system, uint32_t seed)
{
    Particles_Init(system, NULL, seed);
    for (unsigned i = 0; i < 32U; i++)
    {
        unsigned cell = (i * 37U + 11U) % 128U;
        assert(Particles_Add(system, cell % 16U, cell / 16U));
    }
    check(system, 32U);
}

static void settle(ParticleSystem *system, Gravity2D g)
{
    for (unsigned i = 0; i < 2000U; i++)
    {
        Particles_Step(system, g, 0U);
        check(system, 32U);
    }
}

int main(void)
{
    ParticleSystem system;
    Particles_Init(&system, NULL, 0U);
    assert(system.random_state != 0U);
    assert(!Particles_Add(&system, -1, 0));
    assert(!Particles_Add(&system, 0, -1));
    assert(!Particles_Add(&system, 16, 0));
    assert(!Particles_Add(&system, 0, 8));
    assert(Particles_Add(&system, 1, 3));
    assert(!Particles_Add(&system, 1, 3));
    assert(!Particles_GetPixel(&system, -1, -1));
    system.particles[0].move_credit = 0U;
    for (unsigned i = 0; i < 3U; i++)
    {
        Particles_Step(&system, (Gravity2D){0.25f, 0.0f}, 0U);
        assert(system.particles[0].x == 1);
    }
    Particles_Step(&system, (Gravity2D){0.25f, 0.0f}, 0U);
    assert(system.particles[0].x == 2 && system.particles[0].y == 3);
    check(&system, 1U);

    seed_water(&system, 123U);
    uint16_t original[DISPLAY_HEIGHT];
    memcpy(original, system.occupied, sizeof(original));
    for (unsigned i = 0; i < 500U; i++)
    {
        Particles_Step(&system, (Gravity2D){0.01f, -0.02f}, 0U);
    }
    assert(memcmp(original, system.occupied, sizeof(original)) == 0);

    settle(&system, (Gravity2D){1.0f, 0.0f});
    for (unsigned i = 0; i < system.count; i++)
        assert(system.particles[i].x >= 12);
    settle(&system, (Gravity2D){-1.0f, 0.0f});
    for (unsigned i = 0; i < system.count; i++)
        assert(system.particles[i].x <= 3);
    settle(&system, (Gravity2D){0.0f, 1.0f});
    for (unsigned i = 0; i < system.count; i++)
        assert(system.particles[i].y >= 6);
    settle(&system, (Gravity2D){0.0f, -1.0f});
    for (unsigned i = 0; i < system.count; i++)
        assert(system.particles[i].y <= 1);

    /* No particle may cross a continuous wall, even under maximum shake. */
    uint16_t walls[DISPLAY_HEIGHT];
    for (unsigned y = 0; y < DISPLAY_HEIGHT; y++)
        walls[y] = 1U << 8U;
    Particles_Init(&system, walls, 99U);
    assert(!Particles_Add(&system, 8, 3));
    for (unsigned i = 0; i < 32U; i++)
        assert(Particles_Add(&system, i % 4U, i / 4U));
    for (unsigned step = 0; step < 3000U; step++)
    {
        Particles_Step(&system, (Gravity2D){1.0f, 0.25f}, 255U);
        check(&system, 32U);
        for (unsigned i = 0; i < system.count; i++)
            assert(system.particles[i].x < 8);
    }

    /* An empty diagonal destination cannot bypass two solid corner neighbors. */
    memset(walls, 0, sizeof(walls));
    walls[0] = 1U << 1U;
    walls[1] = 1U;
    Particles_Init(&system, walls, 55U);
    assert(Particles_Add(&system, 0, 0));
    for (unsigned i = 0; i < 1000U; i++)
    {
        Particles_Step(&system, (Gravity2D){0.7f, 0.7f}, 255U);
        check(&system, 1U);
        assert(system.particles[0].x == 0 && system.particles[0].y == 0);
    }

    /* Seeded runs must reproduce; arbitrary rotations and shake conserve mass.
     * Each individual particle gets at most one adjacent move per step.
     */
    ParticleSystem reference;
    seed_water(&system, 567U);
    seed_water(&reference, 567U);
    const Gravity2D directions[] = {
        {1.0f, 0.0f}, {-1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, -1.0f},
        {0.7f, 0.7f}, {-0.7f, 0.7f}, {0.7f, -0.7f}, {-0.7f, -0.7f}
    };
    for (unsigned step = 0; step < 10000U; step++)
    {
        Particle before[MAX_PARTICLES];
        memcpy(before, system.particles, system.count * sizeof(Particle));
        Gravity2D g = directions[(step / 37U) % 8U];
        uint8_t shake = step % 100U < 30U ? 255U : 0U;
        Particles_Step(&system, g, shake);
        Particles_Step(&reference, g, shake);
        check(&system, 32U);
        assert(memcmp(system.occupied, reference.occupied, sizeof(system.occupied)) == 0);
        for (unsigned i = 0; i < system.count; i++)
        {
            int dx = system.particles[i].x - before[i].x;
            int dy = system.particles[i].y - before[i].y;
            assert(dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1);
            assert(system.particles[i].x == reference.particles[i].x);
            assert(system.particles[i].y == reference.particles[i].y);
        }
    }

    Particles_Init(&system, NULL, 9U);
    for (unsigned i = 0; i < MAX_PARTICLES; i++)
        assert(Particles_Add(&system, i % 16U, i / 16U));
    assert(!Particles_Add(&system, 15, 7));
    check(&system, MAX_PARTICLES);
    puts("PASS: conservation, four-direction settling, flip, fractional speed, walls, corners, capacity, deterministic stress");
    return 0;
}
