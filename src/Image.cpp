#include "Image.h"
#include "Stream.h"
#include <png.h>
#include <stdexcept>

using namespace cs;

static void PngWarning(png_structp, const char* b)
{
}

static void PngError(png_structp, const char* b)
{
}

static void PngWriteData(png_structp png_ptr, png_bytep data, png_size_t length)
{
    auto stream = static_cast<Stream*>(png_get_io_ptr(png_ptr));
    stream->write(data, length);
}

static void PngFlush(png_structp png_ptr)
{
}

void Image::WriteToPng(Stream& stream) const
{
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
}
