#include "Display.h"

#include "TimeFormatter.h"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <string>

namespace stopwatch {
namespace {

constexpr int kColorRunning = 1;
constexpr int kColorPaused  = 2;
constexpr int kColorAccent  = 3;

constexpr int kInputTimeoutMs       = 33;     // ~30 FPS key polling
constexpr int kRenderIntervalNormal = 33;     // ~30 FPS rendering
constexpr int kRenderIntervalSaver  = 2000;   // 0.5 FPS for screensaver

void mvaddstr_centered(int row, int cols, const char* text) {
    const int len = static_cast<int>(std::strlen(text));
    int col = (cols - len) / 2;
    if (col < 0) col = 0;
    mvaddstr(row, col, text);
}

// Zero-pad on the left so the field is at least two characters wide.
std::string pad2(long long value) {
    std::string s = std::to_string(value);
    if (s.size() < 2) s.insert(s.begin(), '0');
    return s;
}

} // namespace

Display::Display(const Options& opts)
    : opts_(opts),
      supports_color_(false),
      screen_(nullptr),
      tty_in_(nullptr),
      tty_out_(nullptr) {

    // Prefer /dev/tty so stdout (where we'll print the final time) and
    // stdin (which may be redirected) don't interfere with rendering.
    tty_in_  = std::fopen("/dev/tty", "r");
    tty_out_ = std::fopen("/dev/tty", "w");

    if (tty_in_ && tty_out_) {
        screen_ = newterm(nullptr, tty_out_, tty_in_);
        if (screen_) {
            set_term(screen_);
        }
    }
    if (!screen_) {
        if (tty_in_)  { std::fclose(tty_in_);  tty_in_  = nullptr; }
        if (tty_out_) { std::fclose(tty_out_); tty_out_ = nullptr; }
        initscr();
    }

    raw();                          // capture Ctrl+C as a regular keypress
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(input_timeout_ms());

    if (!opts_.no_color && has_colors()) {
        supports_color_ = true;
        start_color();
        use_default_colors();
        init_pair(kColorRunning, COLOR_GREEN,  -1);
        init_pair(kColorPaused,  COLOR_YELLOW, -1);
        init_pair(kColorAccent,  COLOR_CYAN,   -1);
    }
}

Display::~Display() {
    endwin();
    if (screen_)  delscreen(screen_);
    if (tty_in_)  std::fclose(tty_in_);
    if (tty_out_) std::fclose(tty_out_);
}

int Display::input_timeout_ms()   const { return kInputTimeoutMs; }
int Display::render_interval_ms() const {
    return opts_.screensaver ? kRenderIntervalSaver : kRenderIntervalNormal;
}

int Display::color_pair_for(Stopwatch::State state) const {
    return state == Stopwatch::State::Running ? kColorRunning : kColorPaused;
}

std::vector<Display::Token>
Display::build_tokens(std::chrono::steady_clock::duration t) const {
    std::vector<Token> out;
    const TimeParts p = decompose(t);

    auto push_digit = [&](int d) {
        out.push_back({Token::Type::Digit, d});
    };
    auto push_string_digits = [&](const std::string& s) {
        for (char c : s) push_digit(c - '0');
    };

    if (opts_.seconds_only) {
        const long long secs = static_cast<long long>(p.hours) * 3600LL
                             + static_cast<long long>(p.minutes) * 60LL
                             + p.seconds;
        push_string_digits(pad2(secs));
        out.push_back({Token::Type::Dot, 0});
        push_digit(p.centiseconds / 10);
        push_digit(p.centiseconds % 10);
        return out;
    }

    if (opts_.no_hour) {
        const long long mins = static_cast<long long>(p.hours) * 60LL + p.minutes;
        push_string_digits(pad2(mins));
        out.push_back({Token::Type::Colon, 0});
        push_digit(p.seconds / 10);
        push_digit(p.seconds % 10);
        out.push_back({Token::Type::Dot, 0});
        push_digit(p.centiseconds / 10);
        push_digit(p.centiseconds % 10);
        return out;
    }

    // Default HH:MM:SS.cs, with hours growing past 99 if necessary.
    push_string_digits(pad2(p.hours));
    out.push_back({Token::Type::Colon, 0});
    push_digit(p.minutes / 10);
    push_digit(p.minutes % 10);
    out.push_back({Token::Type::Colon, 0});
    push_digit(p.seconds / 10);
    push_digit(p.seconds % 10);
    out.push_back({Token::Type::Dot, 0});
    push_digit(p.centiseconds / 10);
    push_digit(p.centiseconds % 10);
    return out;
}

const DigitGlyph& Display::glyph_for(const Token& tok, bool blink_off) const {
    switch (tok.type) {
        case Token::Type::Digit: return DigitFont::digit(tok.value);
        case Token::Type::Colon: return blink_off ? DigitFont::blank()
                                                  : DigitFont::colon();
        case Token::Type::Dot:   return DigitFont::dot();
    }
    return DigitFont::blank();
}

int Display::sequence_width(const std::vector<Token>& tokens) const {
    // Width is independent of blink state because the blank colon has the
    // same width as the visible colon, so the clock never shifts sideways.
    int w = 0;
    for (size_t i = 0; i < tokens.size(); ++i) {
        w += glyph_for(tokens[i], /*blink_off=*/false).width;
        if (i + 1 < tokens.size()) w += 1;  // single-column gap
    }
    return w;
}

void Display::draw_big(const std::vector<Token>& tokens, bool blink_off,
                       int top, int left, Stopwatch::State state) {
    const attr_t on_attr = supports_color_
        ? (COLOR_PAIR(color_pair_for(state)) | A_REVERSE)
        : A_REVERSE;

    int col = left;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const DigitGlyph& g = glyph_for(tokens[i], blink_off);
        for (int r = 0; r < DigitGlyph::kRows; ++r) {
            const std::uint8_t bits = g.rows[r];
            for (int c = 0; c < g.width; ++c) {
                const bool on = (bits >> (g.width - 1 - c)) & 1;
                if (on) {
                    attron(on_attr);
                    mvaddch(top + r, col + c, ' ');
                    attroff(on_attr);
                }
            }
        }
        col += g.width + 1;
    }
}

void Display::draw_status(Stopwatch::State state, int row, int cols) {
    if (row < 0) return;
    const char* label = (state == Stopwatch::State::Running)
                            ? "[ RUNNING ]"
                            : "[ PAUSED  ]";
    const attr_t attrs = supports_color_
        ? (COLOR_PAIR(color_pair_for(state)) | A_BOLD)
        : A_BOLD;
    attron(attrs);
    mvaddstr_centered(row, cols, label);
    attroff(attrs);
}

void Display::draw_clock(int row, int cols) {
    // System clock in 24-hour format. No localization; HH:MM:SS comes
    // straight from the broken-down local time, so output is identical
    // across locales.
    std::time_t now = std::time(nullptr);
    std::tm     tm_local{};
    localtime_r(&now, &tm_local);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                  tm_local.tm_hour, tm_local.tm_min, tm_local.tm_sec);
    const attr_t attrs = supports_color_
        ? (COLOR_PAIR(kColorAccent) | A_BOLD)
        : A_BOLD;
    attron(attrs);
    mvaddstr_centered(row, cols, buf);
    attroff(attrs);
}

void Display::draw_help(int row, int cols) {
    const char* help = "[Space/P] pause   [R] reset   [Q] quit";
    const attr_t attrs = supports_color_
        ? (COLOR_PAIR(kColorAccent) | A_DIM)
        : A_DIM;
    attron(attrs);
    mvaddstr_centered(row, cols, help);
    attroff(attrs);
}

void Display::draw_small(std::chrono::steady_clock::duration t,
                         Stopwatch::State state, int rows, int cols) {
    const TimeParts p = decompose(t);
    char buf[64];
    if (opts_.seconds_only) {
        const long long secs = static_cast<long long>(p.hours) * 3600LL
                             + static_cast<long long>(p.minutes) * 60LL
                             + p.seconds;
        std::snprintf(buf, sizeof(buf), "%lld.%02d", secs, p.centiseconds);
    } else if (opts_.no_hour) {
        const long long mins = static_cast<long long>(p.hours) * 60LL + p.minutes;
        std::snprintf(buf, sizeof(buf), "%02lld:%02d.%02d",
                      mins, p.seconds, p.centiseconds);
    } else {
        std::snprintf(buf, sizeof(buf), "%02ld:%02d:%02d.%02d",
                      p.hours, p.minutes, p.seconds, p.centiseconds);
    }
    const int row = rows / 2;
    const int len = static_cast<int>(std::strlen(buf));
    int col = (cols - len) / 2;
    if (col < 0) col = 0;
    const attr_t attrs = supports_color_
        ? (COLOR_PAIR(color_pair_for(state)) | A_BOLD)
        : A_BOLD;
    attron(attrs);
    mvaddstr(row, col, buf);
    attroff(attrs);
}

void Display::render(std::chrono::steady_clock::duration t,
                     Stopwatch::State state) {
    erase();

    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);

    const bool blink_off =
        opts_.blink && (total_seconds_count(t) % 2 != 0);

    const std::vector<Token> tokens = build_tokens(t);
    const int width  = sequence_width(tokens);
    const int height = DigitGlyph::kRows;

    // Need enough room for the big clock plus status / help / optional
    // clock bar. Otherwise drop to a single centered line.
    const int min_rows = height + 6 + (opts_.show_clock ? 2 : 0);
    if (cols < width + 2 || rows < min_rows) {
        draw_small(t, state, rows, cols);
        refresh();
        return;
    }

    int top  = (rows - height) / 2;
    if (opts_.show_clock && top < 4) top = 4;
    int left = (cols - width) / 2;
    if (left < 0) left = 0;

    if (opts_.show_clock) draw_clock(1, cols);
    draw_status(state, top - 2, cols);
    draw_big(tokens, blink_off, top, left, state);
    draw_help(rows - 1, cols);

    refresh();
}

} // namespace stopwatch
