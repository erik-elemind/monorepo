################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../cnn/conv2dDirectOptimizedColMajor.c \
../cnn/main.c \
../cnn/predict.c \
../cnn/rtGetInf.c \
../cnn/rtGetNaN.c \
../cnn/rt_nonfinite.c \
../cnn/sleepstagescorer.c \
../cnn/sleepstagescorer_data.c \
../cnn/sleepstagescorer_initialize.c \
../cnn/sleepstagescorer_terminate.c 

OBJS += \
./cnn/conv2dDirectOptimizedColMajor.o \
./cnn/main.o \
./cnn/predict.o \
./cnn/rtGetInf.o \
./cnn/rtGetNaN.o \
./cnn/rt_nonfinite.o \
./cnn/sleepstagescorer.o \
./cnn/sleepstagescorer_data.o \
./cnn/sleepstagescorer_initialize.o \
./cnn/sleepstagescorer_terminate.o 

C_DEPS += \
./cnn/conv2dDirectOptimizedColMajor.d \
./cnn/main.d \
./cnn/predict.d \
./cnn/rtGetInf.d \
./cnn/rtGetNaN.d \
./cnn/rt_nonfinite.d \
./cnn/sleepstagescorer.d \
./cnn/sleepstagescorer_data.d \
./cnn/sleepstagescorer_initialize.d \
./cnn/sleepstagescorer_terminate.d 


# Each subdirectory must supply rules for building sources it contributes
cnn/%.o: ../cnn/%.c cnn/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MIMXRT685SFVKB -DCPU_MIMXRT685SFVKB_cm33 -DBOOT_HEADER_ENABLE=1 -DFSL_SDK_DRIVER_QUICK_ACCESS_ENABLE=1 -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE=1 -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Workspace\morpheus_fw_imxrt685\board" -I"C:\Workspace\morpheus_fw_imxrt685\source" -I"C:\Workspace\morpheus_fw_imxrt685\utilities" -I"C:\Workspace\morpheus_fw_imxrt685\drivers" -I"C:\Workspace\morpheus_fw_imxrt685\device" -I"C:\Workspace\morpheus_fw_imxrt685\component\uart" -I"C:\Workspace\morpheus_fw_imxrt685\flash_config" -I"C:\Workspace\morpheus_fw_imxrt685\component\lists" -I"C:\Workspace\morpheus_fw_imxrt685\MIMXRT685S" -I"C:\Workspace\morpheus_fw_imxrt685\CMSIS" -I"C:\Workspace\morpheus_fw_imxrt685\evkmimxrt685\demo_apps\hello_world" -I"C:\Workspace\morpheus_fw_imxrt685\cnn" -O0 -fno-common -g3 -mcpu=cortex-m33 -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


