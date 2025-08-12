#include "token.h"
#include <stdlib.h>
#include <string.h>

static bool ALLOWED_DICE[101] = {0};

void init_allowed_dice(void) {
  unsigned allowed[] = {2, 4, 6, 8, 10, 12, 20, 100};
  for (size_t i = 0; i < sizeof(allowed)/sizeof(allowed[0]); i++) {
    ALLOWED_DICE[allowed[i]] = true;
  }
}

int parse_token(const char* s, Token* out) {
  if (strcmp(s, "+") == 0) { out->type = T_PLUS; return 0; }
  if (strcmp(s, "-") == 0) { out->type = T_MINUS; return 0; }

  char* end;
  unsigned count = (unsigned)strtoul(s, &end, 10);
  if (end == s) count = 1;
  if (*end == '\0') {
    out->type = T_CONST;
    out->value = count;
    return 0;
  }
  if (*end != 'd' && *end != 'D') return -1;
  unsigned sides = (unsigned)strtoul(end + 1, NULL, 10);
  if (sides > 100 || !ALLOWED_DICE[sides]) return -2;

  out->type = T_ROLL;
  out->count = count;
  out->sides = sides;
  return 0;
}
