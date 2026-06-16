#pragma once

#include "Options.h"
#include "Stopwatch.h"
#include "Terminal.h"

#include <chrono>

namespace stopwatch {

// Renders the (display_time, state) pair into terminal frames.
//
// Frames are built into an off-screen cell buffer and serialised to a single
// ANSI string, so each refresh is one write() and never flickers. The screen
// shows tenths of a second while the underlying model keeps full centisecond
// precision; decoupling displayed from measured precision lets the caller
// refresh at a low rate while measurement stays exact.
class Display {
public:
    Display(const Options& opts, const Terminal& terminal);

    Display(const Display&)            = delete;
    Display& operator=(const Display&) = delete;
    Display(Display&&)                 = delete;
    Display& operator=(Display&&)      = delete;

    void render(std::chrono::steady_clock::duration display_time,
                Stopwatch::State state);

private:
    const Options&  opts_;
    const Terminal& terminal_;
    bool            use_color_;
};

}