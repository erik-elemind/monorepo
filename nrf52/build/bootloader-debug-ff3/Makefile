# Platform specific Makefile
PROJECT_NAME := morpheus_ble_bootloader_debug_ff3

# Add any platform specific options
CFLAGS += -DBOARD_CUSTOM
CFLAGS += -DBOARD_FF3
ASMFLAGS += -DBOARD_CUSTOM
ASMFLAGS += -DBOARD_FF3

# Enable crypto on FF3
CFLAGS += -DBL_CRYPTO_DISABLE=0

# Enable logs
CFLAGS += -DNRF_LOG_ENABLED=1

# Use non-deferred logs, which helps with the long running tests, and 
# reduces the buffers needed.
CFLAGS += -DNRF_LOG_DEFERRED=0
CFLAGS += -DNRF_LOG_BUFSIZE=128

# Specify the linker script which has a larger code space for flash.
# We must completely erase the chip any time we switch the bootloader 
# memory map to update the booloader start addr in UICR and MBR.
# Use nrfjprog --recover to erase the chip.
LINKER_SCRIPT ?= ../nrf52832_bootloader_test.ld

# Proceed to the common bootloader makefile.
include ../nrf52832_bootloader.mk
