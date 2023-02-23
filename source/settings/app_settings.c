/*
 * app_settings.c
 *
 *  Created on: Feb 22, 2023
 *      Author: tyler
 */
#include "app_settings.h"

#define SETTINGS_KEY_DEEP_SLEEP				"app-settings.deep-sleep"
#define SETTINGS_KEY_SLEEP_TAILOR			"app-settings.sleep-tailor"
#define SETTINGS_KEY_RESTART_AUDIO_ON_WAKE	"app-settings.restart-audio-on-wake"
#define SETTINGS_KEY_DATA_COLLECTION		"app-settings.data-collection"

#define SETTINGS_DEEP_SLEEP_BIT_OFFSET				0
#define SETTINGS_SLEEP_TAILOR_BIT_OFFSET			1
#define SETTINGS_RESTART_AUDIO_ON_WAKE_BIT_OFFSET	2
#define SETTINGS_DATA_COLLECTION_BIT_OFFSET		    3

// Saves app settings to external settings.ini file
uint8_t save_app_settings(uint8_t app_settings)
{
	uint8_t temp = 0;

	temp = (app_settings & (1<<SETTINGS_DEEP_SLEEP_BIT_OFFSET)) ? 1 : 0;
	if(settings_set_bool(SETTINGS_KEY_DEEP_SLEEP, temp) < 0)
	{
		return APP_SETTINGS_RESULT_ERROR;
	}

	temp = (app_settings & (1<<SETTINGS_SLEEP_TAILOR_BIT_OFFSET)) ? 1 : 0;
	if(settings_set_bool(SETTINGS_KEY_SLEEP_TAILOR, temp) < 0)
	{
		return APP_SETTINGS_RESULT_ERROR;
	}

	temp = (app_settings & (1<<SETTINGS_RESTART_AUDIO_ON_WAKE_BIT_OFFSET)) ? 1 : 0;
	if(settings_set_bool(SETTINGS_KEY_RESTART_AUDIO_ON_WAKE, temp) < 0)
	{
		return APP_SETTINGS_RESULT_ERROR;
	}

	temp = (app_settings & (1<<SETTINGS_DATA_COLLECTION_BIT_OFFSET)) ? 1 : 0;
	if(settings_set_bool(SETTINGS_KEY_DATA_COLLECTION, temp) < 0)
	{
		return APP_SETTINGS_RESULT_ERROR;
	}

	return APP_SETTINGS_RESULT_SUCCESS;
}

// Reads app settings from external settings.ini file
uint8_t read_app_settings(uint8_t* app_settings)
{
	// Read off app settings from the settings.ini file
	bool temp = 0;

	if(settings_get_bool(SETTINGS_KEY_DEEP_SLEEP, &temp) == 0)
	{
		(*app_settings) |= temp<<SETTINGS_DEEP_SLEEP_BIT_OFFSET;
	}
	else
	{
		return APP_SETTINGS_RESULT_ERROR;
	}

	if(settings_get_bool(SETTINGS_KEY_SLEEP_TAILOR, &temp) == 0)
	{
		(*app_settings) |= temp<<SETTINGS_SLEEP_TAILOR_BIT_OFFSET;
	}
	else
	{
		return APP_SETTINGS_RESULT_ERROR;
	}

	if(settings_get_bool(SETTINGS_KEY_RESTART_AUDIO_ON_WAKE, &temp) == 0)
	{
		(*app_settings) |= temp<<SETTINGS_RESTART_AUDIO_ON_WAKE_BIT_OFFSET;
	}
	else
	{
		return APP_SETTINGS_RESULT_ERROR;
	}

	if(settings_get_bool(SETTINGS_KEY_DATA_COLLECTION, &temp) == 0)
	{
		(*app_settings) |= temp<<SETTINGS_DATA_COLLECTION_BIT_OFFSET;
	}
	else
	{
		return APP_SETTINGS_RESULT_ERROR;
	}

	return APP_SETTINGS_RESULT_SUCCESS;
}
