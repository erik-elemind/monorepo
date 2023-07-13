# Platform specific Makefile
PROJECT_NAME := morpheus_ble_bootloader_test_ff3

# Add any platform specific options
CFLAGS += -DBOARD_CUSTOM
CFLAGS += -DBOARD_FF3
ASMFLAGS += -DBOARD_CUSTOM
ASMFLAGS += -DBOARD_FF3

# Enable the test flag
CFLAGS += -DEXT_FSTORAGE_TEST=1

# Enable crypto
CFLAGS += -DBL_CRYPTO_DISABLE=0

# Enable logs to see the test results
CFLAGS += -DNRF_LOG_ENABLED=1

# Use non-deferred logs, which helps with the long running tests, and 
# reduces the buffers needed.
CFLAGS += -DNRF_LOG_DEFERRED=0
CFLAGS += -DNRF_LOG_BUFSIZE=128

# Reduce ram consumption to enable the test buffers to fit.
# Important note: this can make the DFU transfer fail artificially.
CFLAGS += -DNRF_DFU_BLE_BUFFERS_OVERRIDE=1
CFLAGS += -DNRF_DFU_BLE_BUFFERS=1

# Disable the inactivity timer so that we only run the tests once and stop.
CFLAGS += -DNRF_BL_DFU_INACTIVITY_TIMEOUT_MS=0

# Specify the linker script which has a larger code space for flash.
# We must completely erase the chip any time we switch the bootloader 
# memory map to update the booloader start addr in UICR and MBR.
# Use nrfjprog --recover to erase the chip.
LINKER_SCRIPT ?= ../nrf52832_bootloader_test.ld

# Proceed to the common bootloader makefile.
include ../nrf52832_bootloader.mk
