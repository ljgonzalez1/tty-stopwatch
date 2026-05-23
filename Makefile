# tty-stopwatch
# Cross-platform Makefile for Linux (amd64, arm64, armhf) and macOS (amd64,
# arm64). Indented with TABs as required by GNU make.

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  PREFIX ?= /usr/local
  LDLIBS := -lncurses
else
  PREFIX ?= /usr
  LDLIBS := -lncursesw
endif

CXX      ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
LDFLAGS  ?=

SRC_DIR := src
OBJ_DIR := build
BIN     := tty-stopwatch

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS    := $(OBJECTS:.o=.d)

BINDIR  := $(DESTDIR)$(PREFIX)/bin

.PHONY: all clean run install uninstall

all: $(BIN)

$(BIN): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -MMD -MP -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

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
