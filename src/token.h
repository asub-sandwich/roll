#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>

typedef enum {
  T_ROLL,
  T_CONST,
  T_PLUS,
  T_MINUS
} TokenType;

typedef struct {
  TokenType type;
  unsigned count;
  unsigned sides;
  unsigned value;
} Token;

void init_allowed_dice(void);
int parse_token(const char* s, Token* out);

#endif
