/*
 * data_log_parse.h
 *
 * Copyright (C) 2022 Elemind Technologies, Inc.
 *
 * Created: Apr, 2022
 * Author:  Paul Adelsbach
 *
 * Description: Data log parsing (supports post-processing EEG
 * compression).
 */

#ifndef DATA_LOG_PARSE_H
#define DATA_LOG_PARSE_H

#include "data_log_packet.h" // needed for data_log_packet_t
#include "config.h"

#if (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))

// Callback function that will be called with COBS packet.
typedef void (*data_log_parse_cb_t)(const uint8_t* buf, size_t bufsz);

// Parse a data log file at the given path, and call the given
// function for each packet.
bool data_log_parse(const char* filename, data_log_parse_cb_t packet_callback);

// Stop an ongoing parse.
// This function is safe to call from other threads, even if the parser 
// is not running.
void data_log_parse_stop(void);

// Get the stats.
// This function is safe to call from other threads, and may be called while
// the parser is running.
void data_log_parse_stats(size_t* bytes_in_file, size_t* bytes_processed);

#endif // (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))

#endif  // DATA_LOG_PARSE_H
