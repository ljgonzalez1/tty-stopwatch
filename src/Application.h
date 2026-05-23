#pragma once

#include "Display.h"
#include "Options.h"
#include "Stopwatch.h"

#include <chrono>

namespace stopwatch {

// Top-level coordinator. Owns the Stopwatch model and the Display, runs
// the input/render loop, and exposes the final time so main() can print
// it to stdout after the ncurses session has ended.
class Application {
public:
    explicit Application(const Options& opts);

    // Runs the main loop until the user exits or the timer completes.
    // Returns a process exit code (always 0 today, reserved for future use).
    int run();

    // Whatever should be written to stdout once the ncurses session is
    // gone. For countdowns this is either the original duration (natural
    // completion) or the elapsed time so far (interrupted).
    std::chrono::steady_clock::duration final_time() const;

private:
    void handle_input(int key);
    std::chrono::steady_clock::duration display_time() const;

    Options   opts_;
    Stopwatch watch_;
    Display   display_;
    bool      running_;
    bool      timer_completed_;
};

} // namespace stopwatch
