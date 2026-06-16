#include "Notification.h"

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>

namespace stopwatch {
namespace Notification {
namespace {

void run_silent(const char* program, const char* const argv[]) {
    pid_t pid = fork();
    if (pid == 0) {
        // Silence the child so a missing notifier can't leak "command not
        // found" onto the rendered screen.
        int dn = open("/dev/null", O_WRONLY);
        if (dn >= 0) {
            dup2(dn, STDOUT_FILENO);
            dup2(dn, STDERR_FILENO);
            close(dn);
        }
        execvp(program, const_cast<char* const*>(argv));
        _exit(127);
    } else if (pid > 0) {
        int status = 0;
        waitpid(pid, &status, 0);
    }
}

void terminal_bell() {
    // Written to /dev/tty so it is audible even when stdout is piped elsewhere.
    if (FILE* tty = std::fopen("/dev/tty", "w")) {
        std::fputc('\a', tty);
        std::fflush(tty);
        std::fclose(tty);
    } else {
        std::fputc('\a', stderr);
        std::fflush(stderr);
    }
}

}

// The bell always fires; the two notifiers are attempted in turn, and the one
// absent on the host (osascript on Linux, notify-send on macOS) fails silently.
void send(const std::string& title, const std::string& body) {
    terminal_bell();

    {
        const char* argv[] = {
            "notify-send",
            "--app-name=tty-stopwatch",
            title.c_str(),
            body.c_str(),
            nullptr
        };
        run_silent("notify-send", argv);
    }

    {
        const std::string script = "display notification \"" + body
                                 + "\" with title \"" + title + "\"";
        const char* argv[] = {
            "osascript",
            "-e",
            script.c_str(),
            nullptr
        };
        run_silent("osascript", argv);
    }
}

}
}
