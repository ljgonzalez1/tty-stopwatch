#pragma once

namespace stopwatch {
namespace TerminalLauncher {

// True if /dev/tty can be opened, which is the case in any interactive
// shell session, including when stdout/stdin are redirected.
bool has_terminal();

// Re-executes the current process inside a terminal so the program can run
// when it was started without a controlling tty: double-clicked in a file
// manager on Linux, or launched from its .app bundle on macOS. On Linux it
// tries the known graphical terminal emulators in turn; on macOS it asks
// Terminal.app to run the program. It only returns if no terminal could be
// launched; on success the process image is replaced and control never
// returns to the caller.
void exec_in_terminal(int argc, char* argv[]);

}
}
