#pragma once

#include <chrono>
#include <string>

namespace stopwatch {

struct FormattedTime {
    int hours;
    int minutes;
    int seconds;
    int centiseconds;
};

// Breaks a duration into hours/minutes/seconds/centiseconds.
// Negative inputs are clamped to zero. Hours are capped at 99.
FormattedTime decompose(std::chrono::steady_clock::duration d);

// "HH:MM:SS.cs" representation, useful for the compact fallback layout.
std::string format_compact(std::chrono::steady_clock::duration d);

} // namespace stopwatch
