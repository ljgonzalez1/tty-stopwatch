#include "Application.h"

#include "Notification.h"

#include <ncurses.h>

#include <algorithm>
#include <chrono>
#include <csignal>

namespace stopwatch {
namespace {

constexpr int kCtrlC = 3;  // ASCII ETX, delivered by raw() for Ctrl+C

// SIGTERM (and SIGHUP) are handled by setting this flag; the main loop
// polls it. SIGINT is delivered as the character 0x03 thanks to raw()
// and handled inline.
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

} // namespace

Application::Application(const Options& opts)
    : opts_(opts),
      watch_(),
      display_(opts),
      running_(true),
      timer_completed_(false) {
    install_signal_handlers();
}

int Application::run() {
    watch_.start();  // auto-start as required

    using clock = std::chrono::steady_clock;
    const auto render_interval =
        std::chrono::milliseconds(display_.render_interval_ms());

    // Force a frame as soon as we enter the loop.
    auto last_render = clock::now() - std::chrono::hours(1);

    while (running_) {
        const int key = getch();
        if (key != ERR) {
            handle_input(key);
        }
        if (g_should_exit) {
            running_ = false;
        }

        // Timer completion (only checked once).
        if (opts_.timer_mode && !timer_completed_ &&
            watch_.elapsed() >= opts_.timer_duration) {
            timer_completed_ = true;
            Notification::send("tty-stopwatch", "Timer finished");
            running_ = false;
        }

        const auto now = clock::now();
        if (now - last_render >= render_interval || !running_) {
            display_.render(display_time(), watch_.state());
            last_render = now;
        }
    }
    return 0;
}

std::chrono::steady_clock::duration Application::display_time() const {
    if (opts_.timer_mode) {
        const auto e      = watch_.elapsed();
        const auto target =
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                opts_.timer_duration);
        if (opts_.reverse) {
            return std::min(e, target);
        }
        if (e >= target) {
            return std::chrono::steady_clock::duration::zero();
        }
        return target - e;
    }
    return watch_.elapsed();
}

void Application::handle_input(int key) {
    // Ignore terminal resize signals: render() rereads dimensions on its
    // own at the start of every frame.
    if (key == KEY_RESIZE) return;

    // Universal exit keys, valid in every mode.
    if (key == 'q' || key == 'Q' || key == kCtrlC) {
        running_ = false;
        return;
    }

    if (opts_.quit_on_press) {
        // In screensaver / quit-on-press mode any normal key dismisses
        // the program. Resize, function keys and other special codes do
        // not.
        if (is_printable_ascii(key) || key == '\n' || key == '\r' ||
            key == KEY_ENTER) {
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
    const auto target =
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            opts_.timer_duration);
    if (opts_.timer_mode) {
        if (timer_completed_) return target;
        return std::min(watch_.elapsed(), target);
    }
    return watch_.elapsed();
}

} // namespace stopwatch
