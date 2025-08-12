#define _POSIX_C_SOURCE 200809L
#include "repl.h"
#include "token.h"
#include "roll.h"
#include "color.h"
#include "rng.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void print_repl_help(void) {
  puts("");
  puts("Commands: help | color on | color off | quit | exit");
  puts("Examples:");
  puts("  d20");
  puts("  5d6 + 4");
  puts("  2d8 + d10 - 2");
  puts("");
}

static bool iscmd(const char* s, const char* cmd) {
  size_t n = strlen(cmd);
  if (strncmp(s, cmd, n) != 0) return false;
  const char* p = s + n;
  while (*p) { if (!isspace((unsigned char)*p)) return false; ++p;}
  return true;
}

static char* trim(char* s) {
  while (isspace((unsigned char)*s)) ++s;
  if (*s == '\0') return s;
  char* e = s + strlen(s) - 1;
  while (e > s && isspace((unsigned char)*e)) --e;
  e[1] = '\0';
  return s;
}

static bool scan_tokens_inline(const char* s, Token** out, size_t* out_len) {
    *out = NULL; *out_len = 0;             

    Token* tokens = NULL;
    size_t tlen = 0, tcap = 0;

    const char* p = s;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) ++p;
        if (!*p) break;

        char* tmp = NULL;
        if (*p == '+' || *p == '-') {
            tmp = (char*)malloc(2);
            if (!tmp) goto oom;
            tmp[0] = *p++;
            tmp[1] = '\0';
        } else {
            const char* start = p;
            while (*p && !isspace((unsigned char)*p) && *p != '+' && *p != '-') ++p;
            size_t len = (size_t)(p - start);
            tmp = (char*)malloc(len + 1);
            if (!tmp) goto oom;
            memcpy(tmp, start, len);
            tmp[len] = '\0';
        }

        if (tlen == tcap) {
            size_t ncap = tcap ? tcap * 2 : 16;
            Token* nt = (Token*)realloc(tokens, ncap * sizeof(*nt));
            if (!nt) { free(tmp); goto oom; }
            tokens = nt; tcap = ncap;
        }

        int rc = parse_token(tmp, &tokens[tlen]);
        if (rc != 0) {
            fprintf(stderr, "Error: invalid token '");
            for (const char* q = tmp; *q; ++q) {
                int ic = (unsigned char)*q;
                if (isprint(ic)) fputc(ic, stderr);
                else fprintf(stderr, "\\x%02X", (unsigned)ic);
            }
            fputs("'\n", stderr);
            free(tmp);
            free(tokens);
            return false;                 
        }
        free(tmp);
        tlen++;
    }

    *out = tokens;
    *out_len = tlen;
    return true;

oom:
    fprintf(stderr, "OOM!\n");
    free(tokens);
    return false;                         
}

void repl_start() {
  init_allowed_dice();

  bool color = should_color();

  term_clear();

  Rng rng = rng_new();

  char* line = NULL;
  size_t cap = 0;

  puts("roll REPL - type 'help' for help, 'quit' to exit.");
  for (;;) {

    // init
    fputs("roll> ", stdout);
    fflush(stdout);

    ssize_t nread = getline(&line, &cap, stdin);
    if (nread < 0) {
      fputc('\n', stdout);
      break;
    }
    char* s = trim(line);
    
    // commands
    if (iscmd(s, "quit") || iscmd(s, "exit")) { puts(""); break; };
    if (iscmd(s, "help")) { print_repl_help(); continue; }
    if (iscmd(s, "clear")) { term_clear(); continue; }
    if (iscmd(s, "color on")) {
      if (should_color()) { puts("Turning color on"); color = true; }
      else { puts("Cannot turn on color"); color = false; }
      continue;
    }
    if (iscmd(s, "color off")) {
      puts("Turning off color");color = false; continue;
    }

    // tokenize
    Token* tokens = NULL;
    size_t tlen=0;

    if (!scan_tokens_inline(s, &tokens, &tlen)) continue;
    
    if (tlen == 0 || (tlen % 2) == 0) {
      fprintf(stderr, "Error: no dice or bad expression\n");
      free(tokens);
      continue;
    }

    //evaluate
    unsigned total = 0;
    SpecialVec specials = (SpecialVec){0};
    bool err = false;

    RollResult rr = value_of(&tokens[0], &rng, &err);
    if (err) {
      fprintf(stderr, "Error: bad first operand\n");
      free(tokens);
      uv_free(&rr.rolls);
      continue;
    }
    maybe_collect_special(&tokens[0], &rr, &specials);
    total += rr.sum;
    puts("");
    print_step(&tokens[0], &rr, total, color, stdout);
    uv_free(&rr.rolls);

    for (size_t i = 1; i+1 < tlen; i += 2) {
      Token op = tokens[i], rhs = tokens[i + 1];
      rr = value_of(&rhs, &rng, &err);
      if (err) { fprintf(stderr, "Error: missing operand\n"); uv_free(&rr.rolls); break; }
      maybe_collect_special(&rhs, &rr, &specials);

      if (op.type == T_PLUS) total += rr.sum;
      else if (op.type == T_MINUS) total = (rr.sum > total) ? 0u : total - rr.sum;
      else {
        fprintf(stderr, "Error: expected operator\n");
        uv_free(&rr.rolls);
        err = true;
        break;
      }

      print_op(op.type, color, stdout);
      print_step(&rhs, &rr, total, color, stdout);
      uv_free(&rr.rolls);
    }

    if (!err) {
      puts("");
      print_total(total, color, stdout);
      print_specials(&specials, color, stdout);
      term_print_hr('*');
      puts("");
    }

    sv_free(&specials);
    free(tokens);
  }

  free(line);
}
