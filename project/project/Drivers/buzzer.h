#ifndef BUZZER_H
#define BUZZER_H

#include <stdbool.h>
#include <stdint.h>

void Buzzer_Init(void);
void Buzzer_StartAlarm(uint32_t now);
void Buzzer_Stop(void);
void Buzzer_Update(uint32_t now);
bool Buzzer_IsPlaying(void);

#endif
