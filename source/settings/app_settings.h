/*
 * app_settings.h
 *
 *  Created on: Feb 22, 2023
 *      Author: Tyler Gage
 */

#ifndef APP_SETTINGS_H_
#define APP_SETTINGS_H_

#include <stdint.h>
#include <stdbool.h>
#include "settings.h"

#define APP_SETTINGS_RESULT_SUCCESS 0
#define APP_SETTINGS_RESULT_ERROR 	1

// Saves app settings to external settings.ini file
uint8_t save_app_settings(uint8_t app_settings);

// Reads app settings from external settings.ini file
uint8_t read_app_settings(uint8_t* app_settings);

#endif /* APP_SETTINGS_H_ */
