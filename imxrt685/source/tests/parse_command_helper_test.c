/*
 * parse_command_helper_test.c
 *
 *  Created on: May 22, 2022
 *      Author: DavidWang
 */

#include "parse_command_helper_test.h"
#include "command_parser.h"
#include "utils.h"
#include "loglevels.h"


#if (defined(ENABLE_PARSE_DOUBLE_QUOTE_TEST) && (ENABLE_PARSE_DOUBLE_QUOTE_TEST > 0U))

extern void parse_command_helper(char *buf, size_t buf_size, char **argv, size_t argv_size, int *argc);

void parser_test_helper(char* buf){
  size_t buf_size = strlen(buf);
  char tmp_buf[buf_size+1];
  memcpy(tmp_buf, buf, buf_size);
  tmp_buf[buf_size] = '\0';

  char *argv[ARGC_MAX];
  int argc = 0;

  parse_command_helper(tmp_buf, buf_size, argv, ARGC_MAX, &argc);

  char scratch[256];
  debug_uart_puts("");
  for(int i=0; i<argc; i++){
    snprintf(scratch, sizeof(scratch), "argv[%d] = |%s|", i, argv[i]);
    debug_uart_puts(scratch);
  }
}

void parser_test(){
  parser_test_helper(" asdasdasd");
  parser_test_helper("asdasdasd");
  parser_test_helper("dog hello \\\"1 2 3 \\\" bird");
  parser_test_helper("dog hello \"1 2 3 \" bird");
  parser_test_helper("dog hello \"1 2 3  bird");
}

#endif // (defined(ENABLE_PARSE_DOUBLE_QUOTE_TEST) && (ENABLE_PARSE_DOUBLE_QUOTE_TEST > 0U))
