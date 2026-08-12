#include <stdint.h>
#include "stm32wb0x_hal.h"
#include "lsm6dsv16x_reg.h"
#include "imu_bus.h"
#include "imu.h"
#include <string.h>
#include <math.h>
#define LSM6DSV16X_WAKE_UP_SRC_REG 0x1B

static stmdev_ctx_t ctx;
static imu_rate_t s_current_rate = (imu_rate_t)0xFF;   /* force the first set */
static uint8_t s_gyro_enabled = 0;

static int16_t s_accel[3];
static int16_t s_gyro[3];

static void imu_apply_training_mode(void);

int32_t imu_init(I2C_HandleTypeDef *hi2c)
{
	ctx.write_reg = imu_bus_write; //binding st driver to bus function
	ctx.read_reg = imu_bus_read;
	ctx.mdelay= imu_bus_delay_ms;
	ctx.handle = (void *)hi2c;


	//opt; sanity check
	uint8_t who = 0;
	if(lsm6dsv16x_device_id_get(&ctx, &who)!=0)
	{
		return -1; //i2c read failed
	}


	(void)lsm6dsv16x_sw_reset(&ctx);
	 ctx.mdelay(20);

	 //xl config
	 (void)lsm6dsv16x_xl_full_scale_set(&ctx, LSM6DSV16X_2g);
	 (void)lsm6dsv16x_xl_mode_set(&ctx, LSM6DSV16X_XL_HIGH_ACCURACY_ODR_MD);
	 (void)imu_set_rate(IMU_DEFAULT_RATE);

	 //gyro config
	 imu_gyro_off();

	/*
	    From the datasheet (for ±250 dps ):
		- **Sensitivity = 8.75 mdps/LSB** (milli-degrees per second per LSB)
		- Or: **0.00875 dps/LSB**

		#eg:
		gy_raw = 27509
		gy_dps = 27509 × 0.00875 = 240.7 dps

	 */

	 /* The LSM6DSV16X defaults to push-pull ACTIVE-HIGH, which means INT1/INT2
	    idle DRIVEN LOW. They sit on PB9/PB8 against the board's 4.7k pull-ups
	    (R1/R2), so each pin sinks 3.3V/4.7k = ~702 uA continuously -> ~1.4 mA
	    total */
	 {
		 lsm6dsv16x_if_cfg_t if_cfg;
		 if (lsm6dsv16x_read_reg(&ctx, LSM6DSV16X_IF_CFG, (uint8_t *)&if_cfg, 1) != 0) return -3;
		 if_cfg.pp_od     = 1;   /* open-drain */
		 if_cfg.h_lactive = 1;   /* active-low */
		 if (lsm6dsv16x_write_reg(&ctx, LSM6DSV16X_IF_CFG, (uint8_t *)&if_cfg, 1) != 0) return -3;
	 }

	 /* Route accel data-ready to INT1. */
	 {
		 lsm6dsv16x_interrupt_mode_t int_mode = {0};
		 int_mode.enable = 1;
		 int_mode.lir    = 0;
		 if (lsm6dsv16x_interrupt_enable_set(&ctx, int_mode) != 0) return -2;
		 if (lsm6dsv16x_data_ready_mode_set(&ctx, LSM6DSV16X_DRDY_PULSED) != 0) return -2;

		 lsm6dsv16x_pin_int_route_t route = {0};
		 if (lsm6dsv16x_pin_int1_route_get(&ctx, &route) != 0) return -3;
		 route.drdy_xl = 1;
		 if (lsm6dsv16x_pin_int1_route_set(&ctx, &route) != 0) return -3;
	 }

	 imu_apply_training_mode(); // only takes effect if training flag set


	return 0;

}

int32_t imu_enable_wakeup_int1(uint8_t threshold, uint8_t duration)
{
	lsm6dsv16x_act_thresholds_t thr = {0};

	thr.threshold = threshold;
	thr.duration = duration;

	int32_t ret;
	ret=lsm6dsv16x_act_thresholds_set(&ctx, &thr);
	if (ret != 0) return ret;

	lsm6dsv16x_pin_int_route_t route = {0};
	ret=lsm6dsv16x_pin_int1_route_get(&ctx, &route);
	if (ret!=0) return ret;

	route.wakeup=1; //enable wkup on int1

	ret=lsm6dsv16x_pin_int1_route_set(&ctx, &route);
	if(ret!=0) return ret;

	return 0;

}
int32_t imu_read_wakeup_src(uint8_t *src)
{
	lsm6dsv16x_all_sources_t all = {0};

	int32_t ret;
	ret=lsm6dsv16x_all_sources_get(&ctx, &all);
	if (ret!=0) return ret;

////	*src=(all.wake_up?1U:0U);

	*src = 0;  // Start with 0
	if (all.wake_up_x) *src |= 0x01;  // ..001
	if (all.wake_up_y) *src |= 0x02;  // ..011
	if (all.wake_up_z) *src |= 0x04;  // ..111

	return 0;
}


void imu_gyro_on(void)
{
#if (TRAINING_MODE_ENABLED && !TRAINING_MODE_GYRO_ON)
	imu_gyro_off();
	return;
#else
	(void)lsm6dsv16x_gy_data_rate_set(&ctx, LSM6DSV16X_ODR_AT_7Hz5);
	(void)lsm6dsv16x_gy_full_scale_set(&ctx, LSM6DSV16X_1000dps);
	(void)lsm6dsv16x_gy_mode_set(&ctx, LSM6DSV16X_GY_LOW_POWER_MD);
	s_gyro_enabled = 1U;
#endif
}

void imu_gyro_off(void)
{
	(void)lsm6dsv16x_gy_data_rate_set(&ctx, LSM6DSV16X_ODR_OFF);
	s_gyro_enabled = 0U;
}


/* Only ever called from the DRDY interrupt path, so the sensor has already
   said data is ready  */
int32_t imu_poll(void)
{
	if (lsm6dsv16x_acceleration_raw_get(&ctx, s_accel) != 0) return -1;

	if (s_gyro_enabled)
	{
		/* Gyro shares the accel ODR, so its sample is in step with this one. */
		(void)lsm6dsv16x_angular_rate_raw_get(&ctx, s_gyro);
	}
	return 0;
}


int32_t imu_get_tilt_and_magnitude(float *tilt_deg, float *mag_g)
{
	float ax = (float)s_accel[0] * 0.000061f;
	float ay = (float)s_accel[1] * 0.000061f;
	float az = (float)s_accel[2] * 0.000061f;

	float mag = sqrtf(ax*ax + ay*ay + az*az);
	if (mag_g != NULL) *mag_g = mag;

	if (mag < 0.5f) return -1;   /* freefall */

	float az_norm = az / mag;
	if (az_norm >  1.0f) az_norm =  1.0f;
	if (az_norm < -1.0f) az_norm = -1.0f;
	if (tilt_deg != NULL) *tilt_deg = acosf(fabsf(az_norm)) * (180.0f/3.14159265f);
	return 0;
}

int32_t imu_get_gyro_dps_magnitude(float *magnitude)
{
	if (!s_gyro_enabled) return -1;
	float gx = (float)s_gyro[0] * 0.035f;
	float gy = (float)s_gyro[1] * 0.035f;
	float gz = (float)s_gyro[2] * 0.035f;
	*magnitude = sqrtf(gx*gx + gy*gy + gz*gz);
	return 0;
}


/* The single place a rung maps to its ODR register value. */
int32_t imu_set_rate(imu_rate_t rate)
{
	if (TRAINING_MODE_ENABLED)
	{
		rate = TRAINING_MODE_RATE;
	}

	if (rate == s_current_rate) return 0;
	s_current_rate = rate;

	lsm6dsv16x_data_rate_t odr;
	switch (rate)
	{
		case IMU_RATE_1HZ875: odr = LSM6DSV16X_ODR_AT_1Hz875; break;
		case IMU_RATE_15HZ:   odr = LSM6DSV16X_ODR_AT_15Hz;   break;
		case IMU_RATE_30HZ:   odr = LSM6DSV16X_ODR_AT_30Hz;   break;
		case IMU_RATE_60HZ:   odr = LSM6DSV16X_ODR_AT_60Hz;   break;
		case IMU_RATE_120HZ:  odr = LSM6DSV16X_ODR_AT_120Hz;  break;
		case IMU_RATE_240HZ:  odr = LSM6DSV16X_ODR_AT_240Hz;  break;
		case IMU_RATE_7HZ5:
		default:              odr = LSM6DSV16X_ODR_AT_7Hz5;   break;
	}

	int32_t ret = lsm6dsv16x_xl_data_rate_set(&ctx, odr);

	/* only re-rate gyro if it's currently on*/
	lsm6dsv16x_data_rate_t gy_now;
	if (lsm6dsv16x_gy_data_rate_get(&ctx, &gy_now) == 0 && gy_now != LSM6DSV16X_ODR_OFF)
	{
		ret += lsm6dsv16x_gy_data_rate_set(&ctx, odr);
	}
	return ret;
}


void imu_get_last_sample(int16_t accel[3], int16_t gyro[3])
{
	accel[0] = s_accel[0]; accel[1] = s_accel[1]; accel[2] = s_accel[2];
	gyro[0]  = s_gyro[0];  gyro[1]  = s_gyro[1];  gyro[2]  = s_gyro[2];
}


static void imu_apply_training_mode(void)
{
	if (TRAINING_MODE_ENABLED)
	{
#if TRAINING_MODE_GYRO_ON
		(void)imu_gyro_on();
#else
		(void)imu_gyro_off();
#endif
		(void)imu_set_rate(TRAINING_MODE_RATE);
	}
}


float imu_rate_period_ms(imu_rate_t rate)
{
    switch (rate)
    {
        case IMU_RATE_1HZ875: return 533.333f;
        case IMU_RATE_7HZ5:   return 133.333f;
        case IMU_RATE_15HZ:   return 66.667f;
        case IMU_RATE_30HZ:   return 33.333f;
        case IMU_RATE_60HZ:   return 16.667f;
        case IMU_RATE_120HZ:  return 8.333f;
        case IMU_RATE_240HZ:  return 4.167f;
        default:              return 533.333f;
    }
}
