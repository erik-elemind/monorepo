################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utilities/subdir.mk
-include source/subdir.mk
-include flash_config/subdir.mk
-include drivers/subdir.mk
-include component/uart/subdir.mk
-include component/lists/subdir.mk
-include cnn/subdir.mk
-include board/subdir.mk
-include MIMXRT685S/mcuxpresso/subdir.mk
-include MIMXRT685S/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := morpheus_fw_imxrt685
BUILD_ARTIFACT_EXTENSION := axf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: morpheus_fw_imxrt685.axf

# Tool invocations
morpheus_fw_imxrt685.axf: $(OBJS) $(USER_OBJS) makefile objects.mk $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -Map="morpheus_fw_imxrt685.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -T evkmimxrt685_hello_world_Debug.ld -o "morpheus_fw_imxrt685.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) morpheus_fw_imxrt685.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "morpheus_fw_imxrt685.axf"; # arm-none-eabi-objcopy -v -O binary "morpheus_fw_imxrt685.axf" "morpheus_fw_imxrt685.bin" ; # checksum -p MIMXRT685S -d "morpheus_fw_imxrt685.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
