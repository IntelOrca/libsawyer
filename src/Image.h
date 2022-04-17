#pragma once

#include "Palette.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace cs
{
    class Stream;

    class Image
    {
    public:
        // Meta
        uint32_t width{};
        uint32_t height{};
        uint32_t depth{};

        // Data
        std::vector<uint8_t> pixels;
        std::unique_ptr<Palette> palette;
        uint32_t stride{};

        static Image fromPng(Stream& stream);
        void toPng(Stream& stream) const;
    };
}
