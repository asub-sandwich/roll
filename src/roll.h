#ifndef ROLL_H
#define ROLL_H

#include <stdbool.h>
#include <stdio.h>
#include "token.h"
#include "rng.h"

typedef struct { unsigned* data; size_t len, cap; } UIntVec;
typedef struct { int* data; size_t len, cap; } SpecialVec;
typedef struct { UIntVec rolls; unsigned sum; } RollResult;

enum { SP_NAT20, SP_NAT1 };

RollResult roll_dice(unsigned count, unsigned sides, Rng* rng);
RollResult value_of(const Token* t, Rng* rng, bool* err);
void maybe_collect_special(const Token* t, const RollResult* rr, SpecialVec* specials);
void format_operand(const Token* t, const RollResult* rr, FILE* out);
void print_specials(const SpecialVec* specials, bool color, FILE* out);
void print_step(const Token* t, const RollResult* rr, unsigned total, bool color, FILE* out);
void print_op(TokenType op, bool color, FILE* out);
void print_total(unsigned total, bool color, FILE* out);

void uv_free(UIntVec* v);
void sv_free(SpecialVec* v);

#endif
