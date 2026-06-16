#include "HelpScreen.h"

#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace stopwatch {
namespace HelpScreen {
namespace {

struct Palette {
    const char* reset;
    const char* heading;  // section titles
    const char* flag;     // flag names
    const char* arg;      // metavariables
    const char* example;  // example commands
};

Palette make_palette(bool color, std::FILE* out) {
    if (!color || !isatty(fileno(out))) {
        return {"", "", "", "", ""};
    }
    return {
        "\x1b[0m",        // reset
        "\x1b[1;36m",     // bold cyan
        "\x1b[1;32m",     // bold green
        "\x1b[33m",       // yellow
        "\x1b[33m"        // yellow
    };
}

void heading(std::FILE* out, const Palette& p, const char* text) {
    std::fprintf(out, "%s%s%s\n", p.heading, text, p.reset);
}

// Renders one option row with the short and long forms aligned in a uniform
// column. Alignment math uses the visible width only, so the row is rebuilt
// piece by piece rather than colorized in place.
void option(std::FILE* out, const Palette& p,
            const char* shortf, const char* longf,
            const char* metavar, const char* desc) {
    char left[80];
    if (metavar && *metavar) {
        std::snprintf(left, sizeof(left), "%s, %s %s", shortf, longf, metavar);
    } else {
        std::snprintf(left, sizeof(left), "%s, %s", shortf, longf);
    }
    const int width = 26;
    const int len   = static_cast<int>(std::strlen(left));
    const int pad   = (len < width) ? (width - len) : 1;

    if (metavar && *metavar) {
        std::fprintf(out, "    %s%s%s, %s%s%s %s%s%s%*s%s\n",
                     p.flag, shortf, p.reset,
                     p.flag, longf,  p.reset,
                     p.arg,  metavar, p.reset,
                     pad, "", desc);
    } else {
        std::fprintf(out, "    %s%s%s, %s%s%s%*s%s\n",
                     p.flag, shortf, p.reset,
                     p.flag, longf,  p.reset,
                     pad, "", desc);
    }
}

}

void print(std::FILE* out, bool color) {
    const Palette p = make_palette(color, out);

    std::fprintf(out, "%sUsage:%s tty-stopwatch [options]\n\n", p.heading, p.reset);

    heading(out, p, "Description:");
    std::fprintf(out, "    Terminal stopwatch and countdown timer with a big-digit clock.\n");
    std::fprintf(out, "    The timer auto-starts; use Space or P to pause.\n\n");

    heading(out, p, "Options:");
    option(out, p, "-h", "--help",          "",          "Show this help and exit.");
    option(out, p, "-t", "--timer",         "DURATION",  "Run as countdown for DURATION (see below).");
    option(out, p, "-u", "--until",         "HH:MM[:SS]","Count down to the next 24h wall-clock time.");
    option(out, p, "-r", "--reverse",       "",          "Show timer counting up (requires -t or -u).");
    option(out, p, "-H", "--no-hour",       "",          "Hide hours; minutes grow without bound.");
    option(out, p, "-S", "--seconds-only",  "",          "Show only seconds, growing without bound.");
    option(out, p, "-B", "--blink",         "",          "Blink the colon separators each second.");
    option(out, p, "-c", "--clock",         "",          "Show a small system clock at the top.");
    option(out, p, "-n", "--no-color",      "",          "Disable colors; render in monochrome.");
    option(out, p, "-q", "--quit-on-press", "",          "Exit on any non-control key.");
    option(out, p, "-s", "--screensaver",   "",          "0.5 FPS refresh; implies -q and -a.");
    option(out, p, "-a", "--no-output",     "",          "Do not print the final time on exit.");
    std::fputc('\n', out);

    heading(out, p, "Duration syntax (-t):");
    std::fprintf(out,
        "    Combine h/H, m/M and s/S units in any order, with no spaces.\n"
        "    Examples: %s1h30m45s%s, %s90s%s, %s5m%s, %s2h%s, %s30m5h%s, %s45m%s.\n\n",
        p.example, p.reset, p.example, p.reset, p.example, p.reset,
        p.example, p.reset, p.example, p.reset, p.example, p.reset);

    heading(out, p, "Wall-clock target (-u):");
    std::fprintf(out,
        "    Give a time of day in 24-hour %sHH:MM%s or %sHH:MM:SS%s form. The\n"
        "    countdown runs until the next occurrence of that time: if it has\n"
        "    already passed today, the target rolls over to tomorrow.\n"
        "    Examples: %s-u 20:32%s, %s--until 06:00:00%s, %s-u 9:05%s.\n\n",
        p.arg, p.reset, p.arg, p.reset,
        p.example, p.reset, p.example, p.reset, p.example, p.reset);

    heading(out, p, "Controls:");
    std::fprintf(out, "    %sSPACE%s, %sP%s              Pause / resume.\n",
                 p.flag, p.reset, p.flag, p.reset);
    std::fprintf(out, "    %sR%s                     Reset to zero (paused).\n",
                 p.flag, p.reset);
    std::fprintf(out, "    %sQ%s, %sCtrl+C%s            Exit.\n\n",
                 p.flag, p.reset, p.flag, p.reset);

    heading(out, p, "Output:");
    std::fprintf(out,
        "    On exit, the program writes the final time to stdout in\n"
        "    %sHH:MM:SS.cs%s format, suitable for piping. For a countdown\n"
        "    that finished naturally, the original duration is printed;\n"
        "    if interrupted, the elapsed time is printed instead. The big\n"
        "    clock itself is rendered on /dev/tty, so output redirection\n"
        "    is safe. Use %s-a%s to suppress this stdout output.\n\n",
        p.example, p.reset, p.flag, p.reset);

    heading(out, p, "Examples:");
    const struct { const char* cmd; const char* desc; } examples[] = {
        {"tty-stopwatch",                  "Plain stopwatch counting upward."},
        {"tty-stopwatch -t 5m",            "Five-minute countdown timer."},
        {"tty-stopwatch -u 20:32",         "Count down to the next 20:32."},
        {"tty-stopwatch -t 1h30m -c -B",   "90-minute timer with system clock and blinking colons."},
        {"tty-stopwatch -t 30s -r",        "30-second timer shown counting up."},
        {"tty-stopwatch -nS",              "Monochrome stopwatch, seconds only."},
        {"tty-stopwatch -s -t 25m",        "Screensaver-style 25-minute Pomodoro."},
        {"elapsed=$(tty-stopwatch -t 10s)","Capture the resulting duration in a shell variable."},
    };
    for (const auto& e : examples) {
        std::fprintf(out, "    %s$ %s%s\n        %s\n", p.example, e.cmd, p.reset, e.desc);
    }
    std::fputc('\n', out);

    std::fprintf(out, "Short flags can be bundled: %s-nBc -t 5m%s, %s-nBct5m%s, %s--blink -nc%s.\n",
                 p.example, p.reset, p.example, p.reset, p.example, p.reset);
}

}
}

