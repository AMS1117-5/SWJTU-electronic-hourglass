#ifndef PARTICLES_H
#define PARTICLES_H

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"
#include "motion.h"

#define MAX_PARTICLES 40U

typedef struct
{
    int8_t x;
    int8_t y;
    uint16_t move_credit;
} Particle;

/* Called after collision checks, immediately before committing a move.
 * May reserve a flow token when returning true; a rejected move is not committed.
 */
typedef bool (*ParticleMoveGate)(void *context, int from_x, int from_y,
                                 int to_x, int to_y);

typedef struct
{
    Particle particles[MAX_PARTICLES];
    uint16_t occupied[DISPLAY_HEIGHT];
    uint16_t walls[DISPLAY_HEIGHT];
    uint32_t random_state;
    uint8_t count;
    ParticleMoveGate move_gate;
    void *move_context;
} ParticleSystem;

/* walls may be NULL for a plain rectangular container. No HAL dependencies. */
void Particles_Init(ParticleSystem *system, const uint16_t *walls, uint32_t seed);
void Particles_SetMoveGate(ParticleSystem *system, ParticleMoveGate gate, void *context);
bool Particles_Add(ParticleSystem *system, int x, int y);
bool Particles_GetPixel(const ParticleSystem *system, int x, int y);
/* One fixed physics tick. Shake = 0..255; all moves remain wall/collision checked. */
void Particles_Step(ParticleSystem *system, Gravity2D gravity, uint8_t shake);

#endif
