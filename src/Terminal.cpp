#include "Terminal.h"

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace stopwatch {
namespace {

// ANSI/VT100 control sequences used to set up and tear down the session.
constexpr const char* kEnterAltScreen = "\x1b[?1049h"; // alternate buffer on
constexpr const char* kLeaveAltScreen = "\x1b[?1049l"; // alternate buffer off
constexpr const char* kHideCursor     = "\x1b[?25l";
constexpr const char* kShowCursor     = "\x1b[?25h";
constexpr const char* kDisableWrap    = "\x1b[?7l";    // no auto-wrap at EOL
constexpr const char* kEnableWrap     = "\x1b[?7h";
constexpr const char* kResetAttrs     = "\x1b[0m";

// Writes the whole buffer, retrying on short writes and EINTR. Errors are
// ignored on purpose: a best-effort terminal update must never abort the
// program or throw across the render loop.
void write_all(int fd, const char* data, std::size_t len) {
    std::size_t written = 0;
    while (written < len) {
        const ssize_t n = ::write(fd, data + written, len - written);
        if (n > 0) {
            written += static_cast<std::size_t>(n);
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }
}

void write_literal(int fd, const char* s) {
    write_all(fd, s, std::strlen(s));
}

} // namespace

Terminal::Terminal()
    : in_fd_(-1),
      out_fd_(-1),
      owns_fds_(false),
      raw_active_(false),
      saved_termios_{} {
    enter_session();
}

Terminal::~Terminal() {
    leave_session();
}

// Opens /dev/tty for both reading and writing when possible, falling back to
// the standard streams. Then installs raw mode and the screen setup.
void Terminal::enter_session() {
    const int tty = ::open("/dev/tty", O_RDWR);
    if (tty >= 0) {
        in_fd_    = tty;
        out_fd_   = tty;
        owns_fds_ = true;
    } else {
        in_fd_    = STDIN_FILENO;
        out_fd_   = STDOUT_FILENO;
        owns_fds_ = false;
    }

    if (::tcgetattr(in_fd_, &saved_termios_) == 0) {
        struct termios raw = saved_termios_;
        // Disable canonical mode, echo, signal generation (so Ctrl+C is
        // delivered as the byte 0x03), and extended input processing.
        raw.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
        // Disable flow control, CR-to-NL translation and input stripping.
        raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
        // Disable output post-processing so escape sequences pass through.
        raw.c_oflag &= ~(OPOST);
        raw.c_cflag |= CS8;
        // Pure non-blocking reads; timing is handled with poll().
        raw.c_cc[VMIN]  = 0;
        raw.c_cc[VTIME] = 0;
        if (::tcsetattr(in_fd_, TCSANOW, &raw) == 0) {
            raw_active_ = true;
        }
    }

    write_literal(out_fd_, kEnterAltScreen);
    write_literal(out_fd_, kHideCursor);
    write_literal(out_fd_, kDisableWrap);
}

// Restores every terminal setting changed by enter_session(). Safe to call
// even if setup partially failed.
void Terminal::leave_session() noexcept {
    if (out_fd_ >= 0) {
        write_literal(out_fd_, kResetAttrs);
        write_literal(out_fd_, kEnableWrap);
        write_literal(out_fd_, kShowCursor);
        write_literal(out_fd_, kLeaveAltScreen);
    }
    if (raw_active_) {
        ::tcsetattr(in_fd_, TCSANOW, &saved_termios_);
        raw_active_ = false;
    }
    if (owns_fds_ && in_fd_ >= 0) {
        ::close(in_fd_);
    }
    in_fd_  = -1;
    out_fd_ = -1;
}

Terminal::Size Terminal::size() const {
    struct winsize ws{};
    if (out_fd_ >= 0 && ::ioctl(out_fd_, TIOCGWINSZ, &ws) == 0 &&
        ws.ws_row > 0 && ws.ws_col > 0) {
        return Size{static_cast<int>(ws.ws_row), static_cast<int>(ws.ws_col)};
    }
    return Size{24, 80};
}

int Terminal::read_key(int timeout_ms) const {
    struct pollfd pfd{};
    pfd.fd     = in_fd_;
    pfd.events = POLLIN;

    const int ready = ::poll(&pfd, 1, timeout_ms);
    if (ready <= 0) {
        return kNoKey;  // timeout, or interrupted by a signal
    }

    unsigned char buf[32];
    const ssize_t n = ::read(in_fd_, buf, sizeof(buf));
    if (n <= 0) {
        return kNoKey;
    }

    // Return the first byte we recognise. An ESC (0x1b) introduces an escape
    // sequence we do not use (arrows, function keys); those are ignored.
    for (ssize_t i = 0; i < n; ++i) {
        const unsigned char c = buf[i];
        if (c == 0x1b) {
            return kNoKey;  // discard the rest of the sequence
        }
        return static_cast<int>(c);
    }
    return kNoKey;
}

void Terminal::write_frame(const std::string& frame) const {
    if (out_fd_ >= 0) {
        write_all(out_fd_, frame.data(), frame.size());
    }
}

}