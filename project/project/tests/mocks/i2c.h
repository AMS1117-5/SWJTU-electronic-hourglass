#ifndef TEST_I2C_H
#define TEST_I2C_H

#include <stdint.h>
#include "stm32f0xx_hal.h"

typedef struct { unsigned unused; } I2C_HandleTypeDef;
#define I2C_MEMADD_SIZE_8BIT 1U
extern I2C_HandleTypeDef hi2c1;

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *bus, uint16_t address,
                                  uint16_t reg, uint16_t address_size,
                                  uint8_t *data, uint16_t length, uint32_t timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *bus, uint16_t address,
                                   uint16_t reg, uint16_t address_size,
                                   uint8_t *data, uint16_t length, uint32_t timeout);

#endif
