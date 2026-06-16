#include "Application.h"

#include "Notification.h"

#include <algorithm>
#include <climits>
#include <csignal>
#include <ctime>

namespace stopwatch {
namespace {

constexpr int kCtrlC = 3;  // ASCII ETX, delivered as a byte under raw mode

constexpr int       kInputPollMs   = 50;
constexpr long long kTenthMs       = 100;
constexpr long long kScreensaverMs = 2000;

// Set by SIGTERM/SIGHUP and polled by the loop. SIGINT instead arrives as the
// byte 0x03 (raw mode) and is handled inline.
volatile std::sig_atomic_t g_should_exit = 0;

void term_handler(int) { g_should_exit = 1; }

void install_signal_handlers() {
    struct sigaction sa{};
    sa.sa_handler = term_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGHUP,  &sa, nullptr);
}

bool is_printable_ascii(int key) {
    return key >= 32 && key <= 126;
}

// Monotonic-clock duration until the next occurrence of the given wall-clock
// time. Anchored to the real clock with sub-second precision, then run as a
// steady-clock countdown so it is immune to later wall-clock adjustments.
std::chrono::steady_clock::duration
duration_until(int hour, int minute, int second) {
    struct timespec now_ts{};
    clock_gettime(CLOCK_REALTIME, &now_ts);
    const std::time_t now = now_ts.tv_sec;

    std::tm local{};
    localtime_r(&now, &local);

    std::tm target   = local;
    target.tm_hour   = hour;
    target.tm_min    = minute;
    target.tm_sec    = second;
    target.tm_isdst  = -1;  // let mktime resolve DST for the target instant
    std::time_t when = std::mktime(&target);

    // Not strictly in the future: target tomorrow, rebuilding the broken-down
    // time so mktime handles DST and month rollovers.
    if (when <= now) {
        std::tm tomorrow  = local;
        tomorrow.tm_mday += 1;
        tomorrow.tm_hour  = hour;
        tomorrow.tm_min   = minute;
        tomorrow.tm_sec   = second;
        tomorrow.tm_isdst = -1;
        when = std::mktime(&tomorrow);
    }

    long long remaining_ns =
        static_cast<long long>(when - now) * 1'000'000'000LL - now_ts.tv_nsec;
    if (remaining_ns < 0) remaining_ns = 0;

    return std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::nanoseconds(remaining_ns));
}

} // namespace

Application::Application(const Options& opts)
    : opts_(opts),
      terminal_(),
      display_(opts_, terminal_),
      watch_(),
      timer_active_(false),
      effective_(Duration::zero()),
      timer_completed_(false),
      running_(true) {
    install_signal_handlers();
}

void Application::resolve_countdown() {
    if (opts_.timer_mode) {
        timer_active_ = true;
        effective_    =
            std::chrono::duration_cast<Duration>(opts_.timer_duration);
    } else if (opts_.until_mode) {
        timer_active_ = true;
        effective_    =
            duration_until(opts_.until_hour, opts_.until_min, opts_.until_sec);
    } else {
        timer_active_ = false;
    }
}

int Application::run() {
    resolve_countdown();
    watch_.start();

    FrameKey last_key{LLONG_MIN, Stopwatch::State::Paused, LLONG_MIN};

    while (running_) {
        const int key = terminal_.read_key(kInputPollMs);
        if (key != Terminal::kNoKey) {
            handle_input(key);
        }
        if (g_should_exit) {
            running_ = false;
        }

        if (timer_active_ && !timer_completed_ &&
            watch_.elapsed() >= effective_) {
            timer_completed_ = true;
            Notification::send("tty-stopwatch", "Timer finished");
            running_ = false;
        }

        const Duration shown = display_time();
        const FrameKey key_now = compute_frame_key(shown);
        if (key_now != last_key || !running_) {
            display_.render(shown, watch_.state());
            last_key = key_now;
        }
    }
    return 0;
}

Application::Duration Application::display_time() const {
    if (timer_active_) {
        const Duration e = watch_.elapsed();
        if (opts_.reverse) {
            return std::min(e, effective_);
        }
        if (e >= effective_) {
            return Duration::zero();
        }
        return effective_ - e;
    }
    return watch_.elapsed();
}

Application::FrameKey
Application::compute_frame_key(Duration shown) const {
    if (opts_.screensaver) {
        const long long ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        return FrameKey{ms / kScreensaverMs, watch_.state(), 0};
    }

    const long long shown_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(shown).count();
    const long long wall_sec =
        opts_.show_clock ? static_cast<long long>(std::time(nullptr)) : 0;
    return FrameKey{shown_ms / kTenthMs, watch_.state(), wall_sec};
}

void Application::handle_input(int key) {
    if (key == 'q' || key == 'Q' || key == kCtrlC) {
        running_ = false;
        return;
    }

    if (opts_.quit_on_press) {
        // Any ordinary key dismisses the program; escape sequences are already
        // filtered out by the Terminal.
        if (is_printable_ascii(key) || key == '\n' || key == '\r') {
            running_ = false;
        }
        return;
    }

    switch (key) {
        case ' ':
        case 'p':
        case 'P':
            watch_.toggle();
            break;
        case 'r':
        case 'R':
            watch_.reset();
            break;
        default:
            break;
    }
}

std::chrono::steady_clock::duration Application::final_time() const {
    if (timer_active_) {
        if (timer_completed_) return effective_;
        return std::min(watch_.elapsed(), effective_);
    }
    return watch_.elapsed();
}

}