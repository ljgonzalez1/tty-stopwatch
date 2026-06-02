#pragma once

#include <string>
#include <termios.h>

namespace stopwatch {

/**
 * @brief RAII owner of the interactive terminal session.
 *
 * Terminal replaces the previous dependency on the ncurses library. It talks
 * to the controlling terminal directly using only POSIX system calls
 * (termios, poll, ioctl, read, write) and ANSI/VT100 escape sequences. This
 * removes every runtime dependency other than the C library, which is what
 * lets a single statically linked Linux binary run unchanged on Alpine
 * (musl), Debian, and Arch, and a single macOS binary run on any machine of
 * the same architecture without extra packages.
 *
 * The session is opened on /dev/tty so that the program's stdout (used to
 * print the final time) and stdin (which may be redirected) never interfere
 * with rendering.
 *
 * On construction it switches the terminal into raw mode, enters the
 * alternate screen buffer, hides the cursor, and disables line wrapping. The
 * destructor restores every one of those settings, so the user's shell is
 * always left in a clean state.
 */
class Terminal {
public:
    /// Sentinel returned by read_key() when no actionable key was available.
    static constexpr int kNoKey = -1;

    /// Visible character grid size, in rows and columns.
    struct Size {
        int rows;
        int cols;
    };

    Terminal();
    ~Terminal();

    Terminal(const Terminal&)            = delete;
    Terminal& operator=(const Terminal&) = delete;
    Terminal(Terminal&&)                 = delete;
    Terminal& operator=(Terminal&&)      = delete;

    /// Current terminal dimensions, re-queried on every call so that window
    /// resizes are picked up without relying on SIGWINCH. Falls back to a
    /// conservative 80x24 when the size cannot be determined.
    Size size() const;

    /**
     * @brief Wait up to @p timeout_ms for a keypress and return it.
     *
     * Returns the raw byte for the simple keys the application cares about
     * (printable ASCII, Enter, and Ctrl+C as 0x03). Multi-byte escape
     * sequences (arrow keys, function keys) are drained and reported as
     * kNoKey, as is a timeout with no input.
     */
    int read_key(int timeout_ms) const;

    /// Write a fully composed frame to the terminal in a single system call.
    void write_frame(const std::string& frame) const;

private:
    void enter_session();
    void leave_session() noexcept;

    int            in_fd_;        // readable end of the controlling terminal
    int            out_fd_;       // writable end of the controlling terminal
    bool           owns_fds_;     // true when fds came from open("/dev/tty")
    bool           raw_active_;   // true once raw mode has been installed
    struct termios saved_termios_;
};

} // namespace stopwatch
