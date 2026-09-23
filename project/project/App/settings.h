#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

/* RAM settings: reset restores defaults. UI edits saturate at both endpoints. */
void Settings_Init(void);
uint16_t Settings_GetGrainIntervalMs(void);
uint16_t Settings_GetDurationSec(void);
uint8_t Settings_GetBrightness(void);
bool Settings_SetGrainIntervalMs(uint16_t interval_ms);
bool Settings_SetBrightness(uint8_t level);
void Settings_AdjustTime(bool increase);
void Settings_AdjustBrightness(bool increase);

#endif
