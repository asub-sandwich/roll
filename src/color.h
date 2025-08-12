#ifndef COLOR_H
#define COLOR_H

#include <stdbool.h>
#include <stdio.h>

bool should_color(void);
void boldu(unsigned v, bool color, FILE* out);
void term_clear(void);
int term_columns(void);
void term_print_hr(char ch);
#define WRAP(code, s, color, out) \
  do { \
    if (color) fprintf(out, "\x1b[%sm%s\x1b[0m", code, s); \
    else fputs(s, out); \
  } while(0)
#define GREEN(s, color, out) WRAP("32", s, color, out)
#define RED(s, color, out)   WRAP("31", s, color, out)
#define BOLD(s, color, out)  WRAP("1",  s, color, out)
#endif
