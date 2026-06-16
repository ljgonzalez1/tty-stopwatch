#pragma once

#include "Display.h"
#include "Options.h"
#include "Stopwatch.h"
#include "Terminal.h"

#include <chrono>

namespace stopwatch {

// Top-level coordinator: owns the Terminal, Display and Stopwatch and runs the
// input/render loop until the user exits or a countdown completes.
//
// Both --timer and --until are normalised at start-up into a single effective
// countdown measured against the monotonic clock. The loop separates
// measurement from presentation: elapsed time is read at full precision, but a
// frame is emitted only when the visible value changes (a new tenth, a state
// change, or a new wall-clock second when the clock bar is shown), keeping idle
// CPU low while the displayed time stays smooth.
class Application {
public:
    explicit Application(const Options& opts);

    int run();

    // What to write to stdout once the session is gone. For countdowns this is
    // the original duration (natural completion) or the elapsed time so far
    // (interrupted).
    std::chrono::steady_clock::duration final_time() const;

private:
    using Duration = std::chrono::steady_clock::duration;

    // Identifies the currently visible frame; equal keys render identically, so
    // a frame is redrawn only when the key changes.
    struct FrameKey {
        long long        tick;       // tenth-of-second bucket, or saver bucket
        Stopwatch::State state;
        long long        wall_sec;   // varies only when the clock bar is shown
        bool operator!=(const FrameKey& o) const {
            return tick != o.tick || state != o.state || wall_sec != o.wall_sec;
        }
    };

    void     resolve_countdown();
    Duration display_time() const;
    FrameKey compute_frame_key(Duration shown) const;
    void     handle_input(int key);

    Options   opts_;
    Terminal  terminal_;
    Display   display_;
    Stopwatch watch_;

    bool      timer_active_;     // true for both --timer and --until
    Duration  effective_;        // resolved countdown length
    bool      timer_completed_;
    bool      running_;
};

}
