#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSMCDUGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSMCDUGATEWAY_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

struct FslabsMcduScreen
{
    static constexpr int kRows = 14;
    static constexpr int kColumns = 24;
    static constexpr int kCells = kRows * kColumns;
    static constexpr int kBlankGlyph = 0;
    static constexpr int kFirstPrintableGlyph = 32;
    static constexpr int kLastPrintableGlyph = 126;

    std::array<std::uint8_t, kCells> glyphs{};

    [[nodiscard]] static bool IsBlankGlyph(const int glyph)
    {
        return glyph == kBlankGlyph || glyph == kFirstPrintableGlyph;
    }

    [[nodiscard]] static bool IsFilledGlyph(const int glyph)
    {
        return glyph > kFirstPrintableGlyph && glyph <= kLastPrintableGlyph;
    }

    [[nodiscard]] int GlyphAt(const int row, const int column) const
    {
        return glyphs[static_cast<std::size_t>(row * kColumns + column)];
    }

    [[nodiscard]] bool IsBlank() const
    {
        return std::ranges::all_of(glyphs, [](const std::uint8_t glyph) { return IsBlankGlyph(glyph); });
    }

    [[nodiscard]] bool IsFilledBetween(const int row, const int firstColumn, const int lastColumn) const
    {
        for (int column = firstColumn; column <= lastColumn; ++column)
        {
            if (!IsFilledGlyph(GlyphAt(row, column)))
            {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] std::string Row(const int row) const
    {
        std::string text;
        for (int column = 0; column < kColumns; ++column)
        {
            const int glyph = GlyphAt(row, column);
            const bool printable = glyph >= kFirstPrintableGlyph && glyph <= kLastPrintableGlyph;
            text += printable ? static_cast<char>(glyph) : ' ';
        }

        return text;
    }
};

class FslabsMcduDisplay
{
public:
    virtual ~FslabsMcduDisplay() = default;

    virtual void Poll() = 0;
    [[nodiscard]] virtual bool IsAvailable() const = 0;
    [[nodiscard]] virtual const FslabsMcduScreen& Screen() const = 0;
    [[nodiscard]] virtual std::uint64_t ScreenSerial() const = 0;
};

class FslabsMcduGateway : public FslabsMcduDisplay
{
public:
    virtual void Press(int key) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSMCDUGATEWAY_H
