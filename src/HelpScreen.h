#pragma once

#include <cstdio>

namespace stopwatch {
namespace HelpScreen {

// Prints the help text to the given stream. When `color` is true and the
// stream is connected to a terminal, ANSI escape sequences highlight
// section headings, flags, and examples.
void print(std::FILE* out, bool color);

} // namespace HelpScreen
} // namespace stopwatch
