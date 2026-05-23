#include "Application.h"

#include <ncurses.h>

#include <cstdio>
#include <cstdlib>
#include <exception>

int main() {
    try {
        stopwatch::Application app;
        return app.run();
    } catch (const std::exception& e) {
        endwin();
        std::fprintf(stderr, "stopwatch: %s\n", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        endwin();
        std::fprintf(stderr, "stopwatch: unknown error\n");
        return EXIT_FAILURE;
    }
}
