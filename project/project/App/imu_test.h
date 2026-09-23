#ifndef IMU_TEST_H
#define IMU_TEST_H

#include <stdbool.h>
#include <stdint.h>

void ImuTest_Init(void);
void ImuTest_Update(uint32_t now);
void ImuTest_SelectPage(bool next);
void ImuTest_Render(void);

#endif
