
#ifndef TILT_DETECTOR_H
#define TILT_DETECTOR_H

#include <stdint.h>
#include "imu.h"   /* imu_rate_t: states select an ODR rung, not a millisecond value */


#define TRIGGER_NONE       0x00U
#define TRIGGER_ANGLE      0x01U   // bit0
#define TRIGGER_RATE       0x02U   // bit1
#define TRIGGER_LARGE_DEV  0x04U   // bit2
#define TRIGGER_ACCELERATION_GRAVITY_DEVIATION    0x08U   // bit3 -- angle-independent accel magnitude step

uint8_t tilt_detector_get_trigger_reason(void);

/*------Calibration------*/
/* 450 x 133.33 ms (IMU_RATE_7HZ5) = 1 s. */
#define CALIB_SAMPLES 450U

/* ----- Tilt angle thresholds (deviation from baseline) ----- */
#define WARNING_DEG             5.0f
#define CRITICAL_DEG           12.0f

/* ----- Gyro threshold (sudden collapse/shock) ------ */
#define WARNING_GYRO_DPS      40.0f

/*-----Confirm windows------*/
#define CONFIRM_UP 2U //readings to escalate [NORMAL -> WARNING -> CRITICAL]
//readings to de-escalate [CRITICAL -> WARNING -> NORMAL]
#define CONFIRM_DOWN_WARNING   3U
#define CONFIRM_DOWN_CRITICAL  5U

/*----Broadcast intervals (ms)--- */
#define BROADCAST_HIST_MS            1U
#define BROADCAST_NORMAL_MS      60000U   // 1 min
#define BROADCAST_WARNING_MS      1000U   // 1 sec / 1 hz
#define BROADCAST_CRITICAL_MS       100U   // 20 ms/ 50 hz


/*-----Fast advertising duration on CRITICAL (ms) --------------*/
#define FAST_ADV_DURATION_MS      10000U //10 sec
// how long to advertise at fast rate when CRITICAL is first triggered

/*-------slow tilt-rate thresholds (deg per hour)-----------
 * 0.1 deg/h : warning threshold (Uchimura et al. 2015)
 */
#define TILT_RATE_WARN_DPH      0.1f
#define TILT_RATE_CRIT_DPH      0.9f

/* Acceleration-magnitude z-score threshold, angle-independent */
#define ACCELERATION_GRAVITY_DEVIATION_K_SIGMA        4.0f   // z-score multiplier, matches literature margin for step/impact events
#define ACCELERATION_GRAVITY_DEVIATION_MIN_SIGMA    0.01f  // floor on std (1g units) so threshold can't collapse to ~0 if bench is unusually quiet
#define ACCELERATION_GRAVITY_DEVIATION_LATCH_MS     2000U  // hold the reason bit this long after the last true reading, for a slower BLE check/read to still catches a transient event

/*-------rate channel windowing------
 * 12 snapshots x 5 min = 1h rate window.
 * keep the std during prod;
 */
#define SNAP_PERIOD_MS    300000U // 10000U  //300000U //std
#define RATE_SNAPS         12U //  6U //  12U  //std //compares the newest snapshot against the one from 12 snapshots ago -- 1 hr ago


/*------STATES---------*/
typedef enum {
	TILT_STATE_CALIBRATING = 0,
	TILT_STATE_NORMAL = 1,
	TILT_STATE_WARNING = 2,
	TILT_STATE_CRITICAL = 3,
}TiltState_t;

/*------API------------*/
void tilt_detector_reset(void);

//
TiltState_t tilt_detector_update(float tilt_deg, float gyro_mag, float acceleration_magnitude_g);
float tilt_detector_get_rate_dph(void);   // slow tilt rate, deg/hour

TiltState_t tilt_detector_get_state(void);


imu_rate_t tilt_detector_get_rate(void);
float tilt_detector_get_deviation(void);
float tilt_detector_get_baseline(void);
uint8_t tilt_detector_get_calib_percent(void);




#endif /* INC_TILT_DETECTOR_H_ */
