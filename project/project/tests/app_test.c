#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "app.h"
#include "buttons.h"
#include "buzzer.h"
#include "display.h"
#include "hourglass.h"
#include "main.h"
#include "max7219.h"
#include "menu.h"
#include "motion.h"
#include "mpu6050.h"
#include "settings.h"
#include "tim.h"

TIM_HandleTypeDef htim3;
static uint32_t now;
static uint16_t keys_down;
static bool motion_valid;
static Mpu6050Status sensor_status;
static uint8_t hardware_brightness;
static unsigned display_writes;
static unsigned pwm_starts;
static bool pwm_on;

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    assert(port == GPIOA);
    return (keys_down & pin) != 0U ? GPIO_PIN_RESET : GPIO_PIN_SET;
}
bool Motion_IsValid(void) { return motion_valid; }
Gravity2D Motion_GetDisplayGravity(void) { return (Gravity2D){1.0f, 0.0f}; }
uint8_t Motion_GetShakeStrength(void) { return 0U; }
Mpu6050Status Mpu6050_GetStatus(void) { return sensor_status; }

void Max7219_Init(uint8_t intensity) { hardware_brightness = intensity; }
void Max7219_SetIntensity(uint8_t intensity)
{
    assert(intensity <= 2U);
    hardware_brightness = intensity;
}
void Max7219_WriteRegister(uint8_t reg, const uint8_t values[2])
{
    (void)values;
    assert(reg >= 1U && reg <= 8U);
    display_writes++;
}

void HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef *clocks, uint32_t *latency)
{
    clocks->APB1CLKDivider = RCC_HCLK_DIV1;
    *latency = 1U;
}
uint32_t HAL_RCC_GetPCLK1Freq(void) { return 48000000U; }
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *timer, uint32_t channel)
{
    assert(timer == &htim3 && channel == TIM_CHANNEL_3);
    pwm_starts++;
    pwm_on = true;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *timer, uint32_t channel)
{
    assert(timer == &htim3 && channel == TIM_CHANNEL_3);
    pwm_on = false;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_GenerateEvent(TIM_HandleTypeDef *timer, uint32_t event)
{
    assert(timer == &htim3 && event == TIM_EVENTSOURCE_UPDATE);
    return HAL_OK;
}

static void boot(uint32_t tick)
{
    now = tick;
    keys_down = 0U;
    motion_valid = true;
    sensor_status = MPU6050_READY;
    htim3.Init.Prescaler = 47U;
    Buttons_Init(now);
    Buzzer_Init();
    App_Init(now);
    assert(App_GetMode() == APP_HOURGLASS);
    assert(Settings_GetGrainIntervalMs() == 10000U && Settings_GetBrightness() == 0U);
    assert(hardware_brightness == 0U);
}

static void advance(unsigned milliseconds)
{
    assert(milliseconds % 5U == 0U);
    for (unsigned t = 0U; t < milliseconds; t += 5U)
    {
        now += 5U;
        Buttons_Update(now);
        App_Update(now);
        Buzzer_Update(now);
    }
}

static void tap(uint16_t pin)
{
    keys_down = pin;
    advance(80U);
    keys_down = 0U;
    advance(40U);
}

static void confirm(void)
{
    keys_down = K1_Pin;
    advance(800U);
    keys_down = 0U;
    advance(40U);
}

static void check_frozen(HourglassState original)
{
    HourglassState current = Hourglass_GetState();
    assert(current.sand_a == original.sand_a && current.sand_b == original.sand_b);
    assert(current.particles_a == original.particles_a && current.particles_b == original.particles_b);
    assert(current.initialized == original.initialized && current.complete == original.complete);
}

int main(void)
{
    boot(0U);
    advance(2020U);
    assert(Hourglass_GetState().initialized);
    tap(K2_Pin); /* Product short press is not the diagnostic alarm preview. */
    assert(pwm_starts == 0U);
    confirm();
    assert(App_GetMode() == APP_MENU && Menu_GetSelection() == MENU_HOURGLASS);
    HourglassState paused = Hourglass_GetState();
    advance(10000U);
    check_frozen(paused);
    assert(App_GetMode() == APP_MENU); /* Held K1 did not auto-confirm twice. */

    tap(K1_Pin); /* Menu wraps from item 1 to item 4. */
    assert(Menu_GetSelection() == MENU_BRIGHTNESS);
    confirm();
    assert(App_GetMode() == APP_BRIGHTNESS);
    tap(K1_Pin);
    assert(hardware_brightness == 1U && Settings_GetBrightness() == 1U);
    tap(K1_Pin);
    tap(K1_Pin);
    assert(hardware_brightness == 2U && Display_GetBrightness() == 2U);
    for (unsigned i = 0; i < 3U; i++) tap(K2_Pin);
    assert(hardware_brightness == 0U);
    tap(K1_Pin);
    tap(K1_Pin);
    confirm();
    assert(App_GetMode() == APP_MENU && Menu_GetSelection() == MENU_BRIGHTNESS);
    check_frozen(paused);

    tap(K1_Pin);
    assert(Menu_GetSelection() == MENU_TIME);
    confirm();
    assert(App_GetMode() == APP_SETTINGS && Settings_GetDurationSec() == 240U);
    tap(K2_Pin);
    assert(Settings_GetDurationSec() == 210U && Hourglass_GetGrainIntervalMs() == 8750U);
    for (unsigned i = 0; i < 7U; i++) tap(K2_Pin);
    assert(Settings_GetDurationSec() == 30U && Hourglass_GetGrainIntervalMs() == 1250U);
    assert(hardware_brightness == 2U);
    check_frozen(paused);
    confirm();
    assert(App_GetMode() == APP_MENU && Menu_GetSelection() == MENU_TIME);

    tap(K1_Pin);
    assert(Menu_GetSelection() == MENU_OCEAN);
    confirm();
    assert(App_GetMode() == APP_OCEAN);
    advance(2000U);
    check_frozen(paused);
    confirm();
    assert(App_GetMode() == APP_MENU && Menu_GetSelection() == MENU_OCEAN);
    tap(K1_Pin);
    confirm();
    assert(App_GetMode() == APP_HOURGLASS);
    HourglassState resumed = Hourglass_GetState();
    assert(resumed.sand_a < paused.sand_a && paused.sand_a - resumed.sand_a < 10000U);
    assert(Settings_GetGrainIntervalMs() == 1250U && hardware_brightness == 2U);

    /* Finish with the new rate, then operate the menu during the real buzzer FSM. */
    for (unsigned waited = 0U; waited < 32000U && !Buzzer_IsPlaying(); waited += 20U)
        advance(20U);
    assert(Buzzer_IsPlaying() && pwm_on && Hourglass_GetState().complete);
    paused = Hourglass_GetState();
    assert(paused.sand_a == 0U);
    confirm();
    assert(App_GetMode() == APP_MENU && Buzzer_IsPlaying());
    unsigned before_display = display_writes;
    tap(K1_Pin);
    confirm();
    assert(App_GetMode() == APP_BRIGHTNESS && Buzzer_IsPlaying());
    tap(K2_Pin);
    assert(hardware_brightness == 1U && display_writes > before_display);
    advance(6000U);
    assert(!Buzzer_IsPlaying() && !pwm_on);
    assert(pwm_starts == 8U);
    check_frozen(paused);

    /* Reboot restores defaults. A failed sensor must not trap the user in a mode. */
    boot(now);
    advance(2020U);
    paused = Hourglass_GetState();
    motion_valid = false;
    sensor_status = MPU6050_BUS_ERROR;
    advance(1000U);
    check_frozen(paused);
    assert(Display_GetPixel(0U, 0U) && Display_GetPixel(0U, 6U)); /* E */
    assert(Display_GetPixel(1U, 0U) && !Display_GetPixel(1U, 1U));
    confirm();
    assert(App_GetMode() == APP_MENU);
    motion_valid = true;
    sensor_status = MPU6050_READY;
    advance(1000U);
    check_frozen(paused);
    confirm();
    assert(App_GetMode() == APP_HOURGLASS);
    advance(1000U);
    assert(Hourglass_GetState().sand_a < paused.sand_a);

    boot(UINT32_MAX - 300U);
    confirm();
    assert(App_GetMode() == APP_MENU && Menu_GetSelection() == MENU_HOURGLASS);
    puts("PASS: real key debounce -> menu/settings -> physical brightness, frozen/resumed sand, live rate changes, alarm/menu concurrency, recovery, reboot, tick wrap");
    return 0;
}
