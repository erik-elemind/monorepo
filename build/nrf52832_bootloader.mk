# Common makefile for 52832 bootloader builds

TARGETS          := nrf52832_xxaa_s132
OUTPUT_DIRECTORY := _build

SDK_ROOT ?= ../../../../..

# We must completely erase the chip any time we switch the bootloader 
# memory map to update the booloader start addr in UICR and MBR.
# Use nrfjprog --recover to erase the chip.
LINKER_SCRIPT ?= ../nrf52832_bootloader.ld

# Board specific SRC_FILES
SRC_FILES += $(SDK_ROOT)/modules/nrfx/mdk/gcc_startup_nrf52.S
SRC_FILES += $(SDK_ROOT)/modules/nrfx/mdk/system_nrf52.c

# Board specific INC_FOLDERS
INC_FOLDERS += $(SDK_ROOT)/components/softdevice/s132/headers
INC_FOLDERS += $(SDK_ROOT)/components/softdevice/s132/headers/nrf52

# Add the SRC_FILES and INC_FOLDERS common to all bootloader builds
include ../bootloader_src.mk

# Optimization flags
OPT = -Os -g3
# Uncomment the line below to enable link time optimization
#OPT += -flto

# C flags common to all targets
CFLAGS += $(OPT)
CFLAGS += -DBLE_STACK_SUPPORT_REQD
CFLAGS += -DCONFIG_GPIO_AS_PINRESET
CFLAGS += -DFLOAT_ABI_HARD
CFLAGS += -DNRF52
CFLAGS += -DNRF52832_XXAA
CFLAGS += -DNRF52_PAN_74
CFLAGS += -DNRF_DFU_SETTINGS_VERSION=2
CFLAGS += -DNRF_DFU_SVCI_ENABLED
CFLAGS += -DNRF_SD_BLE_API_VERSION=7
CFLAGS += -DS132
CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -DCONFIG_NFCT_PINS_AS_GPIOS
CFLAGS += -DSVC_INTERFACE_CALL_AS_NORMAL_FUNCTION
CFLAGS += -DuECC_ENABLE_VLI_API=0
CFLAGS += -DuECC_OPTIMIZATION_LEVEL=3
CFLAGS += -DuECC_SQUARE_FUNC=0
CFLAGS += -DuECC_SUPPORT_COMPRESSED_POINT=0
CFLAGS += -DuECC_VLI_NATIVE_LITTLE_ENDIAN=1
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs
CFLAGS += -Wall -ggdb
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
# Don't warn about unused functions since they are discarded.
CFLAGS += -Wno-unused-function
CFLAGS += -fno-builtin -fshort-enums

# C++ flags common to all targets
CXXFLAGS += $(OPT)
# Assembler flags common to all targets
ASMFLAGS += -g3
ASMFLAGS += -mcpu=cortex-m4
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
ASMFLAGS += -DBLE_STACK_SUPPORT_REQD
ASMFLAGS += -DCONFIG_GPIO_AS_PINRESET
ASMFLAGS += -DFLOAT_ABI_HARD
ASMFLAGS += -DNRF52
ASMFLAGS += -DNRF52832_XXAA
ASMFLAGS += -DNRF52_PAN_74
ASMFLAGS += -DNRF_DFU_SETTINGS_VERSION=2
ASMFLAGS += -DNRF_DFU_SVCI_ENABLED
ASMFLAGS += -DNRF_SD_BLE_API_VERSION=7
ASMFLAGS += -DS132
ASMFLAGS += -DSOFTDEVICE_PRESENT
ASMFLAGS += -DSVC_INTERFACE_CALL_AS_NORMAL_FUNCTION
ASMFLAGS += -DuECC_ENABLE_VLI_API=0
ASMFLAGS += -DuECC_OPTIMIZATION_LEVEL=3
ASMFLAGS += -DuECC_SQUARE_FUNC=0
ASMFLAGS += -DuECC_SUPPORT_COMPRESSED_POINT=0
ASMFLAGS += -DuECC_VLI_NATIVE_LITTLE_ENDIAN=1

# Linker flags
LDFLAGS += $(OPT)
LDFLAGS += -mthumb -mabi=aapcs -L$(SDK_ROOT)/modules/nrfx/mdk -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs

nrf52832_xxaa_s132: CFLAGS += -D__HEAP_SIZE=0
nrf52832_xxaa_s132: CFLAGS += -D__STACK_SIZE=2048
nrf52832_xxaa_s132: ASMFLAGS += -D__HEAP_SIZE=0
nrf52832_xxaa_s132: ASMFLAGS += -D__STACK_SIZE=2048


# Add standard libraries at the very end of the linker input, after all objects
# that may need symbols provided by these libraries.
LIB_FILES += -lc -lnosys -lm

# Default build output
APP_HEX := $(OUTPUT_DIRECTORY)/nrf52832_xxaa_s132.hex
# Named output
NAMED_HEX := $(OUTPUT_DIRECTORY)/$(PROJECT_NAME)_$(GIT_INFO).hex
# Sotfdevice hex
SD_HEX := $(SDK_ROOT)/components/softdevice/s132/hex/s132_nrf52_7.0.1_softdevice.hex
# Package app for DFU
PACKAGE_NAME := $(OUTPUT_DIRECTORY)/$(PROJECT_NAME)_$(GIT_INFO)_package.zip

.PHONY: default help

# Firmware ID for softdevice--see "nrfutil pkg generate --help"
SD132_7_0_1_FW_ID := 0xCB

# Default target - first one defined
default: nrf52832_xxaa_s132
	@echo Creating $(NAMED_HEX)
	cp $(APP_HEX) $(NAMED_HEX)
	@echo Packaging $(PACKAGE_NAME)
	nrfutil pkg generate --hw-version 52 --bootloader-version 1 \
		--sd-req $(SD132_7_0_1_FW_ID) \
		--softdevice $(SD_HEX) \
		--bootloader $(APP_HEX) \
		--app-boot-validation VALIDATE_GENERATED_SHA256 \
		--key-file $(SOURCE_DIR)/../private-key.pem \
		$(PACKAGE_NAME)	

# Print all targets that can be built
help:
	@echo following targets are available:
	@echo		nrf52832_xxaa_s132
	@echo		flash_softdevice
	@echo		sdk_config - starting external tool for editing sdk_config.h
	@echo		flash      - flashing binary

TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc


include $(TEMPLATE_PATH)/Makefile.common

$(foreach target, $(TARGETS), $(call define_target, $(target)))

.PHONY: flash flash_softdevice erase

# Flash the program
# Use a JLink script rather than nrfjprog which can cause the RTT log session
# to freeze. This does add quite a bit of text to the command line though.
flash: default
	@echo Flashing: $(APP_HEX)
	../flash.sh -f $(APP_HEX) -d NRF52832_XXAA

# Flash softdevice
flash_softdevice:
	@echo Flashing: $(SD_HEX)
	../flash.sh -f $(SD_HEX) -d NRF52832_XXAA
	
erase:
	nrfjprog -f nrf52 --eraseall

SDK_CONFIG_FILE := ../config/sdk_config.h
CMSIS_CONFIG_TOOL := $(SDK_ROOT)/external_tools/cmsisconfig/CMSIS_Configuration_Wizard.jar
sdk_config:
	java -jar $(CMSIS_CONFIG_TOOL) $(SDK_CONFIG_FILE)

# # Firmware ID for softdevice--see "nrfutil pkg generate --help"
# SD132_7_0_1_FW_ID := 0xCB
# package: default
# 	@echo Packaging $(PACKAGE_NAME)
# 	nrfutil pkg generate --hw-version 52 --bootloader-version 1 \
# 		--sd-req $(SD132_7_0_1_FW_ID) \
# 		--bootloader $(APP_HEX) \
# 		--app-boot-validation VALIDATE_GENERATED_SHA256 \
# 		--key-file $(SOURCE_DIR)/../private-key.pem \
# 		$(PACKAGE_NAME)
