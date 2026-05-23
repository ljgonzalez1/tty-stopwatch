#pragma once

#include <chrono>

namespace stopwatch {

// Pure timing model: no I/O, no rendering.
//
// State transitions:
//   Paused  --start()-->  Running
//   Running --pause()-->  Paused
//   *       --reset()-->  Paused with elapsed() == 0
class Stopwatch {
public:
    using Clock    = std::chrono::steady_clock;
    using Duration = Clock::duration;

    enum class State { Running, Paused };

    Stopwatch();

    void start();
    void pause();
    void toggle();
    void reset();

    State    state()   const noexcept;
    Duration elapsed() const noexcept;

private:
    State              state_;
    Duration           accumulated_;
    Clock::time_point  segment_start_;
};

} // namespace stopwatch
