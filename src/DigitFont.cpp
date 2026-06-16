#include "DigitFont.h"

#include <stdexcept>

namespace stopwatch {
namespace {

// Bitmaps mimic seven-segment digits while reading cleanly as reverse-video
// blocks; each digit is 5 columns wide.
constexpr DigitGlyph kDigits[10] = {
    {{0b11111, 0b10001, 0b10001, 0b10001, 0b11111}, 5}, // 0
    {{0b00100, 0b00100, 0b00100, 0b00100, 0b00100}, 5}, // 1
    {{0b11111, 0b00001, 0b11111, 0b10000, 0b11111}, 5}, // 2
    {{0b11111, 0b00001, 0b11111, 0b00001, 0b11111}, 5}, // 3
    {{0b10001, 0b10001, 0b11111, 0b00001, 0b00001}, 5}, // 4
    {{0b11111, 0b10000, 0b11111, 0b00001, 0b11111}, 5}, // 5
    {{0b11111, 0b10000, 0b11111, 0b10001, 0b11111}, 5}, // 6
    {{0b11111, 0b00001, 0b00001, 0b00001, 0b00001}, 5}, // 7
    {{0b11111, 0b10001, 0b11111, 0b10001, 0b11111}, 5}, // 8
    {{0b11111, 0b10001, 0b11111, 0b00001, 0b11111}, 5}, // 9
};

// Separators share the blank's 3-column width so the clock keeps a constant
// overall width while blinking.
constexpr DigitGlyph kColon = {{0b000, 0b010, 0b000, 0b010, 0b000}, 3};
constexpr DigitGlyph kDot   = {{0b000, 0b000, 0b000, 0b000, 0b010}, 3};
constexpr DigitGlyph kBlank = {{0b000, 0b000, 0b000, 0b000, 0b000}, 3};

}

const DigitGlyph& DigitFont::digit(int d) {
    if (d < 0 || d > 9) {
        throw std::out_of_range("DigitFont::digit: out of range");
    }
    return kDigits[d];
}

const DigitGlyph& DigitFont::colon() { return kColon; }
const DigitGlyph& DigitFont::dot()   { return kDot;   }
const DigitGlyph& DigitFont::blank() { return kBlank; }

}