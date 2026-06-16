#pragma once

#include <string>

namespace stopwatch {
namespace Notification {

// Best-effort desktop notification plus a terminal bell. Delivery is silent
// and never interrupts rendering; if no notifier is available, the bell still
// serves as a universal fallback.
void send(const std::string& title, const std::string& body);

}
}