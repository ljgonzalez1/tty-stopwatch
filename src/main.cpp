#include "Application.h"
#include "HelpScreen.h"
#include "Options.h"
#include "TerminalLauncher.h"
#include "TimeFormatter.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>

// Program entry point.
//
// Responsibilities are deliberately thin: parse the command line, handle the
// help/error short-circuits, make sure we have a controlling terminal, then
// hand control to Application and print the final time. All terminal state is
// owned by Application (through its Terminal member), whose destructor always
// restores the tty even when an exception unwinds through this function, so
// there is nothing terminal-specific to clean up here.
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

    // When invoked without any terminal at all (double-clicked from a file
    // manager on Linux, or launched from its .app bundle on macOS), re-exec
    // inside a terminal: a graphical terminal emulator on Linux, Terminal.app
    // on macOS. The call only returns if no terminal could be launched.
    if (!TerminalLauncher::has_terminal()) {
        TerminalLauncher::exec_in_terminal(argc, argv);
        std::fprintf(stderr,
            "tty-stopwatch: no terminal available; please run from a terminal.\n");
        return EXIT_FAILURE;
    }

    std::chrono::steady_clock::duration to_print{};
    const bool should_print = !opts.no_output;
    int        exit_code    = EXIT_SUCCESS;

    try {
        Application app(opts);
        exit_code = app.run();
        to_print  = app.final_time();
        // Application's destructor restores the terminal as this scope exits.
    } catch (const std::exception& e) {
        std::fprintf(stderr, "tty-stopwatch: %s\n", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        std::fprintf(stderr, "tty-stopwatch: unknown error\n");
        return EXIT_FAILURE;
    }

    if (should_print) {
        std::cout << format_canonical(to_print) << '\n';
        std::cout.flush();
    }
    return exit_code;
}

