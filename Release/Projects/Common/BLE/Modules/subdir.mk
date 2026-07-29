################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../Projects/Common/BLE/Modules/blue_unit_conversion.s \
../Projects/Common/BLE/Modules/osal_memcpy.s 

C_SRCS += \
../Projects/Common/BLE/Modules/miscutil.c \
../Projects/Common/BLE/Modules/osal.c \
../Projects/Common/BLE/Modules/stm_list.c 

OBJS += \
./Projects/Common/BLE/Modules/blue_unit_conversion.o \
./Projects/Common/BLE/Modules/miscutil.o \
./Projects/Common/BLE/Modules/osal.o \
./Projects/Common/BLE/Modules/osal_memcpy.o \
./Projects/Common/BLE/Modules/stm_list.o 

S_DEPS += \
./Projects/Common/BLE/Modules/blue_unit_conversion.d \
./Projects/Common/BLE/Modules/osal_memcpy.d 

C_DEPS += \
./Projects/Common/BLE/Modules/miscutil.d \
./Projects/Common/BLE/Modules/osal.d \
./Projects/Common/BLE/Modules/stm_list.d 


# Each subdirectory must supply rules for building sources it contributes
Projects/Common/BLE/Modules/%.o: ../Projects/Common/BLE/Modules/%.s Projects/Common/BLE/Modules/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m0plus -c -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@" "$<"
Projects/Common/BLE/Modules/%.o Projects/Common/BLE/Modules/%.su Projects/Common/BLE/Modules/%.cyclo: ../Projects/Common/BLE/Modules/%.c Projects/Common/BLE/Modules/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -DUSE_HAL_DRIVER -DSTM32WB07 -DUSE_FULL_LL_DRIVER -c -I../Core/Inc -I../Drivers/STM32WB0x_HAL_Driver/Inc -I../Drivers/STM32WB0x_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32WB0X/Include -I../Drivers/CMSIS/Include -I../STM32_BLE/App -I../STM32_BLE/Target -I../System/Config/Debug_GPIO -I../System/Interfaces -I../Utilities/trace/adv_trace -I../Projects/Common/BLE/Interfaces -I../Projects/Common/BLE/Modules -I../Projects/Common/BLE/Modules/RTDebug -I../Projects/Common/BLE/Modules/RADIO_utils/Inc -I../Projects/Common/BLE/Modules/Profiles/Inc -I../Projects/Common/BLE/Modules/PKAMGR/Inc -I../Projects/Common/BLE/Modules/NVMDB/Inc -I../Projects/Common/BLE/Modules/Flash -I../Projects/Common/BLE/Startup -I../Utilities/misc -I../Utilities/sequencer -I../Utilities/lpm/tiny_lpm -I../Middlewares/ST/STM32_BLE -I../Middlewares/ST/STM32_BLE/cryptolib/Inc -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/Common -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/CBC -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/CMAC -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/Common -I../Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/ECB -I../Middlewares/ST/STM32_BLE/evt_handler/inc -I../Middlewares/ST/STM32_BLE/queued_writes/inc -I../Middlewares/ST/STM32_BLE/stack/include -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Projects-2f-Common-2f-BLE-2f-Modules

clean-Projects-2f-Common-2f-BLE-2f-Modules:
	-$(RM) ./Projects/Common/BLE/Modules/blue_unit_conversion.d ./Projects/Common/BLE/Modules/blue_unit_conversion.o ./Projects/Common/BLE/Modules/miscutil.cyclo ./Projects/Common/BLE/Modules/miscutil.d ./Projects/Common/BLE/Modules/miscutil.o ./Projects/Common/BLE/Modules/miscutil.su ./Projects/Common/BLE/Modules/osal.cyclo ./Projects/Common/BLE/Modules/osal.d ./Projects/Common/BLE/Modules/osal.o ./Projects/Common/BLE/Modules/osal.su ./Projects/Common/BLE/Modules/osal_memcpy.d ./Projects/Common/BLE/Modules/osal_memcpy.o ./Projects/Common/BLE/Modules/stm_list.cyclo ./Projects/Common/BLE/Modules/stm_list.d ./Projects/Common/BLE/Modules/stm_list.o ./Projects/Common/BLE/Modules/stm_list.su

.PHONY: clean-Projects-2f-Common-2f-BLE-2f-Modules

