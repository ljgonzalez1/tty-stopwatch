#pragma once

#include <string>

namespace stopwatch {
namespace Notification {

// Best-effort desktop notification plus a terminal bell.
//
// The notification is delivered without relying on any external program or
// library: on Linux it speaks the D-Bus wire protocol directly to the
// session bus (org.freedesktop.Notifications); on macOS it uses the
// always-present osascript bridge. The terminal bell is always emitted and
// serves as a universal fallback. Every step is best-effort and silent;
// failures are ignored and never interrupt rendering.
void send(const std::string& title, const std::string& body);

} // namespace Notification
} // namespace stopwatch
