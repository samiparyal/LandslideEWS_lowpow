################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/SEGGER_RTT.c \
../Core/Src/SEGGER_RTT_printf.c \
../Core/Src/app_entry.c \
../Core/Src/gpio.c \
../Core/Src/i2c.c \
../Core/Src/imu.c \
../Core/Src/imu_bus.c \
../Core/Src/landslide_service.c \
../Core/Src/lsm6dsv16x_reg.c \
../Core/Src/main.c \
../Core/Src/pka.c \
../Core/Src/radio.c \
../Core/Src/radio_timer.c \
../Core/Src/rng.c \
../Core/Src/stm32wb0x_hal_msp.c \
../Core/Src/stm32wb0x_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32wb0x.c \
../Core/Src/tilt_detector.c \
../Core/Src/usart.c 

OBJS += \
./Core/Src/SEGGER_RTT.o \
./Core/Src/SEGGER_RTT_printf.o \
./Core/Src/app_entry.o \
./Core/Src/gpio.o \
./Core/Src/i2c.o \
./Core/Src/imu.o \
./Core/Src/imu_bus.o \
./Core/Src/landslide_service.o \
./Core/Src/lsm6dsv16x_reg.o \
./Core/Src/main.o \
./Core/Src/pka.o \
./Core/Src/radio.o \
./Core/Src/radio_timer.o \
./Core/Src/rng.o \
./Core/Src/stm32wb0x_hal_msp.o \
./Core/Src/stm32wb0x_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32wb0x.o \
./Core/Src/tilt_detector.o \
./Core/Src/usart.o 

C_DEPS += \
./Core/Src/SEGGER_RTT.d \
./Core/Src/SEGGER_RTT_printf.d \
./Core/Src/app_entry.d \
./Core/Src/gpio.d \
./Core/Src/i2c.d \
./Core/Src/imu.d \
./Core/Src/imu_bus.d \
./Core/Src/landslide_service.d \
./Core/Src/lsm6dsv16x_reg.d \
./Core/Src/main.d \
./Core/Src/pka.d \
./Core/Src/radio.d \
./Core/Src/radio_timer.d \
./Core/Src/rng.d \
./Core/Src/stm32wb0x_hal_msp.d \
./Core/Src/stm32wb0x_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32wb0x.d \
./Core/Src/tilt_detector.d \
./Core/Src/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32WB07 -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32WB0x_HAL_Driver/Inc -I../Drivers/STM32WB0x_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32WB0X/Include -I../Drivers/CMSIS/Include -I../STM32_BLE/App -I../STM32_BLE/Target -I../System/Config/Debug_GPIO -I../System/Interfaces -I../Utilities/trace/adv_trace -I../Projects/Common/BLE/Interfaces -I../Projects/Common/BLE/Modules -I../Projects/Common/BLE/Modules/RTDebug -I../Projects/Common/BLE/Modules/RADIO_utils/Inc -I../Projects/Common/BLE/Modules/Profiles/Inc -I../Projects/Common/BLE/Modules/PKAMGR/Inc -I../Projects/Common/BLE/Modules/NVMDB/Inc -I../Projects/Common/BLE/Modules/Flash -I../Projects/Common/BLE/Startup -I../Utilities/misc -I../Utilities/sequencer -I../Utilities/lpm/tiny_lpm -I../Middlewares/ST/STM32_BLE -I../Middlewares/ST/STM32_BLE/cryptolib/Inc -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/Common -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/CBC -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/CMAC -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/Common -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/ECB -I../Middlewares/ST/STM32_BLE/evt_handler/inc -I../Middlewares/ST/STM32_BLE/queued_writes/inc -I../Middlewares/ST/STM32_BLE/stack/include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/SEGGER_RTT.cyclo ./Core/Src/SEGGER_RTT.d ./Core/Src/SEGGER_RTT.o ./Core/Src/SEGGER_RTT.su ./Core/Src/SEGGER_RTT_printf.cyclo ./Core/Src/SEGGER_RTT_printf.d ./Core/Src/SEGGER_RTT_printf.o ./Core/Src/SEGGER_RTT_printf.su ./Core/Src/app_entry.cyclo ./Core/Src/app_entry.d ./Core/Src/app_entry.o ./Core/Src/app_entry.su ./Core/Src/gpio.cyclo ./Core/Src/gpio.d ./Core/Src/gpio.o ./Core/Src/gpio.su ./Core/Src/i2c.cyclo ./Core/Src/i2c.d ./Core/Src/i2c.o ./Core/Src/i2c.su ./Core/Src/imu.cyclo ./Core/Src/imu.d ./Core/Src/imu.o ./Core/Src/imu.su ./Core/Src/imu_bus.cyclo ./Core/Src/imu_bus.d ./Core/Src/imu_bus.o ./Core/Src/imu_bus.su ./Core/Src/landslide_service.cyclo ./Core/Src/landslide_service.d ./Core/Src/landslide_service.o ./Core/Src/landslide_service.su ./Core/Src/lsm6dsv16x_reg.cyclo ./Core/Src/lsm6dsv16x_reg.d ./Core/Src/lsm6dsv16x_reg.o ./Core/Src/lsm6dsv16x_reg.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/pka.cyclo ./Core/Src/pka.d ./Core/Src/pka.o ./Core/Src/pka.su ./Core/Src/radio.cyclo ./Core/Src/radio.d ./Core/Src/radio.o ./Core/Src/radio.su ./Core/Src/radio_timer.cyclo ./Core/Src/radio_timer.d ./Core/Src/radio_timer.o ./Core/Src/radio_timer.su ./Core/Src/rng.cyclo ./Core/Src/rng.d ./Core/Src/rng.o ./Core/Src/rng.su ./Core/Src/stm32wb0x_hal_msp.cyclo ./Core/Src/stm32wb0x_hal_msp.d ./Core/Src/stm32wb0x_hal_msp.o ./Core/Src/stm32wb0x_hal_msp.su ./Core/Src/stm32wb0x_it.cyclo ./Core/Src/stm32wb0x_it.d ./Core/Src/stm32wb0x_it.o ./Core/Src/stm32wb0x_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32wb0x.cyclo ./Core/Src/system_stm32wb0x.d ./Core/Src/system_stm32wb0x.o ./Core/Src/system_stm32wb0x.su ./Core/Src/tilt_detector.cyclo ./Core/Src/tilt_detector.d ./Core/Src/tilt_detector.o ./Core/Src/tilt_detector.su ./Core/Src/usart.cyclo ./Core/Src/usart.d ./Core/Src/usart.o ./Core/Src/usart.su

.PHONY: clean-Core-2f-Src

