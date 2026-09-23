#ifndef MOTION_H
#define MOTION_H

#include <stdbool.h>
#include <stdint.h>

/* Projection of the normalized 3D acceleration into virtual display axes.
 * In the charging-port-down pose: +x is physically down, +y is right.
 * The 2D magnitude is intentionally below one when tilted out of the plane.
 */
typedef struct
{
    float x;
    float y;
} Gravity2D;

void Motion_Init(void);
void Motion_Update(uint32_t now);
bool Motion_IsValid(void);
Gravity2D Motion_GetDisplayGravity(void);
/* High-pass acceleration activity, 0 (quiet) .. 255 (strong disturbance). */
uint8_t Motion_GetShakeStrength(void);

#endif
