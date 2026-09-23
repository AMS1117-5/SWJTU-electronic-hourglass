#ifndef OCEAN_H
#define OCEAN_H

#include <stdint.h>

void Ocean_Init(uint32_t now);
void Ocean_Resume(uint32_t now);
void Ocean_Update(uint32_t now);
void Ocean_Render(void);

#endif
