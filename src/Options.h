#pragma once

#include <chrono>
#include <string>

namespace stopwatch {

/**
 * @brief Parsed command-line options.
 *
 * A plain data aggregate of flags so callers can read them directly without
 * going through accessors. Validation and population are the responsibility
 * of OptionParser.
 */
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

    bool timer_mode    = false;  // -t, --timer (fixed-duration countdown)
    std::chrono::milliseconds timer_duration{0};

    // -u, --until: count down to the next occurrence of a 24-hour wall-clock
    // time. The concrete remaining duration is computed at run time from the
    // system clock, so only the requested time-of-day is stored here.
    bool until_mode = false;
    int  until_hour = 0;  // 0..23
    int  until_min  = 0;  // 0..59
    int  until_sec  = 0;  // 0..59

    // Set by OptionParser when something went wrong. A non-empty message
    // makes the caller print help and exit non-zero.
    std::string error_message;
    bool has_error() const { return !error_message.empty(); }
};

/**
 * @brief Stateless command-line parser for Options.
 */
class OptionParser {
public:
    // Parses argv[1..argc). Never throws. On error, returns Options whose
    // error_message is non-empty. -h / --h / --help anywhere in argv take
    // precedence over both errors and validation.
    static Options parse(int argc, char* argv[]);
};

}
