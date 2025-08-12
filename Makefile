.RECIPEPREFIX := >

CC       ?= cc
CSTD     ?= -std=c99
WARN     ?= -Wall -Wextra -Wpedantic
OPT_REL  ?= -O3 -flto -DNDEBUG
OPT_DBG  ?= -O0 -g3
LDFLAGS_REL ?= -flto -s
LDFLAGS_DBG ?=

PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin

SRCDIR   ?= src
OBJDIR   ?= build
BINDIR_LOCAL ?= bin
TARGET   ?= $(BINDIR_LOCAL)/roll

SRCS     := $(wildcard $(SRCDIR)/*.c)
OBJS     := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
DEPS     := $(OBJS:.o=.d)

# ---- ASAN (separate artifacts) ----
OBJDIR_ASAN   ?= build-asan
OBJS_ASAN     := $(patsubst $(SRCDIR)/%.c,$(OBJDIR_ASAN)/%.o,$(SRCS))
TARGET_ASAN   ?= $(BINDIR_LOCAL)/roll-asan
CFLAGS_ASAN   ?= $(CSTD) $(WARN) -O1 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS_ASAN  ?=

INSTALL  ?= install
STRIP    ?= strip

TEST_SCRIPT ?= test.sh
TRIALS      ?= 10000
DICE_TYPES  ?= 2 4 6 8 10 12 20 100

CPPFLAGS  ?=
CFLAGS_COMMON := $(CSTD) $(WARN) -MMD -MP $(CPPFLAGS)
LDFLAGS_COMMON :=

.PHONY: all release debug asan clean clean-asan install uninstall test

all: release

# ---- release build ----
release: CFLAGS := $(CFLAGS_COMMON) $(OPT_REL)
release: LDFLAGS := $(LDFLAGS_COMMON) $(LDFLAGS_REL)
release: $(TARGET)

# ---- debug build ----
debug: CFLAGS := $(CFLAGS_COMMON) $(OPT_DBG)
debug: LDFLAGS := $(LDFLAGS_COMMON) $(LDFLAGS_DBG)
debug: $(BINDIR_LOCAL)/roll-debug

# ---- ASAN build ----
asan: CFLAGS := $(CFLAGS_ASAN)
asan: LDFLAGS := $(LDFLAGS_ASAN)
asan: $(TARGET_ASAN)

# ---- link rules ----
$(TARGET): $(OBJS)
> mkdir -p $(BINDIR_LOCAL)
> $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BINDIR_LOCAL)/roll-debug: $(OBJS)
> mkdir -p $(BINDIR_LOCAL)
> $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_ASAN): $(OBJS_ASAN)
> mkdir -p $(BINDIR_LOCAL)
> $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# ---- compile rules ----
$(OBJDIR)/%.o: $(SRCDIR)/%.c
> mkdir -p $(OBJDIR)
> $(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR_ASAN)/%.o: $(SRCDIR)/%.c
> mkdir -p $(OBJDIR_ASAN)
> $(CC) $(CFLAGS) -c -o $@ $<

# ---- housekeeping ----
clean:
> rm -rf $(OBJDIR) $(BINDIR_LOCAL)/roll $(BINDIR_LOCAL)/roll-debug

clean-asan:
> rm -rf $(OBJDIR_ASAN) $(TARGET_ASAN)

install: release
> $(INSTALL) -d "$(DESTDIR)$(BINDIR)"
> $(INSTALL) -m 0755 "$(TARGET)" "$(DESTDIR)$(BINDIR)/roll"
> -$(STRIP) "$(DESTDIR)$(BINDIR)/roll"

uninstall:
> rm -f "$(DESTDIR)$(BINDIR)/roll"

# ---- tests ----
test: release $(TEST_SCRIPT)
> @chmod +x $(TEST_SCRIPT)
> @echo
> @echo "Running probability test with TRIALS=$(TRIALS) on dice: $(DICE_TYPES)"
> @TRIALS="$(TRIALS)" DICE_TYPES="$(DICE_TYPES)" BIN="./$(TARGET)" bash $(TEST_SCRIPT)

-include $(DEPS)
