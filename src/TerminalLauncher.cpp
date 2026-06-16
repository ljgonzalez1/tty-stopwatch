#include "TerminalLauncher.h"

#include <fcntl.h>
#include <unistd.h>

#include <climits>
#include <cstring>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <mach-o/dyld.h>

#include <cstdint>
#include <cstdlib>
#endif

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

#elif defined(__APPLE__)
// Absolute, symlink-resolved path of the running executable. Inside an .app
// bundle this is <App>.app/Contents/MacOS/tty-stopwatch, which is exactly what
// we want Terminal.app to run.
std::string resolve_self_path(const char* argv0) {
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);  // first call reports the size needed
    std::string buf(size, '\0');
    if (_NSGetExecutablePath(buf.data(), &size) != 0) {
        return argv0 ? std::string(argv0) : std::string("tty-stopwatch");
    }
    buf.resize(std::strlen(buf.c_str()));  // trim the trailing NUL padding

    char resolved[PATH_MAX];
    if (realpath(buf.c_str(), resolved) != nullptr) {
        return std::string(resolved);
    }
    return buf;
}

// Wrap a string in POSIX single quotes so the shell treats it as one literal
// argument, embedded single quotes included. Safe for paths with spaces.
std::string shell_single_quote(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    out += '\'';
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else           out += c;
    }
    out += '\'';
    return out;
}

// Escape a string for inclusion inside an AppleScript double-quoted literal.
std::string applescript_quote(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        if (c == '\\' || c == '"') out += '\\';
        out += c;
    }
    return out;
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
#elif defined(__APPLE__)
    // macOS has no exec-a-command-in-a-new-window terminal flag, so we ask
    // Terminal.app to run us. The original (Finder-launched) process becomes
    // osascript via execvp and never returns on success, matching the Linux
    // contract; only a missing osascript (which ships with every macOS) would
    // let control fall through to the caller.
    const std::string self = resolve_self_path(argc > 0 ? argv[0] : nullptr);

    std::string command = shell_single_quote(self);
    for (int i = 1; i < argc; ++i) {
        command += ' ';
        command += shell_single_quote(argv[i]);
    }
    command += "; exit";  // close the window when the stopwatch ends, as on Linux

    const std::string script =
        "tell application \"Terminal\"\n"
        "    do script \"" + applescript_quote(command) + "\"\n"
        "    activate\n"
        "end tell";

    const char* osa_argv[] = {"osascript", "-e", script.c_str(), nullptr};
    execvp("osascript", const_cast<char* const*>(osa_argv));
#else
    (void)argc; (void)argv;
#endif
}

}
}
