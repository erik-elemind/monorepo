/*
 * amp.c
 *
 *  Created on: Jan 4, 2023
 *      Author: tyler
 */
#include "amp.h"

// Function pointers for driver interface
typedef void (*fpAmpInit)(i2c_rtos_handle_t* i2c_handle);
typedef void (*fpAmpConfig)();
typedef void (*fpAmpMute)(bool mute);
typedef void (*fpAmpSetVolume)(uint8_t volume);
typedef status_t (*fpAmpPrintDetailedStatus)();

// Driver interface
typedef struct
{
	fpAmpInit init;
	fpAmpConfig config;
	fpAmpMute mute;
	fpAmpSetVolume setVolume;
	fpAmpPrintDetailedStatus printDetailedStatus;
} ampDriver;

static ampDriver amp;

void amp_init(i2c_rtos_handle_t *i2c_handle)
{
#if(USE_SSM2518)
	amp.init = &SSM2518_Init;
	amp.config  = &SSM2518_Config;
	amp.mute = &SSM2518_Mute;
	amp.setVolume = &SSM2518_SetVolume;
	amp.printDetailedStatus = &SSM2518_print_detailed_status;
#elif(USE_SSM2529)
	amp.init = &SSM2529_Init;
	amp.config  = &SSM2529_Config;
	amp.mute = &SSM2529_Mute;
	amp.setVolume = &SSM2529_SetVolume;
	amp.printDetailedStatus = &SSM2529_print_detailed_status;
#else
	// ToDo: Assert
#endif

	amp.init(i2c_handle);

	// setup shutdown pin
	gpio_pin_config_t SSM2518_SHTDNn_config = {
	    .pinDirection = kGPIO_DigitalOutput,
	    .outputLogic = 0U
	};
	GPIO_PinInit(
	    BOARD_INITPINS_SSM2518_SHTDNn_GPIO,
	    BOARD_INITPINS_SSM2518_SHTDNn_PORT,
	    BOARD_INITPINS_SSM2518_SHTDNn_PIN,
	    &SSM2518_SHTDNn_config);
}

void amp_config(){
	amp.config();
}

void amp_mute(bool mute)
{
	amp.mute(mute);
}

void amp_set_volume(uint8_t volume)
{
	amp.setVolume(volume);
}

void amp_power(bool on){
    GPIO_PinWrite(
        BOARD_INITPINS_SSM2518_SHTDNn_GPIO,
        BOARD_INITPINS_SSM2518_SHTDNn_PORT,
        BOARD_INITPINS_SSM2518_SHTDNn_PIN,
        on);
}

status_t amp_print_detailed_status(){
	return amp.printDetailedStatus();
}
