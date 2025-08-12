#ifndef VEC_H
#define  VEC_H

#include <stdio.h>
#include <stdlib.h>

#define GROW_VEC(vec, type) \
  do { \
    if ((vec)->len == (vec)->cap) { \
      size_t ncap = (vec)->cap ? (vec)->cap * 2 : 8; \
      type* nd = realloc((vec)->data, ncap * sizeof(type)); \
      if (!nd) { fprintf(stderr, "Damn bruh, close your chrome tabs!"); exit(1); }\
      (vec)->data = nd; \
      (vec)->cap = ncap; \
    } \
  } while (0)

#endif
