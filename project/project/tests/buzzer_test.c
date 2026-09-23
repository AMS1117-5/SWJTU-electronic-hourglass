#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "buzzer.h"
#include "tim.h"

TIM_HandleTypeDef htim3;
static bool output_on;
static bool start_failure;
static bool divided_apb;
static unsigned starts;

void HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef *clocks, uint32_t *latency)
{
    clocks->APB1CLKDivider = divided_apb ? 2U : RCC_HCLK_DIV1;
    *latency = 1U;
}
uint32_t HAL_RCC_GetPCLK1Freq(void) { return divided_apb ? 24000000U : 48000000U; }
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *timer, uint32_t channel)
{
    assert(timer == &htim3 && channel == TIM_CHANNEL_3);
    starts++;
    if (start_failure) return HAL_ERROR;
    output_on = true;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *timer, uint32_t channel)
{
    assert(timer == &htim3 && channel == TIM_CHANNEL_3);
    output_on = false;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_GenerateEvent(TIM_HandleTypeDef *timer, uint32_t event)
{
    assert(timer == &htim3 && event == TIM_EVENTSOURCE_UPDATE);
    assert(timer->counter == 0U);
    return HAL_OK;
}

static void check_tone(unsigned hz)
{
    assert(output_on && Buzzer_IsPlaying());
    unsigned period = (1000000U + hz / 2U) / hz;
    assert(htim3.arr + 1U == period && htim3.compare == period / 2U);
}

int main(void)
{
    htim3.Init.Prescaler = 47U;
    Buzzer_Init();
    assert(!output_on && !Buzzer_IsPlaying() && htim3.compare == 0U);
    Buzzer_StartAlarm(100U);
    check_tone(523U);
    Buzzer_Update(599U);
    check_tone(523U);
    assert(starts == 1U);
    Buzzer_Update(600U);
    check_tone(659U);
    Buzzer_Update(1600U);
    check_tone(1047U);
    Buzzer_Update(2600U);
    check_tone(784U);
    Buzzer_Update(5099U);
    check_tone(523U);
    Buzzer_Update(5100U);
    assert(!output_on && !Buzzer_IsPlaying() && htim3.compare == 0U);

    unsigned previous = starts;
    Buzzer_StartAlarm(0U);
    Buzzer_Update(4900U);
    check_tone(523U);
    assert(starts == previous + 2U); /* Skip expired notes, don't play them late. */
    Buzzer_Update(5100U);
    assert(!Buzzer_IsPlaying());

    Buzzer_StartAlarm(UINT32_MAX - 1000U);
    Buzzer_Update((uint32_t)(UINT32_MAX - 1000U + 5000U));
    assert(!Buzzer_IsPlaying());

    divided_apb = true;
    Buzzer_Init();
    Buzzer_StartAlarm(0U);
    check_tone(523U);
    Buzzer_Stop();
    assert(!output_on && !Buzzer_IsPlaying());

    start_failure = true;
    Buzzer_StartAlarm(0U);
    assert(!output_on && !Buzzer_IsPlaying() && htim3.compare == 0U);
    puts("PASS: TIM3_CH3 notes, 5-second one-shot, nonblocking catch-up, APB clock, stop, failure, tick wrap");
    return 0;
}
