# tty-stopwatch

A terminal stopwatch and countdown timer with a big-digit clock face,
written in modern C++17 on top of `ncurses`. In the spirit of
[`tty-clock`][1], but for measuring elapsed time instead of displaying
the wall clock.

[1]: https://github.com/xorg62/tty-clock

## Features

- **Big block-digit display** with automatic horizontal and vertical centering.
- **Stopwatch mode** (counts up indefinitely) and **countdown mode** (`-t`).
- **Pipe-friendly output**: the final time is written to `stdout` in
  `HH:MM:SS.cs` format; the big clock itself is drawn on `/dev/tty`, so
  output redirection works.
- **Desktop notifications** when a countdown finishes (`notify-send` on
  Linux, `osascript` on macOS), with a terminal bell as a fallback.
- **Many display modes**: hide hours, seconds-only, blinking colons,
  optional system clock at the top, monochrome.
- **Screensaver mode** with a 0.5 FPS refresh and quit-on-keypress.
- **Auto-launches a terminal** when double-clicked from a graphical file
  manager (Linux only).
- **Compact text fallback** when the terminal is too small for the big
  digits; switches back automatically when there's room.
- **No locale dependency.** All rendering uses ASCII; output uses 24-hour
  numeric time and never depends on `LANG` / `LC_*`.
- **Cross-platform**: Linux (amd64, arm64, armhf) and macOS (amd64, arm64).

## Dependencies

### Debian / Ubuntu (and other apt-based distributions)

```bash
sudo apt update
sudo apt install build-essential libncurses-dev
```

For desktop notifications, also install `libnotify-bin`:

```bash
sudo apt install libnotify-bin
```

### Other Linux distributions

You need a C++17 toolchain, GNU make, and the wide-character `ncurses`
development headers:

- Fedora / RHEL: `sudo dnf install gcc-c++ make ncurses-devel`
- Arch: `sudo pacman -S base-devel ncurses`
- Alpine: `apk add g++ make ncurses-dev`

### macOS (amd64 / arm64)

The Xcode Command Line Tools provide `clang++`, `make`, and the system
`ncurses` headers:

```bash
xcode-select --install
```

Desktop notifications use the built-in `osascript`.

## Build and install

```bash
git clone https://git.ljgonzalez.cl/ljgonzalez/tty-stopwatch
cd tty-stopwatch
make
sudo make install
```

On Linux this installs `tty-stopwatch` to `/usr/bin/`. On macOS it
installs to `/usr/local/bin/`. To pick a different prefix:

```bash
sudo make install PREFIX=/opt/local
```

To remove the installed binary:

```bash
sudo make uninstall
```

To clean intermediate build artifacts:

```bash
make clean
```

The build is architecture-independent C++17. On Linux it works on amd64,
arm64, and armhf without any source changes; on macOS it works on both
Intel (amd64) and Apple Silicon (arm64).

## Usage

```text
tty-stopwatch [options]

Options:
    -h, --help              Show help and exit.
    -t, --timer DURATION    Run as countdown timer for DURATION.
    -r, --reverse           Show timer counting up instead of down (requires -t).
    -H, --no-hour           Hide hours; minutes grow without bound.
    -S, --seconds-only      Show seconds only, growing without bound.
    -B, --blink             Blink the colon separators each second.
    -c, --clock             Show a small system clock at the top.
    -n, --no-color          Render in monochrome.
    -q, --quit-on-press     Exit on any non-control key.
    -s, --screensaver       0.5 FPS refresh; implies -q and -a.
    -a, --no-output         Do not print the final time on exit.

Controls:
    SPACE, P                Pause / resume.
    R                       Reset to zero (paused).
    Q, Ctrl+C               Exit.
```

The stopwatch starts running automatically.

### Duration syntax (`-t`)

A duration combines hour (`h`), minute (`m`), and second (`s`) units in
any order, with no spaces. Both upper- and lower-case unit letters are
accepted.

| Input         | Meaning                                |
|---------------|----------------------------------------|
| `-t 1h30m45s` | 1 hour, 30 minutes, 45 seconds         |
| `-t 90s`      | 90 seconds                             |
| `-t 5m`       | 5 minutes                              |
| `-t 2h`       | 2 hours                                |
| `-t 30m5h`    | 5 hours, 30 minutes (any order)        |
| `-t 1H30M`    | 1 hour, 30 minutes (case-insensitive)  |

Durations of zero or with a trailing bare number (e.g. `1m30`) are
rejected.

### Output format

On exit, the program writes one line to `stdout` in `HH:MM:SS.cs`
format. The `HH` field is always at least two digits and grows naturally
beyond 99 if needed.

- In **stopwatch mode**, the elapsed time is printed.
- In **countdown mode** (`-t`):
  - If the timer **finished naturally**, the **original duration** is printed.
  - If the timer was **interrupted** (Q, Ctrl+C, SIGTERM, etc.), the
    **elapsed time** is printed instead.
- The `-a` / `--no-output` flag suppresses this output entirely.

```bash
# Five-minute Pomodoro; capture the resulting duration
duration=$(tty-stopwatch -t 5m)
echo "Worked for $duration"

# The big clock still renders on the terminal while stdout is redirected
tty-stopwatch -t 1m > timer.log
```

### Flag bundling

Short flags can be combined freely. The bundle is parsed left to right;
`-t` always consumes the rest of the current token (if any) or the next
argument as its value:

```bash
tty-stopwatch -nBc -t 5m       # -n -B -c -t 5m
tty-stopwatch -nBct 5m         # same as above
tty-stopwatch -nBct5m          # same again, value joined to -t
tty-stopwatch --blink -nc      # mix of long and short forms
tty-stopwatch -n -n -n -B      # redundancy is fine
```

Unknown flags, malformed durations, and contradictory combinations
(currently `-H` together with `-S`, or `-r` without `-t`) print the help
screen to stderr and exit with status 1. `-h`, `--h`, and `--help` take
precedence over any other validation, including errors.

### Examples

```bash
tty-stopwatch                          # plain stopwatch
tty-stopwatch -t 5m                    # 5-minute countdown
tty-stopwatch -t 1h30m -c -B           # 90-minute timer + clock + blink
tty-stopwatch -t 30s -r                # 30s timer shown counting up
tty-stopwatch -nS                      # monochrome, seconds only
tty-stopwatch -s -t 25m                # screensaver-style Pomodoro
elapsed=$(tty-stopwatch -t 10s)        # capture into a variable
```

## Behaviour notes

### Double-click launches (Linux only)

When `tty-stopwatch` is started without a controlling terminal — for
example by double-clicking the binary in a graphical file manager — it
re-executes itself inside the system's preferred terminal emulator. The
following terminals are tried, in order:

`x-terminal-emulator`, `gnome-terminal`, `konsole`, `xfce4-terminal`,
`mate-terminal`, `lxterminal`, `alacritty`, `kitty`, `terminator`,
`tilix`, `xterm`.

The terminal window closes as soon as `tty-stopwatch` exits.

On macOS this feature is intentionally disabled; run the program from
Terminal.app, iTerm2, or any other terminal you prefer.

### Resizing and small terminals

The display is recentered on every frame, so resizing the window or
changing font size always recenters the clock. If the terminal becomes
too small to hold the big-digit clock, the program falls back to a
single line of plain text showing only the current time, and switches
back to the big display automatically once there's room again.

### Notifications

When a countdown timer completes, the program issues:

1. A terminal bell (`\a`) written directly to `/dev/tty`.
2. A best-effort desktop notification via `notify-send` (Linux).
3. A best-effort macOS notification via `osascript`.

All three are attempted independently; missing tools cause no errors and
nothing leaks onto the screen mid-render.

## Project layout

```
tty-stopwatch/
├── Makefile
├── README.md
└── src/
    ├── Application.{h,cpp}      Main loop, input handling, glue
    ├── DigitFont.{h,cpp}        Bitmap glyphs for the big digits
    ├── Display.{h,cpp}          ncurses session, rendering
    ├── HelpScreen.{h,cpp}       Help text (with optional ANSI colors)
    ├── Notification.{h,cpp}     Bell + desktop notifications
    ├── Options.{h,cpp}          Command-line parser
    ├── Stopwatch.{h,cpp}        Pure timing model
    ├── TerminalLauncher.{h,cpp} Re-launch inside a terminal emulator
    ├── TimeFormatter.{h,cpp}    Duration decomposition and formatting
    └── main.cpp                 Entry point
```

Each module has a single, focused responsibility, with no I/O in the
model layer and no UI logic outside of `Display` and `HelpScreen`.
