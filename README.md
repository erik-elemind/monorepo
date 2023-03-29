# Morpheus NRF

## Prerequisites 
[WSL](https://learn.microsoft.com/en-us/windows/wsl/install) (for Windows) and Make is needed to build the firmware.

A JLink is needed for wired flashing (optional). Please download and install the 
[Segger Software and Documentation pack](https://www.segger.com/downloads/jlink/#J-LinkSoftwareAndDocumentationPack).


### OS Setup
Install WSL in order to set up a Linux environment on Windows.

Once WSL is installed, open a Ubuntu terminal (may require administrative rights) and input the following:
The commands are adapted from the Dockerfile within this repo. Slight variations may be needed

If on Mac/Linux, follow the same commands but ignore WSL-references

#### Install standard tools for build system
- `sudo apt-get install update && apt update`
- `sudo apt-get install -y wget`
- `sudo apt-get install -y git`
- `sudo apt-get install -y make`
- `sudo apt install -y curl`
- `sudo apt install -y zip`
- `sudo rm /var/lib/apt/lists/*`

At this point, we can clone this repo into our WSL. Follow [Github's instructions](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent) for setting up the SSH Keys. 

#### Install Conda
- `sudo wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh`
- `sudo mkdir /root/.conda`
- `sudo bash Miniconda3-latest-Linux-x86_64.sh`
- `sudo conda --version`
- `sudo rm -f Miniconda3-latest-Linux-x86_64.sh`
- `sudo conda init bash`
- `sudo conda config --set auto_activate_base false`

#### Create Conda Environment
Enter cloned repo in WSL and run
- `conda env create -n nrf_env --file scripts/environment.yml`
This should only be run once unless the environment.yml is changed

The YAML file installs build tool dependencies into `nrf_env` allowing for a consistent build environment among developers.

#### Run Conda Environment
Once the `nrf_env` conda environment has been created, activate the environment:
- `conda activate nrf_env`
- `conda deactivate`

#### Secret Keys
In order to generate the .zip files, contact :( . 

#### VS Code
To directly edit the cloned repo in WSL via VS Code, following the [instructions](https://learn.microsoft.com/en-us/windows/wsl/tutorials/wsl-vscode).

1. Install VS Code
2. Install WSL extension (may need to uninstall CMake extensions)
3. In Ubuntu terminal (cloned repo): `code .` will open a VS Code window for the WSL cloned repo to edit.

<!-- ### Compiler & tools -->
<!-- `make` checks for the ARMGCC to exist in `$PATH`. If not, it can be downloaded 
via `make dist`. Currently, we use version 2019-q3, which is available from the [ARM developer site](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads).

`make dist` also downloads [nrfjprog and mergehex](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools) which are used in the build. -->

## Build process

To build the main NRF application firmware, run the following command.
```
make app
```

<!-- To flash the firmware via JLink, add `F=1` to the command line:
```
make app F=1
``` -->

To specify a particular board, add the `BOARD` option:
```
make app BOARD=ff4 F=1
```

To build the bootloader, run the following (`F=1` also works here).
```
make bootloader
```

To clean the build:
```
make clean
```

<!-- To remove the downloaded tools in the `dist` directory:
```
make distclean
``` -->

More options can be found in `make help`.

<!-- ## Generating LPC OTA packages -->
<!-- To generate an LPC firmware package, run the `lpcpkg.sh` script

First step is to generate a hex file from MCUxpresso. In the project explorer, expand "Binaries", and right click morpheus_firmware.axf. Then select "Binary Utilities" and "Create hex". 

Then provide the path to the LPC hex file to the script, like so:

```
./lpcpkg.sh -f ../../images/morpheus_firmware.hex
``` -->

## External source: NRF SDK

This is an unzipped copy of the Nordic nRF5 SDK from here:

https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v16.x.x/nRF5_SDK_16.0.0_98a08e2.zip

The ./examples/ directory has been pruned to reduce download size, but several
relevant examples were kept in git for easy reference. A complete
list of all the examples from the SDK is in ./examples/README.md.
Feel free to add or remove code from ./examples/, it is just SDK example code.

The ./source_code/ directory contains all custom or modified code. It is 
templated from the apps in the ./examples/ directory, and uses the same
libraries and board configuration system.

### sdk_config.h and custom_board.h

The Nordic sdk_config.h file is a large, twelve-thousand line monolithic 
config file that is kept for each application. The morpheus sdk_config.h file
for the main firmware is here:

    ./nrf/source_code/app/config/sdk_config.h

The sdk_config.h configures app-specific build time settings for the included 
libraries under ./components, ./modules, ./external, etc. It uses #defines.
This includes both software-specific options like

    APP_TIMER_CONFIG_INITIAL_LOG_LEVEL, LED_SOFTBLINK_ENABLED, PDM_CONFIG_DEBUG_COLOR

and also hardware-specific options like

    NRFX_UART_ENABLED, NRFX_QSPI_CONFIG_FREQUENCY, HCI_UART_RX_PIN, etc.

There are also default sdk_config.h files provided for each devkit board under 
./config/BOARDNAME/. These files have most #defines set to zero but can be used 
as a starting point to get the libraries to compile without complaining about 
missing #defines. (It is probably easier to start with the sdk_config.h file 
from a working example app.) Just know that the files under ./config/BOARDNAME/
are not used by the build system, they are empty starting templates.

Next, note that the Nordic SDK code conflates the chip name (pca10056) with the
devkit board name (nrf52840) in the support libraries and build config. 
The ./pca10056/ subdirectory uses nrf52840_xxaa as the build target name, 
but defines BOARD_PCA10056, and includes these files in the build:

    ./modules/nrfx/mdk/nrf52840.h
    ./modules/nrfx/mdk/system_nrf52840.c
    ./components/boards/pca10056.h

Creating a new custom board name to replace the name nrf52840 in the build 
would require editing the SDK .h and .c files directly, which is not 
recommended. Instead, it is recommended to keep the board name from Nordic 
(nrf52840) that goes with the associated chip name (pca10056).

Finally, the define BOARD_CUSTOM will cause the build to use the file
custom_board.h (instead of ./components/boards/pca10056.h). This allows us 
to use our own edited file for some of the pin configuration:

    ./source_code/app/config/custom_board.h

However, the board file is only used by the Nordic SDK to configure
about a hundred high-level pin names, for buttons, LEDs, QSPI, UART, and
the Arduino example's pins. Some of the names here come from the example apps 
and some are from the bsp_ library, and the names are not consistent. But this 
file can also be used for any custom app defines. 

## External source: Micro-ECC

Micro-ECC is used for crypto operations. This repo contains a checked-in snapshot
of the micro-ecc repo: 

https://github.com/kmackay/micro-ecc/tree/24c60e243580c7868f4334a1ba3123481fe1aa48
