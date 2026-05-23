#pragma once

#include <array>
#include <cstdint>

namespace stopwatch {

// A monospaced 5-row pixel glyph for the big clock face. Each glyph is
// expressed as a small bitmap so we can render with pure ASCII (using
// reverse-video spaces) and stay independent of the system locale.
//
// Row layout: the MSB of `width` bits in each row maps to the leftmost
// visible column. For width 5, bit 4 is column 0 and bit 0 is column 4.
struct DigitGlyph {
    static constexpr int kRows = 5;
    std::array<std::uint8_t, kRows> rows;
    int width;
};

class DigitFont {
public:
    static const DigitGlyph& digit(int d);   // 0..9
    static const DigitGlyph& colon();        // separator between fields
    static const DigitGlyph& dot();          // separator before centiseconds
    static const DigitGlyph& blank();        // same width as colon, but empty
};

} // namespace stopwatch
