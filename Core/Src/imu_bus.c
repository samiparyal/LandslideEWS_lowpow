#include "imu_bus.h"
#include "i2c.h"

#define LSM6DSV16X_I2C_ADDR_7BIT  (0x6A) //7-bit //but HAL expects 8-bit write base

int32_t imu_bus_write(void *handle, uint8_t reg, const uint8_t *buf, uint16_t len)
{
	I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)handle;

	if(HAL_I2C_Mem_Write(hi2c, (LSM6DSV16X_I2C_ADDR_7BIT << 1), reg, I2C_MEMADD_SIZE_8BIT, (uint8_t *)buf, len, 100) == HAL_OK) //(uint8_t *) = force typecasting
			{
				return 0;
			}
	return -1;
}

int32_t imu_bus_read(void *handle, uint8_t reg, uint8_t *buf, uint16_t len)
{
	I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)handle;

	if (HAL_I2C_Mem_Read(hi2c, (LSM6DSV16X_I2C_ADDR_7BIT << 1), reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100) == HAL_OK)
			{
				return 0;
			}
	return -1;
}

void imu_bus_delay_ms(uint32_t ms)
{
	HAL_Delay(ms);
}
