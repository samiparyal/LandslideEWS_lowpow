/*
 * landslide_service.h
 *
 *  Created on: Feb 9, 2026
 *      Author: Samip
 */

#ifndef INC_LANDSLIDE_SERVICE_H_
#define INC_LANDSLIDE_SERVICE_H_

#include <stdint.h>
#include "ble.h"

#define LANDSLIDE_SERVICE_UUID 0xFF01U //service uuid
 //characteristics UUIDs
#define ALERT_STATUS_CHAR_UUID    0xFF02U
#define TILT_DATA_CHAR_UUID       0xFF03U
#define RAW_IMU_DATA_CHAR_UUID    0xFF04U


typedef struct {
    int16_t ax, ay, az, gx, gy, gz;
    uint16_t dev_x100;
    uint16_t vel_x100;
    uint64_t timestamp_ms;   /* wide enough to never wrap for a months-long deployment */
    uint8_t  is_hist_burst;  /* 1 = replayed historical sample, 0 = live current sample */
} RawImuSample_t;

extern uint8_t s_imu_pending;
extern RawImuSample_t rawImuValue;

//fn prototyps
tBleStatus Landslide_Service_Init(void);
tBleStatus Landslide_Update_Alert_Status(uint8_t alert_level, uint8_t trigger_reason);
tBleStatus Landslide_Update_Tilt_Data(uint16_t deviation_deg, uint16_t velocity_x100);


//call from connection & disconnection events
void Landslide_Service_ConnectionComplete(uint16_t connhandle);
void Landslide_Service_Disconnection(void);

//call from "attribute modified" event to detect CCCD writes
void Landslide_Service_AttributeModified(uint16_t attr_handle, const uint8_t *data, uint8_t data_len);

//
tBleStatus Landslide_Buffer_Sample(uint16_t deviation_x100, uint16_t velocity_x100,
                                    const int16_t accel[3], const int16_t gyro[3],
                                    uint8_t gyro_was_off);

#define IMU_BUFFER_LEN 50U
tBleStatus Landslide_Send_Next_Pending(void);
void       Landslide_Mark_Pending_As_Historical(void);
uint8_t    Landslide_Historical_Pending(void);

#endif /* INC_LANDSLIDE_SERVICE_H_ */
