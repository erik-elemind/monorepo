/*
 * test_commands.c
 *
 *  Created on: May 22, 2022
 *      Author: DavidWang
 */

#include "test_commands.h"
#include "parse_command_helper_test.h"
#include "parse_dot_notation_test.h"

#if (defined(ENABLE_PARSE_DOUBLE_QUOTE_TEST) && (ENABLE_PARSE_DOUBLE_QUOTE_TEST > 0U))
void parse_double_quote_test_command(int argc, char **argv){
  parser_test();
}
#endif // (defined(ENABLE_PARSE_DOUBLE_QUOTE_TEST) && (ENABLE_PARSE_DOUBLE_QUOTE_TEST > 0U))

#if (defined(ENABLE_PARSE_DOT_NOTATION_TEST) && (ENABLE_PARSE_DOT_NOTATION_TEST > 0U))
void parse_dot_notation_test_command(int argc, char **argv){
  test_parse_dot_notation();
}
#endif // (defined(ENABLE_PARSE_DOT_NOTATION_TEST) && (ENABLE_PARSE_DOT_NOTATION_TEST > 0U))
