#include "DigitFont.h"

#include <stdexcept>

namespace stopwatch {
namespace {

// 5x5 block glyphs. The "█" character is U+2588 (FULL BLOCK), which
// renders as exactly one display column under ncursesw with a UTF-8 locale.
constexpr DigitFont::Glyph kDigits[10] = {
    {{"█████",
      "█   █",
      "█   █",
      "█   █",
      "█████"}},
    {{"  █  ",
      "  █  ",
      "  █  ",
      "  █  ",
      "  █  "}},
    {{"█████",
      "    █",
      "█████",
      "█    ",
      "█████"}},
    {{"█████",
      "    █",
      "█████",
      "    █",
      "█████"}},
    {{"█   █",
      "█   █",
      "█████",
      "    █",
      "    █"}},
    {{"█████",
      "█    ",
      "█████",
      "    █",
      "█████"}},
    {{"█████",
      "█    ",
      "█████",
      "█   █",
      "█████"}},
    {{"█████",
      "    █",
      "    █",
      "    █",
      "    █"}},
    {{"█████",
      "█   █",
      "█████",
      "█   █",
      "█████"}},
    {{"█████",
      "█   █",
      "█████",
      "    █",
      "█████"}}
};

constexpr DigitFont::Glyph kColon = {{
    "   ",
    " █ ",
    "   ",
    " █ ",
    "   "
}};

constexpr DigitFont::Glyph kDot = {{
    "   ",
    "   ",
    "   ",
    "   ",
    " █ "
}};

} // namespace

const DigitFont::Glyph& DigitFont::glyph_for_digit(int digit) {
    if (digit < 0 || digit > 9) {
        throw std::out_of_range("DigitFont: digit must be in [0, 9]");
    }
    return kDigits[digit];
}

const DigitFont::Glyph& DigitFont::glyph_for_colon() {
    return kColon;
}

const DigitFont::Glyph& DigitFont::glyph_for_dot() {
    return kDot;
}

} // namespace stopwatch
