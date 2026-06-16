# macOS packaging (.app + .dmg)

This directory holds everything needed to turn the compiled `tty-stopwatch`
binary into a double-clickable macOS application wrapped in a drag-to-install
disk image. The rest of the project is untouched and still builds and runs the
same way on Linux.

## Files

- `Info.plist.in` — the bundle's property-list template. Placeholders such as
  `@VERSION@` and `@BUNDLE_ID@` are filled in at build time from values defined
  once in the top-level `Makefile`, so the metadata can never drift out of sync.
- `bundle.sh` — POSIX `sh` script that assembles the `.app`, ad-hoc signs it,
  and builds the `.dmg`. It uses only tools that ship with macOS (`codesign`,
  `hdiutil`), so there is nothing extra to install.

## Requirements

- macOS 11 (Big Sur) or newer.
- The Xcode Command Line Tools (`xcode-select --install`), which provide the
  C++ compiler, `codesign`, and `hdiutil`.

## Where the logo goes

The bundle uses the icon the `Makefile` already references:

```
assets/logo/icns/TTY-Stopwatch.icns
```

That is the only image the `.app` needs — an `.icns` already contains every
resolution macOS asks for, so the individual PNGs under `assets/logo/png/` are
not used by the bundle (keep them for other purposes if you like). If you ever
regenerate the icon, overwrite that one `.icns` file and rebuild; no other
change is required.

## Building a disk image

Run any of these from the project root:

```
make dmg          # universal: one image that runs on Intel and Apple Silicon
make dmg-intel    # thin x86_64 image (Intel Macs)
make dmg-arm64    # thin arm64 image (Apple Silicon Macs)
```

Each target compiles the matching binary from a clean state and writes the
image to:

```
dist/tty-stopwatch_1.4.0_<label>.dmg     # label = universal | x86_64 | arm64
```

`make dmg` alone is enough to cover every modern Mac; the per-architecture
targets exist only when you want a smaller, single-slice image.

## Installing and removing

Open the `.dmg` and drag **TTY-Stopwatch.app** onto the **Applications**
shortcut in the same window. The app then appears in Launchpad and the
Applications folder like any other program. To uninstall, drag it from
Applications to the Trash — there is nothing else to clean up.

Double-clicking the app opens a Terminal window and starts the stopwatch,
exactly as running the binary from a shell would. The same binary still works
as a normal command-line tool when invoked from a terminal (with all the usual
flags); the bundle just lets Finder launch it too.

## First launch: the Automation prompt

The first time the app starts, macOS asks whether **TTY-Stopwatch** may control
**Terminal**. This is expected: the app opens a Terminal window to draw the
big-digit clock, and that requires Automation permission. Click **OK** (or allow
it later under System Settings → Privacy & Security → Automation). The prompt
text comes from the `NSAppleEventsUsageDescription` key in `Info.plist`.

## Why ad-hoc signing

`bundle.sh` signs the app with the ad-hoc identity (`codesign --sign -`): no
Apple Developer account, no certificate. Two reasons:

- Apple Silicon (arm64) binaries must carry at least an ad-hoc signature to run
  at all.
- On the machine that built it, an ad-hoc-signed, unquarantined app opens
  without the "unidentified developer" wall, which is exactly what you want for
  a locally built tool.

## Sharing the image with others

A `.dmg` you build locally is not quarantined on your own machine, so it opens
freely there. But anything **downloaded** from the internet (including a `.dmg`
someone else fetches) gets a `com.apple.quarantine` flag, and an ad-hoc
signature is not enough to satisfy Gatekeeper for downloaded apps. If you send
the image to someone else, tell them to clear the flag once after copying the
app to Applications:

```
xattr -dr com.apple.quarantine "/Applications/TTY-Stopwatch.app"
```

Distributing without that step would require a paid Apple Developer ID
signature plus notarization, which is outside the scope of this local build.
