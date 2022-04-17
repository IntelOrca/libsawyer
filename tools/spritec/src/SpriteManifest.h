#pragma once

#include <sawyer/FileSystem.hpp>
#include <string>

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
    };

    std::vector<Entry> entries;

    static SpriteManifest fromFile(const fs::path& path);
};
