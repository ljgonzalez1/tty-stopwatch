#pragma once

#include <string>

namespace stopwatch {
namespace Notification {

// Best-effort desktop notification plus a terminal bell. Tries notify-send
// (Linux desktops) and osascript (macOS); both invocations are silenced and
// failures are ignored.
void send(const std::string& title, const std::string& body);

} // namespace Notification
} // namespace stopwatch
