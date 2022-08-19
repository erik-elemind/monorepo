/*
 * data_log_eeg_comp.h
 *
 * Copyright (C) 2022 Elemind Technologies, Inc.
 *
 * Created: Apr, 2022
 * Author:  Bradey Honsinger
 *
 * Description: Data log EEG compression support.
 */

#ifndef DATA_LOG_EEG_COMP_H
#define DATA_LOG_EEG_COMP_H

#include "data_log_internal.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))

bool compress_log_eeg(const char* log_filename_to_compress);

#endif // (defined(ENABLE_OFFLINE_EEG_COMPRESSION) && (ENABLE_OFFLINE_EEG_COMPRESSION > 0U))

#ifdef __cplusplus
}
#endif


#endif  // DATA_LOG_EEG_COMP_H
