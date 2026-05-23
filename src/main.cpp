#include "Application.h"
#include "HelpScreen.h"
#include "Options.h"
#include "TerminalLauncher.h"
#include "TimeFormatter.h"

#include <ncurses.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>

int main(int argc, char* argv[]) {
    using namespace stopwatch;

    const Options opts = OptionParser::parse(argc, argv);

    if (opts.has_error()) {
        std::fprintf(stderr, "tty-stopwatch: %s\n\n", opts.error_message.c_str());
        HelpScreen::print(stderr, !opts.no_color);
        return EXIT_FAILURE;
    }
    if (opts.show_help) {
        HelpScreen::print(stdout, !opts.no_color);
        return EXIT_SUCCESS;
    }

    // When invoked without any terminal at all (e.g. double-clicked from
    // a file manager on Linux), re-exec inside the default terminal
    // emulator. On macOS the program is always run from a terminal, so
    // exec_in_terminal() is a no-op there.
    if (!TerminalLauncher::has_terminal()) {
        TerminalLauncher::exec_in_terminal(argc, argv);
        std::fprintf(stderr,
            "tty-stopwatch: no terminal available; please run from a terminal.\n");
        return EXIT_FAILURE;
    }

    std::chrono::steady_clock::duration to_print{};
    bool should_print = !opts.no_output;
    int  exit_code    = EXIT_SUCCESS;

    try {
        Application app(opts);
        exit_code = app.run();
        to_print  = app.final_time();
        // The Application's destructor tears down the ncurses session
        // when this scope exits.
    } catch (const std::exception& e) {
        endwin();
        std::fprintf(stderr, "tty-stopwatch: %s\n", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        endwin();
        std::fprintf(stderr, "tty-stopwatch: unknown error\n");
        return EXIT_FAILURE;
    }

    if (should_print) {
        std::cout << format_canonical(to_print) << '\n';
        std::cout.flush();
    }
    return exit_code;
}
