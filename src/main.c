
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "token.h"
#include "roll.h"
#include "color.h"
#include "rng.h"
#include "repl.h"

static void print_help(void) {
  fprintf(stderr,
"\nUsage:\n"
"     roll <cound>d<die> [+|- <count>d<die> | <constant>]...\n"
"          [--no-color | -n] [--quiet | -q]\n\n"
"Examples:\n"
"     roll d20\n"
"     roll 5d6 + 4\n"
"     roll 2d8 + d10 - 2\n\n"
"Allowed Dice: d2 d4 d6 d8 d10 d12 d20 d100\n\n"
);
}

/* ---------- Main ---------- */
int main(int argc, char** argv) {

  init_allowed_dice();

  if (argc == 1) {
    repl_start();
    return 0;
  }
  
  if (strcmp(argv[1], "-h")==0 || strcmp(argv[1], "--help") == 0) {
    print_help();
    return 0;
  }

  bool force_no_color = false, quiet = false;
  Token* tokens = NULL;
  size_t tlen = 0, tcap = 0;

  for (int i=1; i<argc; i++) {
    if (strcmp(argv[i], "--no-color") == 0 || strcmp(argv[i], "-n") == 0) {
      force_no_color = true;
      continue;
    }

    if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
        quiet = true; force_no_color = true;
        continue;
      }
    
    if (tlen==tcap) {
      size_t ncap = tcap ? tcap * 2 : 16;
      Token* nt = (Token*)realloc(tokens, ncap*sizeof(Token));
      if (!nt) { fprintf(stderr, "OOM!\n"); free(tokens); return 1; }
      tokens = nt; tcap = ncap;
    }

    if (parse_token(argv[i], &tokens[tlen]) != 0) {
      print_help();
      fprintf(stderr, "Error: invalid token '%s'\n", argv[i]);
      free(tokens); return 1;
    }
    tlen++;
  }

  bool color = should_color() && !force_no_color;
  if (tlen == 0 || (tlen % 2) == 0) {
    fprintf(stderr, "Error: No dice or bad expression!\n");
    free(tokens); return 1;
  }

  Rng rng = rng_new();
  unsigned total = 0;
  SpecialVec specials = {0};
  bool err = false;

  RollResult rr = value_of(&tokens[0], &rng, &err);
  if (err){ fprintf(stderr, "Error: Bad first value!\n"); free(tokens); return 1; }
  maybe_collect_special(&tokens[0], &rr, &specials);
  
  total += rr.sum;
  if (!quiet) print_step(&tokens[0], &rr, total, color, stdout);
  uv_free(&rr.rolls);

  for (size_t i = 1; i + 1 < tlen; i += 2) {
    Token op = tokens[i], rhs = tokens[i + 1];
    rr = value_of(&rhs, &rng, &err);
    if (err) { fprintf(stderr, "Error: Missing operand!\n"); free(tokens); return 1; }
    maybe_collect_special(&rhs, &rr, &specials);

    if (op.type == T_PLUS) {
      total += rr.sum;
    } else if (op.type == T_MINUS) {
      total = (rr.sum > total) ? 0u : total - rr.sum;
    } else {
      fprintf(stderr, "Error: expected operator\n");
      free(tokens);
      uv_free(&rr.rolls);
      sv_free(&specials);
      return 1;
    }
    
    if (!quiet) {
      print_op(op.type, color, stdout);
      print_step(&rhs, &rr, total, color, stdout);
    }
    uv_free(&rr.rolls);
  }

  if (!quiet) {
    print_total(total, color, stdout);
    print_specials(&specials, color, stdout);
  } else {
    printf("%u\n", total);
    fflush(stdout);
  }

  sv_free(&specials);
  free(tokens);
  return 0;
}
