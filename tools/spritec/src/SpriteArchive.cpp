#include "SpriteArchive.h"
#include <algorithm>
#include <sawyer/Stream.h>

using namespace cs;
using namespace spritec;

SpriteArchive SpriteArchive::fromFile(const fs::path& path)
{
    SpriteArchive archive;

    FileStream fs(path, StreamFlags::read);
    BinaryReader br(fs);

    // Read header
    GxHeader header;
    header.numEntries = br.read<uint32_t>();
    header.dataSize = br.read<uint32_t>();

    // Read entries
    std::vector<uint32_t> offsets;
    archive._entries.resize(header.numEntries);
    for (uint32_t i = 0; i < header.numEntries; i++)
    {
        auto& entry = archive._entries[i];
        auto& gx = entry.gx;

        entry.dataOffset = br.read<uint32_t>();
        gx.width = br.read<uint16_t>();
        gx.height = br.read<uint16_t>();
        gx.offsetX = br.read<uint16_t>();
        gx.offsetY = br.read<uint16_t>();
        gx.flags = br.read<uint16_t>();
        gx.zoomOffset = br.read<uint16_t>();

        offsets.push_back(static_cast<uint32_t>(entry.dataOffset));
    }

    // Read data
    archive._data.resize(header.dataSize);
    fs.read(archive._data.data(), header.dataSize);

    // Determine data length of each entry by capping at offset of next
    std::sort(offsets.begin(), offsets.end());
    for (uint32_t i = 0; i < header.numEntries; i++)
    {
        auto& entry = archive._entries[i];
        auto it = std::lower_bound(offsets.begin(), offsets.end(), static_cast<uint32_t>(entry.dataOffset + 1));
        if (it != offsets.end())
        {
            entry.dataLength = *it - entry.dataOffset;
        }
        else
        {
            entry.dataLength = header.dataSize - entry.dataOffset;
        }
    }

    return archive;
}

uint32_t SpriteArchive::getNumEntries() const
{
    return static_cast<uint32_t>(_entries.size());
}

uint32_t SpriteArchive::getDataSize() const
{
    return static_cast<uint32_t>(_data.size());
}

const SpriteArchive::Entry& SpriteArchive::getEntry(uint32_t index) const
{
    return _entries[index];
}

const void* SpriteArchive::getEntryData(uint32_t index) const
{
    return _data.data() + _entries[index].dataOffset;
}

void SpriteArchive::writeToFile(const fs::path& path)
{
    FileStream fs(path, StreamFlags::write);
    BinaryWriter bw(fs);

    // Write header
    bw.write(getNumEntries());
    bw.write(getDataSize());

    // Write entries
    for (size_t i = 0; i < _entries.size(); i++)
    {
        auto& entry = _entries[i];
        bw.write(static_cast<uint32_t>(entry.dataOffset));
        bw.write(entry.gx.width);
        bw.write(entry.gx.height);
        bw.write(entry.gx.offsetX);
        bw.write(entry.gx.offsetY);
        bw.write(entry.gx.flags);
        bw.write(entry.gx.zoomOffset);
    }

    // Write data
    fs.write(_data.data(), _data.size());
}
