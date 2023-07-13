TARGETS          := nrf52832_xxaa
OUTPUT_DIRECTORY := _build

SDK_ROOT ?= ../../../../../..
LINKER_SCRIPT  ?= ../nrf52832_app.ld

# Board specific SRC_FILES
# Note that we are running the 51810 code on the pca10040. 
# See the DEVELOP_IN_NRF52832 compiler flag below.
SRC_FILES += $(SDK_ROOT)/modules/nrfx/mdk/gcc_startup_nrf52.S
SRC_FILES += $(SDK_ROOT)/modules/nrfx/mdk/system_nrf52.c

# Board specific INC_FOLDERS
INC_FOLDERS += $(SDK_ROOT)/components/softdevice/s132/headers
INC_FOLDERS += $(SDK_ROOT)/components/softdevice/s132/headers/nrf52

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
CFLAGS += -DBL_SETTINGS_ACCESS_ONLY
CFLAGS += -DCONFIG_GPIO_AS_PINRESET
CFLAGS += -DFLOAT_ABI_HARD
CFLAGS += -DNRF52
CFLAGS += -DNRF52832_XXAA
CFLAGS += -DNRF52_PAN_74
CFLAGS += -DNRF_DFU_SVCI_ENABLED
CFLAGS += -DNRF_DFU_TRANSPORT_BLE=1
CFLAGS += -DNRF_SD_BLE_API_VERSION=7
CFLAGS += -DS132
CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -DCONFIG_NFCT_PINS_AS_GPIOS
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs
CFLAGS += -Wall -Werror
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin -fshort-enums

# enable file and line numbers
CFLAGS += -DDEBUG=1

# C++ flags common to all targets
CXXFLAGS += $(OPT)
# Assembler flags common to all targets
ASMFLAGS += -g3
ASMFLAGS += -mcpu=cortex-m4
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
ASMFLAGS += -DAPP_TIMER_V2
ASMFLAGS += -DAPP_TIMER_V2_RTC1_ENABLED
ASMFLAGS += -DBL_SETTINGS_ACCESS_ONLY
ASMFLAGS += -DCONFIG_GPIO_AS_PINRESET
ASMFLAGS += -DFLOAT_ABI_HARD
ASMFLAGS += -DNRF52
ASMFLAGS += -DNRF52832_XXAA
ASMFLAGS += -DNRF52_PAN_74
ASMFLAGS += -DNRF_DFU_SVCI_ENABLED
ASMFLAGS += -DNRF_DFU_TRANSPORT_BLE=1
ASMFLAGS += -DNRF_SD_BLE_API_VERSION=7
ASMFLAGS += -DS132
ASMFLAGS += -DSOFTDEVICE_PRESENT

# Linker flags
LDFLAGS += $(OPT)
LDFLAGS += -mthumb -mabi=aapcs -L$(SDK_ROOT)/modules/nrfx/mdk -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs

nrf52832_xxaa: CFLAGS += -D__HEAP_SIZE=2048
nrf52832_xxaa: CFLAGS += -D__STACK_SIZE=2048
nrf52832_xxaa: ASMFLAGS += -D__HEAP_SIZE=2048
nrf52832_xxaa: ASMFLAGS += -D__STACK_SIZE=2048

# Add standard libraries at the very end of the linker input, after all objects
# that may need symbols provided by these libraries.
LIB_FILES += -lc -lnosys -lm

# Default build output
APP_HEX := $(OUTPUT_DIRECTORY)/nrf52832_xxaa.hex
# Bootloader settings hex file
BL_SETTINGS_HEX := $(OUTPUT_DIRECTORY)/bootloader_settings.hex
# Merged app+bl settings, aka NAMED_HEX
APP_WITH_SETTINGS_HEX := $(OUTPUT_DIRECTORY)/$(PROJECT_NAME)_$(GIT_INFO)_with_settings.hex
# Softdevice hex
SD_HEX := $(SDK_ROOT)/components/softdevice/s132/hex/s132_nrf52_7.0.1_softdevice.hex
# Package app for DFU
PACKAGE_NAME := $(OUTPUT_DIRECTORY)/$(PROJECT_NAME)_$(GIT_INFO)_package.zip

.PHONY: default help

# Default target - first one defined
default: $(APP_WITH_SETTINGS_HEX) $(PACKAGE_NAME)

# Print all targets that can be built
help:
	@echo 'following targets are available:'
	@echo '	nrf52832_xxaa (default)'
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

.PHONY: flash flash_softdevice erase package

# Flash softdevice
flash_softdevice:
	@echo Flashing: $(SD_HEX)
	../flash.sh -f $(SD_HEX) -d NRF52832_XXAA

# Build app with bootloader settings
$(APP_WITH_SETTINGS_HEX): nrf52832_xxaa
	@echo Building app with bootloader settings: $(APP_WITH_SETTINGS_HEX)
	$(Q) nrfutil settings generate --family NRF52 \
		--application $(APP_HEX) \
		--application-version 0 --bootloader-version 0 \
		--bl-settings-version 2 \
		$(BL_SETTINGS_HEX) > /dev/null
	$(Q) mergehex --merge \
		$(APP_HEX) \
		$(BL_SETTINGS_HEX) \
		--output $(APP_WITH_SETTINGS_HEX) > /dev/null
	$(Q) rm -f $(APP_HEX)
	$(Q) rm -f $(BL_SETTINGS_HEX)


# Flash app and bootloader settings (required if secure bootloader installed)
flash: $(APP_WITH_SETTINGS_HEX)
	@echo Flashing app with bootloader settings
	../flash.sh -f $(APP_WITH_SETTINGS_HEX) -d NRF52832_XXAA

erase:
	nrfjprog -f nrf52 --eraseall

# Configure SDK settings
SDK_CONFIG_FILE := ../config/sdk_config.h
CMSIS_CONFIG_TOOL := $(SDK_ROOT)/external_tools/cmsisconfig/CMSIS_Configuration_Wizard.jar
sdk_config:
	java -jar $(CMSIS_CONFIG_TOOL) $(SDK_CONFIG_FILE)

# Firmware ID for softdevice--see "nrfutil pkg generate --help"
SD132_7_0_1_FW_ID := 0xCB
package $(PACKAGE_NAME): nrf52832_xxaa
	@echo Creating OTA package $(PACKAGE_NAME)
	nrfutil pkg generate --hw-version 52 --application-version 1 \
		--sd-req $(SD132_7_0_1_FW_ID) \
		--application $(APP_HEX) \
		--app-boot-validation VALIDATE_GENERATED_SHA256 \
		--key-file $(SOURCE_DIR)/../private-key.pem \
		$(PACKAGE_NAME)
