#ifndef TEST_STM32_HAL_H
#define TEST_STM32_HAL_H

#include <stdint.h>

typedef enum { HAL_OK = 0, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET } GPIO_PinState;
typedef struct { unsigned unused; } GPIO_TypeDef;
typedef struct { unsigned unused; } SPI_HandleTypeDef;
#define GPIO_PIN_0 1U
#define GPIO_PIN_1 2U
#define GPIO_PIN_4 16U
#define GPIOA ((GPIO_TypeDef *)(uintptr_t)1U)
#define GPIOB ((GPIO_TypeDef *)(uintptr_t)2U)

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *spi, uint8_t *data,
                                  uint16_t length, uint32_t timeout);

#endif
