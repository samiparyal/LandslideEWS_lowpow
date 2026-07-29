#ifndef INC_IMU_BUS_H_
#define INC_IMU_BUS_H_

#include <stdint.h>
#include "stm32wb0x_hal.h"

int32_t imu_bus_write(void *handle, uint8_t reg, const uint8_t *buf, uint16_t len);
int32_t imu_bus_read(void *handle, uint8_t reg, uint8_t *buf, uint16_t len);
void imu_bus_delay_ms(uint32_t ms);


#endif /* INC_IMU_BUS_H_ */
