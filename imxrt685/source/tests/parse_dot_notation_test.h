/*
 * parse_dot_notation_test.h
 *
 *  Created on: May 22, 2022
 *      Author: DavidWang
 */

#ifndef TESTS_PARSE_DOT_NOTATION_TEST_H_
#define TESTS_PARSE_DOT_NOTATION_TEST_H_

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(ENABLE_PARSE_DOT_NOTATION_TEST) && (ENABLE_PARSE_DOT_NOTATION_TEST > 0U))
void test_parse_dot_notation();
#endif

#ifdef __cplusplus
}
#endif

#endif /* TESTS_PARSE_DOT_NOTATION_TEST_H_ */
