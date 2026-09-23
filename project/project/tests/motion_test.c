#include <assert.h>
#include <stdio.h>

#include "motion.h"
#include "mpu6050.h"

static Mpu6050Sample source;
static bool available;
static uint32_t now;

bool Mpu6050_GetSample(Mpu6050Sample *sample, uint32_t tick)
{
    if (!available || (uint32_t)(tick - source.timestamp_ms) >= 100U)
    {
        return false;
    }
    *sample = source;
    return true;
}

static float absolute(float value) { return value < 0.0f ? -value : value; }

static void begin(uint32_t tick)
{
    now = tick;
    source = (Mpu6050Sample){0};
    available = true;
    Motion_Init();
}

static void feed(int16_t ax, int16_t ay, int16_t az, unsigned count)
{
    for (unsigned i = 0U; i < count; i++)
    {
        now += 10U;
        source.timestamp_ms = now;
        source.sequence++;
        source.accel[0] = ax;
        source.accel[1] = ay;
        source.accel[2] = az;
        Motion_Update(now);
    }
}

static void expect(float x, float y, float tolerance)
{
    assert(Motion_IsValid());
    Gravity2D g = Motion_GetDisplayGravity();
    assert(absolute(g.x - x) < tolerance);
    assert(absolute(g.y - y) < tolerance);
}

static void expect_invalid(void)
{
    assert(!Motion_IsValid());
    Gravity2D g = Motion_GetDisplayGravity();
    assert(g.x == 0.0f && g.y == 0.0f);
    assert(Motion_GetShakeStrength() == 0U);
}

int main(void)
{
    /* Measured pose A: raw X negative -> physical board down. */
    begin(0U);
    feed(-16384, 0, -2048, 40U);
    expect_invalid();
    feed(-16384, 0, -2048, 1U);
    expect(0.9923f, 0.0f, 0.002f);
    assert(Motion_GetShakeStrength() == 0U);

    /* Pose B: raw Y positive -> original right edge, now facing down. */
    begin(0U);
    feed(0, 16384, 0, 41U);
    expect(0.0f, 1.0f, 0.001f);

    /* Pose C, including small lateral noise, must stay near center. */
    begin(0U);
    feed(80, -50, -16384, 41U);
    expect(0.0f, 0.0f, 0.01f);

    /* Out-of-plane tilt must reduce flow projection rather than normalize to 1. */
    begin(0U);
    feed(-11585, 0, -11585, 41U);
    expect(0.7071f, 0.0f, 0.002f);
    begin(0U);
    feed(-11585, 11585, 0, 41U);
    expect(0.7071f, 0.7071f, 0.002f);

    begin(0U);
    feed(16384, 0, 0, 41U);
    expect(-1.0f, 0.0f, 0.001f);
    begin(0U);
    feed(0, -16384, 0, 41U);
    expect(0.0f, -1.0f, 0.001f);

    /* One changed sample should be smoothed, and repeated polling must not
     * reapply the filter to that sample or accelerate the response.
     */
    feed(-16384, 0, 0, 1U);
    Gravity2D first = Motion_GetDisplayGravity();
    uint8_t first_shake = Motion_GetShakeStrength();
    assert(first_shake > 0U);
    assert(first.x > 0.0f && first.x < 0.3f && first.y < -0.9f);
    for (unsigned i = 1U; i < 10U; i++)
    {
        Motion_Update(now + i);
        Gravity2D repeat = Motion_GetDisplayGravity();
        assert(repeat.x == first.x && repeat.y == first.y);
        assert(Motion_GetShakeStrength() == first_shake);
    }
    feed(-16384, 0, 0, 100U);
    expect(1.0f, 0.0f, 0.001f);

    /* A transient must decay to zero after the same pose is held still. */
    assert(Motion_GetShakeStrength() == 0U);

    /* Missing/old samples stop gravity immediately. Recovery settles again. */
    available = false;
    Motion_Update(now);
    expect_invalid();
    available = true;
    feed(-16384, 0, 0, 40U);
    expect_invalid();
    feed(-16384, 0, 0, 1U);
    expect(1.0f, 0.0f, 0.001f);
    Motion_Update(now + 100U);
    expect_invalid();

    /* Zero or very small magnitude must not divide by zero/amplify noise. */
    begin(0U);
    feed(0, 0, 0, 41U);
    expect_invalid();
    begin(0U);
    feed(100, -100, 100, 41U);
    expect_invalid();

    /* Full-scale values exercise the >INT32_MAX sum-of-squares case. */
    begin(0U);
    feed(-32768, -32768, -32768, 41U);
    expect(0.57735f, -0.57735f, 0.002f);
    feed(32767, 32767, 32767, 1U);
    assert(Motion_GetShakeStrength() == 255U);

    begin(UINT32_MAX - 200U);
    feed(-16384, 0, 0, 41U);
    expect(1.0f, 0.0f, 0.001f);
    puts("PASS: measured poses, inverse directions, 3D projection, filtering, stale data, recovery, range, tick wrap");
    return 0;
}
