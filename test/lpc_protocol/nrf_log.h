/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: July, 2021
 * Author:  Paul Adelsbach
 *
 * Description: Log redirection for use when running on a PC host.
 */
#pragma once

#include <stdio.h>      // for printf

#define LOG_LEVEL_OFF       0
#define LOG_LEVEL_ERROR     1
#define LOG_LEVEL_WARNING   2
#define LOG_LEVEL_INFO      3
#define LOG_LEVEL_DEBUG     4

#ifndef NRF_LOG_LEVEL
#define NRF_LOG_LEVEL   LOG_LEVEL_WARNING
#endif

#define _LOG   printf

#if NRF_LOG_LEVEL >= LOG_LEVEL_ERROR
#define NRF_LOG_ERROR(fmt, ...)    \
    do { \
        _LOG("E:" fmt "\n", ##__VA_ARGS__); \
    } while(0);
#else
#define NRF_LOG_ERROR(fmt, ...)
#endif

#if NRF_LOG_LEVEL >= LOG_LEVEL_WARNING
#define NRF_LOG_WARNING(fmt, ...)    \
    do { \
        _LOG("W:" fmt "\n", ##__VA_ARGS__); \
    } while(0);
#else
#define NRF_LOG_WARNING(fmt, ...)
#endif

#if NRF_LOG_LEVEL >= LOG_LEVEL_INFO
#define NRF_LOG_INFO(fmt, ...)    \
    do { \
        _LOG("I:" fmt "\n", ##__VA_ARGS__); \
    } while(0);
#else
#define NRF_LOG_INFO(fmt, ...)
#endif

#if NRF_LOG_LEVEL >= LOG_LEVEL_DEBUG
#define NRF_LOG_DEBUG(fmt, ...)    \
    do { \
        _LOG("D:" fmt "\n", ##__VA_ARGS__); \
    } while(0);
#else
#define NRF_LOG_DEBUG(fmt, ...)
#endif

#define NRF_LOG_MODULE_REGISTER()