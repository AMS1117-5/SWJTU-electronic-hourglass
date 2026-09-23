#include "max7219.h"

#include "main.h"
#include "spi.h"

#define MAX7219_REG_DECODE_MODE  0x09U
#define MAX7219_REG_INTENSITY    0x0AU
#define MAX7219_REG_SCAN_LIMIT   0x0BU
#define MAX7219_REG_SHUTDOWN     0x0CU
#define MAX7219_REG_DISPLAY_TEST 0x0FU

void Max7219_WriteRegister(uint8_t reg, const uint8_t values[MAX7219_DEVICE_COUNT])
{
    uint8_t tx[4] = {
        reg, values[1],
        reg, values[0]
    };

    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
    (void)HAL_SPI_Transmit(&hspi1, tx, sizeof(tx), 10U);
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);
}

void Max7219_SetIntensity(uint8_t intensity)
{
    if (intensity > 15U)
    {
        intensity = 15U;
    }
    uint8_t values[MAX7219_DEVICE_COUNT] = {intensity, intensity};
    Max7219_WriteRegister(MAX7219_REG_INTENSITY, values);
}

void Max7219_Init(uint8_t intensity)
{
    uint8_t values[MAX7219_DEVICE_COUNT];

    values[0] = 0U;
    values[1] = 0U;
    Max7219_WriteRegister(MAX7219_REG_SHUTDOWN, values);
    Max7219_WriteRegister(MAX7219_REG_DISPLAY_TEST, values);
    Max7219_WriteRegister(MAX7219_REG_DECODE_MODE, values);

    values[0] = 7U;
    values[1] = 7U;
    Max7219_WriteRegister(MAX7219_REG_SCAN_LIMIT, values);

    Max7219_SetIntensity(intensity);

    /* Clear display RAM while shut down, avoiding a power-on garbage flash. */
    values[0] = 0U;
    values[1] = 0U;
    for (uint8_t row = 1U; row <= 8U; row++)
    {
        Max7219_WriteRegister(row, values);
    }

    values[0] = 1U;
    values[1] = 1U;
    Max7219_WriteRegister(MAX7219_REG_SHUTDOWN, values);
}
