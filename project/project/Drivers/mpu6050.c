#include "mpu6050.h"

#include <stddef.h>

#include "app_config.h"
#include "i2c.h"

#define MPU6050_HAL_ADDRESS (MPU6050_ADDRESS_7BIT << 1U)
#define REG_SMPLRT_DIV  0x19U
#define REG_CONFIG      0x1AU
#define REG_GYRO_CONFIG 0x1BU
#define REG_ACCEL_CONFIG 0x1CU
#define REG_INT_ENABLE  0x38U
#define REG_INT_STATUS  0x3AU
#define REG_PWR_MGMT_1  0x6BU
#define REG_PWR_MGMT_2  0x6CU
#define REG_WHO_AM_I    0x75U

typedef enum
{
    STATE_POWER_WAIT,
    STATE_PROBE,
    STATE_RESET,
    STATE_RESET_WAIT,
    STATE_CONFIGURE,
    STATE_SAMPLE,
    STATE_RETRY
} InitState;

static const uint8_t configuration[][2] = {
    {REG_PWR_MGMT_1, 0x01U}, /* Wake, gyro X PLL clock. */
    {REG_PWR_MGMT_2, 0x00U}, /* Enable all six axes. */
    {REG_CONFIG, 0x03U},     /* DLPF: accel 44 Hz, gyro 42 Hz; 1 kHz base rate. */
    {REG_SMPLRT_DIV, 9U},    /* 1000 / (1 + 9) = 100 Hz. */
    {REG_GYRO_CONFIG, 0U},   /* +/-250 degrees/second. */
    {REG_ACCEL_CONFIG, 0U},  /* +/-2 g. */
    {REG_INT_ENABLE, 0x01U}  /* Poll DATA_RDY in status; no INT pin required. */
};

static InitState state;
static Mpu6050Status status;
static Mpu6050Sample latest;
static uint8_t who_am_i;
static uint8_t config_index;
static uint32_t state_since;
static uint32_t last_poll;
static uint32_t last_data_at;
static bool has_sample;

static bool read_registers(uint8_t reg, uint8_t *data, uint16_t length)
{
    /* Short bounded transactions; F0 HAL also has its own 25 ms BUSY timeout. */
    return HAL_I2C_Mem_Read(&hi2c1, MPU6050_HAL_ADDRESS, reg,
                            I2C_MEMADD_SIZE_8BIT, data, length,
                            MPU6050_IO_TIMEOUT_MS) == HAL_OK;
}

static bool write_register(uint8_t reg, uint8_t value)
{
    return HAL_I2C_Mem_Write(&hi2c1, MPU6050_HAL_ADDRESS, reg,
                             I2C_MEMADD_SIZE_8BIT, &value, 1U,
                             MPU6050_IO_TIMEOUT_MS) == HAL_OK;
}

static void fail(Mpu6050Status reason, uint32_t now)
{
    status = reason;
    has_sample = false;
    state = STATE_RETRY;
    state_since = now;
}

static int16_t signed_be16(const uint8_t *bytes)
{
    uint16_t value = (uint16_t)(((uint16_t)bytes[0] << 8U) | bytes[1]);
    return (int16_t)((value & 0x8000U) != 0U ? (int32_t)value - 65536 : value);
}

void Mpu6050_Init(uint32_t now)
{
    state = STATE_POWER_WAIT;
    status = MPU6050_STARTING;
    latest = (Mpu6050Sample){0};
    who_am_i = 0U;
    config_index = 0U;
    state_since = now;
    last_poll = now;
    last_data_at = now;
    has_sample = false;
}

void Mpu6050_Update(uint32_t now)
{
    switch (state)
    {
        case STATE_POWER_WAIT:
        case STATE_RESET_WAIT:
            if ((uint32_t)(now - state_since) >= 100U)
            {
                state = state == STATE_POWER_WAIT ? STATE_PROBE : STATE_CONFIGURE;
            }
            break;

        case STATE_PROBE:
            who_am_i = 0U;
            if (!read_registers(REG_WHO_AM_I, &who_am_i, 1U))
            {
                fail(MPU6050_BUS_ERROR, now);
            }
            else if (who_am_i != 0x68U)
            {
                fail(MPU6050_ID_ERROR, now);
            }
            else
            {
                state = STATE_RESET;
            }
            break;

        case STATE_RESET:
            if (!write_register(REG_PWR_MGMT_1, 0x80U))
            {
                fail(MPU6050_BUS_ERROR, now);
                break;
            }
            state = STATE_RESET_WAIT;
            state_since = now;
            config_index = 0U;
            break;

        case STATE_CONFIGURE:
            /* Allow the clock/gyro to settle after waking, without HAL_Delay. */
            if (config_index == 1U && (uint32_t)(now - state_since) < 50U)
            {
                break;
            }
            if (!write_register(configuration[config_index][0],
                                configuration[config_index][1]))
            {
                fail(MPU6050_BUS_ERROR, now);
                break;
            }
            config_index++;
            state_since = now;
            if (config_index == sizeof(configuration) / sizeof(configuration[0]))
            {
                state = STATE_SAMPLE;
                last_poll = now;
                last_data_at = now;
            }
            break;

        case STATE_SAMPLE:
        {
            uint8_t bytes[15];
            if ((uint32_t)(now - last_poll) < MPU6050_SAMPLE_PERIOD_MS)
            {
                break;
            }
            last_poll = now;
            /* Status + contiguous 14-byte accel/temperature/gyro snapshot. */
            if (!read_registers(REG_INT_STATUS, bytes, sizeof(bytes)))
            {
                fail(MPU6050_BUS_ERROR, now);
                break;
            }
            if ((bytes[0] & 1U) == 0U)
            {
                if ((uint32_t)(now - last_data_at) >= MPU6050_STALE_MS)
                {
                    fail(MPU6050_DATA_ERROR, now);
                }
                break;
            }
            for (uint8_t axis = 0U; axis < 3U; axis++)
            {
                latest.accel[axis] = signed_be16(&bytes[1U + axis * 2U]);
                latest.gyro[axis] = signed_be16(&bytes[9U + axis * 2U]);
            }
            latest.temperature = signed_be16(&bytes[7]);
            latest.timestamp_ms = now;
            latest.sequence++;
            last_data_at = now;
            has_sample = true;
            status = MPU6050_READY;
            break;
        }

        case STATE_RETRY:
            if ((uint32_t)(now - state_since) >= MPU6050_RETRY_PERIOD_MS)
            {
                status = MPU6050_STARTING;
                state = STATE_PROBE;
            }
            break;
    }
}

Mpu6050Status Mpu6050_GetStatus(void)
{
    return status;
}

uint8_t Mpu6050_GetWhoAmI(void)
{
    return who_am_i;
}

bool Mpu6050_GetSample(Mpu6050Sample *sample, uint32_t now)
{
    if (sample == NULL || status != MPU6050_READY || !has_sample ||
        (uint32_t)(now - latest.timestamp_ms) >= MPU6050_STALE_MS)
    {
        return false;
    }
    *sample = latest;
    return true;
}
