################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Utilities/trace/adv_trace/subdir.mk
-include Utilities/sequencer/subdir.mk
-include Utilities/misc/subdir.mk
-include Utilities/lpm/tiny_lpm/subdir.mk
-include System/Interfaces/subdir.mk
-include System/Config/Debug_GPIO/subdir.mk
-include STM32_BLE/Target/subdir.mk
-include STM32_BLE/App/subdir.mk
-include Projects/Common/BLE/Startup/subdir.mk
-include Projects/Common/BLE/Modules/RTDebug/subdir.mk
-include Projects/Common/BLE/Modules/RADIO_utils/Src/subdir.mk
-include Projects/Common/BLE/Modules/Profiles/Src/subdir.mk
-include Projects/Common/BLE/Modules/PKAMGR/Src/subdir.mk
-include Projects/Common/BLE/Modules/NVMDB/Src/subdir.mk
-include Projects/Common/BLE/Modules/Flash/subdir.mk
-include Projects/Common/BLE/Modules/subdir.mk
-include Projects/Common/BLE/Interfaces/subdir.mk
-include Middlewares/ST/STM32_BLE/stack/config/subdir.mk
-include Middlewares/ST/STM32_BLE/queued_writes/src/subdir.mk
-include Middlewares/ST/STM32_BLE/evt_handler/src/subdir.mk
-include Drivers/STM32WB0x_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.elf \

MAP_FILES += \
IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.list \

OBJCOPY_HEX += \
IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.hex \


# All Target
all: main-build

# Main-build Target
main-build: IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.elf secondary-outputs

# Tool invocations
IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.elf IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.map: $(OBJS) $(USER_OBJS) C:\Users\Samip\STM32CubeIDE\workspace_1.19.0\IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main\STM32WB07CCVX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.elf" @"objects.list" $(USER_OBJS) -Wl,--start-group $(LIBS) -Wl,--end-group -mcpu=cortex-m0plus -T"C:\Users\Samip\STM32CubeIDE\workspace_1.19.0\IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main\STM32WB07CCVX_FLASH.ld" --specs=nosys.specs -Wl,-Map="IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.map" -Wl,--gc-sections -static -L../Middlewares/ST/STM32_BLE/cryptolib -L../Middlewares/ST/STM32_BLE/stack/lib --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.list"
	@echo 'Finished building: $@'
	@echo ' '

IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.hex: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O ihex $(EXECUTABLES) "IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.hex"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.elf IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.hex IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.list IMU_LED_BLE_Lgatt_lowpow_lecoded_main_main.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_HEX)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
