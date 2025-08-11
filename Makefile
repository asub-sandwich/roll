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

# ---- phony targets ----
.PHONY: all release debug clean install uninstall

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
