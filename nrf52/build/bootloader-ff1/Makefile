# Platform specific Makefile
PROJECT_NAME := morpheus_ble_bootloader_ff1

# Add any platform specific options
CFLAGS += -DBOARD_CUSTOM
CFLAGS += -DBOARD_FF1
ASMFLAGS += -DBOARD_CUSTOM
ASMFLAGS += -DBOARD_FF1

# Disable crypto to save space
CFLAGS += -DBL_CRYPTO_DISABLE=1

# Disable logs
CFLAGS += -DNRF_LOG_ENABLED=0

# Use non-deferred logs, which helps with the long running tests, and 
# reduces the buffers needed.
CFLAGS += -DNRF_LOG_DEFERRED=0
CFLAGS += -DNRF_LOG_BUFSIZE=128

# Proceed to the common bootloader makefile.
include ../nrf52810_bootloader.mk
