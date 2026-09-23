#include "buzzer.h"

#include "tim.h"

typedef struct
{
    uint16_t frequency;
    uint16_t duration_ms;
} Note;

/* C5 E5 G5 C6 G5 E5 D5 C5: exactly 5000 ms, timer-driven PWM. */
static const Note alarm_notes[] = {
    {523U, 500U}, {659U, 500U}, {784U, 500U}, {1047U, 1000U},
    {784U, 500U}, {659U, 500U}, {587U, 500U}, {523U, 1000U}
};
static uint32_t timer_tick_hz;
static uint32_t note_started_at;
static uint8_t note_index;
static bool playing;

void Buzzer_Stop(void)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0U);
    (void)HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
    playing = false;
}

static bool play_tone(uint16_t frequency)
{
    uint32_t period = (timer_tick_hz + frequency / 2U) / frequency;
    if (period < 2U || period > 65536U)
    {
        return false;
    }
    (void)HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
    __HAL_TIM_SET_AUTORELOAD(&htim3, period - 1U);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, period / 2U);
    __HAL_TIM_SET_COUNTER(&htim3, 0U);
    /* Latch PWM compare/prescaler before starting a new note. */
    if (HAL_TIM_GenerateEvent(&htim3, TIM_EVENTSOURCE_UPDATE) != HAL_OK)
    {
        return false;
    }
    return HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3) == HAL_OK;
}

void Buzzer_Init(void)
{
    RCC_ClkInitTypeDef clocks = {0};
    uint32_t flash_latency;
    HAL_RCC_GetClockConfig(&clocks, &flash_latency);
    uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();
    if (clocks.APB1CLKDivider != RCC_HCLK_DIV1)
    {
        timer_clock *= 2U;
    }
    timer_tick_hz = timer_clock / (htim3.Init.Prescaler + 1U);
    note_index = 0U;
    note_started_at = 0U;
    Buzzer_Stop();
}

void Buzzer_StartAlarm(uint32_t now)
{
    note_index = 0U;
    note_started_at = now;
    playing = true;
    if (!play_tone(alarm_notes[0].frequency))
    {
        Buzzer_Stop();
    }
}

void Buzzer_Update(uint32_t now)
{
    bool changed = false;
    while (playing &&
           (uint32_t)(now - note_started_at) >= alarm_notes[note_index].duration_ms)
    {
        note_started_at += alarm_notes[note_index].duration_ms;
        note_index++;
        if (note_index == sizeof(alarm_notes) / sizeof(alarm_notes[0]))
        {
            Buzzer_Stop();
            return;
        }
        changed = true;
    }
    if (changed && !play_tone(alarm_notes[note_index].frequency))
    {
        Buzzer_Stop();
    }
}

bool Buzzer_IsPlaying(void)
{
    return playing;
}
