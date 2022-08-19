/*
 * command_helpers.h
 *
 * Copyright (C) 2020 Elemind Technologies, Inc.
 *
 * Created: July, 2020
 * Author:  Bradey Honsinger
 *
 * Description: Helper functions for debug shell commands.
 *
 */
#ifndef COMMAND_HELPERS_H
#define COMMAND_HELPERS_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline bool is_whitespace(char c)
{
  return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

static inline bool is_dblquote(char c)
{
  return (c == '"');
}

static inline bool is_escape(char c)
{
  return (c == '\\');
}

static inline bool is_dot(char c)
{
  return (c == '.');
}

#define CHK_ARGC(min, max) \
  if (argc < min || (max > 0 && argc > max)) { \
    printf("Error: invalid number of arguments\n"); \
    return; \
  }

bool
parse_uint8_arg(char *command, char *arg, uint8_t *p_value);

bool
parse_uint8_arg_max(char *command, char *arg, uint8_t max, uint8_t *p_value);

bool
parse_uint8_arg_min_max(
  char *command,
  char *arg,
  uint8_t min,
  uint8_t max,
  uint8_t *p_value
  );

bool
parse_uint32_arg(char *command, char *arg, uint32_t *p_value);

bool
parse_uint32_arg_max(char *command, char *arg, uint32_t max, uint32_t *p_value);

bool
parse_uint32_arg_min_max(
  char *command,
  char *arg,
  uint32_t min,
  uint32_t max,
  uint32_t *p_value
  );

bool
parse_uint64_arg(char *command, char *arg, uint64_t *p_value);

bool
parse_uint64_arg_max(char *command, char *arg, uint64_t max, uint64_t *p_value);

bool
parse_uint64_arg_min_max(
  char *command,
  char *arg,
  uint64_t min,
  uint64_t max,
  uint64_t *p_value
  );


bool
parse_double_arg(char *command, char *arg, double *p_value);

bool
parse_double_arg_max(char *command, char *arg, double max, double *p_value);

bool
parse_double_arg_min_max(
  char *command,
  char *arg,
  double min,
  double max,
  double *p_value
  );

/** Maximum number of tokens that can be pointed-to by token_t (any more won't get parsed). */
#define TOKENC_MAX 16

typedef struct{
  char* v[TOKENC_MAX];
  int c;
} token_t;

void parse_dot_notation(char *str, size_t str_len, token_t* tokens);

typedef enum{
  PARSE_VAR_RETURN_NOTHING = 0,
  PARSE_VAR_RETURN_ARG,
  PARSE_VAR_RETURN_SETTINGS_VALUE,
  PARSE_VAR_RETURN_SETTINGS_KEY,
} parse_variable_return_t;

parse_variable_return_t parse_variable_from_tokens(token_t* tokens, char *value, size_t value_size);
parse_variable_return_t parse_variable_from_string(char* arg, char *value, size_t value_size);

void
hex_dump(const uint8_t* data, size_t size, size_t offset);


#ifdef __cplusplus
}
#endif

#endif  // COMMAND_HELPERS
