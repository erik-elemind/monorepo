/*
 * test_commands.h
 *
 *  Created on: May 22, 2022
 *      Author: DavidWang
 */

#ifndef TESTS_TEST_COMMANDS_H_
#define TESTS_TEST_COMMANDS_H_

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(ENABLE_PARSE_DOUBLE_QUOTE_TEST) && (ENABLE_PARSE_DOUBLE_QUOTE_TEST > 0U))
void parse_double_quote_test_command(int argc, char **argv);
#endif

#if (defined(ENABLE_PARSE_DOT_NOTATION_TEST) && (ENABLE_PARSE_DOT_NOTATION_TEST > 0U))
void parse_dot_notation_test_command(int argc, char **argv);
#endif

#ifdef __cplusplus
}
#endif


#endif /* TESTS_TEST_COMMANDS_H_ */
