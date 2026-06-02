#pragma once

#include "Options.h"
#include "Stopwatch.h"
#include "Terminal.h"

#include <chrono>

namespace stopwatch {

/**
 * @brief Renders the (display_time, state) pair into terminal frames.
 *
 * Display contains no terminal-ownership logic of its own: it borrows a
 * Terminal for sizing and output and concentrates purely on composing a
 * frame. Frames are built into an off-screen cell buffer and serialised to a
 * single ANSI string, so each refresh is a single write() and never flickers.
 *
 * The on-screen clock shows time to a tenth of a second (one decimal). The
 * underlying timing model keeps full centisecond precision; the extra digit
 * is only revealed in the program's final stdout output. Decoupling the
 * displayed precision from the measured precision lets the caller refresh the
 * screen at a low rate (driven by tenth-of-a-second changes) while the
 * measurement itself stays exact, which keeps idle CPU usage minimal.
 */
class Display {
public:
    Display(const Options& opts, const Terminal& terminal);

    Display(const Display&)            = delete;
    Display& operator=(const Display&) = delete;
    Display(Display&&)                 = delete;
    Display& operator=(Display&&)      = delete;

    /// Composes one frame for the given displayed duration and state and
    /// writes it to the terminal.
    void render(std::chrono::steady_clock::duration display_time,
                Stopwatch::State state);

private:
    const Options&  opts_;
    const Terminal& terminal_;
    bool            use_color_;
};

} // namespace stopwatch
