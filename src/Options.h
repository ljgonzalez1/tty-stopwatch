#pragma once

#include <chrono>
#include <string>

namespace stopwatch {

// Parsed command-line options. A single struct with plain flags so callers
// can read them without going through accessors.
struct Options {
    bool show_help     = false;  // -h, --help, --h
    bool no_hour       = false;  // -H, --no-hour
    bool blink         = false;  // -B, --blink
    bool no_color      = false;  // -n, --no-color
    bool show_clock    = false;  // -c, --clock
    bool seconds_only  = false;  // -S, --seconds-only
    bool screensaver   = false;  // -s, --screensaver
    bool quit_on_press = false;  // -q, --quit-on-press
    bool reverse       = false;  // -r, --reverse
    bool no_output     = false;  // -a, --no-output
    bool timer_mode    = false;  // -t, --timer

    std::chrono::milliseconds timer_duration{0};

    // Filled in by OptionParser when something went wrong. The presence of
    // an error message makes the caller print help and exit non-zero.
    std::string error_message;
    bool has_error() const { return !error_message.empty(); }
};

class OptionParser {
public:
    // Parses argv[1..argc). Never throws. On error, returns Options whose
    // error_message is non-empty. -h / --h / --help anywhere in argv take
    // precedence over both errors and validation.
    static Options parse(int argc, char* argv[]);
};

} // namespace stopwatch
