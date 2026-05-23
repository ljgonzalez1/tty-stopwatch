#include "Options.h"

#include <cctype>
#include <cstring>

namespace stopwatch {
namespace {

// Parses a duration string like "1h30m45s", "90s", "5m", "2h", "30m5h".
// Units h/H, m/M, s/S can appear in any order; numbers may be of any width.
// No whitespace allowed. Returns true on success.
bool parse_duration(const std::string& s, std::chrono::milliseconds& out) {
    if (s.empty()) return false;

    long long total_ms   = 0;
    long long acc        = 0;
    bool      have_digits = false;
    bool      any_unit   = false;

    for (char c : s) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            if (acc > 1'000'000'000LL) return false;  // sanity guard
            acc = acc * 10 + (c - '0');
            have_digits = true;
            continue;
        }

        if (!have_digits) return false;  // unit without preceding number

        long long mult = 0;
        switch (c) {
            case 'h': case 'H': mult = 3'600'000LL; break;
            case 'm': case 'M': mult =    60'000LL; break;
            case 's': case 'S': mult =     1'000LL; break;
            default: return false;
        }
        total_ms   += acc * mult;
        acc         = 0;
        have_digits = false;
        any_unit    = true;
    }
    // Trailing digits without a unit, or a string with no unit at all,
    // are rejected. Zero or negative totals are also rejected.
    if (have_digits || !any_unit || total_ms <= 0) return false;

    out = std::chrono::milliseconds(total_ms);
    return true;
}

bool apply_short(char c, Options& o) {
    switch (c) {
        case 'h': o.show_help     = true; return true;
        case 'H': o.no_hour       = true; return true;
        case 'S': o.seconds_only  = true; return true;
        case 'B': o.blink         = true; return true;
        case 'n': o.no_color      = true; return true;
        case 'c': o.show_clock    = true; return true;
        case 's': o.screensaver   = true; return true;
        case 'q': o.quit_on_press = true; return true;
        case 'r': o.reverse       = true; return true;
        case 'a': o.no_output     = true; return true;
        default:  return false;
    }
}

bool apply_long(const std::string& name, Options& o) {
    if (name == "help" || name == "h")  { o.show_help     = true; return true; }
    if (name == "no-hour")              { o.no_hour       = true; return true; }
    if (name == "seconds-only")         { o.seconds_only  = true; return true; }
    if (name == "blink")                { o.blink         = true; return true; }
    if (name == "no-color")             { o.no_color      = true; return true; }
    if (name == "clock")                { o.show_clock    = true; return true; }
    if (name == "screensaver")          { o.screensaver   = true; return true; }
    if (name == "quit-on-press")        { o.quit_on_press = true; return true; }
    if (name == "reverse")              { o.reverse       = true; return true; }
    if (name == "no-output")            { o.no_output     = true; return true; }
    return false;
}

// First pass: detect -h / --h / --help anywhere in argv. In bundled short
// options we stop scanning at 't' because the rest of the bundle is the
// timer's value, not flags.
bool detect_help_request(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "-h" || a == "--h" || a == "--help") return true;
        if (a.size() > 1 && a[0] == '-' && a[1] != '-') {
            for (size_t k = 1; k < a.size(); ++k) {
                if (a[k] == 'h') return true;
                if (a[k] == 't') break;
            }
        }
    }
    return false;
}

} // namespace

Options OptionParser::parse(int argc, char* argv[]) {
    Options opts;

    if (detect_help_request(argc, argv)) {
        opts.show_help = true;
        return opts;
    }

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];

        // Long option.
        if (a.size() > 2 && a[0] == '-' && a[1] == '-') {
            std::string name    = a.substr(2);
            const auto  eq      = name.find('=');
            std::string opt_key = name.substr(0, eq);

            if (opt_key == "timer" || opt_key == "t") {
                opts.timer_mode = true;
                std::string value;
                if (eq != std::string::npos) {
                    value = name.substr(eq + 1);
                } else if (i + 1 < argc) {
                    value = argv[++i];
                } else {
                    opts.error_message = "missing value for --timer";
                    return opts;
                }
                if (!parse_duration(value, opts.timer_duration)) {
                    opts.error_message = "invalid duration: " + value;
                    return opts;
                }
                continue;
            }

            if (!apply_long(opt_key, opts)) {
                opts.error_message = "unknown option: " + a;
                return opts;
            }
            continue;
        }

        // Short options (possibly bundled, e.g. -nBct 5m).
        if (a.size() > 1 && a[0] == '-') {
            for (size_t k = 1; k < a.size(); ++k) {
                char c = a[k];
                if (c == 't') {
                    opts.timer_mode = true;
                    std::string value;
                    if (k + 1 < a.size()) {
                        value = a.substr(k + 1);     // -t5m or -nt5m
                    } else if (i + 1 < argc) {
                        value = argv[++i];           // -t 5m or -nt 5m
                    } else {
                        opts.error_message = "missing value for -t";
                        return opts;
                    }
                    if (!parse_duration(value, opts.timer_duration)) {
                        opts.error_message = "invalid duration: " + value;
                        return opts;
                    }
                    break;  // -t consumes the rest of the bundle
                }
                if (!apply_short(c, opts)) {
                    opts.error_message = std::string("unknown option: -") + c;
                    return opts;
                }
            }
            continue;
        }

        // Bare dash or stray positional argument.
        opts.error_message = "unexpected argument: " + a;
        return opts;
    }

    // Validation of mutually exclusive / nonsensical combinations.
    if (opts.no_hour && opts.seconds_only) {
        opts.error_message = "--no-hour and --seconds-only cannot be combined";
        return opts;
    }
    if (opts.reverse && !opts.timer_mode) {
        opts.error_message = "--reverse requires --timer";
        return opts;
    }

    // Screensaver implies quit-on-press and silent exit.
    if (opts.screensaver) {
        opts.quit_on_press = true;
        opts.no_output     = true;
    }
    return opts;
}

} // namespace stopwatch
