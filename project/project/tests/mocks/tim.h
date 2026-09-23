#ifndef TEST_TIM_H
#define TEST_TIM_H

#include "i2c.h"

#define TIM_CHANNEL_3 3U
#define TIM_EVENTSOURCE_UPDATE 1U
#define RCC_HCLK_DIV1 1U
typedef struct { uint32_t APB1CLKDivider; } RCC_ClkInitTypeDef;
typedef struct
{
    struct { uint32_t Prescaler; } Init;
    uint32_t arr;
    uint32_t compare;
    uint32_t counter;
} TIM_HandleTypeDef;
extern TIM_HandleTypeDef htim3;

#define __HAL_TIM_SET_COMPARE(h, channel, value) ((h)->compare = (value))
#define __HAL_TIM_SET_AUTORELOAD(h, value) ((h)->arr = (value))
#define __HAL_TIM_SET_COUNTER(h, value) ((h)->counter = (value))

void HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef *clocks, uint32_t *latency);
uint32_t HAL_RCC_GetPCLK1Freq(void);
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *timer, uint32_t channel);
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *timer, uint32_t channel);
HAL_StatusTypeDef HAL_TIM_GenerateEvent(TIM_HandleTypeDef *timer, uint32_t event);

#endif
