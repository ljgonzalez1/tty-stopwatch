#pragma once

#include "Display.h"
#include "Options.h"
#include "Stopwatch.h"
#include "Terminal.h"

#include <chrono>

namespace stopwatch {

/**
 * @brief Top-level coordinator.
 *
 * Owns the Terminal session, the Display, and the Stopwatch timing model, and
 * runs the input/render loop until the user exits or a countdown completes.
 *
 * Both countdown variants are normalised at start-up into a single effective
 * duration measured against the monotonic clock:
 *   - --timer uses the duration parsed from the command line;
 *   - --until computes, from the system wall clock, the time remaining to the
 *     next occurrence of the requested 24-hour time-of-day.
 *
 * The loop separates measurement from presentation. The elapsed time is read
 * at full precision from the Stopwatch, but a frame is only emitted when the
 * value visible on screen actually changes (a new tenth of a second, a state
 * change, or a new wall-clock second when the clock bar is shown). That keeps
 * CPU usage low while the displayed time stays smooth.
 */
class Application {
public:
    explicit Application(const Options& opts);

    // Runs the main loop. Returns a process exit code (currently always 0,
    // reserved for future use).
    int run();

    // Whatever should be written to stdout once the terminal session is gone.
    // For countdowns this is either the original duration (natural
    // completion) or the elapsed time so far (interrupted).
    std::chrono::steady_clock::duration final_time() const;

private:
    using Duration = std::chrono::steady_clock::duration;

    // Identifies the currently visible frame. Two equal keys render
    // identically, so a frame is only redrawn when the key changes.
    struct FrameKey {
        long long        tick;       // tenth-of-second bucket, or saver bucket
        Stopwatch::State state;
        long long        wall_sec;   // only varies when the clock bar is shown
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

} // namespace stopwatch
