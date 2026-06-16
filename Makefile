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
# Build variants (macOS):
#   make MACOS_UNIVERSAL=1   Single universal binary (x86_64 + arm64).
#   make MACOS_ARCH=x86_64   Thin binary for Intel only.
#   make MACOS_ARCH=arm64    Thin binary for Apple Silicon only.
#
# Debian package (Linux, Debian-based hosts):
#   make deb             Build dist/<pkg>_<version>_<arch>.deb for the host
#                        architecture (from `dpkg --print-architecture`).
#                        Needs no root and only the `dpkg` package, which is
#                        always present on Debian-based systems.
#
# macOS disk image (.dmg, macOS hosts only):
#   make dmg             Universal .app inside a drag-to-install .dmg (Intel +
#                        Apple Silicon in one image).
#   make dmg-intel       Same, but a thin x86_64 (Intel) image.
#   make dmg-arm64       Same, but a thin arm64 (Apple Silicon) image.
#
# A plain `make` with no variables auto-detects the host operating system and
# compiles a native binary for the current OS and CPU architecture.
#
# Convenience phony targets `static`, `portable`, `macos-universal`, `deb`,
# `dmg`, `dmg-intel`, and `dmg-arm64` re-invoke make with the matching
# variables set or run the corresponding packaging steps.

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

# --- macOS single-architecture binary ---------------------------------------
# MACOS_ARCH=x86_64 or MACOS_ARCH=arm64 produces a thin binary for one slice,
# used by the dmg-intel / dmg-arm64 targets.
ifneq ($(MACOS_ARCH),)
  CXXFLAGS += -arch $(MACOS_ARCH) -mmacosx-version-min=11.0
  LDFLAGS  += -arch $(MACOS_ARCH) -mmacosx-version-min=11.0
endif

SRC_DIR := src
OBJ_DIR := build
BIN     := tty-stopwatch

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS    := $(OBJECTS:.o=.d)

BINDIR  := $(DESTDIR)$(PREFIX)/bin

# --- Shared package metadata ------------------------------------------------
PKG_NAME         := tty-stopwatch
VERSION_UPSTREAM := 1.4.0
DIST_DIR         := dist

# --- Debian package metadata ------------------------------------------------
DEB_REVISION     := 1
DEB_VERSION      := $(VERSION_UPSTREAM)-$(DEB_REVISION)
DEB_SECTION      := utils
DEB_PRIORITY     := optional
DEB_MAINTAINER   := ljgonzalez1 <luis.alejandro@ljgonzalez.cl>
DEB_HOMEPAGE     := https://github.com/ljgonzalez1/tty-stopwatch

# --- macOS .app / .dmg metadata ---------------------------------------------
MACOS_APP_NAME   := TTY-Stopwatch
MACOS_BUNDLE_ID  := cl.ljgonzalez.tty-stopwatch
MACOS_VOL_NAME   := TTY Stopwatch
MACOS_ICNS       := assets/logo/icns/TTY-Stopwatch.icns
MACOS_PLIST_IN   := packaging/macos/Info.plist.in
MACOS_BUNDLE_SH  := packaging/macos/bundle.sh

.PHONY: all clean run install uninstall static portable macos-universal deb \
        dmg dmg-intel dmg-arm64 macos-package

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

# Build an installable .deb for the host architecture into $(DIST_DIR).
# Needs no root: the staging tree is assembled in a private temporary
# directory (removed automatically, even on failure) and dpkg-deb
# --root-owner-group records root:root ownership without elevated
# privileges. Only the `dpkg` package is required. If the target is ever run
# under sudo, dist/, build/ and the binary are handed back to the invoking
# user so nothing in the working tree is left owned by root.
deb:
	@command -v dpkg-deb >/dev/null 2>&1 || { echo "make deb: dpkg-deb not found; this target needs a Debian-based system." >&2; exit 1; }
	@set -e; umask 022; \
	arch=$$(dpkg --print-architecture); \
	staging=$$(mktemp -d); \
	trap 'rm -rf "$$staging"' EXIT; \
	debfile="$(DIST_DIR)/$(PKG_NAME)_$(DEB_VERSION)_$$arch.deb"; \
	echo "Building $$debfile (architecture: $$arch) ..."; \
	rm -f $(BIN); \
	$(MAKE) --no-print-directory PORTABLE=1 $(BIN); \
	install -D -m 0755 $(BIN) "$$staging/usr/bin/$(PKG_NAME)"; \
	install -D -m 0644 README.md "$$staging/usr/share/doc/$(PKG_NAME)/README.md"; \
	docdir="$$staging/usr/share/doc/$(PKG_NAME)"; \
	printf '%s\n' "$(PKG_NAME) ($(DEB_VERSION)) unstable; urgency=medium" "" "  * Packaged build of upstream $(VERSION_UPSTREAM)." "" " -- $(DEB_MAINTAINER)  $$(date -R)" > "$$docdir/changelog.Debian"; \
	gzip -9n -f "$$docdir/changelog.Debian"; \
	printf '%s\n' "Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/" "Upstream-Name: $(PKG_NAME)" "Source: $(DEB_HOMEPAGE)" "" "Files: *" "Copyright: $(DEB_MAINTAINER)" "License: refer to the license shipped with the upstream source." > "$$docdir/copyright"; \
	install -d -m 0755 "$$staging/DEBIAN"; \
	isize=$$(du -ks "$$staging/usr" | cut -f1); \
	printf '%s\n' "Package: $(PKG_NAME)" "Version: $(DEB_VERSION)" "Section: $(DEB_SECTION)" "Priority: $(DEB_PRIORITY)" "Architecture: $$arch" "Maintainer: $(DEB_MAINTAINER)" "Homepage: $(DEB_HOMEPAGE)" "Depends: libc6" "Installed-Size: $$isize" "Description: terminal stopwatch and countdown timer with big digits" " tty-stopwatch is a self-contained terminal stopwatch and countdown" " timer that draws a large block-digit clock using only ANSI escape" " sequences and POSIX terminal control, with no ncurses or other" " runtime dependency." " ." " It offers a free-running stopwatch, a fixed-duration countdown, and" " a wall-clock countdown to the next occurrence of a 24-hour time of" " day. Desktop notifications are delivered over D-Bus on Linux, with" " a terminal bell as a universal fallback." > "$$staging/DEBIAN/control"; \
	( cd "$$staging" && find usr -type f -print0 | LC_ALL=C sort -z | xargs -0 md5sum > DEBIAN/md5sums ); \
	find "$$staging" -type d -exec chmod 0755 {} +; \
	chmod 0644 "$$staging/DEBIAN/control" "$$staging/DEBIAN/md5sums" "$$docdir/copyright" "$$docdir/changelog.Debian.gz" "$$docdir/README.md"; \
	chmod 0755 "$$staging/usr/bin/$(PKG_NAME)"; \
	mkdir -p "$(DIST_DIR)"; \
	rm -f "$$debfile"; \
	dpkg-deb --root-owner-group --build "$$staging" "$$debfile" >/dev/null; \
	if [ "$$(id -u)" = "0" ] && [ -n "$$SUDO_UID" ]; then chown -R "$$SUDO_UID:$$SUDO_GID" "$(DIST_DIR)" "$(OBJ_DIR)" $(BIN) 2>/dev/null || true; fi; \
	echo ""; \
	echo "Created $$debfile"; \
	echo "  Install:  sudo apt install ./$$debfile"; \
	echo "  Remove:   sudo apt remove $(PKG_NAME)"

# --- macOS disk images ------------------------------------------------------
# The three user-facing targets only differ in which architecture they build;
# each re-invokes the shared `macos-package` recipe with the right compiler
# arch variables and a label for the output file name. Architecture flags
# change the object files, so `macos-package` starts from a clean build.
dmg:
	@$(MAKE) --no-print-directory macos-package MACOS_UNIVERSAL=1 DMG_LABEL=universal

dmg-intel:
	@$(MAKE) --no-print-directory macos-package MACOS_ARCH=x86_64 DMG_LABEL=x86_64

dmg-arm64:
	@$(MAKE) --no-print-directory macos-package MACOS_ARCH=arm64 DMG_LABEL=arm64

# Internal: compile the binary for the selected arch and hand it to the
# packaging script, which assembles the .app, ad-hoc signs it, and wraps it in
# a drag-to-install .dmg under $(DIST_DIR).
macos-package:
	@command -v hdiutil >/dev/null 2>&1 || { echo "make $(DMG_LABEL:%=dmg): macOS-only target (hdiutil not found)." >&2; exit 1; }
	@set -e; \
	rm -rf "$(OBJ_DIR)" "$(BIN)"; \
	$(MAKE) --no-print-directory $(BIN); \
	mkdir -p "$(DIST_DIR)"; \
	dmg="$(DIST_DIR)/$(PKG_NAME)_$(VERSION_UPSTREAM)_$(DMG_LABEL).dmg"; \
	BIN="$(BIN)" \
	APP_NAME="$(MACOS_APP_NAME)" \
	BUNDLE_ID="$(MACOS_BUNDLE_ID)" \
	VERSION="$(VERSION_UPSTREAM)" \
	ICNS="$(MACOS_ICNS)" \
	PLIST_IN="$(MACOS_PLIST_IN)" \
	VOL_NAME="$(MACOS_VOL_NAME)" \
	DMG_PATH="$$dmg" \
	/bin/sh "$(MACOS_BUNDLE_SH)"

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
	rm -rf $(OBJ_DIR) $(BIN) $(DIST_DIR)

-include $(DEPS)
