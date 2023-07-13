/*
 * compression_config.h
 *
 *  Created on: Mar 13, 2022
 *      Author: DavidWang
 */

#ifndef EEG_COMPRESSION_ALGO_COMPRESSION_CONFIG_H_
#define EEG_COMPRESSION_ALGO_COMPRESSION_CONFIG_H_

#include <float.h>
#include "config.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

// Set the floating point type to be used for compression calculations
// "float" or "double"
// TODO: Unify these FLOAT defintions
#ifndef CMPR_FLOAT
#define CMPR_FLOAT float
#endif

#ifndef CMPR_FLOAT_NEG_MAX
#define CMPR_FLOAT_NEG_MAX (-FLT_MAX)
#endif

#ifndef CMPR_FLOAT_POS_MAX
#define CMPR_FLOAT_POS_MAX (FLT_MAX)
#endif


// Enable static allocation of variables internal to compression functions
// 0U - do NOT use static allocation, use the stack of the calling task.
// 1U - USE static allocation, instead of stack.
#ifndef ENABLE_CMPR_STATIC
#define ENABLE_CMPR_STATIC (0U)
#endif

// Enable compression algorithm output to debug UART.
// 0U - disable debug output
// 1U - enable debug output
#ifndef ENABLE_CMPR_DEBUG_UART_OUTPUT
#define ENABLE_CMPR_DEBUG_UART_OUTPUT (0U)
#endif




#if (defined(ENABLE_CMPR_DEBUG_UART_OUTPUT) && (ENABLE_CMPR_DEBUG_UART_OUTPUT > 0U))
#define CMPR_LOG_PUTS(X) debug_uart_puts_nnl(X)
#define CMPR_LOG_PUTI(X) debug_uart_puti_nnl(X)
#else
#define CMPR_LOG_PUTS(X)
#define CMPR_LOG_PUTI(X)
#endif

#ifndef CMPR_STATIC
#define CMPR_STATIC
#else
#if (defined(ENABLE_CMPR_STATIC) && (ENABLE_CMPR_STATIC > 0U))
#define CMPR_STATIC static
#endif
#endif



#ifdef __cplusplus
}
#endif



#endif /* EEG_COMPRESSION_ALGO_COMPRESSION_CONFIG_H_ */
