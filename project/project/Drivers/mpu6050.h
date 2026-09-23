#ifndef MPU6050_H
#define MPU6050_H

#include <stdbool.h>
#include <stdint.h>

#define MPU6050_ADDRESS_7BIT 0x68U
#define MPU6050_ACCEL_LSB_PER_G 16384

typedef enum
{
    MPU6050_STARTING = 0,
    MPU6050_READY,
    MPU6050_BUS_ERROR,
    MPU6050_ID_ERROR,
    MPU6050_DATA_ERROR
} Mpu6050Status;

/* Sensor axes, deliberately independent of display coordinates.
 * Acceleration: 16384 LSB/g. Gyroscope: 131 LSB/(degree/second).
 */
typedef struct
{
    int16_t accel[3];
    int16_t gyro[3];
    int16_t temperature;
    uint32_t timestamp_ms;
    uint32_t sequence;
} Mpu6050Sample;

void Mpu6050_Init(uint32_t now);
void Mpu6050_Update(uint32_t now);
Mpu6050Status Mpu6050_GetStatus(void);
uint8_t Mpu6050_GetWhoAmI(void);
bool Mpu6050_GetSample(Mpu6050Sample *sample, uint32_t now);

#endif
