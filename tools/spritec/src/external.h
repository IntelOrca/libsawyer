// Code that will go into libsawyer

#pragma once

#include <cstdint>
#include <cstdlib>
#include <sawyer/Image.h>
#include <sawyer/Palette.h>
#include <sawyer/Stream.h>
#include <tuple>

namespace cs
{
    typedef uint8_t PaletteIndex;

    namespace GxFlags
    {
        constexpr uint16_t bmp = 1 << 0;       // Image data is encoded as raw pixels
        constexpr uint16_t rle = 1 << 2;       // Image data is encoded using run length encoding
        constexpr uint16_t isPalette = 1 << 3; // Image data is a sequence of palette entries R8G8B8
        constexpr uint16_t hasZoom = 1 << 4;   // Use a different sprite for higher zoom levels
        constexpr uint16_t noZoom = 1 << 5;    // Does not get drawn at higher zoom levels (only zoom 0)
    }

    constexpr uint8_t GxRleRowLengthMask = 0x7F;
    constexpr uint8_t GxRleRowEndFlag = 0x80;

    struct GxHeader
    {
        uint32_t numEntries{};
        uint32_t dataSize{};
    };

    struct GxEntry
    {
        const void* offset{};
        int16_t width{};
        int16_t height{};
        int16_t offsetX{};
        int16_t offsetY{};
        uint16_t flags{};
        uint16_t zoomOffset{};

        /**
         * Calculates the expected length of the entry's data based on the width, height and flags.
         * For RLE, the data needs to be read in order to calculate the size, this may not be safe.
         */
        size_t calculateDataSize() const;
        bool validateData(size_t bufferLen) const;
        std::pair<bool, size_t> calculateDataSize(size_t bufferLen) const;

    private:
        std::pair<bool, size_t> calculateRleSize(size_t bufferLen) const;
    };

    struct ImageBuffer8
    {
        uint16_t width{};
        uint16_t height{};
        uint8_t* data{};
    };

    class GxEncoder
    {
    private:
        struct RLECode
        {
            uint8_t NumPixels{};
            uint8_t OffsetX{};
            const uint8_t* Pixels{};
        };

    public:
        bool isWorthUsingRle(const ImageBuffer8& input);
        void encodeRle(const ImageBuffer8& input, Stream& stream);

    private:
        void pushRun(Stream& stream, RLECode& currentCode);
    };

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

        Image convertTo8bpp(const Image& src, ConvertMode mode);

    private:
        static std::unique_ptr<int16_t[]> createWorkBuffer(const Image& srcImage);

        static PaletteIndex calculatePaletteIndex(ConvertMode mode, int16_t* rgbaSrc, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
        static PaletteIndex dither(const Palette& palette, int16_t* rgbaSrc, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
        static bool isInPalette(const Palette& palette, int16_t* colour);
        static PaletteIndex getPaletteIndex(const Palette& palette, int16_t* colour);
        static bool isTransparentPixel(const int16_t* colour);
        static PaletteIndex getClosestPaletteIndex(const Palette& palette, const int16_t* colour);
        static bool isChangablePixel(PaletteIndex paletteIndex);
        static PaletteIndexType getPaletteIndexType(PaletteIndex paletteIndex);
    };
}
