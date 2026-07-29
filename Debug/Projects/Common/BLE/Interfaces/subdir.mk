################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Projects/Common/BLE/Interfaces/cryptolib_hw_aes.c \
../Projects/Common/BLE/Interfaces/hw_aes.c \
../Projects/Common/BLE/Interfaces/hw_pka.c \
../Projects/Common/BLE/Interfaces/hw_rng.c \
../Projects/Common/BLE/Interfaces/stm32_lpm_if.c 

OBJS += \
./Projects/Common/BLE/Interfaces/cryptolib_hw_aes.o \
./Projects/Common/BLE/Interfaces/hw_aes.o \
./Projects/Common/BLE/Interfaces/hw_pka.o \
./Projects/Common/BLE/Interfaces/hw_rng.o \
./Projects/Common/BLE/Interfaces/stm32_lpm_if.o 

C_DEPS += \
./Projects/Common/BLE/Interfaces/cryptolib_hw_aes.d \
./Projects/Common/BLE/Interfaces/hw_aes.d \
./Projects/Common/BLE/Interfaces/hw_pka.d \
./Projects/Common/BLE/Interfaces/hw_rng.d \
./Projects/Common/BLE/Interfaces/stm32_lpm_if.d 


# Each subdirectory must supply rules for building sources it contributes
Projects/Common/BLE/Interfaces/%.o Projects/Common/BLE/Interfaces/%.su Projects/Common/BLE/Interfaces/%.cyclo: ../Projects/Common/BLE/Interfaces/%.c Projects/Common/BLE/Interfaces/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32WB07 -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32WB0x_HAL_Driver/Inc -I../Drivers/STM32WB0x_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32WB0X/Include -I../Drivers/CMSIS/Include -I../STM32_BLE/App -I../STM32_BLE/Target -I../System/Config/Debug_GPIO -I../System/Interfaces -I../Utilities/trace/adv_trace -I../Projects/Common/BLE/Interfaces -I../Projects/Common/BLE/Modules -I../Projects/Common/BLE/Modules/RTDebug -I../Projects/Common/BLE/Modules/RADIO_utils/Inc -I../Projects/Common/BLE/Modules/Profiles/Inc -I../Projects/Common/BLE/Modules/PKAMGR/Inc -I../Projects/Common/BLE/Modules/NVMDB/Inc -I../Projects/Common/BLE/Modules/Flash -I../Projects/Common/BLE/Startup -I../Utilities/misc -I../Utilities/sequencer -I../Utilities/lpm/tiny_lpm -I../Middlewares/ST/STM32_BLE -I../Middlewares/ST/STM32_BLE/cryptolib/Inc -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/Common -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/CBC -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/CMAC -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/Common -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/ECB -I../Middlewares/ST/STM32_BLE/evt_handler/inc -I../Middlewares/ST/STM32_BLE/queued_writes/inc -I../Middlewares/ST/STM32_BLE/stack/include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Projects-2f-Common-2f-BLE-2f-Interfaces

clean-Projects-2f-Common-2f-BLE-2f-Interfaces:
	-$(RM) ./Projects/Common/BLE/Interfaces/cryptolib_hw_aes.cyclo ./Projects/Common/BLE/Interfaces/cryptolib_hw_aes.d ./Projects/Common/BLE/Interfaces/cryptolib_hw_aes.o ./Projects/Common/BLE/Interfaces/cryptolib_hw_aes.su ./Projects/Common/BLE/Interfaces/hw_aes.cyclo ./Projects/Common/BLE/Interfaces/hw_aes.d ./Projects/Common/BLE/Interfaces/hw_aes.o ./Projects/Common/BLE/Interfaces/hw_aes.su ./Projects/Common/BLE/Interfaces/hw_pka.cyclo ./Projects/Common/BLE/Interfaces/hw_pka.d ./Projects/Common/BLE/Interfaces/hw_pka.o ./Projects/Common/BLE/Interfaces/hw_pka.su ./Projects/Common/BLE/Interfaces/hw_rng.cyclo ./Projects/Common/BLE/Interfaces/hw_rng.d ./Projects/Common/BLE/Interfaces/hw_rng.o ./Projects/Common/BLE/Interfaces/hw_rng.su ./Projects/Common/BLE/Interfaces/stm32_lpm_if.cyclo ./Projects/Common/BLE/Interfaces/stm32_lpm_if.d ./Projects/Common/BLE/Interfaces/stm32_lpm_if.o ./Projects/Common/BLE/Interfaces/stm32_lpm_if.su

.PHONY: clean-Projects-2f-Common-2f-BLE-2f-Interfaces

