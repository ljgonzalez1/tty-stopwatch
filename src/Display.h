#pragma once

#include "DigitFont.h"
#include "Options.h"
#include "Stopwatch.h"

#include <chrono>
#include <cstdio>
#include <vector>

#include <ncurses.h>

namespace stopwatch {

// Owns the ncurses session and translates a (display_time, state) pair
// into a frame. Always renders to /dev/tty when available so that the
// program's stdout can be safely redirected.
class Display {
public:
    explicit Display(const Options& opts);
    ~Display();

    Display(const Display&)            = delete;
    Display& operator=(const Display&) = delete;
    Display(Display&&)                 = delete;
    Display& operator=(Display&&)      = delete;

    int input_timeout_ms()   const;
    int render_interval_ms() const;

    void render(std::chrono::steady_clock::duration display_time,
                Stopwatch::State state);

private:
    // The big-clock face is built from a sequence of "tokens", each of
    // which maps to a single DigitGlyph for rendering.
    struct Token {
        enum class Type { Digit, Colon, Dot };
        Type type;
        int  value;  // only used when type == Digit
    };

    std::vector<Token> build_tokens(std::chrono::steady_clock::duration t) const;

    const DigitGlyph& glyph_for      (const Token& tok, bool blink_off) const;
    int               sequence_width (const std::vector<Token>& tokens) const;

    void draw_big   (const std::vector<Token>& tokens,
                     bool blink_off,
                     int top, int left,
                     Stopwatch::State state);
    void draw_status(Stopwatch::State state, int row, int cols);
    void draw_clock (int row, int cols);
    void draw_help  (int row, int cols);
    void draw_small (std::chrono::steady_clock::duration t,
                     Stopwatch::State state, int rows, int cols);

    int color_pair_for(Stopwatch::State state) const;

    Options opts_;
    bool    supports_color_;
    SCREEN* screen_;
    FILE*   tty_in_;
    FILE*   tty_out_;
};

} // namespace stopwatch
