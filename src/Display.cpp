#include "Display.h"

#include "DigitFont.h"
#include "TimeFormatter.h"

#include <ncurses.h>

#include <algorithm>
#include <clocale>
#include <cstdio>
#include <cstring>
#include <string>

namespace stopwatch {
namespace {

constexpr int kColorRunning = 1;
constexpr int kColorPaused  = 2;
constexpr int kColorStopped = 3;
constexpr int kColorAccent  = 4;

constexpr int kFrameTimeoutMs = 33;  // ~30 FPS, enough for centiseconds

void mvaddstr_centered(int row, int cols, const char* text) {
    const int len = static_cast<int>(std::strlen(text));
    int col = (cols - len) / 2;
    if (col < 0) col = 0;
    mvaddstr(row, col, text);
}

} // namespace

Display::Display() : supports_color_(false) {
    // Make sure UTF-8 block characters are interpreted correctly.
    std::setlocale(LC_ALL, "");

    initscr();
    raw();                     // capture Ctrl+C as a regular keypress
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(kFrameTimeoutMs);  // getch() waits at most this long

    if (has_colors()) {
        supports_color_ = true;
        start_color();
        use_default_colors();
        init_pair(kColorRunning, COLOR_GREEN,  -1);
        init_pair(kColorPaused,  COLOR_YELLOW, -1);
        init_pair(kColorStopped, COLOR_CYAN,   -1);
        init_pair(kColorAccent,  COLOR_WHITE,  -1);
    }
}

Display::~Display() {
    endwin();
}

int Display::color_for_state(Stopwatch::State state) const {
    switch (state) {
        case Stopwatch::State::Running: return kColorRunning;
        case Stopwatch::State::Paused:  return kColorPaused;
        case Stopwatch::State::Stopped: return kColorStopped;
    }
    return kColorStopped;
}

int Display::big_clock_width() const {
    // Layout: D D : D D : D D . D D
    //         (8 digits, 3 separators, 1-column gaps between every glyph)
    constexpr int kDigits     = 8;
    constexpr int kSeparators = 3;
    constexpr int kGaps       = (kDigits + kSeparators) - 1;
    return kDigits      * DigitFont::kDigitWidth
         + kSeparators  * DigitFont::kColonWidth
         + kGaps;
}

void Display::render(const Stopwatch& watch) {
    erase();

    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);

    const int clock_width  = big_clock_width();
    const int clock_height = DigitFont::kRows;

    // Fall back to a compact one-line view if the terminal cannot fit
    // the big layout. This keeps the program usable in narrow windows.
    if (cols < clock_width + 2 || rows < clock_height + 6) {
        draw_compact(watch, rows, cols);
        refresh();
        return;
    }

    int top  = (rows - clock_height) / 2 - 1;
    if (top < 2) top = 2;
    int left = (cols - clock_width) / 2;
    if (left < 0) left = 0;

    draw_status(watch, top - 2);
    draw_big_time(watch, top, left);
    draw_laps(watch, top + clock_height + 2);
    draw_help(rows - 1);

    refresh();
}

void Display::draw_big_time(const Stopwatch& watch, int top_row, int left_col) {
    const auto parts = decompose(watch.elapsed());

    const int digits[8] = {
        parts.hours        / 10, parts.hours        % 10,
        parts.minutes      / 10, parts.minutes      % 10,
        parts.seconds      / 10, parts.seconds      % 10,
        parts.centiseconds / 10, parts.centiseconds % 10
    };

    const int  pair    = color_for_state(watch.state());
    const auto attrs   = supports_color_ ? (COLOR_PAIR(pair) | A_BOLD) : A_BOLD;
    attron(attrs);

    int col = left_col;
    const auto draw_glyph = [&](const DigitFont::Glyph& glyph, int width) {
        for (int row = 0; row < DigitFont::kRows; ++row) {
            mvaddstr(top_row + row, col, glyph[row]);
        }
        col += width + 1;  // single-column gap between glyphs
    };

    draw_glyph(DigitFont::glyph_for_digit(digits[0]), DigitFont::kDigitWidth);
    draw_glyph(DigitFont::glyph_for_digit(digits[1]), DigitFont::kDigitWidth);
    draw_glyph(DigitFont::glyph_for_colon(),          DigitFont::kColonWidth);
    draw_glyph(DigitFont::glyph_for_digit(digits[2]), DigitFont::kDigitWidth);
    draw_glyph(DigitFont::glyph_for_digit(digits[3]), DigitFont::kDigitWidth);
    draw_glyph(DigitFont::glyph_for_colon(),          DigitFont::kColonWidth);
    draw_glyph(DigitFont::glyph_for_digit(digits[4]), DigitFont::kDigitWidth);
    draw_glyph(DigitFont::glyph_for_digit(digits[5]), DigitFont::kDigitWidth);
    draw_glyph(DigitFont::glyph_for_dot(),            DigitFont::kColonWidth);
    draw_glyph(DigitFont::glyph_for_digit(digits[6]), DigitFont::kDigitWidth);
    draw_glyph(DigitFont::glyph_for_digit(digits[7]), DigitFont::kDigitWidth);

    attroff(attrs);
}

void Display::draw_status(const Stopwatch& watch, int row) {
    if (row < 0) return;

    const char* label = "[ STOPPED ]";
    int pair = kColorStopped;
    switch (watch.state()) {
        case Stopwatch::State::Running: label = "[ RUNNING ]"; pair = kColorRunning; break;
        case Stopwatch::State::Paused:  label = "[ PAUSED  ]"; pair = kColorPaused;  break;
        case Stopwatch::State::Stopped: label = "[ STOPPED ]"; pair = kColorStopped; break;
    }

    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);
    (void)rows;

    const auto attrs = supports_color_ ? (COLOR_PAIR(pair) | A_BOLD) : A_BOLD;
    attron(attrs);
    mvaddstr_centered(row, cols, label);
    attroff(attrs);
}

void Display::draw_help(int row) {
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);
    (void)rows;

    const char* help = "[Space] start/pause   [L] lap   [R] reset   [Q] quit";
    const auto attrs = supports_color_ ? (COLOR_PAIR(kColorAccent) | A_DIM) : A_DIM;
    attron(attrs);
    mvaddstr_centered(row, cols, help);
    attroff(attrs);
}

void Display::draw_laps(const Stopwatch& watch, int top_row) {
    const auto& laps = watch.laps();
    if (laps.empty()) return;

    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);

    const int available_rows = rows - top_row - 2;
    if (available_rows <= 0) return;

    const int total       = static_cast<int>(laps.size());
    const int max_display = std::min(total, available_rows);
    const int start_index = total - max_display;

    const auto attrs = supports_color_ ? COLOR_PAIR(kColorAccent) : 0;
    attron(attrs);
    for (int i = 0; i < max_display; ++i) {
        const int   index     = start_index + i;
        const auto  formatted = format_compact(laps[index]);
        char        line[64];
        std::snprintf(line, sizeof(line), "Lap %02d   %s", index + 1, formatted.c_str());
        mvaddstr_centered(top_row + i, cols, line);
    }
    attroff(attrs);
}

void Display::draw_compact(const Stopwatch& watch, int rows, int cols) {
    const char* tag = "";
    int pair        = kColorStopped;
    switch (watch.state()) {
        case Stopwatch::State::Running: tag = " RUN "; pair = kColorRunning; break;
        case Stopwatch::State::Paused:  tag = " PAU "; pair = kColorPaused;  break;
        case Stopwatch::State::Stopped: tag = " --- "; pair = kColorStopped; break;
    }

    const std::string line  = format_compact(watch.elapsed()) + tag;
    const char*       help  = "[Sp]start  [L]lap  [R]reset  [Q]quit";
    const int         mid   = rows / 2;

    const auto attrs = supports_color_ ? (COLOR_PAIR(pair) | A_BOLD) : A_BOLD;
    attron(attrs);
    mvaddstr_centered(mid, cols, line.c_str());
    attroff(attrs);

    const auto dim = supports_color_ ? (COLOR_PAIR(kColorAccent) | A_DIM) : A_DIM;
    attron(dim);
    mvaddstr_centered(mid + 2, cols, help);
    attroff(dim);
}

} // namespace stopwatch
