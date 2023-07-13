/*
 * parse_command_helper_test.h
 *
 *  Created on: May 22, 2022
 *      Author: DavidWang
 */

#ifndef TESTS_PARSE_COMMAND_HELPER_TEST_H_
#define TESTS_PARSE_COMMAND_HELPER_TEST_H_

#include "config.h"
#include "parse_command_helper_test.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(ENABLE_PARSE_DOUBLE_QUOTE_TEST) && (ENABLE_PARSE_DOUBLE_QUOTE_TEST > 0U))
void parser_test();
#endif

#ifdef __cplusplus
}
#endif

#endif /* TESTS_PARSE_COMMAND_HELPER_TEST_H_ */
