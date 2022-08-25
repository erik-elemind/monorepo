#pragma once

#include <minIni.h>
#include <stdint.h>
#include <stdbool.h>

// Specify NULL for section to not require a section label
#define SETTINGS_SECTION  NULL
// Specify the filename to use for settings
#define SETTINGS_FILE     "settings.ini"
// Specify the default numeric value to return
#define SETTINGS_DEF_VALUE  INT32_MAX

// Read a boolean value.
// Returns 0 for success, non-zero for failure.
static inline int settings_get_bool(const char* key, bool* value) {
  int result = ini_getbool(SETTINGS_SECTION, key, SETTINGS_DEF_VALUE, SETTINGS_FILE);
  if (SETTINGS_DEF_VALUE == result) {
    return -1;
  }
  *value = result;
  return 0;
}

// Read a long integer value.
// Returns 0 for success, non-zero for failure.
static inline int settings_get_long(const char* key, long* value) {
  int result = ini_getl(SETTINGS_SECTION, key, SETTINGS_DEF_VALUE, SETTINGS_FILE);
  if (SETTINGS_DEF_VALUE == result) {
    return -1;
  }
  *value = result;
  return 0;
}

// Read a string value.
// Returns 0 for success, non-zero for failure.
static inline int settings_get_string(const char* key, char* value, unsigned len) {
  int result = ini_gets(SETTINGS_SECTION, key, NULL, value, len, SETTINGS_FILE);
  if (0 == result) {
    return -1;
  }
  // setting already copied to `value` from the getstring call
  return 0;
}

// Read a float value.
// Returns 0 for success, non-zero for failure.
static inline int settings_get_float(const char* key, float* value) {
  float result = ini_getf(SETTINGS_SECTION, key, SETTINGS_DEF_VALUE, SETTINGS_FILE);
  if (SETTINGS_DEF_VALUE == result) {
    return -1;
  }
  *value = result;
  return 0;
}

// Read the next key in the settings file.
// Pass 0 for the first index and increment it on each call.
// Returns 0 for success, non-zero for failure (or end of list).
static inline int settings_get_next_key(int idx, char* key, int key_len) {
  return ini_getkey(SETTINGS_SECTION, idx, key, key_len, SETTINGS_FILE) ? 0 : -1;
}

// Set a boolean value.
// Returns 0 for success, non-zero for failure.
static inline int settings_set_bool(const char* key, bool value) {
  return ini_putl(SETTINGS_SECTION, key, value, SETTINGS_FILE) ? 0 : -1;
}

// Set a long integer value.
// Returns 0 for success, non-zero for failure.
static inline int settings_set_long(const char* key, long value) {
  return ini_putl(SETTINGS_SECTION, key, value, SETTINGS_FILE) ? 0 : -1;
}

// Set a string value.
// Returns 0 for success, non-zero for failure.
static inline int settings_set_string(const char* key, const char* value) {
  return ini_puts(SETTINGS_SECTION, key, value, SETTINGS_FILE) ? 0 : -1;
}

// Set a float value.
// Returns 0 for success, non-zero for failure.
static inline int settings_set_float(const char* key, float value) {
  return ini_putf(SETTINGS_SECTION, key, value, SETTINGS_FILE) ? 0 : -1;
}

// Delete a value.
// Returns 0 for success, non-zero for failure.
static inline int settings_delete(const char* key) {
  // Delete the entry by setting a NULL string
  return ini_puts(SETTINGS_SECTION, key, NULL, SETTINGS_FILE) ? 0 : -1;
}
