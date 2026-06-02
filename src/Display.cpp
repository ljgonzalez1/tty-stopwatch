#include "Display.h"

#include "DigitFont.h"
#include "TimeFormatter.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

namespace stopwatch {
namespace {

// Logical colour slots, mapped to ANSI SGR codes at serialisation time.
enum class Color : std::uint8_t { Default = 0, Green, Yellow, Cyan };

// Per-cell attribute flags (bitmask).
constexpr std::uint8_t kAttrReverse = 1u << 0;
constexpr std::uint8_t kAttrBold    = 1u << 1;
constexpr std::uint8_t kAttrDim     = 1u << 2;

/**
 * @brief A single rendered character together with its visual attributes.
 *
 * This is a plain data aggregate; it carries no behaviour.
 */
struct Cell {
    char         ch    = ' ';
    std::uint8_t attr  = 0;
    Color        color = Color::Default;
};

/**
 * @brief An off-screen grid of cells that serialises to ANSI escape codes.
 *
 * The buffer is filled by free drawing helpers and then converted into one
 * escape-coded string. Serialisation repaints every column of every row and
 * positions the cursor explicitly per row, so a previous frame is always
 * fully overwritten without an intervening screen clear (and therefore
 * without flicker).
 */
class ScreenBuffer {
public:
    ScreenBuffer(int rows, int cols)
        : rows_(rows > 0 ? rows : 1),
          cols_(cols > 0 ? cols : 1),
          cells_(static_cast<std::size_t>(rows_) * cols_) {}

    int rows() const { return rows_; }
    int cols() const { return cols_; }

    void put(int r, int c, char ch, std::uint8_t attr, Color color) {
        if (r < 0 || r >= rows_ || c < 0 || c >= cols_) return;
        Cell& cell = cells_[static_cast<std::size_t>(r) * cols_ + c];
        cell.ch    = ch;
        cell.attr  = attr;
        cell.color = color;
    }

    // Writes a string starting at (r, c), centred horizontally is handled by
    // the caller. Characters falling outside the grid are clipped.
    void put_text(int r, int c, const char* text,
                  std::uint8_t attr, Color color) {
        for (int i = 0; text[i] != '\0'; ++i) {
            put(r, c + i, text[i], attr, color);
        }
    }

    std::string serialize(bool use_color) const {
        std::string out;
        out.reserve(static_cast<std::size_t>(rows_) * (cols_ + 16));
        out += "\x1b[H";  // home

        std::uint8_t cur_attr  = 0;
        Color        cur_color = Color::Default;
        bool         sgr_known = false;

        for (int r = 0; r < rows_; ++r) {
            char move[24];
            std::snprintf(move, sizeof(move), "\x1b[%d;1H", r + 1);
            out += move;
            for (int c = 0; c < cols_; ++c) {
                const Cell& cell = cells_[static_cast<std::size_t>(r) * cols_ + c];
                if (!sgr_known || cell.attr != cur_attr || cell.color != cur_color) {
                    out += sgr_for(cell.attr, cell.color, use_color);
                    cur_attr  = cell.attr;
                    cur_color = cell.color;
                    sgr_known = true;
                }
                out += cell.ch;
            }
            out += "\x1b[0m";   // reset before clearing the rest of the line
            out += "\x1b[K";    // clear any residue beyond the reported width
            sgr_known = false;
        }
        return out;
    }

private:
    static std::string sgr_for(std::uint8_t attr, Color color, bool use_color) {
        std::string s = "\x1b[0";
        if (attr & kAttrReverse) s += ";7";
        if (attr & kAttrBold)    s += ";1";
        if (attr & kAttrDim)     s += ";2";
        if (use_color) {
            switch (color) {
                case Color::Green:  s += ";32"; break;
                case Color::Yellow: s += ";33"; break;
                case Color::Cyan:   s += ";36"; break;
                case Color::Default: break;
            }
        }
        s += "m";
        return s;
    }

    int               rows_;
    int               cols_;
    std::vector<Cell> cells_;
};

// One element of the big-clock face: a digit, a colon, or a decimal dot.
struct Token {
    enum class Type { Digit, Colon, Dot };
    Type type;
    int  value;  // only meaningful when type == Digit
};

Color color_for(Stopwatch::State state) {
    return state == Stopwatch::State::Running ? Color::Green : Color::Yellow;
}

// Zero-pads on the left so the field is at least two characters wide.
std::string pad2(long long value) {
    std::string s = std::to_string(value);
    if (s.size() < 2) s.insert(s.begin(), '0');
    return s;
}

// Builds the token sequence for the big clock. The fractional part is shown
// to a single decimal (tenths), derived from the centisecond field.
std::vector<Token> build_tokens(const Options& opts,
                                std::chrono::steady_clock::duration t) {
    std::vector<Token> out;
    const TimeParts p      = decompose(t);
    const int       tenths = p.centiseconds / 10;  // one displayed decimal

    auto push_digit = [&](int d) { out.push_back({Token::Type::Digit, d}); };
    auto push_string_digits = [&](const std::string& s) {
        for (char ch : s) push_digit(ch - '0');
    };

    if (opts.seconds_only) {
        const long long secs = static_cast<long long>(p.hours) * 3600LL
                             + static_cast<long long>(p.minutes) * 60LL
                             + p.seconds;
        push_string_digits(pad2(secs));
        out.push_back({Token::Type::Dot, 0});
        push_digit(tenths);
        return out;
    }

    if (opts.no_hour) {
        const long long mins = static_cast<long long>(p.hours) * 60LL + p.minutes;
        push_string_digits(pad2(mins));
        out.push_back({Token::Type::Colon, 0});
        push_digit(p.seconds / 10);
        push_digit(p.seconds % 10);
        out.push_back({Token::Type::Dot, 0});
        push_digit(tenths);
        return out;
    }

    // Default HH:MM:SS.t, with hours growing past 99 if necessary.
    push_string_digits(pad2(p.hours));
    out.push_back({Token::Type::Colon, 0});
    push_digit(p.minutes / 10);
    push_digit(p.minutes % 10);
    out.push_back({Token::Type::Colon, 0});
    push_digit(p.seconds / 10);
    push_digit(p.seconds % 10);
    out.push_back({Token::Type::Dot, 0});
    push_digit(tenths);
    return out;
}

const DigitGlyph& glyph_for(const Token& tok, bool blink_off) {
    switch (tok.type) {
        case Token::Type::Digit: return DigitFont::digit(tok.value);
        case Token::Type::Colon: return blink_off ? DigitFont::blank()
                                                   : DigitFont::colon();
        case Token::Type::Dot:   return DigitFont::dot();
    }
    return DigitFont::blank();
}

// Total width of the token sequence; independent of blink state because the
// blank colon has the same width as the visible colon.
int sequence_width(const std::vector<Token>& tokens) {
    int w = 0;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        w += glyph_for(tokens[i], /*blink_off=*/false).width;
        if (i + 1 < tokens.size()) w += 1;  // single-column gap
    }
    return w;
}

void draw_big(ScreenBuffer& buf, const std::vector<Token>& tokens,
              bool blink_off, int top, int left, Stopwatch::State state) {
    const Color color = color_for(state);
    int col = left;
    for (const Token& tok : tokens) {
        const DigitGlyph& g = glyph_for(tok, blink_off);
        for (int r = 0; r < DigitGlyph::kRows; ++r) {
            const std::uint8_t bits = g.rows[r];
            for (int c = 0; c < g.width; ++c) {
                const bool on = (bits >> (g.width - 1 - c)) & 1;
                if (on) buf.put(top + r, col + c, ' ', kAttrReverse, color);
            }
        }
        col += g.width + 1;
    }
}

void draw_centered(ScreenBuffer& buf, int row, const char* text,
                   std::uint8_t attr, Color color) {
    if (row < 0 || row >= buf.rows()) return;
    const int len = static_cast<int>(std::strlen(text));
    int col = (buf.cols() - len) / 2;
    if (col < 0) col = 0;
    buf.put_text(row, col, text, attr, color);
}

void draw_status(ScreenBuffer& buf, Stopwatch::State state, int row) {
    const char* label = (state == Stopwatch::State::Running)
                            ? "[ RUNNING ]"
                            : "[ PAUSED  ]";
    draw_centered(buf, row, label, kAttrBold, color_for(state));
}

// System clock in 24-hour format. No localisation: HH:MM:SS comes straight
// from the broken-down local time, so output is identical across locales.
void draw_clock(ScreenBuffer& buf, int row) {
    std::time_t now = std::time(nullptr);
    std::tm     tm_local{};
    localtime_r(&now, &tm_local);
    char text[16];
    std::snprintf(text, sizeof(text), "%02d:%02d:%02d",
                  tm_local.tm_hour, tm_local.tm_min, tm_local.tm_sec);
    draw_centered(buf, row, text, kAttrBold, Color::Cyan);
}

void draw_help(ScreenBuffer& buf, int row) {
    draw_centered(buf, row, "[Space/P] pause   [R] reset   [Q] quit",
                  kAttrDim, Color::Cyan);
}

// Single-line fallback used when the terminal is too small for the big clock.
void draw_small(ScreenBuffer& buf, const Options& opts,
                std::chrono::steady_clock::duration t,
                Stopwatch::State state) {
    const TimeParts p      = decompose(t);
    const int       tenths = p.centiseconds / 10;
    char text[64];
    if (opts.seconds_only) {
        const long long secs = static_cast<long long>(p.hours) * 3600LL
                             + static_cast<long long>(p.minutes) * 60LL
                             + p.seconds;
        std::snprintf(text, sizeof(text), "%lld.%01d", secs, tenths);
    } else if (opts.no_hour) {
        const long long mins = static_cast<long long>(p.hours) * 60LL + p.minutes;
        std::snprintf(text, sizeof(text), "%02lld:%02d.%01d",
                      mins, p.seconds, tenths);
    } else {
        std::snprintf(text, sizeof(text), "%02ld:%02d:%02d.%01d",
                      p.hours, p.minutes, p.seconds, tenths);
    }
    draw_centered(buf, buf.rows() / 2, text, kAttrBold, color_for(state));
}

} // namespace

Display::Display(const Options& opts, const Terminal& terminal)
    : opts_(opts),
      terminal_(terminal),
      use_color_(!opts.no_color && std::getenv("NO_COLOR") == nullptr) {}

void Display::render(std::chrono::steady_clock::duration t,
                     Stopwatch::State state) {
    const Terminal::Size size = terminal_.size();
    ScreenBuffer buf(size.rows, size.cols);

    const std::vector<Token> tokens = build_tokens(opts_, t);
    const bool blink_off =
        opts_.blink && (total_seconds_count(t) % 2 != 0);

    const int width  = sequence_width(tokens);
    const int height = DigitGlyph::kRows;

    // Need room for the big clock plus the status / help / optional clock
    // bars. Otherwise fall back to a single centred line of plain text.
    const int min_rows = height + 6 + (opts_.show_clock ? 2 : 0);
    if (size.cols < width + 2 || size.rows < min_rows) {
        draw_small(buf, opts_, t, state);
        terminal_.write_frame(buf.serialize(use_color_));
        return;
    }

    int top = (size.rows - height) / 2;
    if (opts_.show_clock && top < 4) top = 4;
    int left = (size.cols - width) / 2;
    if (left < 0) left = 0;

    if (opts_.show_clock) draw_clock(buf, 1);
    draw_status(buf, state, top - 2);
    draw_big(buf, tokens, blink_off, top, left, state);
    draw_help(buf, size.rows - 1);

    terminal_.write_frame(buf.serialize(use_color_));
}

} // namespace stopwatch
