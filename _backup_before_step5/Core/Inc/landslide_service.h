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



//fn prototyps
tBleStatus Landslide_Service_Init(void);
tBleStatus Landslide_Update_Alert_Status(uint8_t alert_level, uint8_t trigger_reason);
tBleStatus Landslide_Update_Tilt_Data(uint16_t deviation_x100, uint16_t velocity_x100);


//call from connection & disconnection events
void Landslide_Service_ConnectionComplete(uint16_t connhandle);
void Landslide_Service_Disconnection(void);

//call from "attribute modified" event to detect CCCD writes
void Landslide_Service_AttributeModified(uint16_t attr_handle, const uint8_t *data, uint8_t data_len);



#endif /* INC_LANDSLIDE_SERVICE_H_ */
