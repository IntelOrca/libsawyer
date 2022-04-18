#include "ImageConverter.h"

using namespace cs;

Image ImageConverter::convertTo8bpp(const Image& srcImage, ConvertMode mode, const Palette& palette)
{
    auto workBuffer = createWorkBuffer(srcImage);

    Image dstImage;
    dstImage.depth = 8;
    dstImage.width = srcImage.width;
    dstImage.height = srcImage.height;
    dstImage.stride = dstImage.width;
    dstImage.pixels.resize(dstImage.stride * dstImage.height);

    auto* src = workBuffer.get();
    auto* dst = dstImage.pixels.data();
    for (uint32_t y = 0; y < srcImage.height; y++)
    {
        for (uint32_t x = 0; x < srcImage.width; x++)
        {
            *dst++ = calculatePaletteIndex(mode, palette, src, x, y, srcImage.width, srcImage.height);
            src += 4;
        }
    }

    return dstImage;
}

std::unique_ptr<int16_t[]> ImageConverter::createWorkBuffer(const Image& srcImage)
{
    // Create an RGBA buffer (64-bit colour, 2 bytes per RGBA component)
    auto workBuffer = std::make_unique<int16_t[]>(srcImage.width * srcImage.height * 4);

    const auto* src = srcImage.pixels.data();
    auto* dst = workBuffer.get();
    auto padding = srcImage.stride - (srcImage.width * (srcImage.depth / 8));
    for (uint32_t y = 0; y < srcImage.height; y++)
    {
        for (uint32_t x = 0; x < srcImage.width; x++)
        {
            int16_t r, g, b, a;
            if (srcImage.depth == 8)
            {
                auto srcIndex = *src++;
                auto srcPixel = srcImage.palette->Colour[srcIndex];
                r = srcPixel.Red;
                g = srcPixel.Green;
                b = srcPixel.Blue;
                a = srcPixel.Alpha;
            }
            else if (srcImage.depth == 24)
            {
                r = *src++;
                g = *src++;
                b = *src++;
                a = 255;
            }
            else if (srcImage.depth == 32)
            {
                r = *src++;
                g = *src++;
                b = *src++;
                a = *src++;
            }
            *dst++ = r;
            *dst++ = g;
            *dst++ = b;
            *dst++ = a;
        }

        // Skip any padding
        src += padding;
    }
    return workBuffer;
}

PaletteIndex ImageConverter::calculatePaletteIndex(ConvertMode mode, const Palette& palette, int16_t* rgbaSrc, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    if (isTransparentPixel(rgbaSrc))
        return TransparentIndex;

    auto paletteIndex = getPaletteIndex(palette, rgbaSrc);
    if (mode != ConvertMode::Default && paletteIndex == TransparentIndex)
    {
        if (mode == ConvertMode::Dithering)
        {
            paletteIndex = dither(palette, rgbaSrc, x, y, width, height);
        }
        else if (mode == ConvertMode::Closest)
        {
            paletteIndex = getClosestPaletteIndex(palette, rgbaSrc);
        }
    }
    return paletteIndex;
}

PaletteIndex ImageConverter::dither(const Palette& palette, int16_t* rgbaSrc, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    auto paletteIndex = getClosestPaletteIndex(palette, rgbaSrc);
    auto dr = rgbaSrc[0] - static_cast<int16_t>(palette[paletteIndex].Red);
    auto dg = rgbaSrc[1] - static_cast<int16_t>(palette[paletteIndex].Green);
    auto db = rgbaSrc[2] - static_cast<int16_t>(palette[paletteIndex].Blue);

    // We don't want to dither remappable colours with nonremappable colours, etc
    PaletteIndexType thisIndexType = getPaletteIndexType(paletteIndex);

    if (x + 1 < width)
    {
        if (!isInPalette(palette, rgbaSrc + 4)
            && thisIndexType == getPaletteIndexType(getClosestPaletteIndex(palette, rgbaSrc + 4)))
        {
            // Right
            rgbaSrc[4] += dr * 7 / 16;
            rgbaSrc[5] += dg * 7 / 16;
            rgbaSrc[6] += db * 7 / 16;
        }
    }

    if (y + 1 < height)
    {
        if (x > 0)
        {
            if (!isInPalette(palette, rgbaSrc + 4 * (width - 1))
                && thisIndexType == getPaletteIndexType(getClosestPaletteIndex(palette, rgbaSrc + 4 * (width - 1))))
            {
                // Bottom left
                rgbaSrc[4 * (width - 1)] += dr * 3 / 16;
                rgbaSrc[4 * (width - 1) + 1] += dg * 3 / 16;
                rgbaSrc[4 * (width - 1) + 2] += db * 3 / 16;
            }
        }

        // Bottom
        if (!isInPalette(palette, rgbaSrc + 4 * width)
            && thisIndexType == getPaletteIndexType(getClosestPaletteIndex(palette, rgbaSrc + 4 * width)))
        {
            rgbaSrc[4 * width] += dr * 5 / 16;
            rgbaSrc[4 * width + 1] += dg * 5 / 16;
            rgbaSrc[4 * width + 2] += db * 5 / 16;
        }

        if (x + 1 < width)
        {
            if (!isInPalette(palette, rgbaSrc + 4 * (width + 1))
                && thisIndexType == getPaletteIndexType(getClosestPaletteIndex(palette, rgbaSrc + 4 * (width + 1))))
            {
                // Bottom right
                rgbaSrc[4 * (width + 1)] += dr * 1 / 16;
                rgbaSrc[4 * (width + 1) + 1] += dg * 1 / 16;
                rgbaSrc[4 * (width + 1) + 2] += db * 1 / 16;
            }
        }
    }

    return paletteIndex;
}

bool ImageConverter::isInPalette(const Palette& palette, int16_t* colour)
{
    return !(getPaletteIndex(palette, colour) == TransparentIndex && !isTransparentPixel(colour));
}

PaletteIndex ImageConverter::getPaletteIndex(const Palette& palette, int16_t* colour)
{
    if (!isTransparentPixel(colour))
    {
        for (int32_t i = 0; i < PaletteSize; i++)
        {
            if (static_cast<int16_t>(palette[i].Red) == colour[0] && static_cast<int16_t>(palette[i].Green) == colour[1] && static_cast<int16_t>(palette[i].Blue) == colour[2])
            {
                return i;
            }
        }
    }
    return TransparentIndex;
}

bool ImageConverter::isTransparentPixel(const int16_t* colour)
{
    return colour[3] < 128;
}

PaletteIndex ImageConverter::getClosestPaletteIndex(const Palette& palette, const int16_t* colour)
{
    auto smallestError = static_cast<uint32_t>(-1);
    auto bestMatch = TransparentIndex;
    for (int32_t x = 0; x < PaletteSize; x++)
    {
        if (isChangablePixel(x))
        {
            uint32_t error = (static_cast<int16_t>(palette[x].Red) - colour[0])
                    * (static_cast<int16_t>(palette[x].Red) - colour[0])
                + (static_cast<int16_t>(palette[x].Green) - colour[1]) * (static_cast<int16_t>(palette[x].Green) - colour[1])
                + (static_cast<int16_t>(palette[x].Blue) - colour[2]) * (static_cast<int16_t>(palette[x].Blue) - colour[2]);

            if (smallestError == static_cast<uint32_t>(-1) || smallestError > error)
            {
                bestMatch = x;
                smallestError = error;
            }
        }
    }
    return bestMatch;
}

/**
 * @returns true if palette index is an index not used for a special purpose.
 */
bool ImageConverter::isChangablePixel(PaletteIndex paletteIndex)
{
    PaletteIndexType entryType = getPaletteIndexType(paletteIndex);
    return entryType != PaletteIndexType::special && entryType != PaletteIndexType::primaryRemap;
}

/**
 * @returns the type of palette entry this is.
 */
ImageConverter::PaletteIndexType ImageConverter::getPaletteIndexType(PaletteIndex paletteIndex)
{
    if (paletteIndex <= 9)
        return PaletteIndexType::special;
    if (paletteIndex >= 230 && paletteIndex <= 239)
        return PaletteIndexType::special;
    if (paletteIndex == 255)
        return PaletteIndexType::special;
    if (paletteIndex >= 243 && paletteIndex <= 254)
        return PaletteIndexType::primaryRemap;
    if (paletteIndex >= 202 && paletteIndex <= 213)
        return PaletteIndexType::secondaryRemap;
    if (paletteIndex >= 46 && paletteIndex <= 57)
        return PaletteIndexType::tertiaryRemap;
    return PaletteIndexType::normal;
}
