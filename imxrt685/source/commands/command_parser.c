/*
 * command_parser->c
 *
 *  Created on: May 19, 2021
 *      Author: DavidWang
 */

#include "config.h"
#include "command_parser.h"
#include "command_helpers.h"
#include "commands.h"
#include "app.h"
#include "loglevels.h"

#include "stdio.h"
#include "data_log.h"
#include "utils.h"

void
prompt(command_parser_t *parser)
{
  if(parser->use_prompt){
    putchar(parser->prompt_char);
    putchar(' ');
    fflush(stdout);
  }
}

static size_t
common_prefix_len(char *str1, size_t len1, char *str2, size_t len2) {
  int i;
  for (i = 0; (i < len1) && (i < len2); i++) {
    if (str1[i] != str2[i]) {
      break;
    }
  }

  return i;
}

static void do_command(command_parser_t *parser, int argc, char **argv)
{
  unsigned int i;
  unsigned int nl = strlen(argv[0]);
  unsigned int cl;

  for (i = 0; i < commands_count; i++) {
    cl = strlen(commands[i].name);

    if (cl == nl && commands[i].function != NULL &&
        !strncmp(argv[0], commands[i].name, nl)) {
      parser->status = PARSER_COMMAND_FOUND;
      commands[i].function(argc, argv);
#if defined(ENABLE_DATA_LOG_TASK) && (ENABLE_DATA_LOG_TASK>0)
      // log the command
      data_log_command(parser->bufsaved);
#endif
      if(parser->use_prompt){
        putchar('\n');
      }
      break;
    }
  }

  if (i == commands_count) {
    parser->status = PARSER_COMMAND_NOT_FOUND;
    printf("%s: command not found\n", argv[0]);
  }
}

/*static*/ // static commented out for testing purposes
void parse_command_helper(char *buf, size_t buf_size, char **argv, size_t argv_size, int *argc) {
  unsigned int i = 0;
  char *in_arg = NULL;
  // parsing values for escape and double quotes
  unsigned int dsti = 0;
  uint8_t state = 0;

  // parse the command
  for (i = 0; (i < buf_size) && (*argc < argv_size); i++) {
    char c = buf[i];
    switch (state) {
    case 0:
      if (is_escape(c)) {
        state = 1;
      }
      else if (is_dblquote(c)) {
        state = 2;
      }
      else if (is_whitespace(c)) {
        if (in_arg) {
          buf[dsti++] = '\0';
          in_arg = NULL;
        }
      }
      else {
        if (in_arg) {
          buf[dsti++] = c;
        }
        else {
          in_arg = &buf[dsti];
          argv[*argc] = in_arg;
          (*argc)++;
          buf[dsti++] = c;
        }
      }
      break;

    case 1:
      if (in_arg) {
        buf[dsti++] = c;
      }
      else {
        in_arg = &buf[dsti];
        argv[*argc] = in_arg;
        (*argc)++;
        buf[dsti++] = c;
      }
      state = 0;
      break;

    case 2:
      if (is_escape(c)) {
        state = 3;
      }
      else if (is_dblquote(c)) {
        state = 0;
      }
      else {
        if (in_arg) {
          buf[dsti++] = c;
        }
        else {
          in_arg = &buf[dsti];
          argv[*argc] = in_arg;
          (*argc)++;
          buf[dsti++] = c;
        }
      }
      break;

    case 3:
      buf[dsti++] = c;
      state = 2;
      break;
    }

  }
  buf[dsti] = '\0';
}

void parse_command(command_parser_t *parser) {
  char *argv[ARGC_MAX];
  int argc = 0;

  // copy off the command
  memcpy(parser->bufsaved, parser->buf, sizeof(parser->buf));
  // set the termination on the saved buffer
  parser->bufsaved[sizeof(parser->buf) - 1] = '\0';

  parse_command_helper(parser->buf, parser->index, argv, ARGC_MAX, &argc);

  // execute the command
  if (argc > 0) {
    LOGV("command_parser","command: %s:", parser->bufsaved);
    parser->status = PARSER_LINE_PARSED;
    do_command(parser, argc, argv);
  }
}

static void handle_tab(command_parser_t *parser)
{
  int i;
  unsigned int match_count = 0;
  bool matches[MAX_COMMANDS] = {0};
  const shell_command_t *cmd;
  size_t prefix_len = 0;

  for (i = 0; i < commands_count; i++) {
    if (!strncmp(commands[i].name, parser->buf, parser->index)) {
      match_count++;
      matches[i] = true;
    }
  }

  if (match_count == 1) {
    for (i = 0; i < commands_count; i++) {
      if (matches[i]) {
        cmd = &commands[i];
        memcpy(parser->buf + parser->index, cmd->name + parser->index,
            strlen(cmd->name) - parser->index);
        parser->index += strlen(cmd->name) - parser->index;
      }
    }
  } else if (match_count > 1) {
    for (i = 0, cmd = NULL; i < commands_count; i++) {
      if (matches[i]) {
        printf("\n%s", commands[i].name);

        if (cmd != NULL) {
          prefix_len = common_prefix_len(cmd->name, prefix_len,
            commands[i].name, strlen(commands[i].name));
        }
        else {
          prefix_len = strlen(commands[i].name);
        }
        cmd = &commands[i];
      }
    }

    // Update partial command to longest common prefix
    strncpy(parser->buf, cmd->name, prefix_len);
    parser->buf[prefix_len] = '\0';
    parser->index = strlen(parser->buf);
  }

  if(parser->use_prompt){
    putchar('\n');
    prompt(parser);
    printf("%s", parser->buf);
  }
}

void handle_input(command_parser_t *parser, char c)
{
//  LOGV("ble_shell","received char: %c", c);
  parser->status = PARSER_OK;

  switch (c) {
    case '\r':
    case '\n':
      if(parser->use_prompt){
        putchar('\n');
      }
      if (parser->index > 0) {
        parser->status = PARSER_LINE_FOUND;
        parse_command(parser);
        parser->index = 0;
        memset(parser->buf, 0, sizeof(parser->buf));
      }

      // TODO: should we condition this?
      // Tell app there was activity (even if no/invalid command)
      app_event_shell_activity();

      prompt(parser);
      break;

    case '\b': // backspace
    case '\x7f': // delete
      if (parser->index) {
        parser->buf[parser->index-1] = '\0';
        parser->index--;
        if(parser->use_prompt){
          putchar('\b');
          putchar(' ');
          putchar('\b');
        }
      }
      else {
        if(parser->use_prompt){
          putchar(' ');
          putchar('\b');
        }
      }
      break;

    case '\t':
      handle_tab(parser);
      break;

    default:
      if (parser->index < CMD_BUFFER_LEN - 1) {
        if(parser->use_prompt){
          putchar(c);
        }
        parser->buf[parser->index] = c;
        parser->index++;
      }
  }

}
