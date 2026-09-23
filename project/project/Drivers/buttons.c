#include "buttons.h"

#include "app_config.h"
#include "main.h"

typedef struct
{
    bool raw_pressed;
    bool stable_pressed;
    bool long_reported;
    uint32_t raw_changed_at;
    uint32_t pressed_at;
} ButtonState;

static ButtonState button_states[BUTTON_COUNT];
static uint32_t last_scan_at;
static uint8_t pending_events;

static bool read_button(ButtonId button)
{
    if (button == BUTTON_K1)
    {
        return HAL_GPIO_ReadPin(K1_GPIO_Port, K1_Pin) == GPIO_PIN_RESET;
    }

    return HAL_GPIO_ReadPin(K2_GPIO_Port, K2_Pin) == GPIO_PIN_RESET;
}

static uint8_t short_event(ButtonId button)
{
    return button == BUTTON_K1 ? BUTTON_EVENT_K1_SHORT : BUTTON_EVENT_K2_SHORT;
}

static uint8_t long_event(ButtonId button)
{
    return button == BUTTON_K1 ? BUTTON_EVENT_K1_LONG : BUTTON_EVENT_K2_LONG;
}

void Buttons_Init(uint32_t now)
{
    for (ButtonId button = BUTTON_K1; button < BUTTON_COUNT; button++)
    {
        bool pressed = read_button(button);
        button_states[button].raw_pressed = pressed;
        button_states[button].stable_pressed = pressed;
        button_states[button].long_reported = false;
        button_states[button].raw_changed_at = now;
        button_states[button].pressed_at = now;
    }

    last_scan_at = now;
    pending_events = BUTTON_EVENT_NONE;
}

void Buttons_Update(uint32_t now)
{
    if ((uint32_t)(now - last_scan_at) < BUTTON_SCAN_PERIOD_MS)
    {
        return;
    }
    last_scan_at = now;

    for (ButtonId button = BUTTON_K1; button < BUTTON_COUNT; button++)
    {
        ButtonState *state = &button_states[button];
        bool pressed = read_button(button);

        if (pressed != state->raw_pressed)
        {
            state->raw_pressed = pressed;
            state->raw_changed_at = now;
        }

        if ((state->raw_pressed != state->stable_pressed) &&
            ((uint32_t)(now - state->raw_changed_at) >= BUTTON_DEBOUNCE_MS))
        {
            state->stable_pressed = state->raw_pressed;
            if (state->stable_pressed)
            {
                state->pressed_at = now;
                state->long_reported = false;
            }
            else if (!state->long_reported)
            {
                pending_events |= short_event(button);
            }
        }

        if (state->stable_pressed && !state->long_reported &&
            ((uint32_t)(now - state->pressed_at) >= BUTTON_LONG_PRESS_MS))
        {
            state->long_reported = true;
            pending_events |= long_event(button);
        }
    }
}

uint8_t Buttons_TakeEvents(void)
{
    uint8_t events = pending_events;
    pending_events = BUTTON_EVENT_NONE;
    return events;
}

bool Buttons_IsPressed(ButtonId button)
{
    if (button >= BUTTON_COUNT)
    {
        return false;
    }
    return button_states[button].stable_pressed;
}
