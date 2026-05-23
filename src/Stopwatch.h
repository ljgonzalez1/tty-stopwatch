#pragma once

#include <chrono>
#include <vector>

namespace stopwatch {

// Pure timing model. Has no knowledge of input or rendering.
//
// Lifecycle:
//   Stopped  --start()--> Running
//   Running  --pause()--> Paused
//   Paused   --start()--> Running   (resumes, keeping previous elapsed)
//   *        --reset()--> Stopped   (elapsed and laps are cleared)
class Stopwatch {
public:
    using Clock    = std::chrono::steady_clock;
    using Duration = Clock::duration;

    enum class State {
        Stopped,
        Running,
        Paused
    };

    Stopwatch();

    void start();
    void pause();
    void toggle();
    void reset();
    void record_lap();

    State                        state()   const noexcept;
    Duration                     elapsed() const noexcept;
    const std::vector<Duration>& laps()    const noexcept;

private:
    State              state_;
    Duration           accumulated_;     // time before the current run segment
    Clock::time_point  segment_start_;   // start of the current run segment
    std::vector<Duration> laps_;
};

} // namespace stopwatch
