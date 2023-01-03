/*
 * amp.c
 *
 *  Created on: Jan 4, 2023
 *      Author: tyler
 */
#include "amp.h"

// Function pointers for driver interface
typedef void (*fpAmpInit)(i2c_rtos_handle_t* i2c_handle);
typedef void (*fpAmpMute)(bool mute);
typedef void (*fpAmpSetVolume)(uint8_t volume);

// Driver interface
typedef struct
{
	fpAmpInit init;
	fpAmpMute mute;
	fpAmpSetVolume setVolume;
} ampDriver;

static ampDriver amp;


void amp_init(i2c_rtos_handle_t *i2c_handle)
{
#if(USE_SSM2518)
	amp.init  = &SSM2518_Init;
	amp.mute = &SSM2518_Mute;
	amp.setVolume = &SSM2518_SetVolume;
#elif(USE_SSM2529)
	amp.init  = &SSM2529_Init;
	amp.mute = &SSM2529_Mute;
	amp.setVolume = &SSM2529_SetVolume;
#else
	// ToDo: Assert
#endif

	amp.init(i2c_handle);
}

void amp_mute(bool mute)
{
	amp.mute(mute);
}

void amp_set_volume(uint8_t volume)
{
	amp.setVolume(volume);
}
