/*
 * settings.c
 *
 *  Created on: Feb 22, 2023
 *      Author: tyler
 */
#include "settings.h"

#define SETTINGS_KEY_PATH_SELECT			"bgwav.path.select"
#define SETTINGS_KEY_PATH_DEFAULT			"bgwav.path.default"
#define SETTINGS_KEY_PATH_0					"bgwav.path.0"
#define SETTINGS_KEY_PATH_1					"bgwav.path.1"
#define SETTINGS_KEY_AUDIO_VOLUME			"audio.volume"
#define SETTINGS_KEY_DATALOG_UID			"datalog.uid"
#define SETTINGS_KEY_ALARM_FLAGS			"alarm-flags"
#define SETTINGS_KEY_ALARM_MINUTES			"alarm-minutes"
#define SETTINGS_KEY_ALARM_PATH_SELECT		"alarm.path.select"
#define SETTINGS_KEY_ALARM_PATH_DEFAULT		"alarm.path.default"
#define SETTINGS_KEY_ALARM_PATH_0			"alarm.path.0"
#define SETTINGS_KEY_ALARM_VOLUME			"alarm.volume"
#define SETTINGS_KEY_DEEP_SLEEP				"app-settings.deep-sleep"
#define SETTINGS_KEY_SLEEP_TAILOR			"app-settings.sleep-tailor"
#define SETTINGS_KEY_RESTART_AUDIO_ON_WAKE	"app-settings.restart-audio"
#define SETTINGS_KEY_DATA_COLLECTION		"app-settings.data-collection"

#define DEFAULT_PATH_SELECT					"0"
#define DEFAULT_PATH_DEFAULT				"/audio/RAIN_22M.wav"
#define DEFAULT_PATH_0						"/audio/RAIN_22M.wav"
#define DEFAULT_PATH_1						"/audio/WATERFALL_22M.wav"
#define DEFAULT_AUDIO_VOLUME				"60"
#define DEFAULT_DATALOG_UID					"0"
#define DEFAULT_ALARM_FLAGS					0
#define DEFAULT_ALARM_MINUTES				0
#define DEFAULT_ALARM_PATH_SELECT			"0"
#define DEFAULT_ALARM_PATH_DEFAULT			"/audio/ALARM_DEFAULT.wav"
#define DEFAULT_ALARM_PATH_0				"/audio/ALARM_DEFAULT.wav"
#define DEFAULT_ALARM_VOLUME				"128"
#define DEFAULT_DEEP_SLEEP					0
#define DEFAULT_SLEEP_TAILOR				0
#define DEFAULT_RESTART_AUDIO_ON_WAKE		0
#define DEFAULT_DATA_COLLECTION				1

// Needed to translate BLE characteristic values from app settings
#define SETTINGS_DEEP_SLEEP_BIT_OFFSET				0
#define SETTINGS_SLEEP_TAILOR_BIT_OFFSET			1
#define SETTINGS_RESTART_AUDIO_ON_WAKE_BIT_OFFSET	2
#define SETTINGS_DATA_COLLECTION_BIT_OFFSET		    3

// Set all the settings to their default values
settings_ret_t reset_default_settings(void)
{
	if(settings_set_string(SETTINGS_KEY_PATH_SELECT, DEFAULT_PATH_SELECT) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_string(SETTINGS_KEY_PATH_DEFAULT, DEFAULT_PATH_DEFAULT) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_string(SETTINGS_KEY_PATH_0, DEFAULT_PATH_0) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_string(SETTINGS_KEY_PATH_1, DEFAULT_PATH_1) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_string(SETTINGS_KEY_AUDIO_VOLUME, DEFAULT_AUDIO_VOLUME) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_string(SETTINGS_KEY_DATALOG_UID, DEFAULT_DATALOG_UID) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_long(SETTINGS_KEY_ALARM_FLAGS, DEFAULT_ALARM_FLAGS) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_long(SETTINGS_KEY_ALARM_MINUTES, DEFAULT_ALARM_MINUTES) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_string(SETTINGS_KEY_ALARM_PATH_SELECT, DEFAULT_ALARM_PATH_SELECT) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_string(SETTINGS_KEY_ALARM_PATH_DEFAULT, DEFAULT_ALARM_PATH_DEFAULT) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_string(SETTINGS_KEY_ALARM_PATH_0, DEFAULT_ALARM_PATH_0) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_string(SETTINGS_KEY_ALARM_VOLUME, DEFAULT_ALARM_VOLUME) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_bool(SETTINGS_KEY_DEEP_SLEEP, DEFAULT_DEEP_SLEEP) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_bool(SETTINGS_KEY_SLEEP_TAILOR, DEFAULT_SLEEP_TAILOR) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_bool(SETTINGS_KEY_RESTART_AUDIO_ON_WAKE, DEFAULT_RESTART_AUDIO_ON_WAKE) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_set_bool(SETTINGS_KEY_DATA_COLLECTION, DEFAULT_DATA_COLLECTION) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	return SETTINGS_RESULT_SUCCESS;

}

// Saves app settings to external settings.ini file
settings_ret_t write_app_settings(uint8_t app_settings)
{
	uint8_t temp = 0;

	temp = (app_settings & (1<<SETTINGS_DEEP_SLEEP_BIT_OFFSET)) ? 1 : 0;
	if(settings_set_bool(SETTINGS_KEY_DEEP_SLEEP, temp) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	temp = (app_settings & (1<<SETTINGS_SLEEP_TAILOR_BIT_OFFSET)) ? 1 : 0;
	if(settings_set_bool(SETTINGS_KEY_SLEEP_TAILOR, temp) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	temp = (app_settings & (1<<SETTINGS_RESTART_AUDIO_ON_WAKE_BIT_OFFSET)) ? 1 : 0;
	if(settings_set_bool(SETTINGS_KEY_RESTART_AUDIO_ON_WAKE, temp) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	temp = (app_settings & (1<<SETTINGS_DATA_COLLECTION_BIT_OFFSET)) ? 1 : 0;
	if(settings_set_bool(SETTINGS_KEY_DATA_COLLECTION, temp) < 0)
	{
		return SETTINGS_RESULT_ERROR;
	}

	return SETTINGS_RESULT_SUCCESS;
}

// Reads app settings from external settings.ini file
settings_ret_t read_app_settings(uint8_t* app_settings)
{
	// Read off app settings from the settings.ini file
	bool temp = 0;

	if(settings_get_bool(SETTINGS_KEY_DEEP_SLEEP, &temp) == 0)
	{
		(*app_settings) |= temp<<SETTINGS_DEEP_SLEEP_BIT_OFFSET;
	}
	else
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_get_bool(SETTINGS_KEY_SLEEP_TAILOR, &temp) == 0)
	{
		(*app_settings) |= temp<<SETTINGS_SLEEP_TAILOR_BIT_OFFSET;
	}
	else
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_get_bool(SETTINGS_KEY_RESTART_AUDIO_ON_WAKE, &temp) == 0)
	{
		(*app_settings) |= temp<<SETTINGS_RESTART_AUDIO_ON_WAKE_BIT_OFFSET;
	}
	else
	{
		return SETTINGS_RESULT_ERROR;
	}

	if(settings_get_bool(SETTINGS_KEY_DATA_COLLECTION, &temp) == 0)
	{
		(*app_settings) |= temp<<SETTINGS_DATA_COLLECTION_BIT_OFFSET;
	}
	else
	{
		return SETTINGS_RESULT_ERROR;
	}

	return SETTINGS_RESULT_SUCCESS;
}
