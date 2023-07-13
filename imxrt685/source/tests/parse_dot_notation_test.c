/*
 * parse_dot_notation_test.c
 *
 *  Created on: May 22, 2022
 *      Author: DavidWang
 */

#include "parse_dot_notation_test.h"
#include "command_helpers.h"
#include "loglevels.h"

#if (defined(ENABLE_PARSE_DOT_NOTATION_TEST) && (ENABLE_PARSE_DOT_NOTATION_TEST > 0U))

#define PRINT_PASS(success) \
    if( success ) { \
      LOGV("parse_dot","PASS!"); \
    } else { \
      LOGV("parse_dot","FAIL!"); \
    }

#define TOKEN_EQUALS(I,STR) \
        (strcmp(tokens.v[I],STR) == 0)

void print_dot_notation(char* literal, token_t* tokens){
  printf("TEST PARSE TOKEN: \"%s\"\n", literal);
  printf(" num tokens: %d\n", tokens->c);
  for(unsigned int i=0; i<tokens->c; i++){
    if(tokens->v[i] == NULL){
      printf(" token: %u is NULL\n", i );
    }else{
      printf(" token: %u is \"%s\"\n", i, tokens->v[i] );
    }
  }
}

// Test parse dot notation

// "a.b.c" -> 3 tokens: [a][b][c]
// ".b.c" -> 3 tokens: NULL[b][c]
// "a.b." -> 2 tokens: [a][b]
// "a..c" -> 3 tokens: [a]NULL[c]
// ".b. "  -> NULL[b]
// ".b. c  "  -> NULL[b][c]
// " a b " -> [a b]
// "a " -> [a]
// "" ->
// " " ->
void test_parse_dot_notation(){
  char str[256];
  token_t tokens;

  {
    char* literal = "a.b.c";
    strcpy(str,literal);
    parse_dot_notation(str, sizeof(str), &tokens);
    print_dot_notation(literal, &tokens);
    bool success = true;
    success &= (tokens.c == 3);
    success &= TOKEN_EQUALS(0,"a");
    success &= TOKEN_EQUALS(1,"b");
    success &= TOKEN_EQUALS(2,"c");
    PRINT_PASS(success);
  }
  {
    char* literal = ".b.c";
    strcpy(str,literal);
    parse_dot_notation(str, sizeof(str), &tokens);
    print_dot_notation(literal, &tokens);
    bool success = true;
    success &= (tokens.c == 3);
    success &= TOKEN_EQUALS(0,NULL);
    success &= TOKEN_EQUALS(1,"b");
    success &= TOKEN_EQUALS(2,"c");
    PRINT_PASS(success);
  }
  {
    char* literal = "a.b.";
    strcpy(str,literal);
    parse_dot_notation(str, sizeof(str), &tokens);
    print_dot_notation(literal, &tokens);
    bool success = true;
    success &= (tokens.c == 2);
    success &= TOKEN_EQUALS(0,"a");
    success &= TOKEN_EQUALS(1,"b");
    PRINT_PASS(success);
  }
  {
    char* literal = "a..c";
    strcpy(str,literal);
    parse_dot_notation(str, sizeof(str), &tokens);
    print_dot_notation(literal, &tokens);
    bool success = true;
    success &= (tokens.c == 3);
    success &= TOKEN_EQUALS(0,"a");
    success &= TOKEN_EQUALS(1,NULL);
    success &= TOKEN_EQUALS(2,"c");
    PRINT_PASS(success);
  }
  {
    char* literal = ".b. ";
    strcpy(str,literal);
    parse_dot_notation(str, sizeof(str), &tokens);
    print_dot_notation(literal, &tokens);
    bool success = true;
    success &= (tokens.c == 2);
    success &= TOKEN_EQUALS(0,NULL);
    success &= TOKEN_EQUALS(1,"b");
    PRINT_PASS(success);
  }
  {
    char* literal = ".b. c  ";
    strcpy(str,literal);
    parse_dot_notation(str, sizeof(str), &tokens);
    print_dot_notation(literal, &tokens);
    bool success = true;
    success &= (tokens.c == 3);
    success &= TOKEN_EQUALS(0,NULL);
    success &= TOKEN_EQUALS(1,"b");
    success &= TOKEN_EQUALS(2,"c");
    PRINT_PASS(success);
  }
  {
    char* literal = " a b ";
    strcpy(str,literal);
    parse_dot_notation(str, sizeof(str), &tokens);
    print_dot_notation(literal, &tokens);
    bool success = true;
    success &= (tokens.c == 1);
    success &= TOKEN_EQUALS(0,"a b");
    PRINT_PASS(success);
  }
  {
    char* literal = "a ";
    strcpy(str,literal);
    parse_dot_notation(str, sizeof(str), &tokens);
    print_dot_notation(literal, &tokens);
    bool success = true;
    success &= (tokens.c == 1);
    success &= TOKEN_EQUALS(0,"a");
    PRINT_PASS(success);
  }
  {
    char* literal = "";
    strcpy(str,literal);
    parse_dot_notation(str, sizeof(str), &tokens);
    print_dot_notation(literal, &tokens);
    bool success = true;
    success &= (tokens.c == 0);
    PRINT_PASS(success);
  }
  {
    char* literal = " ";
    strcpy(str,literal);
    parse_dot_notation(str, sizeof(str), &tokens);
    print_dot_notation(literal, &tokens);
    bool success = true;
    success &= (tokens.c == 0);
    PRINT_PASS(success);
  }

}


#endif // (defined(ENABLE_PARSE_DOT_NOTATION_TEST) && (ENABLE_PARSE_DOT_NOTATION_TEST > 0U))
