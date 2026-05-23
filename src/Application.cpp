#include "Application.h"

#include <ncurses.h>

namespace stopwatch {
namespace {

constexpr int kCtrlC = 3;  // ASCII ETX, what raw() delivers for Ctrl+C

} // namespace

Application::Application() : watch_(), display_(), running_(true) {}

int Application::run() {
    while (running_) {
        const int key = getch();   // blocks up to the configured timeout
        if (key != ERR) {
            handle_input(key);
        }
        display_.render(watch_);
    }
    return 0;
}

void Application::handle_input(int key) {
    switch (key) {
        case ' ':
            watch_.toggle();
            break;
        case 'l':
        case 'L':
            watch_.record_lap();
            break;
        case 'r':
        case 'R':
            watch_.reset();
            break;
        case 'q':
        case 'Q':
        case kCtrlC:
            running_ = false;
            break;
        case KEY_RESIZE:
            // The next render() will pick up the new dimensions.
            break;
        default:
            break;
    }
}

} // namespace stopwatch
