DIST_DIR = dist
WORK_DIR = .tmp

# Get OS type and bitness
OS_NAME := $(shell uname -s)
OS_BITS := $(shell getconf LONG_BIT)
 
################################################################################

# see https://developer.nordicsemi.com/nRF5_SDK/
NRFSDK_ROOT = nRF5_SDK_16.0.0_98a08e2
NRFSDK_FN   = $(NRFSDK_ROOT).zip
NRFSDK_URL  = https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v16.x.x/$(NRFSDK_FN)

# these are the subdirs of the SDK zip that will be unpacked.
NRFSDK_PARTS = components config examples external integration modules
NRFSDK_EXP   = $(addprefix $(WORK_DIR)/, $(NRFSDK_PARTS))

NRFSDK_FP = $(WORK_DIR)/$(NRFSDK_FN)

$(DIST_DIR)/sdk:
	$(Q) -mkdir -p $(WORK_DIR)
	@echo [Downloading nrf sdk: $(NRFSDK_URL)]
	$(Q) curl $(NRFSDK_URL) -o $(NRFSDK_FP)
	@echo [Processing archive: $(NRFSDK_FN)]
	$(Q) unzip -q $(NRFSDK_FP) -d $(WORK_DIR)
	$(Q) mkdir -p $@
	$(Q) mv $(NRFSDK_EXP) $@
	$(Q) -rm -r $(WORK_DIR)
	@echo [Successfully unpacked: $@]

.PHONY: sdk
sdk: $(DIST_DIR)/sdk

# export the full path
export SDK_ROOT := $(CURDIR)/$(DIST_DIR)/sdk

################################################################################
MICROECC_COMMIT = 24c60e243580c7868f4334a1ba3123481fe1aa48
MICROECC_FN = $(MICROECC_COMMIT).zip
MICROECC_FP = $(WORK_DIR)/$(MICROECC_FN)
MICROECC_UNPACKED = $(WORK_DIR)/micro-ecc-$(MICROECC_COMMIT)
MICROECC_URL = https://github.com/kmackay/micro-ecc/archive/$(MICROECC_FN)

$(DIST_DIR)/micro-ecc:
	$(Q) -mkdir $(WORK_DIR)
	@echo [Downloading micro-ecc: $(MICROECC_URL)]
	$(Q) curl -L $(MICROECC_URL) -o $(MICROECC_FP)
	$(Q) unzip -q -o $(MICROECC_FP) -d .tmp
	$(Q) mkdir $@
	$(Q) mv $(MICROECC_UNPACKED)/* $@
	$(Q) -rm -r $(WORK_DIR)
	@echo [Successfully unpacked: $@]

.PHONY: micro-ecc
micro-ecc: $(DIST_DIR)/micro-ecc

# export the full path
export MICRO_ECC_SOURCE_DIR := $(CURDIR)/$(DIST_DIR)/micro-ecc

################################################################################

# see https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF-Command-Line-Tools/Download
# https://www.nordicsemi.com/-/media/Software-and-other-downloads/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/10-12-1/nRF-Command-Line-Tools_10_12_1_OSX.tar

NRFTOOLS_VER  = 10_12_1
NRFTOOLS_VERH = 10-12-1
NRFTOOLS_ROOT = nRF-Command-Line-Tools_$(NRFTOOLS_VER)
# The naming and packaging of nRF Command Line Tools files on nRF's website for LINUX 
# and MAC is inconsistent. We're falling back on the OSX distribution for LINUX
# because the OSX distro works on LINUX (tested on Ubuntu 20.04LTS, 64bit).
ifeq ($(OS_NAME),Linux)
	ifeq ($(OS_BITS),64)
		# NRFTOOLS_FN   = $(NRFTOOLS_ROOT)_Linux-amd64.zip
		NRFTOOLS_FN   = $(NRFTOOLS_ROOT)_OSX.tar
	else
		# NRFTOOLS_FN   = $(NRFTOOLS_ROOT)_Linux-i386.zip
		NRFTOOLS_FN   = $(NRFTOOLS_ROOT)_OSX.tar
	endif
endif
ifeq ($(OS_NAME),Darwin)
	NRFTOOLS_FN   = $(NRFTOOLS_ROOT)_OSX.tar
endif
NRFTOOLS_URL  = https://www.nordicsemi.com/-/media/Software-and-other-downloads/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/$(NRFTOOLS_VERH)/$(NRFTOOLS_FN)
NRFTOOLS_FP   = $(WORK_DIR)/$(NRFTOOLS_FN)
NRFTOOLS_IN   = $(WORK_DIR)/$(NRFTOOLS_ROOT).tar

NRFTOOLS_DIR  = $(DIST_DIR)/nrftools

$(DIST_DIR)/nrftools:
	$(Q) -mkdir -p $(WORK_DIR)
	@echo [Downloading nrftools: $(NRFTOOLS_URL)]
	$(Q) curl -L $(NRFTOOLS_URL) -o $(NRFTOOLS_FP)
	@echo [Processing archive: $(NRFTOOLS_FN)]
	$(Q) tar -xf $(NRFTOOLS_FP) -C $(WORK_DIR)
	$(Q) tar -xf $(NRFTOOLS_IN) -C $(WORK_DIR)
	$(Q) -mkdir -p $@
	$(Q) mv $(WORK_DIR)/mergehex $(WORK_DIR)/nrfjprog $@
	$(Q) -rm -r $(WORK_DIR)
	@echo [Successfully unpacked: $@]

# provide a top level rule
.PHONY: nrftools
nrftools: $(DIST_DIR)/nrftools

# Add nrfjprog to the path
export PATH := $(CURDIR)/$(DIST_DIR)/nrftools/nrfjprog:$(PATH)
# Add mergehex to the path
export PATH := $(CURDIR)/$(DIST_DIR)/nrftools/mergehex:$(PATH)

################################################################################

# https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads
ARMGCC_ROOT = gcc-arm-none-eabi-8-2019-q3-update
ifeq ($(OS_NAME),Linux)
	ARMGCC_FN  = $(ARMGCC_ROOT)-linux.tar.bz2 
endif
ifeq ($(OS_NAME),Darwin)
	ARMGCC_FN  = $(ARMGCC_ROOT)-mac.tar.bz2
endif
ARMGCC_URL = https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/8-2019q3/RC1.1/$(ARMGCC_FN)
ARMGCC_FP  = $(WORK_DIR)/$(ARMGCC_FN)

$(DIST_DIR)/gcc:
	$(Q) -mkdir $(WORK_DIR)
	@echo [Downloading armgcc: $(ARMGCC_URL)]
	$(Q) curl -L $(ARMGCC_URL) -o $(ARMGCC_FP)
	@echo [Processing archive: $(ARMGCC_FN)]
	$(Q) tar -xjf $(ARMGCC_FP) -C $(WORK_DIR)
	$(Q) -mkdir -p $@
	$(Q) mv $(WORK_DIR)/$(ARMGCC_ROOT)/* $@
	$(Q) -rm -r $(WORK_DIR)
	@echo [Successfully unpacked: $@]

.PHONY: gcc
gcc: $(DIST_DIR)/gcc

ARMGCC_BIN = $(DIST_DIR)/gcc/bin

################################################################################

.PHONY: dist
dist: gcc nrftools

distclean: 
	-rm -rf $(WORK_DIR)
	-rm -rf $(DIST_DIR)

