#include "Image.h"
#include "Stream.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

#ifdef CS_ENABLE_LIBPNG
#include <png.h>
#endif

using namespace cs;

#ifdef CS_ENABLE_LIBPNG
static void PngWarning(png_structp, const char* b)
{
}

static void PngError(png_structp, const char* b)
{
}

static void PngReadData(png_structp png_ptr, png_bytep data, png_size_t length)
{
    auto stream = static_cast<Stream*>(png_get_io_ptr(png_ptr));
    stream->read(data, length);
}

static void PngWriteData(png_structp png_ptr, png_bytep data, png_size_t length)
{
    auto stream = static_cast<Stream*>(png_get_io_ptr(png_ptr));
    stream->write(data, length);
}

static void PngFlush(png_structp png_ptr)
{
}
#endif

Image Image::fromPng(Stream& stream)
{
#ifdef CS_ENABLE_LIBPNG
    png_structp pngPtr;
    png_infop infoPtr;

    try
    {
        pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (pngPtr == nullptr)
        {
            throw std::runtime_error("png_create_read_struct failed.");
        }

        infoPtr = png_create_info_struct(pngPtr);
        if (infoPtr == nullptr)
        {
            throw std::runtime_error("png_create_info_struct failed.");
        }

        // Set error handling
        if (setjmp(png_jmpbuf(pngPtr)))
        {
            throw std::runtime_error("png error.");
        }

        // Setup PNG reading
        int sig_read = 0;
        png_set_read_fn(pngPtr, &stream, PngReadData);
        png_set_sig_bytes(pngPtr, sig_read);

        uint32_t readFlags = PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING;
        png_read_png(pngPtr, infoPtr, readFlags, nullptr);

        // Read header
        png_uint_32 pngWidth, pngHeight;
        int bitDepth, colourType, interlaceType;
        png_get_IHDR(pngPtr, infoPtr, &pngWidth, &pngHeight, &bitDepth, &colourType, &interlaceType, nullptr, nullptr);

        // Read pixels as 32bpp RGBA data
        auto rowBytes = png_get_rowbytes(pngPtr, infoPtr);
        auto rowPointers = png_get_rows(pngPtr, infoPtr);

        Image img;
        img.width = pngWidth;
        img.height = pngHeight;
        switch (colourType)
        {
            case PNG_COLOR_TYPE_PALETTE:
            {
                // 8-bit paletted or greyscale
                if (rowBytes != pngWidth)
                    throw std::runtime_error("Unexpected row size");

                img.depth = 8;
                img.stride = pngWidth;
                img.pixels = std::vector<uint8_t>(img.stride * pngHeight);

                // Get palette
                png_colorp pngPalette{};
                int paletteSize{};
                png_get_PLTE(pngPtr, infoPtr, &pngPalette, &paletteSize);

                img.palette = std::make_unique<Palette>();
                auto colours = img.palette->Colour;
                for (auto i = 0; i < paletteSize; i++)
                {
                    auto& src = pngPalette[i];
                    auto& dst = colours[i];
                    dst.Red = src.red;
                    dst.Green = src.green;
                    dst.Blue = src.blue;
                    dst.Alpha = 255;
                }

                // Get transparent index
                png_byte* alphaValues{};
                int alphaCount{};
                png_get_tRNS(pngPtr, infoPtr, &alphaValues, &alphaCount, nullptr);
                for (int i = 0; i < alphaCount; i++)
                {
                    img.palette->Colour[i].Alpha = alphaValues[i];
                }

                auto dst = img.pixels.data();
                for (png_uint_32 i = 0; i < pngHeight; i++)
                {
                    std::copy_n(rowPointers[i], rowBytes, dst);
                    dst += rowBytes;
                }
                break;
            }
            case PNG_COLOR_TYPE_RGB:
            {
                // 24-bit PNG (no alpha)
                if (rowBytes != pngWidth * 3)
                    throw std::runtime_error("Unexpected row size");

                img.depth = 24;
                img.stride = pngWidth * 3;
                img.pixels = std::vector<uint8_t>(img.stride * pngHeight);

                auto dst = img.pixels.data();
                for (png_uint_32 i = 0; i < pngHeight; i++)
                {
                    auto src = rowPointers[i];
                    for (png_uint_32 x = 0; x < pngWidth; x++)
                    {
                        *dst++ = *src++;
                        *dst++ = *src++;
                        *dst++ = *src++;
                    }
                }
                break;
            }
            case PNG_COLOR_TYPE_RGBA:
            {
                // 32-bit PNG (with alpha)
                if (rowBytes != pngWidth * 4)
                    throw std::runtime_error("Unexpected row size");

                img.depth = 32;
                img.stride = pngWidth * 4;
                img.pixels = std::vector<uint8_t>(img.stride * pngHeight);

                auto dst = img.pixels.data();
                for (png_uint_32 i = 0; i < pngHeight; i++)
                {
                    std::copy_n(rowPointers[i], rowBytes, dst);
                    dst += rowBytes;
                }
                break;
            }
        }

        // Close the PNG
        png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);

        // Return the output data
        return img;
    }
    catch (const std::exception&)
    {
        png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
        throw;
    }
#else
    throw std::runtime_error("libpng not available");
#endif
}

void Image::toPng(Stream& stream) const
{
#ifdef CS_ENABLE_LIBPNG
    png_structp pngPtr{};
    png_colorp pngPalette{};
    try
    {
        pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, PngError, PngWarning);
        if (pngPtr == nullptr)
        {
            throw std::runtime_error("png_create_write_struct failed.");
        }

        png_text pngText[1]{};
        pngText[0].key = const_cast<char*>("Software");
        pngText[0].text = const_cast<char*>("libsawyer");
        pngText[0].compression = PNG_TEXT_COMPRESSION_zTXt;

        auto info_ptr = png_create_info_struct(pngPtr);
        if (info_ptr == nullptr)
        {
            throw std::runtime_error("png_create_info_struct failed.");
        }

        if (depth == 8)
        {
            if (palette == nullptr)
            {
                throw std::runtime_error("Expected a palette for 8-bit image.");
            }

            // Set the palette
            pngPalette = static_cast<png_colorp>(png_malloc(pngPtr, PNG_MAX_PALETTE_LENGTH * sizeof(png_color)));
            if (pngPalette == nullptr)
            {
                throw std::runtime_error("png_malloc failed.");
            }
            for (size_t i = 0; i < PNG_MAX_PALETTE_LENGTH; i++)
            {
                const auto& entry = (*palette)[i];
                pngPalette[i].blue = entry.Blue;
                pngPalette[i].green = entry.Green;
                pngPalette[i].red = entry.Red;
            }
            png_set_PLTE(pngPtr, info_ptr, pngPalette, PNG_MAX_PALETTE_LENGTH);
        }

        png_set_write_fn(pngPtr, &stream, PngWriteData, PngFlush);

        // Set error handler
        if (setjmp(png_jmpbuf(pngPtr)))
        {
            throw std::runtime_error("PNG ERROR");
        }

        // Write header
        auto colourType = PNG_COLOR_TYPE_RGB_ALPHA;
        if (depth == 8)
        {
            png_byte transparentIndex = 0;
            png_set_tRNS(pngPtr, info_ptr, &transparentIndex, 1, nullptr);
            colourType = PNG_COLOR_TYPE_PALETTE;
        }
        png_set_text(pngPtr, info_ptr, pngText, 1);
        png_set_IHDR(
            pngPtr, info_ptr, width, height, 8, colourType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_write_info(pngPtr, info_ptr);

        // Write pixels
        auto pixels8 = pixels.data();
        for (uint32_t y = 0; y < height; y++)
        {
            png_write_row(pngPtr, const_cast<png_byte*>(pixels8));
            pixels8 += stride;
        }

        png_write_end(pngPtr, nullptr);
        png_destroy_info_struct(pngPtr, &info_ptr);
        png_free(pngPtr, pngPalette);
        png_destroy_write_struct(&pngPtr, nullptr);
    }
    catch (const std::exception&)
    {
        png_free(pngPtr, pngPalette);
        png_destroy_write_struct(&pngPtr, nullptr);
        throw;
    }
#else
    throw std::runtime_error("libpng not available");
#endif
}

Image Image::crop(int32_t cropX, int32_t cropY, uint32_t cropWidth, uint32_t cropHeight) const
{
    auto bytesPerPixel = depth / 8;

    Image dstImage;
    dstImage.depth = depth;
    dstImage.width = cropWidth;
    dstImage.height = cropHeight;
    dstImage.stride = cropWidth * bytesPerPixel;
    dstImage.palette = std::make_unique<Palette>(*palette);

    dstImage.pixels.resize(dstImage.stride * dstImage.height);

    const auto* src = pixels.data();
    auto dst = dstImage.pixels.data();

    size_t srcLineOffset{};
    size_t dstLineOffset{};
    auto copyWidth = cropWidth;
    if (cropX < 0)
    {
        dstLineOffset = (-cropX) * bytesPerPixel;
        copyWidth += cropX;
    }
    else
    {
        srcLineOffset = cropX * bytesPerPixel;
    }
    copyWidth = std::min(copyWidth, width);
    size_t lineCopyLength = copyWidth * bytesPerPixel;

    auto copyHeight = cropHeight;
    if (cropY < 0)
    {
        dst += -cropY * stride;
        copyHeight += cropY;
    }
    else
    {
        src += cropY * stride;
    }
    copyHeight = std::min(copyHeight, height);

    for (uint32_t y = 0; y < copyHeight; y++)
    {
        auto srcLine = src + srcLineOffset;
        auto dstLine = dst + dstLineOffset;
        std::memcpy(dstLine, srcLine, lineCopyLength);

        src += stride;
        dst += dstImage.stride;
    }
    return dstImage;
}

Image Image::copy()
{
    Image dstImage;
    dstImage.depth = depth;
    dstImage.width = width;
    dstImage.height = height;
    dstImage.stride = stride;
    if (palette)
    {
        dstImage.palette = std::make_unique<Palette>(*palette);
    }
    dstImage.pixels = pixels;

    return dstImage;
}
