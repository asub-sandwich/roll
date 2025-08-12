#include "roll.h"
#include "vec.h"
#include "color.h"
#include <stdlib.h>

static void sv_push(SpecialVec* v, int s) {
  GROW_VEC(v, int);
  v->data[v->len++] = s;
}

void uv_free(UIntVec* v) {
  free(v->data);
  v->data = NULL;
  v->len = v->cap = 0;
}

void sv_free(SpecialVec* v) {
  free(v->data);
  v->data = NULL;
  v->len = v->cap = 0;
}

RollResult roll_dice(unsigned count, unsigned sides, Rng* rng) {
  RollResult rr = {0};
  rr.rolls.data = malloc(count * sizeof(unsigned));
  rr.rolls.cap = count;
  rr.rolls.len = 0;
  rr.sum = 0;
  for (unsigned i = 0; i < count; i++) {
    unsigned r = rng_range(rng, 1u, sides);
    rr.rolls.data[rr.rolls.len++] = r;
    rr.sum += r;
  }
  return rr;
}

RollResult value_of(const Token* t, Rng* rng, bool* err) {
  RollResult rr = {0};
  switch (t->type) {
    case T_CONST: rr.sum = t->value; return rr;
    case T_ROLL: return roll_dice(t->count, t->sides, rng);
    default: *err = true; return rr;
  }
}

void maybe_collect_special(const Token* t, const RollResult* rr, SpecialVec* specials) {
  if (
    t->type == T_ROLL &&
    t->count == 1 &&
    t->sides == 20 &&
    rr->rolls.len >= 1
  ) {
      if (rr->rolls.data[0] == 20) sv_push(specials, SP_NAT20);
      else if (rr->rolls.data[0] == 1) sv_push(specials, SP_NAT1);
    }
}

void format_operand(const Token *t, const RollResult *rr, FILE *out) {
  if (t->type == T_CONST) {
    fprintf(out, "%u", rr->sum);
  } else if (t->type == T_ROLL) {
    fprintf(out, "%ud%u -> [", t->count, t->sides);
    for (size_t i=0; i < rr->rolls.len; i++) {
      if (i) fputs(", ", out);
      fprintf(out, "%u", rr->rolls.data[i]);
    }
    fputc(']', out);
  }
}

void print_specials(const SpecialVec *specials, bool color, FILE *out) {
  for (size_t i=0; i < specials->len; i++) {
    if (specials->data[i] == SP_NAT20) {
      BOLD("", color, out);
      GREEN("NATURAL 20", color, out);
      BOLD("", color, out);
      fputc('\n', out);
    } else {
      BOLD("", color, out);
      RED("NATURAL 1", color, out);
      BOLD("", color, out);
      fputc('\n', out);
    }
  }
}

void print_step(const Token *t, const RollResult *rr, unsigned int total, bool color, FILE *out) {
  format_operand(t, rr, out);
  fputs(" = ", out);
  boldu(total, color, out);
  fputc('\n', out);
}

void print_op(TokenType op, bool color, FILE *out) {
  if (op == T_PLUS) GREEN("+", color, out);
  else if (op == T_MINUS) RED("-", color, out);
  fputc(' ', out);
}

void print_total(unsigned int total, bool color, FILE *out) {
  fputs("Total: ", out);
  boldu(total, color, out);
  fputc('\n', out);
}
