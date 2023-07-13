/*
 * command_helpers.c
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Helper functions for debug shell commands.
 *
 */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "command_helpers.h"
#include "settings.h"

bool
parse_uint8_arg(char *command, char *arg, uint8_t *p_value)
{
  return parse_uint8_arg_max(command, arg, UINT8_MAX, p_value);
}

bool
parse_uint8_arg_max(char *command, char *arg, uint8_t max, uint8_t *p_value)
{
  return parse_uint8_arg_min_max(command, arg, 0, max, p_value);
}

bool
parse_uint8_arg_min_max(
  char *command,
  char *arg,
  uint8_t min,
  uint8_t max,
  uint8_t *p_value
  )
{
  char* endptr;
  long raw_value = strtol(arg, &endptr, 0);
  if ((*endptr != '\0') || (raw_value < 0) ||
    (raw_value < min) || (raw_value > max)) {
    printf("Error: Command %s: invalid argument %s "
      "(must be integer between %d and %d)",
      command, arg, min, max);
    return false;
  }
  *p_value = (uint8_t)raw_value;

  return true;
}

bool
parse_uint32_arg(char *command, char *arg, uint32_t *p_value)
{
  return parse_uint32_arg_max(command, arg, UINT32_MAX, p_value);
}

bool
parse_uint32_arg_max(char *command, char *arg, uint32_t max, uint32_t *p_value)
{
  return parse_uint32_arg_min_max(command, arg, 0, max, p_value);
}

bool
parse_uint32_arg_min_max(
  char *command,
  char *arg,
  uint32_t min,
  uint32_t max,
  uint32_t *p_value
  )
{
  char* endptr;
  unsigned long raw_value = strtoul(arg, &endptr, 0);
  if ((*endptr != '\0') || (raw_value < 0) ||
    (raw_value < min) || (raw_value > max)) {
    printf("Error: Command %s: invalid argument %s "
      "(must be integer between %ld and %ld)",
      command, arg, min, max);
    return false;
  }
  *p_value = (uint32_t)raw_value;

  return true;
}



bool
parse_uint64_arg(char *command, char *arg, uint64_t *p_value)
{
  return parse_uint64_arg_max(command, arg, UINT64_MAX, p_value);
}

bool
parse_uint64_arg_max(char *command, char *arg, uint64_t max, uint64_t *p_value)
{
  return parse_uint64_arg_min_max(command, arg, 0, max, p_value);
}

bool
parse_uint64_arg_min_max(
  char *command,
  char *arg,
  uint64_t min,
  uint64_t max,
  uint64_t *p_value
  )
{
  char* endptr;
  unsigned long long raw_value = strtoull(arg, &endptr, 0);
  if ((*endptr != '\0') || (raw_value < 0) ||
    (raw_value < min) || (raw_value > max)) {
    printf("Error: Command %s: invalid argument %s "
      "(must be integer between %llu and %llu)",
      command, arg, min, max);
    return false;
  }
  *p_value = (uint32_t)raw_value;

  return true;
}


bool
parse_double_arg(char *command, char *arg, double *p_value){
  return parse_double_arg_max(command, arg, DBL_MAX, p_value);
}

bool
parse_double_arg_max(char *command, char *arg, double max, double *p_value){
  return parse_double_arg_min_max(command, arg, 0, max, p_value);
}

bool
parse_double_arg_min_max(
  char *command,
  char *arg,
  double min,
  double max,
  double *p_value
  ){
  char* endptr;
  double raw_value = strtod(arg, &endptr);
  if ((*endptr != '\0') || (raw_value < 0) ||
    (raw_value < min) || (raw_value > max)) {
    printf("Error: Command %s: invalid argument %s "
      "(must be double between %lf and %lf)",
      command, arg, min, max);
    return false;
  }
  *p_value = (double)raw_value;

  return true;
}


typedef enum {
  PARSE_DOT_STATE_NO_TOKEN = 0,
  PARSE_DOT_STATE_IN_TOKEN_NONWS,
  PARSE_DOT_STATE_IN_TOKEN_WS
} parse_dot_state_t;

/*
 * Parses dot notation, given:
 * str - a pointer to a char array holding a c-string,
 * str_len - the length of the string minus the terminating null-character
 * tokens - a pointer to a token_t data structure to store the token information into.
 *
 * Examples:
 * "a.b.c" -> 3 tokens: [a][b][c]
 * ".b.c" -> 3 tokens: NULL[b][c]
 * "a.b." -> 2 tokens: [a][b]
 * "a..c" -> 3 tokens: [a]NULL[c]
 * ".b. "  -> NULL[b]
 * ".b. c  "  -> NULL[b][c]
 * " a b " -> [a b]
 * "a " -> [a]
 */
void parse_dot_notation(char *str, size_t str_len, token_t* tokens)
{
  unsigned int i;
  char* token_start = NULL;
  char* ws_start = NULL;
  parse_dot_state_t state = PARSE_DOT_STATE_NO_TOKEN;

  // initialize tokens
  tokens->c = 0;

  // parse the command
  for (i = 0; (i < str_len) && (tokens->c < TOKENC_MAX); i++) {

    char c = str[i];
    if(c=='\0'){
      break;
    }

    // turn the crank of the state machine
    switch(state){
    case PARSE_DOT_STATE_NO_TOKEN:
      if(is_dot(c)){
        // DOT = terminate & save token
        // null terminate
        str[i] = '\0';
        // save token
        tokens->v[tokens->c] = token_start; // should always be NULL
        tokens->c++;
        token_start = NULL;
      }else if(is_whitespace(c)){
        // WHITESPACE = do nothing
      }else{
        // NON-WHITESPACE = save start ptr & goto IN_TOKEN_NONWS
        // save start ptr
        token_start = &str[i];
        // goto IN_TOKEN_NONWS
        state = PARSE_DOT_STATE_IN_TOKEN_NONWS;
      }
      break;
    case PARSE_DOT_STATE_IN_TOKEN_NONWS:
      if(is_dot(c)){
        // DOT = terminate & save token & goto NO_TOKEN
        // null terminate
        str[i] = '\0'; // null terminate
        // save token
        tokens->v[tokens->c] = token_start;
        tokens->c++;
        token_start = NULL;
        // goto NO_TOKEN
        state = PARSE_DOT_STATE_NO_TOKEN;
      }else if(is_whitespace(c)){
        // WHITESPACE = goto IN_TOKEN_WS
        state = PARSE_DOT_STATE_IN_TOKEN_WS;
        // save white space ptr
        ws_start = &str[i];
      }else{
        // NON-WHITESPACE = do nothing
      }
      break;
    case PARSE_DOT_STATE_IN_TOKEN_WS:
      if(is_dot(c)){
        // DOT = terminate & save token & goto NO_TOKEN
        // null terminate to previous white space start
        for(char* p=ws_start; p!=NULL && p<=&str[i]; p++){
          *p = '\0';
        }
        // save token
        tokens->v[tokens->c] = token_start;
        tokens->c++;
        token_start = NULL;
        // goto NO_TOKEN
        state = PARSE_DOT_STATE_NO_TOKEN;
      }else if(is_whitespace(c)){
        // WHITESPACE = do nothing
      }else{
        // NON-WHITESPACE = reset white space ptr & goto IN_TOKEN_NONWS
        // reset white space ptr
        ws_start = NULL;
        // goto IN_TOKEN_NONWS
        state = PARSE_DOT_STATE_IN_TOKEN_NONWS;
      }
      break;
    }
  }

  // end a started token
  if (token_start != NULL) {
    // terminate & save token
    // null terminate to previous white space start
    for(char* p=ws_start; p!=NULL && p<=&str[i]; p++){
      *p = '\0';
    }
    // save token
    if(tokens->c < TOKENC_MAX){
      tokens->v[tokens->c] = token_start;
      (tokens->c)++;
    }
  }
}

#define CONSUME_WHITESPACE(token_p) \
  while ( is_whitespace(token_p[0]) ) { \
    token_p++; \
  }

typedef enum {
  SOURCE_UNDEFINED = 0,
  SOURCE_SETTINGS,
} parse_var_source_state_t;

/*
 * Parse a variable name and return the string representation of its value.
 * 0  = value contains a copy of arg
 * 1  = value contains a copy of the value retrieved from settings.ini by using X as the key, where X is from parsing arg V.X.
 * -1 = value contains a copy of X, where X is from parsing arg V.X.
 */
parse_variable_return_t parse_variable_from_token(token_t* tokens, char *value, size_t value_size){
  parse_var_source_state_t source = SOURCE_UNDEFINED;
  char* token = NULL;

  // If there are no tokens, do nothing
  if(tokens->c == 0){
    return PARSE_VAR_RETURN_NOTHING;
  }

  // If there 1 token, return nothing if token is NULL
  if(tokens->c == 1){
    if(tokens->v[0] == NULL){
      return PARSE_VAR_RETURN_NOTHING;
    }else{
      goto return_arg;
    }
  }

  // If we get here, there are 2 or more tokens

  // check the first token
  token = tokens->v[0];
  if(token==NULL || strlen(token) != 1){
    goto return_arg;
  }
  switch(token[0]){
  case 'S': // setting file
    source = SOURCE_SETTINGS;
    break;
  // ADD ADDITIONAL CASES HERE
  default:
    goto return_arg;
  }

  // check the second token
  token = tokens->v[1];
  if(token == NULL){
    goto return_arg;
  }
  switch(source){
  case SOURCE_SETTINGS:
  {
    if (0 == settings_get_string(token, value, value_size)) {
      return PARSE_VAR_RETURN_SETTINGS_VALUE;
    }else{
      strncpy(value, token, value_size);
      value[value_size-1] = '\0';
      return PARSE_VAR_RETURN_SETTINGS_KEY;
    }
  }
  default:
    goto return_arg;
  }

return_arg:
  // The first token exists, return it as the value.
  strncpy(value, tokens->v[0], value_size);
  value[value_size-1] = '\0';
  return PARSE_VAR_RETURN_ARG;
}

parse_variable_return_t parse_variable_from_string(char* arg, char *value, size_t value_size){
  parse_var_source_state_t source = SOURCE_UNDEFINED;
  char* token_p = arg;

  if(token_p == NULL){
    return PARSE_VAR_RETURN_NOTHING;
  }

  CONSUME_WHITESPACE(token_p);

  // check first token
  switch(token_p[0]){
  case 'S': // setting file
    source = SOURCE_SETTINGS;
    token_p++;
    break;
  // ADD ADDITIONAL CASES HERE
  default:
    goto return_arg;
  }

  CONSUME_WHITESPACE(token_p);

  // consume dot
  if(token_p[0] == '.'){
    token_p++;
  }else{
    goto return_arg;
  }

  // get settings
  switch(source){
  case SOURCE_SETTINGS:
  {
    if (0 == settings_get_string(token_p, value, value_size)) {
      return PARSE_VAR_RETURN_SETTINGS_VALUE;
    }else{
      strncpy(value, token_p, value_size);
      value[value_size-1] = '\0';
      return PARSE_VAR_RETURN_SETTINGS_KEY;
    }
  }
  default:
    goto return_arg;
  }

return_arg:
  // arg is not null, return it as the value.
  strncpy(value, token_p, value_size);
  value[value_size-1] = '\0';
  return PARSE_VAR_RETURN_ARG;
}

/** Hex dump.

   From https://gist.github.com/ccbrown/9722406, with
   improvements. Used under WTFPL (public domain).

   Modified by David Wang - To allow printing whole rows
   of 16 bytes only if that row intersects with the range
   start_offset to end_offset.
 */
void
hex_dump(const uint8_t* data, size_t end_offset, size_t start_offset) {
  char ascii[17] = {'\0'};

  if((end_offset-start_offset) == 0){
	  return;
  }

  printf("%04x: ", start_offset);
  for (size_t i = start_offset; i < end_offset; ++i) {
    printf("%02X ", data[i]);
    if (isprint(data[i])) {
      ascii[i % 16] = data[i];
    }
    else {
      ascii[i % 16] = '.';
    }
    if ((i+1) % 8 == 0 || i+1 == end_offset) {
      printf(" ");
      if ((i+1) % 16 == 0 && (i+1) < end_offset) {
        printf("|  %s \n", ascii);
        printf("%04x: ", i+1);
      }
      else if (i+1 == end_offset) {
        ascii[(i+1) % 16] = '\0';
        if ((i+1) % 16 <= 8) {
          printf(" ");
        }
        for (size_t j = (i+1) % 16; j < 16; ++j) {
          printf("   ");
        }
        printf("|  %s ", ascii);
      }
    }
  }
  printf("\n");
}
