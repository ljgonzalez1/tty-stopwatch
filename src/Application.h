#pragma once

#include "Display.h"
#include "Stopwatch.h"

namespace stopwatch {

// Top-level coordinator: owns the model and the display,
// and runs the input/render loop until the user quits.
class Application {
public:
    Application();
    int run();

private:
    void handle_input(int key);

    Stopwatch watch_;
    Display   display_;
    bool      running_;
};

} // namespace stopwatch
