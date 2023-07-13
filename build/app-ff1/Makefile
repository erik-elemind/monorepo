PROJECT_NAME     := morpheus_ble_app_ff1
TARGETS          := nrf52810_xxaa
OUTPUT_DIRECTORY := _build

SDK_ROOT ?= ../../../../../..

LINKER_SCRIPT  ?= ../nrf52810_app.ld

# Board specific SRC_FILES
SRC_FILES += $(SDK_ROOT)/modules/nrfx/mdk/gcc_startup_nrf52810.S
SRC_FILES += $(SDK_ROOT)/modules/nrfx/mdk/system_nrf52810.c

# Board specific INC_FOLDERS
INC_FOLDERS += $(SDK_ROOT)/components/softdevice/s112/headers
INC_FOLDERS += $(SDK_ROOT)/components/softdevice/s112/headers/nrf52

# Add the SRC_FILES and INC_FOLDERS common to all app builds
include ../app_src.mk

# Libraries common to all targets
LIB_FILES += \

# Optimization flags
OPT = -Os -g3
# Uncomment the line below to enable link time optimization
#OPT += -flto

# C flags common to all targets
CFLAGS += $(OPT)
CFLAGS += -DAPP_TIMER_V2
CFLAGS += -DAPP_TIMER_V2_RTC1_ENABLED
CFLAGS += -DBOARD_CUSTOM
CFLAGS += -DBOARD_FF1
CFLAGS += -DCONFIG_GPIO_AS_PINRESET
CFLAGS += -DDEVELOP_IN_NRF52832
CFLAGS += -DFLOAT_ABI_SOFT
CFLAGS += -DNRF52810_XXAA
CFLAGS += -DNRF52_PAN_74
CFLAGS += -DNRFX_COREDEP_DELAY_US_LOOP_CYCLES=3
CFLAGS += -DNRF_SD_BLE_API_VERSION=7
CFLAGS += -DNRF_DFU_SVCI_ENABLED
CFLAGS += -DNRF_DFU_TRANSPORT_BLE=1
CFLAGS += -DS112
CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs
CFLAGS += -Wall -Werror
CFLAGS += -mfloat-abi=soft
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
# Don't warn about unused functions since they are discarded.
CFLAGS += -Wno-unused-function
CFLAGS += -fno-builtin -fshort-enums

# enable file and line numbers
CFLAGS += -DDEBUG=1

# C++ flags common to all targets
CXXFLAGS += $(OPT)
# Assembler flags common to all targets
ASMFLAGS += -g3
ASMFLAGS += -mcpu=cortex-m4
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += -mfloat-abi=soft
ASMFLAGS += -DAPP_TIMER_V2
ASMFLAGS += -DAPP_TIMER_V2_RTC1_ENABLED
ASMFLAGS += -DNRF_DFU_SVCI_ENABLED
ASMFLAGS += -DNRF_DFU_TRANSPORT_BLE=1
ASMFLAGS += -DBOARD_CUSTOM
ASMFLAGS += -DBOARD_FF1
ASMFLAGS += -DCONFIG_GPIO_AS_PINRESET
ASMFLAGS += -DDEVELOP_IN_NRF52832
ASMFLAGS += -DFLOAT_ABI_SOFT
ASMFLAGS += -DNRF52810_XXAA
ASMFLAGS += -DNRF52_PAN_74
ASMFLAGS += -DNRFX_COREDEP_DELAY_US_LOOP_CYCLES=3
ASMFLAGS += -DNRF_SD_BLE_API_VERSION=7
ASMFLAGS += -DS112
ASMFLAGS += -DSOFTDEVICE_PRESENT

# Linker flags
LDFLAGS += $(OPT)
LDFLAGS += -mthumb -mabi=aapcs -L$(SDK_ROOT)/modules/nrfx/mdk -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs

nrf52810_xxaa: CFLAGS += -D__HEAP_SIZE=2048
nrf52810_xxaa: CFLAGS += -D__STACK_SIZE=2048
nrf52810_xxaa: ASMFLAGS += -D__HEAP_SIZE=2048
nrf52810_xxaa: ASMFLAGS += -D__STACK_SIZE=2048

# Add standard libraries at the very end of the linker input, after all objects
# that may need symbols provided by these libraries.
LIB_FILES += -lc -lnosys -lm

# Default build output
APP_HEX := $(OUTPUT_DIRECTORY)/nrf52810_xxaa.hex
# Bootloader settings hex
BL_SETTINGS_HEX := $(OUTPUT_DIRECTORY)/bootloader_settings.hex
# Merged app+bl settings
APP_WITH_SETTINGS_HEX := $(OUTPUT_DIRECTORY)/$(PROJECT_NAME)_$(GIT_INFO)_with_settings.hex
# Softdevice hex
SD_HEX := $(SDK_ROOT)/components/softdevice/s112/hex/s112_nrf52_7.0.1_softdevice.hex
# Package app for DFU
PACKAGE_NAME := $(OUTPUT_DIRECTORY)/$(PROJECT_NAME)_$(GIT_INFO)_package.zip


.PHONY: default help

# Default target - first one defined
default: $(APP_WITH_SETTINGS_HEX) $(PACKAGE_NAME)

# Print all targets that can be built
help:
	@echo 'following targets are available:'
	@echo '	nrf52810_xxaa (default)'
	@echo '	flash_softdevice'
	@echo '	sdk_config - starting external tool for editing sdk_config.h'
	@echo '	flash      - flashing binary'
	@echo '	app_with_settings - hex file with bootloader settings'
	@echo '	  (required for flashing with secure bootloader installed)'
	@echo '	flash_app_with_settings - flash app and bootloader settings'
	@echo '	package - DFU .zip package'

TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc


include $(TEMPLATE_PATH)/Makefile.common

$(foreach target, $(TARGETS), $(call define_target, $(target)))

.PHONY: flash flash_softdevice erase

# Flash softdevice
flash_softdevice:
	@echo Flashing: $(SD_HEX)
	../flash.sh -f $(SD_HEX) -d NRF52811_XXAA

# Build app with bootloader settings
$(APP_WITH_SETTINGS_HEX): nrf52810_xxaa
	@echo Building app with bootloader settings: $(APP_WITH_SETTINGS_HEX)
	$(Q) nrfutil settings generate --family NRF52810 \
		--application $(APP_HEX) \
		--application-version 0 --bootloader-version 0 \
		--bl-settings-version 2 \
		$(BL_SETTINGS_HEX) > /dev/null
	$(Q) mergehex --merge \
		$(APP_HEX) \
		$(BL_SETTINGS_HEX) \
		--output $(APP_WITH_SETTINGS_HEX) > /dev/null

# Flash app and bootloader settings (required if secure bootloader installed)
flash: $(APP_WITH_SETTINGS_HEX)
	@echo Flashing app with bootloader settings
	../flash.sh -f  $(APP_WITH_SETTINGS_HEX) -d NRF52811_XXAA

erase:
	nrfjprog -f nrf52 --eraseall

# Configure SDK settings
SDK_CONFIG_FILE := ../config/sdk_config.h
CMSIS_CONFIG_TOOL := $(SDK_ROOT)/external_tools/cmsisconfig/CMSIS_Configuration_Wizard.jar
sdk_config:
	java -jar $(CMSIS_CONFIG_TOOL) $(SDK_CONFIG_FILE)

# Firmware ID for softdevice--see "nrfutil pkg generate --help"
SD112_7_0_1_FW_ID := 0xCD
package $(PACKAGE_NAME): nrf52810_xxaa
	@echo Packaging $(PACKAGE_NAME)
	nrfutil pkg generate --hw-version 52 --application-version 1 \
		--sd-req $(SD112_7_0_1_FW_ID) \
		--application $(APP_HEX) \
		--app-boot-validation VALIDATE_GENERATED_SHA256 \
		--key-file $(SOURCE_DIR)/../private-key.pem \
		$(PACKAGE_NAME)
