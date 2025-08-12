#include "color.h"
#include <asm-generic/ioctls.h>
#include <string.h>
#include <stdlib.h>

#if defined(_WIN32)
    #include <io.h>
    #include <windows.h>
    #define isatty _isatty
    #define STDOUT_FILENO 1
#else
    #include <unistd.h>
    #include <sys/ioctl.h>
#endif

bool should_color(void) {
#if defined(_WIN32)
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  if (h == INVALID_HANDLE_VALUE) return false;
  DWORD mode = 0;
  if (!GetConsoleMode(h, &mode)) return false;
  SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
  if (!isatty(STDOUT_FILENO)) return false;
  const char* noc = getenv("NO_COLOR");
  if (noc && noc[0] != '\0') return false;
  const char* term = getenv("TERM");
  return !(term && strcmp(term, "dumb") ==0 );
}

void boldu(unsigned v, bool color, FILE* out) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%u", v);
  BOLD(buf, color, out);
}

void term_clear(void) {
#if defined(_WIN32)
    /* Try to enable VT sequences so \x1b codes work */
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(h, &mode)) {
            SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
#endif
    if (isatty(STDOUT_FILENO)) {
        /* \x1b[2J = clear screen, \x1b[H = move cursor to home */
        fputs("\x1b[2J\x1b[H", stdout);
        fflush(stdout);
    }
}

int term_columns() {
    int cols = 0;

#if defined(_WIN32)
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE) {
        CONSOLE_SCREEN_BUFFER_INTO csbi;
        if (GetConsoleScreenBufferInfo(h, &csbi)) {
            cols = (int)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
        }
    }
#else
    if (isatty(STDOUT_FILENO)) {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
            cols = (int)ws.ws_col;
    }
#endif

    if (cols <= 0) {
        const char* env = getenv("COLUMNS");
        if (env) {
            long v = strtol(env, NULL, 10);
            if (v > 0 && v < 10000) cols = (int)v;
        }
    }
    if (cols <= 0 || cols > 10000) cols = 80;
    return cols;
}

void term_print_hr(char ch) {
    int cols = term_columns();
    if (cols < 1) cols = 80;

    char buf[128];
    memset(buf, ch, sizeof(buf));
    while (cols > 0) {
        int n = cols < (int)sizeof(buf) ? cols : (int)sizeof(buf);
        fwrite(buf, 1, (size_t)n, stdout);
        cols -= n;
    }
    fputc('\n', stdout);
    fflush(stdout);
}

