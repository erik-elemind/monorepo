/*
 * Copyright (C) 2021 Elemind Technologies, Inc.
 *
 * Created: July, 2021
 * Author:  Paul Adelsbach
 *
 * Description: Delay redirection for use when running on a PC host.
 */
#pragma once

#ifndef __arm__
#include <unistd.h>     // for usleep
#define nrf_delay_us(usec)  usleep(usec)
#endif