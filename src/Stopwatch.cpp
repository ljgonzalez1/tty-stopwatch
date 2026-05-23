#include "Stopwatch.h"

namespace stopwatch {

Stopwatch::Stopwatch()
    : state_(State::Paused),
      accumulated_(Duration::zero()),
      segment_start_() {}

void Stopwatch::start() {
    if (state_ == State::Running) return;
    segment_start_ = Clock::now();
    state_         = State::Running;
}

void Stopwatch::pause() {
    if (state_ != State::Running) return;
    accumulated_ += Clock::now() - segment_start_;
    state_        = State::Paused;
}

void Stopwatch::toggle() {
    if (state_ == State::Running) pause();
    else                          start();
}

void Stopwatch::reset() {
    state_       = State::Paused;
    accumulated_ = Duration::zero();
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

} // namespace stopwatch
