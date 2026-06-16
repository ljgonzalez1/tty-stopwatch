#!/bin/sh
# packaging/macos/bundle.sh
#
# Assemble a macOS .app bundle around an already-compiled tty-stopwatch binary,
# ad-hoc sign it so it launches without the "unidentified developer" wall on
# the machine that built it, and wrap it in a drag-to-install .dmg.
#
# Every input arrives through the environment so the Makefile remains the
# single source of truth for names and versions:
#
#   BIN        path to the compiled binary (plays double duty: CLI + launcher)
#   APP_NAME   bundle name without the .app suffix (e.g. TTY-Stopwatch)
#   BUNDLE_ID  reverse-DNS identifier (e.g. cl.ljgonzalez.tty-stopwatch)
#   VERSION    marketing/build version (e.g. 1.4.0)
#   ICNS       path to the .icns icon
#   PLIST_IN   path to the Info.plist template
#   VOL_NAME   mounted DMG volume name
#   DMG_PATH   output .dmg path
#
# The script is intentionally POSIX sh and uses only tools that ship with
# macOS (codesign, hdiutil), so there is nothing to install to build a release.
set -eu

require() {
    eval "value=\${$1:-}"
    [ -n "$value" ] || { echo "bundle.sh: $1 is required" >&2; exit 1; }
}
require BIN
require APP_NAME
require BUNDLE_ID
require VERSION
require ICNS
require PLIST_IN
require VOL_NAME
require DMG_PATH

case "$(uname -s)" in
    Darwin) ;;
    *) echo "bundle.sh: macOS-only (needs codesign and hdiutil)." >&2; exit 1 ;;
esac

[ -f "$BIN" ]      || { echo "bundle.sh: binary not found: $BIN" >&2; exit 1; }
[ -f "$ICNS" ]     || { echo "bundle.sh: icon not found: $ICNS" >&2; exit 1; }
[ -f "$PLIST_IN" ] || { echo "bundle.sh: plist template not found: $PLIST_IN" >&2; exit 1; }

executable_name=$(basename "$BIN")

workdir=$(mktemp -d)
trap 'rm -rf "$workdir"' EXIT

# --- 1. Bundle skeleton -----------------------------------------------------
app="$workdir/$APP_NAME.app"
contents="$app/Contents"
mkdir -p "$contents/MacOS" "$contents/Resources"

# The real binary is the bundle executable. Launched from Finder it has no
# controlling tty and re-execs itself inside Terminal.app; launched from a
# shell it runs the stopwatch directly.
install -m 0755 "$BIN" "$contents/MacOS/$executable_name"

cp "$ICNS" "$contents/Resources/$APP_NAME.icns"

# Info.plist from the template; the icon key carries no extension by convention.
sed -e "s/@APP_NAME@/$APP_NAME/g" \
    -e "s/@EXECUTABLE_NAME@/$executable_name/g" \
    -e "s/@BUNDLE_ID@/$BUNDLE_ID/g" \
    -e "s/@VERSION@/$VERSION/g" \
    -e "s/@ICON_FILE@/$APP_NAME/g" \
    "$PLIST_IN" > "$contents/Info.plist"

# Legacy type/creator stub; harmless and still expected by some tooling.
printf 'APPL????' > "$contents/PkgInfo"

# --- 2. Ad-hoc signature ----------------------------------------------------
# The "-" identity is an ad-hoc signature: no certificate, no Apple account.
# It is mandatory for arm64 binaries to run at all, and it lets a locally built
# app open without the unsigned-binary wall. Sign the nested executable first,
# then seal the whole bundle.
codesign --force --sign - --identifier "$BUNDLE_ID" "$contents/MacOS/$executable_name"
codesign --force --sign - --identifier "$BUNDLE_ID" "$app"
codesign --verify --deep --strict "$app"

# --- 3. Disk image ----------------------------------------------------------
# Stage the .app next to an /Applications symlink so the mounted image offers
# the familiar drag-to-install gesture.
stage="$workdir/dmg"
mkdir -p "$stage"
cp -R "$app" "$stage/"
ln -s /Applications "$stage/Applications"

mkdir -p "$(dirname "$DMG_PATH")"
rm -f "$DMG_PATH"
hdiutil create \
    -volname "$VOL_NAME" \
    -srcfolder "$stage" \
    -fs HFS+ \
    -format UDZO \
    -ov \
    "$DMG_PATH" >/dev/null

echo ""
echo "Created $DMG_PATH"
echo "  Install: open the .dmg and drag $APP_NAME.app onto Applications."
echo "  Remove:  drag $APP_NAME.app from Applications to the Trash."
