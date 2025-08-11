# Makefile for roll (C dice roller)

# ---- config (override from CLI as needed) ----
CC               ?= cc
TARGET           ?= roll
SRC              ?= roll.c

# Install locations (standard defaults)
PREFIX           ?= /usr/local
BINDIR           ?= $(PREFIX)/bin

# Tools
INSTALL          ?= install
STRIP            ?= strip

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

$(TARGET): $(SRC)
	$(CC) $(CFLAGS_RELEASE) -o $@ $^ $(LDFLAGS_RELEASE)

debug:
	$(CC) $(CFLAGS_DEBUG) -o $(TARGET)-debug $(SRC) $(LDFLAGS_DEBUG)

clean:
	rm -f $(TARGET) $(TARGET)-debug

# Install the optimized build. Honors DESTDIR and PREFIX.
install: release
	$(INSTALL) -d "$(DESTDIR)$(BINDIR)"
	$(INSTALL) -m 0755 "$(TARGET)" "$(DESTDIR)$(BINDIR)/$(TARGET)"
	# Optional: extra strip (harmless if already -s linked)
	-$(STRIP) "$(DESTDIR)$(BINDIR)/$(TARGET)"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/$(TARGET)"
