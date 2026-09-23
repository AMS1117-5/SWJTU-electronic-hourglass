#include <assert.h>
#include <stdio.h>

#include "buttons.h"
#include "main.h"

static uint16_t pressed_pins;
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    assert(port == GPIOA);
    return (pressed_pins & pin) != 0U ? GPIO_PIN_RESET : GPIO_PIN_SET;
}

static void check_key(uint16_t pin, ButtonId key, uint8_t short_event,
                       uint8_t long_event, uint32_t start)
{
    pressed_pins = 0U;
    Buttons_Init(start);
    pressed_pins = pin;
    Buttons_Update(start + 5U);
    Buttons_Update(start + 25U);
    assert(!Buttons_IsPressed(key));
    Buttons_Update(start + 30U);
    assert(Buttons_IsPressed(key));
    assert(!Buttons_IsPressed(key == BUTTON_K1 ? BUTTON_K2 : BUTTON_K1));
    pressed_pins = 0U;
    Buttons_Update(start + 100U);
    Buttons_Update(start + 125U);
    assert(Buttons_TakeEvents() == short_event);
    assert(Buttons_TakeEvents() == 0U);

    pressed_pins = pin;
    Buttons_Update(start + 150U);
    Buttons_Update(start + 175U);
    Buttons_Update(start + 875U);
    assert(Buttons_TakeEvents() == long_event);
    Buttons_Update(start + 900U);
    assert(Buttons_TakeEvents() == 0U);
    pressed_pins = 0U;
    Buttons_Update(start + 925U);
    Buttons_Update(start + 950U);
    assert(Buttons_TakeEvents() == 0U);

    pressed_pins = pin;
    Buttons_Update(start + 975U);
    pressed_pins = 0U;
    Buttons_Update(start + 980U);
    Buttons_Update(start + 1010U);
    assert(Buttons_TakeEvents() == 0U);
}

int main(void)
{
    assert(K1_Pin == GPIO_PIN_1 && K2_Pin == GPIO_PIN_0);
    check_key(K1_Pin, BUTTON_K1, BUTTON_EVENT_K1_SHORT, BUTTON_EVENT_K1_LONG, 0U);
    check_key(K2_Pin, BUTTON_K2, BUTTON_EVENT_K2_SHORT, BUTTON_EVENT_K2_LONG, UINT32_MAX - 300U);
    puts("PASS: physical key mapping, debounce, short/long exclusivity, bounce, tick wrap");
    return 0;
}
