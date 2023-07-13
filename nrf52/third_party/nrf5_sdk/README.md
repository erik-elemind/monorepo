This is an unzipped copy of the Nordic nRF5 SDK from here:

https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v16.x.x/nRF5_SDK_16.0.0_98a08e2.zip

The ./examples/ directory has been pruned to reduce download size, but several
relevant examples were kept in git for easy reference. A complete
list of all the examples from the SDK is in ./examples/README.md.
Feel free to add or remove code from ./examples/, it is just SDK example code.

The ./source_code/ directory contains all custom or modified code. It is 
templated from the apps in the ./examples/ directory, and uses the same
libraries and board configuration system.

## sdk_config.h and custom_board.h

The Nordic sdk_config.h file is a large, twelve-thousand line monolithic 
config file that is kept for each application. It is kept under:

    APPNAME/CHIPNAME/NORDIC_SOFTDEVICE_VERSION/config/sdk_config.h

The sdk_config.h configures app-specific build time settings for the included 
libraries under ./components, ./modules, ./external, etc. It uses #defines.
This includes both software-specific options like

    APP_TIMER_CONFIG_INITIAL_LOG_LEVEL, LED_SOFTBLINK_ENABLED, PDM_CONFIG_DEBUG_COLOR

and also hardware-specific options like

    NRFX_UART_ENABLED, NRFX_QSPI_CONFIG_FREQUENCY, HCI_UART_RX_PIN, etc.

This is the morpheus sdk_config.h file:

    ./source_code/ble_peripheral/ble_app_morpeus/pca10056/s140/config/sdk_config.h

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

    ./source_code/ble_peripheral/ble_app_morpeus/pca10056/s140/config/custom_board.h

However, the board file is only used by the Nordic SDK to configure
about a hundred high-level pin names, for buttons, LEDs, QSPI, UART, and
the Arduino example's pins. Some of the names here come from the example apps 
and some are from the bsp_ library, and the names are not consistent. But this 
file can also be used for any custom app defines. 


