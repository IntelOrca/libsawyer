#pragma once

#include <cstdint>
#include <sawyer/FileSystem.hpp>
#include <string>
#include <vector>

namespace spritec
{
    class SpriteArchive;

    class SpriteManifest
    {
    public:
        enum class Format
        {
            automatic,
            bmp,
            rle,
            palette,
        };

        enum class PaletteKind
        {
            default,
            keep,
        };

        struct Entry
        {
            fs::path path;
            Format format{};
            PaletteKind palette{};
            int32_t offsetX{};
            int32_t offsetY{};
            int32_t zoomOffset{};
            int32_t srcX{};
            int32_t srcY{};
            int32_t srcWidth{};
            int32_t srcHeight{};
        };

        std::vector<Entry> entries;

        static SpriteManifest fromFile(const fs::path& path);
        static std::string buildManifest(const SpriteArchive& archive);
    };
}
