#pragma once

#include <chrono>
#include <string>

namespace stopwatch {

struct TimeParts {
    long hours;         // unbounded (>= 0)
    int  minutes;       // 0..59
    int  seconds;       // 0..59
    int  centiseconds;  // 0..99
};

// Decomposes a duration into hours, minutes, seconds, centiseconds.
// Negative durations are clamped to zero.
TimeParts decompose(std::chrono::steady_clock::duration d);

// Whole-second count of the duration, clamped to zero.
long long total_seconds_count(std::chrono::steady_clock::duration d);

// Canonical HH:MM:SS.cs textual form, used for the program's stdout output.
// Hours always occupy at least two digits and grow naturally beyond 99.
std::string format_canonical(std::chrono::steady_clock::duration d);

} // namespace stopwatch
