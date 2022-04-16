#include "external.h"
#include <sawyer/FileSystem.hpp>

using namespace cs;

namespace spritec
{
    class SpriteArchive
    {
    public:
        struct Entry
        {
            GxEntry gx;
            size_t dataOffset{};
            size_t dataLength{};
        };

        static SpriteArchive fromFile(const fs::path& path);

        uint32_t getNumEntries() const;
        uint32_t getDataSize() const;
        const Entry& getEntry(uint32_t index) const;
        const void* getEntryData(uint32_t index) const;

        void addEntry(const GxEntry& entry);
        void removeEntry(uint32_t index);
        void writeToFile(const fs::path& path);

    private:
        std::vector<Entry> _entries;
        std::vector<uint8_t> _data;
    };
}
