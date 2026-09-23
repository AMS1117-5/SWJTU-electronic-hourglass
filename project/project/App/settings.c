#include "settings.h"

#include "app_config.h"

_Static_assert(HOURGLASS_MIN_DURATION_MS % HOURGLASS_PARTICLE_COUNT == 0U &&
               HOURGLASS_DURATION_STEP_MS % HOURGLASS_PARTICLE_COUNT == 0U,
               "Full-duration settings must convert exactly to millisecond grain intervals");
_Static_assert(HOURGLASS_MIN_GRAIN_INTERVAL_MS > 0U &&
               HOURGLASS_MIN_GRAIN_INTERVAL_MS <= HOURGLASS_GRAIN_INTERVAL_MS &&
               HOURGLASS_GRAIN_INTERVAL_MS <= HOURGLASS_MAX_GRAIN_INTERVAL_MS &&
               HOURGLASS_MAX_GRAIN_INTERVAL_MS <= 10000U,
               "Grain settings must remain positive and capped at ten seconds");
_Static_assert(HOURGLASS_GRAIN_INTERVAL_STEP_MS > 0U &&
               (HOURGLASS_MAX_GRAIN_INTERVAL_MS - HOURGLASS_MIN_GRAIN_INTERVAL_MS) %
                   HOURGLASS_GRAIN_INTERVAL_STEP_MS == 0U,
               "Time setting endpoints must align with the adjustment step");

static uint16_t grain_interval_ms;
static uint8_t brightness;

void Settings_Init(void)
{
    grain_interval_ms = HOURGLASS_GRAIN_INTERVAL_MS;
    brightness = DISPLAY_INTENSITY;
}

uint16_t Settings_GetGrainIntervalMs(void) { return grain_interval_ms; }
uint16_t Settings_GetDurationSec(void)
{
    return (uint16_t)((uint32_t)grain_interval_ms * HOURGLASS_PARTICLE_COUNT / 1000U);
}
uint8_t Settings_GetBrightness(void) { return brightness; }

bool Settings_SetGrainIntervalMs(uint16_t interval_ms)
{
    if (interval_ms < HOURGLASS_MIN_GRAIN_INTERVAL_MS ||
        interval_ms > HOURGLASS_MAX_GRAIN_INTERVAL_MS ||
        (interval_ms - HOURGLASS_MIN_GRAIN_INTERVAL_MS) % HOURGLASS_GRAIN_INTERVAL_STEP_MS != 0U)
    {
        return false;
    }
    grain_interval_ms = interval_ms;
    return true;
}

bool Settings_SetBrightness(uint8_t level)
{
    if (level > DISPLAY_BRIGHTNESS_MAX)
    {
        return false;
    }
    brightness = level;
    return true;
}

void Settings_AdjustTime(bool increase)
{
    if (increase && grain_interval_ms < HOURGLASS_MAX_GRAIN_INTERVAL_MS)
    {
        grain_interval_ms += HOURGLASS_GRAIN_INTERVAL_STEP_MS;
    }
    else if (!increase && grain_interval_ms > HOURGLASS_MIN_GRAIN_INTERVAL_MS)
    {
        grain_interval_ms -= HOURGLASS_GRAIN_INTERVAL_STEP_MS;
    }
}

void Settings_AdjustBrightness(bool increase)
{
    if (increase && brightness < DISPLAY_BRIGHTNESS_MAX)
    {
        brightness++;
    }
    else if (!increase && brightness > 0U)
    {
        brightness--;
    }
}
