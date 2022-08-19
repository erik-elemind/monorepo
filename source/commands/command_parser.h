/*
 * command_parser.h
 *
 *  Created on: May 19, 2021
 *      Author: DavidWang
 */

#ifndef COMMANDS_COMMAND_PARSER_H_
#define COMMANDS_COMMAND_PARSER_H_

#include <stdbool.h>

#ifndef CMD_BUFFER_LEN
#define CMD_BUFFER_LEN  512 // used to be 1024
#endif

/** Maximum number of arguments allowed (any more won't get parsed). */
#define ARGC_MAX 16

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
  PARSER_COMMAND_NOT_FOUND = -1,
  PARSER_OK = 0,
  PARSER_LINE_FOUND = 1,
  PARSER_LINE_PARSED = 2,
  PARSER_COMMAND_FOUND = 3,
} parser_status_t;

typedef struct {
  char buf[CMD_BUFFER_LEN];
  char bufsaved[CMD_BUFFER_LEN];
  unsigned int index;
  char prompt_char;
  bool use_prompt;
  parser_status_t status;
} command_parser_t;

void prompt(command_parser_t *parser);
void parse_command(command_parser_t *parser);
void handle_input(command_parser_t *parser, char c);

#ifdef __cplusplus
}
#endif

#endif /* COMMANDS_COMMAND_PARSER_H_ */
