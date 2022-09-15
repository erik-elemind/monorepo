# Platform specific Makefile
PROJECT_NAME := morpheus_ble_app_ff3

# Add any platform specific options
CFLAGS += -DBOARD_CUSTOM
CFLAGS += -DBOARD_FF3
ASMFLAGS += -DBOARD_CUSTOM
ASMFLAGS += -DBOARD_FF3

# Proceed to the common bootloader makefile.
include ../nrf52832_app.mk
