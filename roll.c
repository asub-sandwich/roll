#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#if defined(_WIN32)
  #include <io.h>
  #include <windows.h>
  #define isatty _isatty
  #define STDOUT_FILENO 1
#else
  #include <unistd.h>
#endif

/* ---------- Utils ---------- */
static const unsigned DICE_TYPES[] = {2, 4, 6, 8, 10, 12, 20, 100};
static const size_t   DICE_TYPES_LEN = sizeof(DICE_TYPES)/sizeof(DICE_TYPES[0]);
static void print_help(void) {
  fprintf(stderr,
  "\nUsage:\n"
  "     roll <cound>d<die> [+|- <count>d<die> | <constant>]... [--no-color]\n\n"
    "Examples:\n"
  "     roll d20\n"
  "     roll 5d6 + 4\n"
  "     roll 2d8 + d10 - 2\n\n"
  "Allowed Dice: d2 d4 d6 d8 d10 d12 d20 d100\n\n"
  );
}

/* ---------- Terminal Coloring ---------- */
static bool should_color(void) {
#if defined(_WIN32)
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  if (h == INVALID_HANDLE_VALUE) return false;
  DWORD mode = 0
  if (!GetConsoleMode(h, &mode)) return false;
  SecConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
  if (!isatty(STDOUT_FILENO)) return false;
  const char* noc = getenv("NO_COLOR");
  if (noc && noc[0] != '\0') return false;
  const char* term = getenv("TERM");
  if (term && strcmp(term, "dumb") == 0) return false;
  return true;
}

static void wrap(const char* code, const char* s, bool color, FILE* out) {
  if (color) fprintf(out, "\x1b[%sm%s\x1b[0m", code, s);
  else       fputs(s, out);
}

static void green(const char* s, bool color, FILE* out) {
  wrap("32", s, color, out);
}

static void red(const char* s, bool color, FILE* out) {
  wrap("31", s, color, out);
}

static void bolds(const char* s, bool color, FILE* out) {
  wrap("1", s, color, out);
}

static void boldu(unsigned v, bool color, FILE* out) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%u", v);
  bolds(buf, color, out);
}

/* ---------- Random Range Generator ---------- */
typedef struct {uint64_t state; } Rng;

static uint64_t seed_from_time(void) {
  struct timespec ts;
#if defined(_WIN32)
  FILETIME ft; GetSystemTimeAsFileTime(&ft);
  uint64_t t = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
  return t ^ (t << 13) ^ (t >> 7);
#else
  clock_gettime(CLOCK_REALTIME, &ts);
  return (uint64_t)ts.tv_sec ^ ((uint64_t)ts.tv_nsec << 32);
#endif
}

static Rng rng_new() {
  Rng r;
  uint64_t seed = seed_from_time();
  r.state = (seed ? seed: 0x9E3779B97F4A7C15ULL) | 1ULL;
  return r;
}

static uint64_t rng_next_u64(Rng* r) {
  uint64_t x = r->state;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  r->state = x;
  return x * 0x2545F4914F6CDD1DULL;
}

static unsigned rng_range(Rng* r, unsigned low, unsigned high) {
  uint64_t span = (uint64_t)high - (uint64_t)low + 1ULL;
  uint64_t zone = UINT64_MAX - (UINT64_MAX % span);
  for (;;) {
    uint64_t v = rng_next_u64(r);
    if (v < zone) return (unsigned)(v % span) + low;
  }
}

/* ---------- Token Parsing ---------- */
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

static bool is_allowed_die(unsigned sides) {
  for (size_t i=0; i<DICE_TYPES_LEN; i++) {
    if (DICE_TYPES[i] == sides) return true;
  }
  return false;
}

static bool is_numeric(const char* s) {
  if (!*s) return false;
  for (; *s; ++s) if (*s < '0' || *s > '9') return false;
  return true;
}

static int parse_token(const char* s, Token* out) {
  if (strcmp(s, "+")==0) {
    out->type = T_PLUS;
    return 0;
  }
  if (strcmp(s, "-")==0) {
    out->type = T_MINUS;
    return 0;
  }
  if (is_numeric(s)) {
    out->type = T_CONST;
    out->value = (unsigned)strtoul(s, NULL, 10);
    return 0;
  }

  // NdM or dM
  char* dpos = strchr(s, 'd');
  if (!dpos) dpos = strchr(s, 'D');
  if (!dpos) return -1;

  unsigned count = 1;
  unsigned sides = 0;

  if (dpos != s) {
    char left[32]={0};
    size_t len = (size_t)(dpos - s);
    if (len >= sizeof(left)) return -1;
    memcpy(left, s, len);
    if (!is_numeric(left)) return -1;
    count = (unsigned)strtoul(left, NULL, 10);
    if (count==0) count = 1;
  }

  const char* rhs = dpos+1;
  if (!is_numeric(rhs)) return -1;
  sides = (unsigned)strtoul(rhs, NULL, 10);
  if (!is_allowed_die(sides)) return -2;

  out->type = T_ROLL;
  out->count = count;
  out->sides = sides;
  return 0;
}

/* ---------- Evaulation ---------- */
// Value Vector
typedef struct {
  unsigned* data;
  size_t len, cap;
} UIntVec;

static void uv_init(UIntVec* v) {
  v->data = NULL;
  v->len = 0;
  v->cap = 0;
}

static void uv_push(UIntVec* v, unsigned x) {
  if (v->len == v->cap) {
    size_t ncap = v->cap? v->cap*2 : 8;
    unsigned* nd = (unsigned*)realloc(v->data, ncap*sizeof(unsigned));
    if (!nd) {
      fprintf(stderr, "Damn bruh, close your chrome tabs!\n");
      exit(1); 
    }
    v->data = nd;
    v->cap = ncap;
  }
  v->data[v->len++] = x;
}

static void uv_free(UIntVec* v) {
  free(v->data);
  v->data = NULL;
  v->len = 0;
  v->cap = 0;
}

// Special Roll Vector
typedef enum {
  SP_NAT20,
  SP_NAT1,
} Special;

typedef struct {
  Special* data;
  size_t len, cap;
} SpecialVec;

static void sv_init(SpecialVec* v) {
  v->data = NULL;
  v->len = 0;
  v->cap = 0; 
}

static void sv_push(SpecialVec* v, Special s) {
  if (v->len == v->cap) {
    size_t ncap = v->cap? v->cap*2 : 8;
    Special* nd = (Special*)realloc(v->data, ncap*sizeof(Special));
    if (!nd) {
      fprintf(stderr, "Damn bruh, close your chrome tabs!\n");
      exit(1);
    }
    v->data = nd;
    v->cap = ncap;
  }
  v->data[v->len++] = s;
}

static void sv_free(SpecialVec* v) {
  free(v->data);
  v->data = NULL;
  v->len = 0;
  v->cap = 0;
}

// Rolling maching
typedef struct {
  UIntVec rolls;
  unsigned sum;
} RollResult;

static RollResult roll_dice(unsigned count, unsigned sides, Rng* rng) {
  RollResult rr;
  uv_init(&rr.rolls);
  rr.sum = 0;
  for (unsigned i=0; i<count; i++) {
    unsigned r = rng_range(rng, 1u, sides);
    uv_push(&rr.rolls, r);
    rr.sum += r;
  }
  return rr;
}

static RollResult value_of(const Token* t, Rng* rng, bool* err) {
  RollResult rr;
  uv_init(&rr.rolls);
  rr.sum = 0;

  switch (t->type) {
    case T_CONST: rr.sum = t->value; return rr;
    case T_ROLL: return roll_dice(t->count, t->sides, rng);
    case T_PLUS:
    case T_MINUS:
    default:
      *err = true;
      return rr;
  }
}

// Format helpers
static void format_operand(const Token* t, const RollResult* rr, FILE* out) {
  if (t->type == T_CONST) {
    fprintf(out, "%u", rr->sum);
  } else if (t->type == T_ROLL) {
    fprintf(out, "%ud%u -> [", t->count, t->sides);
    for (size_t i=0; i<rr->rolls.len; i++) {
      if (i) fputs(", ", out);
      fprintf(out, "%u", rr->rolls.data[i]);
    }
    fputc(']', out);
  }
}

static void format_special(Special s, bool color, FILE* out) {
  if (s == SP_NAT20) {
    char const* msg = "NATURAL 20!";
    char buf[16];
    snprintf(buf, sizeof(buf), "%s", msg);
    bolds("", color, out);
    green(buf, color, out);
    bolds("", color, out);
    fputc('\n', out);
  } else if (s == SP_NAT1) {
    char const* msg = "NATURAL 1!!";
    char buf[16];
    snprintf(buf, sizeof(buf), "%s", msg);
    bolds("", color, out);
    red(buf, color, out);
    bolds("", color, out);
    fputc('\n', out);
  }
}

static void maybe_collect_special(const Token* t, const RollResult* rr, SpecialVec* specials) {
  if (t->type == T_ROLL && t->count == 1 && t->sides == 20 && rr->rolls.len == 1) {
    unsigned v = rr->rolls.data[0];
    if (v == 20) sv_push(specials, SP_NAT20);
    else if (v == 1) sv_push(specials, SP_NAT1);
  }
}

/* ---------- Main ---------- */
int main(int argc, char** argv) {
  if (
    argc <= 1 ||
    strcmp(argv[1], "-h")==0 ||
    strcmp(argv[1], "--help") == 0
  ){
    print_help();
    return 0;
  }

  bool force_no_color = false;

  // Collect tokens
  Token* tokens = NULL;
  size_t tlen = 0, tcap = 0;

  for (int i=1; i<argc; i++) {
    if (strcmp(argv[i], "--no-color")==0) {
      force_no_color = true;
      continue;
    }
    
    if (tlen==tcap) {
      size_t ncap = tcap? tcap*2 : 16;
      Token* nt = (Token*)realloc(tokens, ncap*sizeof(Token));
      if (!nt) {
        fprintf(stderr, "Damn bruh, close your chrome tabs!\n");
        free(tokens);
        return 1;
      }
      tokens = nt;
      tcap = ncap;
    }

    Token tk;
    int rc = parse_token(argv[i], &tk);
    if (rc == -2) {
      print_help();
      fprintf(stderr, "Error: not a valid die option!\n");
      free(tokens);
      return 1;
    } else if (rc != 0) {
      print_help();
      fprintf(stderr, "Error: unrecognized token! %s\n", argv[i]);
      free(tokens);
      return 1;
    }
    tokens[tlen++] = tk;
  }

  bool color = should_color() && !force_no_color;

  if (tlen == 0) {
    print_help();
    free(tokens);
    return 1;
  }

  if ((tlen % 2) == 0) {
    fprintf(stderr, "Error: Operators / operands mismatch!\n");
    free(tokens);
    return 1;
  }

  Rng rng = rng_new();
  unsigned total = 0;

  SpecialVec specials;
  sv_init(&specials);

  // Get first operand
  bool err = false;
  RollResult rr = value_of(&tokens[0], &rng, &err);
  if (err) {
    print_help();
    free(tokens);
    uv_free(&rr.rolls);
    sv_free(&specials);
    return 1;
  }
  maybe_collect_special(&tokens[0], &rr, &specials);
  total += rr.sum;
  format_operand(&tokens[0], &rr, stdout);
  fputs(" = ", stdout);
  boldu(total, color, stdout);
  fputc('\n', stdout);
  uv_free(&rr.rolls);

  // Get pairs of op operand
  for (size_t i=1; i+1<tlen; i+=2) {
    Token op = tokens[i];
    Token rhs = tokens[i + 1];
    RollResult rr = value_of(&rhs, &rng, &err);
    if (err) {
      fprintf(stderr, "Error: expected value after operand!\n");
      free(tokens);
      uv_free(&rr.rolls);
      sv_free(&specials);
      return 1;
    }
    maybe_collect_special(&rhs, &rr, &specials);

    if (op.type == T_PLUS) {
      green("+", color, stdout);
      fputc(' ', stdout);
      total += rr.sum;
    } else if (op.type == T_MINUS) {
      red("-", color, stdout);
      fputc(' ', stdout);
      total = (rr.sum > total) ? 0u : total - rr.sum;
    } else {
      fprintf(stderr, "Error: expected operator!\n");
      free(tokens);
      uv_free(&rr.rolls);
      sv_free(&specials);
      return 1;
    }

    format_operand(&rhs, &rr, stdout);
    fputs(" = ", stdout);
    boldu(total, color, stdout);
    fputc('\n', stdout);
    uv_free(&rr.rolls);
  }
  
  fputs("Total: ", stdout);
  boldu(total, color, stdout);
  fputc('\n', stdout);

  for (size_t i=0; i<specials.len; i++) {
    format_special(specials.data[i], color, stdout);
  }
}
