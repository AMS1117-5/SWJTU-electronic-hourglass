#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "max7219.h"
#include "spi.h"

SPI_HandleTypeDef hspi1;
static uint8_t packets[32][4];
static unsigned count;
static bool cs_high = true;

void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    assert(port == SPI1_CS_GPIO_Port && pin == SPI1_CS_Pin);
    assert(cs_high != (state == GPIO_PIN_SET));
    cs_high = state == GPIO_PIN_SET;
}
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *spi, uint8_t *data,
                                  uint16_t length, uint32_t timeout)
{
    assert(spi == &hspi1 && !cs_high && length == 4U && timeout == 10U);
    assert(count < 32U);
    memcpy(packets[count++], data, 4U);
    return HAL_OK;
}

static void check_pair(unsigned index, uint8_t reg, uint8_t near_value, uint8_t far_value)
{
    assert(packets[index][0] == reg && packets[index][1] == far_value);
    assert(packets[index][2] == reg && packets[index][3] == near_value);
}

int main(void)
{
    Max7219_Init(0U);
    assert(count == 14U && cs_high);
    check_pair(0U, 0x0CU, 0U, 0U);
    check_pair(1U, 0x0FU, 0U, 0U);
    check_pair(2U, 0x09U, 0U, 0U);
    check_pair(3U, 0x0BU, 7U, 7U);
    check_pair(4U, 0x0AU, 0U, 0U);
    for (unsigned row = 1; row <= 8U; row++)
        check_pair(row + 4U, (uint8_t)row, 0U, 0U);
    check_pair(13U, 0x0CU, 1U, 1U);
    Max7219_SetIntensity(2U);
    check_pair(14U, 0x0AU, 2U, 2U);
    Max7219_SetIntensity(255U);
    check_pair(15U, 0x0AU, 15U, 15U);
    const uint8_t values[2] = {0x12U, 0x34U};
    Max7219_WriteRegister(3U, values);
    check_pair(16U, 3U, 0x12U, 0x34U);
    assert(cs_high);
    puts("PASS: cascaded SPI order, CS framing, shutdown/clear/wake, runtime intensity");
    return 0;
}
