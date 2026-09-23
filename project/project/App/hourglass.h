#ifndef HOURGLASS_H
#define HOURGLASS_H

#include <stdbool.h>
#include <stdint.h>

#define HOURGLASS_SAND_TOTAL 1000000U

typedef struct
{
    uint32_t sand_a;
    uint32_t sand_b;
    uint8_t particles_a;
    uint8_t particles_b;
    bool initialized;
    /* Set by a visible nonempty->empty transition; cleared when both have sand. */
    bool complete;
} HourglassState;

void Hourglass_Init(uint32_t now);
void Hourglass_Resume(uint32_t now);
/* Updates future flow only; current sand amounts and particle positions persist. */
bool Hourglass_SetGrainIntervalMs(uint16_t interval_ms);
uint16_t Hourglass_GetGrainIntervalMs(void);
void Hourglass_Update(uint32_t now);
void Hourglass_Render(void);
bool Hourglass_TakeCompletionEvent(void);
HourglassState Hourglass_GetState(void);

#endif
