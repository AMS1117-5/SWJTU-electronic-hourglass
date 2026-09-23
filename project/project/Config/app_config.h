#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define BUTTON_SCAN_PERIOD_MS        5U
#define BUTTON_DEBOUNCE_MS          25U
#define BUTTON_LONG_PRESS_MS       700U

#define DISPLAY_WIDTH               16U
#define DISPLAY_HEIGHT               8U
/* MAX7219 intensity register: 0 = dimmest (still on), 15 = brightest. */
#define DISPLAY_INTENSITY             0U
#define DISPLAY_BRIGHTNESS_MAX        2U
#define DISPLAY_REFRESH_PERIOD_MS    25U

#define DISPLAY_ROTATION_0            0U
#define DISPLAY_ROTATION_90           1U
#define DISPLAY_ROTATION_180          2U
#define DISPLAY_ROTATION_270          3U

/* Device 0 is nearest to MCU DIN and drives the upper panel in the
 * charging-port-down pose. LEFT/RIGHT name the two virtual 8x8 tiles:
 * x=0..7 is upper, x=8..15 is lower. In this pose +x goes down and
 * +y goes toward the observer's right. Physical feedback uses rows/cols 1..8.
 */
#define DISPLAY_LEFT_DEVICE           0U
#define DISPLAY_LEFT_ROTATION         DISPLAY_ROTATION_0
#define DISPLAY_LEFT_FLIP_X           0U
#define DISPLAY_LEFT_FLIP_Y           0U

#define DISPLAY_RIGHT_DEVICE          1U
#define DISPLAY_RIGHT_ROTATION        DISPLAY_ROTATION_0
#define DISPLAY_RIGHT_FLIP_X          0U
#define DISPLAY_RIGHT_FLIP_Y          0U

/* Local x (physical row) -> MAX7219 segment BIT index, not pin number.
 * Local y (physical column) -> DIGIT index (register address minus one).
 * Wiring calibration applies after the optional tile rotation/mirroring.
 */
#define DISPLAY_DEVICE0_SEGMENT_MAP {6U, 5U, 4U, 3U, 2U, 1U, 0U, 7U}
#define DISPLAY_DEVICE1_SEGMENT_MAP {6U, 5U, 4U, 3U, 2U, 1U, 0U, 7U}
#define DISPLAY_DEVICE0_DIGIT_MAP   {7U, 6U, 5U, 4U, 3U, 2U, 1U, 0U}
#define DISPLAY_DEVICE1_DIGIT_MAP   {7U, 6U, 5U, 4U, 3U, 2U, 1U, 0U}

#define BRINGUP_SCAN_PERIOD_MS        80U

#define MPU6050_SAMPLE_PERIOD_MS      10U
#define MPU6050_IO_TIMEOUT_MS          5U
#define MPU6050_RETRY_PERIOD_MS      500U
#define MPU6050_STALE_MS             100U

/* Confirmed by the three physical poses in MPU6050_CALIBRATION.md.
 * Virtual +x = board down (toward charging port), +y = board right.
 */
#define MOTION_DISPLAY_X_AXIS          0U
#define MOTION_DISPLAY_X_SIGN         (-1)
#define MOTION_DISPLAY_Y_AXIS          1U
#define MOTION_DISPLAY_Y_SIGN          1
#define MOTION_SETTLE_MS             400U
/* Higher new-sample weight gives a faster response with less smoothing. */
#define MOTION_FILTER_NEW_PERCENT     20U

#define PHYSICS_PERIOD_MS             20U
#define OCEAN_PARTICLE_COUNT          32U

#define HOURGLASS_PARTICLE_COUNT      24U
/* Settings edit the full duration in 30-second steps. Grain timing is in ms:
 * with 24 grains, a 30-second duration/step corresponds to 1250 ms per grain.
 * The nominal upright interval must never exceed 10 seconds per grain.
 * Tilt still reduces physical flow; the horizontal deadzone still pauses it.
 */
#define HOURGLASS_MAX_GRAIN_INTERVAL_MS 10000U
#define HOURGLASS_MIN_DURATION_MS      30000U
#define HOURGLASS_DURATION_STEP_MS     30000U
#define HOURGLASS_MIN_GRAIN_INTERVAL_MS \
    (HOURGLASS_MIN_DURATION_MS / HOURGLASS_PARTICLE_COUNT)
#define HOURGLASS_GRAIN_INTERVAL_STEP_MS \
    (HOURGLASS_DURATION_STEP_MS / HOURGLASS_PARTICLE_COUNT)
#define HOURGLASS_GRAIN_INTERVAL_MS     10000U
#define HOURGLASS_DURATION_MS \
    (HOURGLASS_PARTICLE_COUNT * HOURGLASS_GRAIN_INTERVAL_MS)
#define HOURGLASS_FLOW_DEADZONE        0.15f

#endif
