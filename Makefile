# tty-stopwatch
# Cross-platform Makefile for Linux (amd64, arm64, armhf) and macOS (amd64,
# arm64). Indented with TABs as required by GNU make.
#
# The program links against nothing but the C/C++ runtime: the terminal UI is
# built on POSIX (termios/poll/ioctl) plus ANSI escape sequences, and desktop
# notifications speak D-Bus (Linux) or use osascript (macOS) directly. There
# is therefore no -lncurses and no other third-party library to find.
#
# Build variants (Linux):
#   make                 Default dynamic build for the host.
#   make STATIC=1        Fully static binary (recommended on Alpine/musl).
#   make PORTABLE=1      glibc build with the C/C++ runtime linked statically.
#
# Build variant (macOS):
#   make MACOS_UNIVERSAL=1   Single universal binary (x86_64 + arm64).
#
# Convenience phony targets `static`, `portable`, and `macos-universal`
# re-invoke make with the matching variable set.

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  PREFIX ?= /usr/local
else
  PREFIX ?= /usr
endif

CXX      ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
LDFLAGS  ?=
LDLIBS   :=

# --- Linux link variants ----------------------------------------------------
ifeq ($(STATIC),1)
  # Fully static. Cleanest with a musl toolchain (e.g. on Alpine), where the
  # result runs on any Linux of the same architecture regardless of libc.
  LDFLAGS += -static
endif

ifeq ($(PORTABLE),1)
  # glibc hosts: keep the dynamic libc (for NSS) but bundle the C++ runtime so
  # the binary does not require a matching libstdc++ on the target machine.
  LDFLAGS += -static-libstdc++ -static-libgcc
endif

# --- macOS universal binary -------------------------------------------------
ifeq ($(MACOS_UNIVERSAL),1)
  CXXFLAGS += -arch x86_64 -arch arm64 -mmacosx-version-min=11.0
  LDFLAGS  += -arch x86_64 -arch arm64 -mmacosx-version-min=11.0
endif

SRC_DIR := src
OBJ_DIR := build
BIN     := tty-stopwatch

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS    := $(OBJECTS:.o=.d)

BINDIR  := $(DESTDIR)$(PREFIX)/bin

.PHONY: all clean run install uninstall static portable macos-universal

all: $(BIN)

$(BIN): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -MMD -MP -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Convenience wrappers for the build variants.
static:
	$(MAKE) STATIC=1

portable:
	$(MAKE) PORTABLE=1

macos-universal:
	$(MAKE) MACOS_UNIVERSAL=1

run: $(BIN)
	./$(BIN)

install: $(BIN)
	install -d $(BINDIR)
	install -m 0755 $(BIN) $(BINDIR)/$(BIN)
	@echo "Installed $(BIN) to $(BINDIR)/$(BIN)"

uninstall:
	rm -f $(BINDIR)/$(BIN)
	@echo "Removed $(BINDIR)/$(BIN) (if it existed)"

clean:
	rm -rf $(OBJ_DIR) $(BIN)

-include $(DEPS)
