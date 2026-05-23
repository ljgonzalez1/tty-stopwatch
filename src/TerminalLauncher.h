#pragma once

namespace stopwatch {
namespace TerminalLauncher {

// True if /dev/tty can be opened, which is the case in any interactive
// shell session, including when stdout/stdin are redirected.
bool has_terminal();

// Re-executes the current process inside a graphical terminal emulator.
// Intended for the Linux double-click scenario; only returns on failure
// (after every known terminal has been tried unsuccessfully). On macOS
// this is a no-op because the program is invoked from Terminal.app.
void exec_in_terminal(int argc, char* argv[]);

} // namespace TerminalLauncher
} // namespace stopwatch
