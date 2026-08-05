#include "tilt_detector.h"
#include <stdint.h>
#include <math.h>
#include "SEGGER_RTT.h"
#include "imu.h"

/*---Calibration state----*/
static uint32_t g_calib_count = 0;
static float g_calib_sum = 0.0f;
static float g_baseline = 0.0f;

static float g_gravity_calib_sum   = 0.0f;
static float g_gravity_calib_sumsq = 0.0f;
static float g_gravity_baseline_g    = 1.0f;
static float g_gravity_deviation_std_g     = ACCELERATION_GRAVITY_DEVIATION_MIN_SIGMA;

/*---Detection state------*/
static TiltState_t g_state = TILT_STATE_CALIBRATING;
static uint8_t g_confirm_up = 0;
static uint8_t g_confirm_dn = 0;

static float g_deviation = 0.0f;

static uint8_t g_gravity_deviation_bypass_cnt = 0;   /* existing: WARNING -> CRITICAL bypass */
static uint8_t g_gravity_deviation_normal_cnt = 0;   /* new: NORMAL -> WARNING debounce */

static uint64_t g_gravity_latch_until = 0U;

/*---Slow tilt-rate channel (deg/hour)---
 * Ring of RATE_SNAPS+1 averages; newest vs oldest spans 1 h.
 * Never reset on state transitions -- only on full reset. */

//accumulator grp - builds one snapshot
static float    g_snap_sum          = 0.0f; //running sum of every dev readings since last snapshot closed
static uint32_t g_snap_count        = 0; //readings/count in that sum

static uint64_t g_snap_closes_at  = 0U;

//ring buf grp - stores snapshot history
#define SNAP_SLOTS (RATE_SNAPS + 1U) //+1 to measure change over 12 intervals (1 hr)
static float    g_snaps[SNAP_SLOTS] = {0}; //ring - each slot holds one 5 min avg deviation
static uint8_t  g_snap_idx          = 0; //Wraps 0-->12-->0 via % SNAP_SLOTS
static uint8_t  g_snaps_filled      = 0; //Until it reaches 13 (~65 min after calib), slots still hold their init zeros'' rate stays 0 until the ring is genuinely full.

//output
static float    g_rate_dph          = 0.0f; //latest computed rate in deg/hour

static uint8_t g_trigger_reason = TRIGGER_NONE;


/* ---------helper fns ------*/
static void set_state(TiltState_t new_state)
{
	/* Gyro duty-cycle: ON in WARNING/CRITICAL, OFF in NORMAL */
	if (new_state == TILT_STATE_NORMAL)
	{
		(void)imu_gyro_off();
		g_gravity_latch_until = 0U;
		g_trigger_reason &= (uint8_t)~TRIGGER_ACCELERATION_GRAVITY_DEVIATION;

	}
	else if (g_state == TILT_STATE_NORMAL)
	{
		(void)imu_gyro_on();
	}
//	if (new_state == TILT_STATE_WARNING)
//	{
//		g_gravity_deviation_bypass_cnt = 0;
//	}

	g_state      = new_state;
	g_confirm_up = 0;
	g_confirm_dn = 0;
}

/*------------*/
void tilt_detector_reset(void)
{
	(void)imu_gyro_on();   /* recalibration: gyro baseline*/

	g_calib_count = 0;
	g_calib_sum = 0.0f;
	g_baseline = 0.0f;
	g_state = TILT_STATE_CALIBRATING;
	g_confirm_up = 0;
	g_confirm_dn = 0;
	g_deviation = 0.0f;

	for (uint8_t i = 0; i < SNAP_SLOTS; i++) g_snaps[i] = 0.0f;
	g_snap_idx = 0;
	g_snaps_filled    = 0;
	g_snap_sum          = 0.0f;
	g_snap_count        = 0;
	g_snap_closes_at  = 0U;   /* re-armed on the first post-calibration sample */
	g_rate_dph          = 0.0f;

	g_gravity_latch_until = 0U;

	g_gravity_deviation_bypass_cnt = 0;

	g_trigger_reason = TRIGGER_NONE;

	g_gravity_calib_sum   = 0.0f;
	g_gravity_calib_sumsq = 0.0f;
}

static float compute_regression_rate_dph(void)
{
    float sum_x = 0.0f, sum_y = 0.0f, sum_xy = 0.0f, sum_xx = 0.0f;

    for (uint8_t i = 0; i < SNAP_SLOTS; i++)
    {
        uint8_t idx = (uint8_t)((g_snap_idx + i) % SNAP_SLOTS);  /* oldest-first order */
        float x = (float)i;
        float y = g_snaps[idx];
        sum_x  += x;
        sum_y  += y;
        sum_xy += x * y;
        sum_xx += x * x;
    }

    //OLS regression formula: slope = (N·Σxy − Σx·Σy) / (N·Σx² − (Σx)²)
    float denom = ((float)SNAP_SLOTS * sum_xx) - (sum_x * sum_x);
    float slope_per_slot = (((float)SNAP_SLOTS * sum_xy) - (sum_x * sum_y)) / denom;
    float slots_per_hour = 3600000.0f / (float)SNAP_PERIOD_MS;  //Snap_period_ms = 300000 (5 min)
    return fabsf(slope_per_slot * slots_per_hour);
}

TiltState_t tilt_detector_update(float tilt_deg, float gyro_mag, float acceleration_magnitude_g)
{
	uint64_t now = HAL_RADIO_TIMER_GetCurrentSysTime();

	//
	if(g_state == TILT_STATE_CALIBRATING)
	{
		g_calib_sum += tilt_deg;
		g_calib_count++;

		g_gravity_calib_sum   += acceleration_magnitude_g;
		g_gravity_calib_sumsq += acceleration_magnitude_g * acceleration_magnitude_g;

		if(g_calib_count >= CALIB_SAMPLES)
		{
			g_baseline = g_calib_sum / (float)g_calib_count;


			g_gravity_baseline_g = g_gravity_calib_sum / (float)g_calib_count;
			float var = (g_gravity_calib_sumsq / (float)g_calib_count) - (g_gravity_baseline_g * g_gravity_baseline_g);
			if (var < 0.0f) var = 0.0f;
			g_gravity_deviation_std_g = sqrtf(var); //standard dev
			if (g_gravity_deviation_std_g < ACCELERATION_GRAVITY_DEVIATION_MIN_SIGMA) g_gravity_deviation_std_g = ACCELERATION_GRAVITY_DEVIATION_MIN_SIGMA;

			set_state(TILT_STATE_NORMAL);
		}
		return g_state;
	}

	//deviation
	float raw_dev = tilt_deg - g_baseline;
	g_deviation = (raw_dev < 0.0f) ? -raw_dev : raw_dev;  //only mag

	// Sudden collapse: gyro bypass -- turned off for now for battery health
//	if (gyro_mag >= WARNING_GYRO_DPS && g_deviation >= WARNING_DEG && g_state == TILT_STATE_NORMAL)
//	{
//		g_trigger_reason = TRIGGER_ANGLE;
//	    set_state(TILT_STATE_WARNING);
//	    return g_state;
//	}

	/* ---- slow tilt-rate channel ---- */
		g_snap_sum += g_deviation;
		g_snap_count++;

		/* First sample after calibration arms the grid. */
		if (g_snap_closes_at == 0U)
		{
			g_snap_closes_at = HAL_RADIO_TIMER_AddSysTimeMs(now, (int32_t)SNAP_PERIOD_MS);
		}

		if (now >= g_snap_closes_at)
		{
			float avg = g_snap_sum / (float)g_snap_count;

			g_snaps[g_snap_idx] = avg;

			//print progress
			for (uint8_t i = 0; i < SNAP_SLOTS; i++)
			{
			    if (i == g_snap_idx && g_snaps[i] > 0.0f)
			    {
			        int avg_int  = (int)g_snaps[i];
			        int avg_frac = (int)((g_snaps[i] - (float)avg_int) * 100.0f);
			        SEGGER_RTT_printf(0, "[ringbuf] slot %d = %d.%02d deg\n", i, avg_int, avg_frac);
			    }
			}

			g_snap_idx = (uint8_t)((g_snap_idx + 1U) % SNAP_SLOTS);

			if (g_snaps_filled < SNAP_SLOTS) g_snaps_filled++;

			if (g_snaps_filled == SNAP_SLOTS)
			{
				/* slot about to be overwritten = oldest = 1 h ago */
//				float change = avg - g_snaps[g_snap_idx];
//				if (change < 0.0f) change = -change;
//
//				float window_h = ((float)SNAP_PERIOD_MS * (float)RATE_SNAPS) / 3600000.0f; //300000*12/3600000 = 1.0 hr
//				g_rate_dph = change / window_h;

				g_rate_dph = compute_regression_rate_dph();
			}

			/* 0.15° of change is invisible sample-to-sample (buried in noise),
			 * but comparing two 100-sample averages taken an hour apart, it's a clean, averaging kills the noise; making tiny slope visible
			 */

			g_snap_sum   = 0.0f;
			g_snap_count = 0;


			g_snap_closes_at = HAL_RADIO_TIMER_AddSysTimeMs(g_snap_closes_at, (int32_t)SNAP_PERIOD_MS);
			if (g_snap_closes_at <= now)
			{
				g_snap_closes_at = HAL_RADIO_TIMER_AddSysTimeMs(now, (int32_t)SNAP_PERIOD_MS);
			}
		}


	//classify
//	uint8_t in_warning   = (g_deviation >= WARNING_DEG || g_rate_dph  >= TILT_RATE_WARN_DPH) ? 1U : 0U;
//
//	uint8_t in_critical  = (g_deviation >= CRITICAL_DEG ||
//						   (g_deviation >= WARNING_DEG &&
//							g_rate_dph  >= TILT_RATE_CRIT_DPH)) ? 1U : 0U;
//
//	uint8_t out_warning  = (g_deviation < WARNING_DEG &&
//							g_rate_dph  < TILT_RATE_WARN_DPH) ? 1U : 0U;
//
//	uint8_t out_critical = (g_deviation < CRITICAL_DEG &&
//							g_rate_dph  < TILT_RATE_CRIT_DPH) ? 1U : 0U;


	uint8_t dev_warn  = (g_deviation >= WARNING_DEG)      ? 1U : 0U;
	uint8_t dev_crit  = (g_deviation >= CRITICAL_DEG)      ? 1U : 0U;
	uint8_t rate_warn = (g_rate_dph  >= TILT_RATE_WARN_DPH) ? 1U : 0U;
	uint8_t rate_crit = (g_rate_dph  >= TILT_RATE_CRIT_DPH) ? 1U : 0U;

	uint8_t gravity_deviation = (fabsf(acceleration_magnitude_g - g_gravity_baseline_g)
	                              >= (ACCELERATION_GRAVITY_DEVIATION_K_SIGMA * g_gravity_deviation_std_g)) ? 1U : 0U;

	if (gravity_deviation)
	{
		g_gravity_latch_until =
			HAL_RADIO_TIMER_AddSysTimeMs(now, (int32_t)ACCELERATION_GRAVITY_DEVIATION_LATCH_MS);

//		SEGGER_RTT_printf(0, "[Grav] mag=%d mg  base=%d mg  thresh=%d mg  dev=%d\n",
//					                      (int)(acceleration_magnitude_g*1000.0f),
//					                      (int)(g_gravity_baseline_g*1000.0f),
//					                      (int)(ACCELERATION_GRAVITY_DEVIATION_K_SIGMA * g_gravity_deviation_std_g * 1000.0f),
//					                      gravity_deviation);
	}
	/* No else: the latch expires by itself once now passes the deadline. */


	uint8_t in_warning  = (dev_warn || rate_warn ) ? 1U : 0U;
	uint8_t in_critical = (dev_crit || rate_crit) ? 1U : 0U;

	uint8_t out_warning  = (!dev_warn && !rate_warn && !gravity_deviation) ? 1U : 0U;
	//uint8_t out_critical = (!dev_crit && !rate_crit) ? 1U : 0U;
	uint8_t out_critical = (!dev_crit && !rate_crit && !gravity_deviation) ? 1U : 0U;

	uint8_t reason = TRIGGER_NONE;
	if (dev_warn  || dev_crit)  reason |= TRIGGER_ANGLE;
	if (rate_warn || rate_crit) reason |= TRIGGER_RATE;
	if (gravity_deviation || (now < g_gravity_latch_until) || g_state != TILT_STATE_NORMAL) reason |= TRIGGER_ACCELERATION_GRAVITY_DEVIATION;



	g_trigger_reason = reason;


	///STATE MACHINE
	switch(g_state)
	{
		case TILT_STATE_NORMAL:
			if (g_deviation >= CRITICAL_DEG * 2.0f)  //large dev bypass -- single reading, no confirm wait
			{
				g_trigger_reason |= TRIGGER_LARGE_DEV;
				set_state(TILT_STATE_CRITICAL);
				return g_state;
			}

			 if (gravity_deviation)
				{
					g_gravity_deviation_normal_cnt++;
					if (g_gravity_deviation_normal_cnt >= 2U)   // 2 consecutive polls, debounce
					{
						g_gravity_deviation_normal_cnt = 0;
				        g_trigger_reason |= TRIGGER_ACCELERATION_GRAVITY_DEVIATION;
						set_state(TILT_STATE_WARNING);
						return g_state;
					}
				}
			 else
				 {
					 g_gravity_deviation_normal_cnt = 0;
				 }

			if(in_critical || in_warning)
			{
				g_confirm_up ++;
				g_confirm_dn = 0;
				if(g_confirm_up >= CONFIRM_UP)
				{
					set_state(TILT_STATE_WARNING);  // always step through WARNING first
				}
			}
			else
			{
				g_confirm_up = 0;
			}
		break;


		case TILT_STATE_WARNING:
			if (g_deviation >= CRITICAL_DEG * 2.0f)
			{
				g_trigger_reason |= TRIGGER_LARGE_DEV;
				set_state(TILT_STATE_CRITICAL);
				return g_state;
			}


			if (gravity_deviation)
			{
				g_gravity_deviation_bypass_cnt++;
				if (g_gravity_deviation_bypass_cnt >= 2U)   // 2 consecutive polls while already in WARNING
				{
					g_gravity_deviation_bypass_cnt = 0;
			        g_trigger_reason |= TRIGGER_ACCELERATION_GRAVITY_DEVIATION;
					set_state(TILT_STATE_CRITICAL);
					return g_state;
				}
			}
			else
			{
				g_gravity_deviation_bypass_cnt = 0;
			}


			if(in_critical)
			{
				g_confirm_up ++;
				g_confirm_dn = 0;
				if(g_confirm_up >= CONFIRM_UP)
				{
					set_state(TILT_STATE_CRITICAL);
				}
			}
			else if(out_warning)
			{
				g_confirm_dn++;
				g_confirm_up = 0;
				if(g_confirm_dn >= CONFIRM_DOWN_WARNING)
				{
					set_state(TILT_STATE_NORMAL);
				}
			}
			else
			{
			    g_confirm_up = 0;
			}
		break;

		case TILT_STATE_CRITICAL:
		    if(out_critical)
			{
				g_confirm_dn++;
				g_confirm_up = 0;
				if(g_confirm_dn >= CONFIRM_DOWN_CRITICAL)
				{
					set_state(TILT_STATE_WARNING);
				}
			}
			else
			{
				g_confirm_dn = 0;
			}
		 break;

		default:
			break;

	}
	return g_state;
}


TiltState_t tilt_detector_get_state(void) {return g_state;}

imu_rate_t tilt_detector_get_rate(void)
{
	switch (g_state)
	{
		case TILT_STATE_CALIBRATING: return IMU_RATE_7HZ5;
		case TILT_STATE_CRITICAL:    return IMU_RATE_30HZ;
		case TILT_STATE_WARNING:     return IMU_RATE_15HZ;
		default:                     return IMU_RATE_1HZ875;
	}
}

float tilt_detector_get_deviation(void) {return g_deviation;}
float tilt_detector_get_baseline(void) {return g_baseline; }
float tilt_detector_get_rate_dph(void) { return g_rate_dph; }


uint8_t tilt_detector_get_calib_percent(void)
{
	if(g_state != TILT_STATE_CALIBRATING) return 100U;
	return (uint8_t)((g_calib_count * 100U)/CALIB_SAMPLES);
}

uint8_t tilt_detector_get_trigger_reason(void) { return g_trigger_reason; }
