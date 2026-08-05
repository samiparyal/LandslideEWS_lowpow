#ifndef IMU_H_
#define IMU_H_

#include <stdint.h>
#include "stm32wb0x_hal.h"

typedef enum {
	IMU_RATE_1HZ875 = 0,   /* 533.3333 ms -- the floor, nothing slower exists */
	IMU_RATE_7HZ5,         /* 133.3333 ms */
	IMU_RATE_15HZ,         /*  66.6667 ms */
	IMU_RATE_30HZ,         /*  33.3333 ms */
	IMU_RATE_60HZ,         /*  16.6667 ms */
	IMU_RATE_120HZ,        /*   8.3333 ms */
	IMU_RATE_240HZ,        /*   4.1667 ms */
} imu_rate_t;

#define TRAINING_MODE_RATE     IMU_RATE_120HZ  /* 8.33 ms, ~120 Hz */
#define TRAINING_MODE_ENABLED  1   /* set to 1 during debugging to collect training data */

/* 1 = keep gyro running during training, 0 = accel only  */
#define TRAINING_MODE_GYRO_ON  0

/* Rate programmed by imu_init(), before the detector picks a state. */
#define IMU_DEFAULT_RATE       IMU_RATE_7HZ5

int32_t imu_init(I2C_HandleTypeDef *hi2c);
int32_t imu_enable_wakeup_int1(uint8_t threshold, uint8_t duration);
int32_t imu_read_wakeup_src(uint8_t *src);

//
void imu_gyro_on(void);
void imu_gyro_off(void);

//
int32_t imu_get_tilt_and_magnitude(float *tilt_deg, float *mag_g);
int32_t imu_get_gyro_dps_magnitude(float *magnitude);

//
int32_t imu_set_rate(imu_rate_t rate);

//
void imu_get_last_sample(int16_t accel[3], int16_t gyro[3]);

//
int32_t imu_poll(void);

#endif


