################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../utilities/fsl_assert.c \
../utilities/fsl_debug_console.c 

OBJS += \
./utilities/fsl_assert.o \
./utilities/fsl_debug_console.o 

C_DEPS += \
./utilities/fsl_assert.d \
./utilities/fsl_debug_console.d 


# Each subdirectory must supply rules for building sources it contributes
utilities/%.o: ../utilities/%.c utilities/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MIMXRT685SFVKB -DCPU_MIMXRT685SFVKB_cm33 -DBOOT_HEADER_ENABLE=1 -DFSL_SDK_DRIVER_QUICK_ACCESS_ENABLE=1 -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE=1 -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Workspace\morpheus_fw_imxrt685\board" -I"C:\Workspace\morpheus_fw_imxrt685\source" -I"C:\Workspace\morpheus_fw_imxrt685\utilities" -I"C:\Workspace\morpheus_fw_imxrt685\drivers" -I"C:\Workspace\morpheus_fw_imxrt685\device" -I"C:\Workspace\morpheus_fw_imxrt685\component\uart" -I"C:\Workspace\morpheus_fw_imxrt685\flash_config" -I"C:\Workspace\morpheus_fw_imxrt685\component\lists" -I"C:\Workspace\morpheus_fw_imxrt685\MIMXRT685S" -I"C:\Workspace\morpheus_fw_imxrt685\CMSIS" -I"C:\Workspace\morpheus_fw_imxrt685\evkmimxrt685\demo_apps\hello_world" -I"C:\Workspace\morpheus_fw_imxrt685\cnn" -O0 -fno-common -g3 -mcpu=cortex-m33 -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


