#pragma once

#include <cassert>
#include <cstdint>

namespace cs
{
    struct PaletteBGRA
    {
        uint8_t Blue{};
        uint8_t Green{};
        uint8_t Red{};
        uint8_t Alpha{};
    };

    constexpr const auto PaletteSize = 256;

    struct Palette
    {
        PaletteBGRA Colour[PaletteSize]{};

        PaletteBGRA& operator[](size_t idx)
        {
            assert(idx < PaletteSize);
            if (idx >= PaletteSize)
            {
                static PaletteBGRA dummy;
                return dummy;
            }

            return Colour[idx];
        }

        const PaletteBGRA operator[](size_t idx) const
        {
            assert(idx < PaletteSize);
            if (idx >= PaletteSize)
                return {};

            return Colour[idx];
        }

        explicit operator uint8_t*()
        {
            return reinterpret_cast<uint8_t*>(Colour);
        }
    };
}
