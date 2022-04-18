#pragma once

#include <cstddef>
#include <sawyer/FileSystem.hpp>
#include <sawyer/Gx.h>
#include <sawyer/Span.hpp>

using namespace cs;

namespace spritec
{
    class SpriteArchive
    {
    public:
        struct Entry
        {
            uint32_t dataOffset{};
            uint32_t dataLength{}; // Not saved
            int16_t width{};
            int16_t height{};
            int16_t offsetX{};
            int16_t offsetY{};
            uint16_t flags{};
            uint16_t zoomOffset{};
        };

        static SpriteArchive fromFile(const fs::path& path);

        uint32_t getNumEntries() const;
        uint32_t getDataSize() const;
        const Entry& getEntry(uint32_t index) const;
        stdx::span<const std::byte> getEntryData(uint32_t index) const;
        GxEntry getGx(uint32_t index) const;

        void addEntry(const Entry& entry, stdx::span<const std::byte> data);
        void writeToFile(const fs::path& path);

    private:
        std::vector<Entry> _entries;
        std::vector<std::byte> _data;
    };
}
