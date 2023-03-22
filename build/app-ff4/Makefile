# Platform specific Makefile
PROJECT_NAME := morpheus_ble_app_ff4

# Add any platform specific options
CFLAGS += -DBOARD_CUSTOM
CFLAGS += -DBOARD_FF4
ASMFLAGS += -DBOARD_CUSTOM
ASMFLAGS += -DBOARD_FF4

# Proceed to the common bootloader makefile.
include ../nrf52832_app.mk
