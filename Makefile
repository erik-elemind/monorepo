# Set V=1 on the command line to enable verbose mode
ifeq ($(V),)
export Q=@
export Q_REDIR= 1> /dev/null
else 
export Q=
export Q_REDIR=
endif

# This file contains VERSION_FILE, etc
include version.mk
# Export the include
export INC_FOLDERS=$(dir $(VERSION_FILE))

# Set the paths. These are used by sub-makefiles
export SOURCE_DIR           := $(CURDIR)/source_code/app
export BL_SOURCE_DIR        := $(CURDIR)/source_code/bootloader
export MICRO_ECC_SOURCE_DIR := $(CURDIR)/third_party/micro-ecc
export SDK_ROOT             := $(CURDIR)/third_party/nrf5_sdk

################################################################################
# Virtual env. Needed for nrfutil python module
VENV_DIR   ?= .venv
VENV        = $(shell pwd)/$(VENV_DIR)/bin/activate

# This command enters the venv. Put this on the same line as any command
# that needs to run in the venv
VENV_ENTER  = source $(VENV)

$(VENV_DIR):
	@echo [Creating python3 venv]
	$(Q) mkdir -p $(VENV_DIR)
	$(Q) python3 -m venv $@
	$(Q) $(VENV_ENTER); pip3 install --upgrade pip

$(VENV): $(VENV_DIR) $(PREFIX)requirements.txt
	@echo [Updating virtualenv]
	$(Q) $(VENV_ENTER); pip3 install -Ur $(PREFIX)requirements.txt
	$(Q) touch $@

.PHONY: venv
venv: $(VENV)

################################################################################

# Check the compiler path and version

MYCCEXE = arm-none-eabi-gcc
MYCC = $(shell which $(MYCCEXE))

ifneq (,$(MYCC))
    MYCC_PATH = $(dir $(shell which $(MYCC)))
    # $(info "found cc in path: $(MYCC)")
else
    MYCC_PATH := $(PWD)/$(ARMGCC_BIN)
    ifeq ("$(wildcard $(MYCC_PATH))","")
        $(warning "$(MYCCEXE) not found in PATH. Run 'make dist' to download it.")
    else
        MYCC = $(MYCC_PATH)/$(MYCCEXE)
        # $(info "using dist cc: $(MYCC)")
    endif
endif

# export the path for the inner makefile
export GNU_INSTALL_ROOT := $(MYCC_PATH)
# and the version (should be 8.3.1)
export GNU_VERSION := $(shell $(MYCC) -dumpversion)

################################################################################

# List of buildable targets
TARGETS := app bootloader bootloader-debug bootloader-test
# List of supported boards/platforms
BOARDS := ff4 ff3 ff1 pca10040
# Default to the first board in the list
BOARD_DEFAULT := $(word 1,$(BOARDS))

# If not set, apply default
ifeq ($(BOARD),)
    BOARD := $(BOARD_DEFAULT)
endif
# Confirm BOARD is in the supported list
ifeq ($(filter $(BOARD),$(BOARDS)),)
    $(error "error: please set BOARD variable to one of $(BOARDS). Eg. BOARD=$(BOARD_DEFAULT)")
endif

# Option to flash the image onto the target
ifeq ($(F),1)
	ARGS += flash
endif

# Option to flash the softdevice onto the target
ifeq ($(SD),1)
	ARGS += flash_softdevice
endif

################################################################################
.DEFAULT_GOAL = help
.PHONY: help
help:
	@echo "The following targets are available:"
	@echo "  app             : build the main firmware"
	@echo "  bootloader      : build the bootloader firmware"
	@echo "  bootloader-test : build the bootloader firmware w/ ext flash tests enabled"
	@echo "The following boards are available. Add BOARD=<board> to the cmd line."
	@echo "  $(BOARDS)"
	@echo "  If unspecified, BOARD=$(BOARD_DEFAULT)"
	@echo "Other options:"
	@echo "  F=1        : flash the image over jlink"
	@echo "  SD=1       : flash the softdevice over jlink"
	@echo "  clean      : cleans all target/board builds"

.PHONY: $(TARGETS)
$(TARGETS): $(VERSION_FILE)
	$(Q) make -C build/$@-$(BOARD) -j $(ARGS)

.PHONY: clean
clean:
	@for target in $(TARGETS); do \
		for board in $(BOARDS); do \
			if [ -d "build/$${target}-$${board}" ]; then \
				echo [Removing objects for target $$target-$$board]; \
				$(MAKE) -k -C build/$${target}-$${board} clean; \
			fi \
		done; \
	done;

