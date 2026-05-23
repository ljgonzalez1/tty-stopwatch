#pragma once

#include <array>

namespace stopwatch {

// Provides the glyph data used to render the large clock face.
// Each glyph is a fixed-height bitmap stored as UTF-8 rows.
class DigitFont {
public:
    static constexpr int kRows        = 5;
    static constexpr int kDigitWidth  = 5;  // visible columns per digit
    static constexpr int kColonWidth  = 3;  // visible columns per separator

    using Glyph = std::array<const char*, kRows>;

    static const Glyph& glyph_for_digit(int digit);
    static const Glyph& glyph_for_colon();
    static const Glyph& glyph_for_dot();
};

} // namespace stopwatch
