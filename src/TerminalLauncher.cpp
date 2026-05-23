#include "TerminalLauncher.h"

#include <fcntl.h>
#include <unistd.h>

#include <climits>
#include <cstring>
#include <string>
#include <vector>

namespace stopwatch {
namespace TerminalLauncher {

namespace {

#if defined(__linux__)
std::string resolve_self_path(const char* argv0) {
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        return std::string(buf);
    }
    return argv0 ? std::string(argv0) : std::string("tty-stopwatch");
}

// Build the argv for "<term> <separator> <self> [original args...]" and
// run it with execvp. Returns only if execvp fails.
void try_terminal(const char* term, const std::string& self,
                  int argc, char* argv[]) {
    const bool needs_double_dash =
        std::strcmp(term, "gnome-terminal") == 0 ||
        std::strcmp(term, "mate-terminal")  == 0 ||
        std::strcmp(term, "tilix")          == 0;

    std::vector<char*> args;
    args.push_back(const_cast<char*>(term));
    args.push_back(const_cast<char*>(needs_double_dash ? "--" : "-e"));
    args.push_back(const_cast<char*>(self.c_str()));
    for (int i = 1; i < argc; ++i) args.push_back(argv[i]);
    args.push_back(nullptr);

    execvp(term, args.data());
}
#endif

} // namespace

bool has_terminal() {
    int fd = open("/dev/tty", O_RDWR);
    if (fd >= 0) {
        close(fd);
        return true;
    }
    return false;
}

void exec_in_terminal(int argc, char* argv[]) {
#if defined(__linux__)
    const std::string self = resolve_self_path(argc > 0 ? argv[0] : nullptr);

    static const char* const terminals[] = {
        "x-terminal-emulator",  // Debian/Ubuntu alternatives system
        "gnome-terminal",
        "konsole",
        "xfce4-terminal",
        "mate-terminal",
        "lxterminal",
        "alacritty",
        "kitty",
        "terminator",
        "tilix",
        "xterm",
        nullptr
    };

    for (const char* const* t = terminals; *t; ++t) {
        try_terminal(*t, self, argc, argv);
        // try_terminal returns only when execvp failed; loop to next.
    }
#else
    (void)argc; (void)argv;
#endif
}

} // namespace TerminalLauncher
} // namespace stopwatch
