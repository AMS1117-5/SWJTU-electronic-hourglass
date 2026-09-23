#include "motion.h"

#include "app_config.h"
#include "mpu6050.h"

#if MOTION_DISPLAY_X_AXIS > 2U || MOTION_DISPLAY_Y_AXIS > 2U || \
    MOTION_DISPLAY_X_AXIS == MOTION_DISPLAY_Y_AXIS
#error "Display gravity requires two different sensor axes"
#endif
#if (MOTION_DISPLAY_X_SIGN != 1 && MOTION_DISPLAY_X_SIGN != -1) || \
    (MOTION_DISPLAY_Y_SIGN != 1 && MOTION_DISPLAY_Y_SIGN != -1)
#error "Motion axis signs must be +1 or -1"
#endif
#if MOTION_FILTER_NEW_PERCENT < 1U || MOTION_FILTER_NEW_PERCENT > 100U
#error "Motion new-sample weight must be between 1 and 100 percent"
#endif

static int32_t filtered_q8[3];
static uint32_t first_sample_at;
static uint32_t last_sequence;
static bool filter_initialized;
static bool valid;
static Gravity2D gravity;
static uint8_t shake_strength;

/* Integer square root avoids a software sqrt call on Cortex-M0.
 * Input can span all uint32_t values; result is floor(sqrt(value)).
 */
static uint32_t integer_sqrt(uint32_t value)
{
    uint32_t result = 0U;
    uint32_t bit = 1UL << 30U;
    while (bit > value)
    {
        bit >>= 2U;
    }
    while (bit != 0U)
    {
        if (value >= result + bit)
        {
            value -= result + bit;
            result = (result >> 1U) + bit;
        }
        else
        {
            result >>= 1U;
        }
        bit >>= 2U;
    }
    return result;
}

void Motion_Init(void)
{
    for (uint8_t axis = 0U; axis < 3U; axis++)
    {
        filtered_q8[axis] = 0;
    }
    first_sample_at = 0U;
    last_sequence = 0U;
    filter_initialized = false;
    valid = false;
    gravity = (Gravity2D){0.0f, 0.0f};
    shake_strength = 0U;
}

void Motion_Update(uint32_t now)
{
    Mpu6050Sample sample;
    if (!Mpu6050_GetSample(&sample, now))
    {
        filter_initialized = false;
        valid = false;
        gravity = (Gravity2D){0.0f, 0.0f};
        shake_strength = 0U;
        return;
    }
    if (filter_initialized && sample.sequence == last_sequence)
    {
        return;
    }

    if (!filter_initialized)
    {
        first_sample_at = sample.timestamp_ms;
    }
    int32_t filtered[3];
    uint32_t magnitude_squared = 0U;
    uint32_t dynamic_sum = 0U;
    for (uint8_t axis = 0U; axis < 3U; axis++)
    {
        int32_t target = (int32_t)sample.accel[axis] * 256;
        if (!filter_initialized)
        {
            filtered_q8[axis] = target;
        }
        else
        {
            /* Configurable low-pass, once per NEW 100 Hz sample.
             * Q8 preserves fractional sensor counts without integer bias.
             */
            filtered_q8[axis] += (target - filtered_q8[axis]) *
                                 (int32_t)MOTION_FILTER_NEW_PERCENT / 100;
        }
        filtered[axis] = filtered_q8[axis] / 256;
        magnitude_squared += (uint32_t)(filtered[axis] * filtered[axis]);
        int32_t dynamic = (int32_t)sample.accel[axis] - filtered[axis];
        dynamic_sum += (uint32_t)(dynamic < 0 ? -dynamic : dynamic);
    }
    filter_initialized = true;
    last_sequence = sample.sequence;

    /* Static bias/tilt produces no sustained disturbance. Ignore small noise
     * and decay the peak once per fresh sample, never once per main loop.
     */
    shake_strength = (uint8_t)((uint16_t)shake_strength * 7U / 8U);
    if (dynamic_sum > MPU6050_ACCEL_LSB_PER_G / 4U)
    {
        uint32_t impulse = (dynamic_sum - MPU6050_ACCEL_LSB_PER_G / 4U) * 255U /
                           MPU6050_ACCEL_LSB_PER_G;
        if (impulse > 255U)
        {
            impulse = 255U;
        }
        if (impulse > shake_strength)
        {
            shake_strength = (uint8_t)impulse;
        }
    }

    uint32_t magnitude = integer_sqrt(magnitude_squared);
    valid = false;
    gravity = (Gravity2D){0.0f, 0.0f};
    if ((uint32_t)(sample.timestamp_ms - first_sample_at) < MOTION_SETTLE_MS ||
        magnitude < MPU6050_ACCEL_LSB_PER_G / 5U)
    {
        /* No stable direction yet, or nearly weightless: never amplify noise. */
        return;
    }

    /* Normalize all THREE axes before projection. Normalizing only AX/AY
     * would turn tiny flat-board noise into full-strength lateral gravity.
     */
    gravity.x = (float)(filtered[MOTION_DISPLAY_X_AXIS] * MOTION_DISPLAY_X_SIGN) /
                (float)magnitude;
    gravity.y = (float)(filtered[MOTION_DISPLAY_Y_AXIS] * MOTION_DISPLAY_Y_SIGN) /
                (float)magnitude;
    valid = true;
}

bool Motion_IsValid(void)
{
    return valid;
}

Gravity2D Motion_GetDisplayGravity(void)
{
    return gravity;
}

uint8_t Motion_GetShakeStrength(void)
{
    return valid ? shake_strength : 0U;
}
