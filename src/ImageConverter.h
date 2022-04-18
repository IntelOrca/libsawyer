#pragma once

#include "Image.h"
#include "Palette.h"
#include <cstdint>
#include <memory>

namespace cs
{
    class ImageConverter
    {
    private:
        static constexpr PaletteIndex TransparentIndex = 0;

    public:
        enum class ConvertMode
        {
            Default,
            Closest,
            Dithering,
        };

        enum class PaletteIndexType : uint8_t
        {
            normal,
            primaryRemap,
            secondaryRemap,
            tertiaryRemap,
            special,
        };

        Image convertTo8bpp(const Image& src, ConvertMode mode, const Palette& palette);

    private:
        static std::unique_ptr<int16_t[]> createWorkBuffer(const Image& srcImage);
        static PaletteIndex calculatePaletteIndex(ConvertMode mode, const Palette& palette, int16_t* rgbaSrc, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
        static PaletteIndex dither(const Palette& palette, int16_t* rgbaSrc, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
        static bool isInPalette(const Palette& palette, int16_t* colour);
        static PaletteIndex getPaletteIndex(const Palette& palette, int16_t* colour);
        static bool isTransparentPixel(const int16_t* colour);
        static PaletteIndex getClosestPaletteIndex(const Palette& palette, const int16_t* colour);
        static bool isChangablePixel(PaletteIndex paletteIndex);
        static PaletteIndexType getPaletteIndexType(PaletteIndex paletteIndex);
    };
}
