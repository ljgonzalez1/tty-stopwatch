#include "TimeFormatter.h"

#include <cstdio>

namespace stopwatch {

TimeParts decompose(std::chrono::steady_clock::duration d) {
    using clock = std::chrono::steady_clock;
    if (d < clock::duration::zero()) d = clock::duration::zero();

    const long long total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
    const long long cs_total = total_ms / 10;
    const int       cs       = static_cast<int>(cs_total % 100);
    const long long s_total  = cs_total / 100;
    const int       s        = static_cast<int>(s_total % 60);
    const long long m_total  = s_total / 60;
    const int       m        = static_cast<int>(m_total % 60);
    const long      h        = static_cast<long>(m_total / 60);
    return TimeParts{h, m, s, cs};
}

long long total_seconds_count(std::chrono::steady_clock::duration d) {
    using clock = std::chrono::steady_clock;
    if (d < clock::duration::zero()) d = clock::duration::zero();
    return std::chrono::duration_cast<std::chrono::seconds>(d).count();
}

std::string format_canonical(std::chrono::steady_clock::duration d) {
    const auto p = decompose(d);
    char buf[64];
    if (p.hours < 100) {
        std::snprintf(buf, sizeof(buf), "%02ld:%02d:%02d.%02d",
                      p.hours, p.minutes, p.seconds, p.centiseconds);
    } else {
        std::snprintf(buf, sizeof(buf), "%ld:%02d:%02d.%02d",
                      p.hours, p.minutes, p.seconds, p.centiseconds);
    }
    return std::string(buf);
}

} // namespace stopwatch
