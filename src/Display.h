#pragma once

#include "Stopwatch.h"

namespace stopwatch {

// Owns the ncurses session. RAII: initscr() in the constructor,
// endwin() in the destructor. Non-copyable to keep the session unique.
class Display {
public:
    Display();
    ~Display();

    Display(const Display&)            = delete;
    Display& operator=(const Display&) = delete;
    Display(Display&&)                 = delete;
    Display& operator=(Display&&)      = delete;

    void render(const Stopwatch& watch);

private:
    void draw_big_time(const Stopwatch& watch, int top_row, int left_col);
    void draw_status  (const Stopwatch& watch, int row);
    void draw_help    (int row);
    void draw_laps    (const Stopwatch& watch, int top_row);
    void draw_compact (const Stopwatch& watch, int rows, int cols);

    int  color_for_state(Stopwatch::State state) const;
    int  big_clock_width() const;

    bool supports_color_;
};

} // namespace stopwatch
