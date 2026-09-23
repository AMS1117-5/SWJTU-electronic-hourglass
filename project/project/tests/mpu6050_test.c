#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "i2c.h"
#include "mpu6050.h"

I2C_HandleTypeDef hi2c1;
static uint8_t registers[256];
static uint32_t now;
static uint32_t reset_time;
static uint32_t wake_time;
static unsigned reads;
static unsigned writes;
static bool read_failure;
static int failed_write_reg;

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *bus, uint16_t address,
                                  uint16_t reg, uint16_t address_size,
                                  uint8_t *data, uint16_t length, uint32_t timeout)
{
    assert(bus == &hi2c1 && address == 0xD0U);
    assert(address_size == I2C_MEMADD_SIZE_8BIT && timeout == 5U);
    assert((reg == 0x75U && length == 1U) || (reg == 0x3AU && length == 15U));
    reads++;
    if (read_failure)
    {
        /* Even a partially modified receive buffer must never be published. */
        data[0] = 1U;
        return HAL_TIMEOUT;
    }
    memcpy(data, &registers[reg], length);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *bus, uint16_t address,
                                   uint16_t reg, uint16_t address_size,
                                   uint8_t *data, uint16_t length, uint32_t timeout)
{
    assert(bus == &hi2c1 && address == 0xD0U);
    assert(address_size == I2C_MEMADD_SIZE_8BIT && timeout == 5U && length == 1U);
    writes++;
    if (reg == failed_write_reg)
    {
        return HAL_ERROR;
    }
    if (reg == 0x6BU && data[0] == 0x80U)
    {
        reset_time = now;
    }
    if (reg == 0x6BU && data[0] == 1U)
    {
        assert((uint32_t)(now - reset_time) >= 100U);
        wake_time = now;
    }
    if (reg == 0x6CU)
    {
        assert((uint32_t)(now - wake_time) >= 50U);
    }
    registers[reg] = data[0];
    return HAL_OK;
}

static void put_word(unsigned reg, int16_t value)
{
    uint16_t bits = (uint16_t)value;
    registers[reg] = (uint8_t)(bits >> 8U);
    registers[reg + 1U] = (uint8_t)bits;
}

static void setup(uint32_t start)
{
    memset(registers, 0, sizeof(registers));
    now = start;
    reads = 0U;
    writes = 0U;
    read_failure = false;
    failed_write_reg = -1;
    registers[0x75] = 0x68;
    registers[0x3A] = 1U;
    put_word(0x3B, 16384);
    put_word(0x3D, -16384);
    put_word(0x3F, -32768);
    put_word(0x41, -340);
    put_word(0x43, 32767);
    put_word(0x45, -1);
    put_word(0x47, 0);
    Mpu6050_Init(now);
}

static void run(unsigned milliseconds)
{
    for (unsigned i = 0; i < milliseconds; i++)
    {
        unsigned before = reads + writes;
        Mpu6050_Update(++now);
        assert(reads + writes - before <= 1U);
    }
}

static void check_decoded_sample(void)
{
    Mpu6050Sample sample;
    assert(Mpu6050_GetStatus() == MPU6050_READY);
    assert(Mpu6050_GetWhoAmI() == 0x68U);
    assert(Mpu6050_GetSample(&sample, now));
    assert(sample.accel[0] == 16384 && sample.accel[1] == -16384);
    assert(sample.accel[2] == -32768 && sample.temperature == -340);
    assert(sample.gyro[0] == 32767 && sample.gyro[1] == -1 && sample.gyro[2] == 0);
    assert((uint32_t)(now - sample.timestamp_ms) < 10U);
    assert(sample.sequence > 0U);
    assert(!Mpu6050_GetSample(&sample, sample.timestamp_ms + 100U));
    assert(!Mpu6050_GetSample(NULL, now));
}

int main(void)
{
    Mpu6050Sample sample;
    setup(0U);
    run(99U);
    assert(reads == 0U && writes == 0U);
    assert(!Mpu6050_GetSample(&sample, now));
    run(251U);
    check_decoded_sample();
    assert(writes == 8U);
    assert(registers[0x6B] == 1U && registers[0x6C] == 0U);
    assert(registers[0x19] == 9U && registers[0x1A] == 3U);
    assert(registers[0x1B] == 0U && registers[0x1C] == 0U);
    assert(registers[0x38] == 1U);

    /* Fresh data must be gated by DATA_RDY, never just by successful I2C. */
    assert(Mpu6050_GetSample(&sample, now));
    uint32_t sequence = sample.sequence;
    registers[0x3A] = 0U;
    run(20U);
    assert(Mpu6050_GetSample(&sample, now));
    assert(sample.sequence == sequence);
    run(100U);
    assert(Mpu6050_GetStatus() == MPU6050_DATA_ERROR);
    assert(!Mpu6050_GetSample(&sample, now));
    registers[0x3A] = 1U;
    run(800U);
    check_decoded_sample();

    /* Bus failure invalidates previous samples and backs off before retry. */
    read_failure = true;
    run(10U);
    assert(Mpu6050_GetStatus() == MPU6050_BUS_ERROR);
    assert(!Mpu6050_GetSample(&sample, now));
    unsigned failed_reads = reads;
    run(400U);
    assert(reads == failed_reads);
    read_failure = false;
    run(500U);
    check_decoded_sample();

    setup(0U);
    registers[0x75] = 0x70U;
    run(350U);
    assert(Mpu6050_GetStatus() == MPU6050_ID_ERROR);
    assert(Mpu6050_GetWhoAmI() == 0x70U);
    assert(reads == 1U && writes == 0U);

    setup(0U);
    failed_write_reg = 0x1B;
    run(350U);
    assert(Mpu6050_GetStatus() == MPU6050_BUS_ERROR);
    assert(!Mpu6050_GetSample(&sample, now));

    /* Initialization and sample ages must survive the millisecond tick wrap. */
    setup(UINT32_MAX - 120U);
    run(350U);
    check_decoded_sample();
    puts("PASS: address, init timing, signed six-axis decode, freshness, failures, recovery, tick wrap");
    return 0;
}
