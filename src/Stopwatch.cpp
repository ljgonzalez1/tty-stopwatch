#include "Stopwatch.h"

namespace stopwatch {

Stopwatch::Stopwatch()
    : state_(State::Stopped),
      accumulated_(Duration::zero()),
      segment_start_() {}

void Stopwatch::start() {
    if (state_ == State::Running) {
        return;
    }
    segment_start_ = Clock::now();
    state_         = State::Running;
}

void Stopwatch::pause() {
    if (state_ != State::Running) {
        return;
    }
    accumulated_ += Clock::now() - segment_start_;
    state_        = State::Paused;
}

void Stopwatch::toggle() {
    if (state_ == State::Running) {
        pause();
    } else {
        start();
    }
}

void Stopwatch::reset() {
    state_       = State::Stopped;
    accumulated_ = Duration::zero();
    laps_.clear();
}

void Stopwatch::record_lap() {
    // Only meaningful while the watch is actively measuring time.
    if (state_ != State::Running) {
        return;
    }
    laps_.push_back(elapsed());
}

Stopwatch::State Stopwatch::state() const noexcept {
    return state_;
}

Stopwatch::Duration Stopwatch::elapsed() const noexcept {
    if (state_ == State::Running) {
        return accumulated_ + (Clock::now() - segment_start_);
    }
    return accumulated_;
}

const std::vector<Stopwatch::Duration>& Stopwatch::laps() const noexcept {
    return laps_;
}

} // namespace stopwatch
