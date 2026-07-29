#ifndef INC_H_
#define INC_H_

#include <stdint.h>
#include "stm32wb0x_hal.h"

#define TRAINING_MODE_RATE_MS  10U //100hz
#define TRAINING_MODE_ENABLED  1   /* set to 1 during debugging to collect training data */

int32_t imu_init(I2C_HandleTypeDef *hi2c);
int32_t imu_enable_wakeup_int1(uint8_t threshold, uint8_t duration);
int32_t imu_read_wakeup_src(uint8_t *src);

//
void imu_gyro_on(void);
void imu_gyro_off(void);

//v2
int32_t imu_compute_tilt_deg(float *tilt_deg);
int32_t imu_get_gyro_dps_magnitude(float *magnitude);

//
int32_t imu_set_rate_ms(uint32_t poll_interval_ms);

//
void imu_get_last_sample(int16_t accel[3], int16_t gyro[3]);

//
int32_t imu_poll(void);
int32_t imu_get_acceleration_magnitude_g(float *acceleration_magnitude_g);

#endif


