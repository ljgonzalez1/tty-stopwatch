#include "TimeFormatter.h"

#include <cstdio>

namespace stopwatch {

FormattedTime decompose(std::chrono::steady_clock::duration d) {
    using clock = std::chrono::steady_clock;
    if (d < clock::duration::zero()) {
        d = clock::duration::zero();
    }

    const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    const long total_cs = total_ms / 10;

    const int centiseconds = static_cast<int>(total_cs % 100);
    const long total_s     = total_cs / 100;
    const int seconds      = static_cast<int>(total_s % 60);
    const long total_m     = total_s / 60;
    const int minutes      = static_cast<int>(total_m % 60);
    int       hours        = static_cast<int>(total_m / 60);

    if (hours > 99) {
        hours = 99;
    }
    return {hours, minutes, seconds, centiseconds};
}

std::string format_compact(std::chrono::steady_clock::duration d) {
    const auto p = decompose(d);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%02d",
                  p.hours, p.minutes, p.seconds, p.centiseconds);
    return std::string(buffer);
}

} // namespace stopwatch
