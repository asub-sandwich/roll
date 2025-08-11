# Makefile for roll (C dice roller)

# ---- config ----
CC       ?= cc
TARGET   ?= bin/roll
SRC      ?= roll.c

# Install locations
PREFIX   ?= /usr/local
BINDIR   ?= $(PREFIX)/bin

INSTALL  ?= install
STRIP    ?= strip

# Flags
WARNFLAGS        := -Wall -Wextra -Wpedantic
CSTD             := -std=c99
CFLAGS_RELEASE   := $(CSTD) $(WARNFLAGS) -O3 -flto -DNDEBUG
LDFLAGS_RELEASE  := -flto -s
CFLAGS_DEBUG     := $(CSTD) $(WARNFLAGS) -O0 -g3
LDFLAGS_DEBUG    :=

TEST_SCRIPT := test.sh

# ---- phony targets ----
.PHONY: all release debug clean install uninstall test

all: release

release: $(TARGET)

# Ensure bin directory exists before building
$(TARGET): $(SRC)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_RELEASE) -o $@ $^ $(LDFLAGS_RELEASE)

debug:
	@mkdir -p $(dir $(TARGET))
	$(CC) $(CFLAGS_DEBUG) -o $(dir $(TARGET))roll-debug $(SRC) $(LDFLAGS_DEBUG)

clean:
	rm -f $(TARGET) $(dir $(TARGET))roll-debug

install: release
	$(INSTALL) -d "$(DESTDIR)$(BINDIR)"
	$(INSTALL) -m 0755 "$(TARGET)" "$(DESTDIR)$(BINDIR)/roll"
	-$(STRIP) "$(DESTDIR)$(BINDIR)/roll"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/roll"

# ---- tests ----
# Usage:
#   make test
#   make test TRIALS=100000
#   make test DICE_TYPES="6 20"
test: release $(TEST_SCRIPT)
	@chmod +x $(TEST_SCRIPT)
	@echo "Running probability test with TRIALS=$${TRIALS:-10000} on dice: $${DICE_TYPES:-2 4 6 8 10 12 20 100}"
	@TRIALS="$${TRIALS:-10000}" DICE_TYPES="$${DICE_TYPES:-2 4 6 8 10 12 20 100}" BIN="./$(TARGET)" bash $(TEST_SCRIPT)
