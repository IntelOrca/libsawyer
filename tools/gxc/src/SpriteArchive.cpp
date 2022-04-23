#include "SpriteArchive.h"
#include <algorithm>
#include <sawyer/Stream.h>

using namespace cs;
using namespace gxc;

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

        entry.dataOffset = br.read<uint32_t>();
        entry.width = br.read<uint16_t>();
        entry.height = br.read<uint16_t>();
        entry.offsetX = br.read<uint16_t>();
        entry.offsetY = br.read<uint16_t>();
        entry.flags = br.read<uint16_t>();
        entry.zoomOffset = br.read<uint16_t>();

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
    if (index >= _entries.size())
        throw std::invalid_argument("Invalid index");

    return _entries[index];
}

stdx::span<const std::byte> SpriteArchive::getEntryData(uint32_t index) const
{
    if (index >= _entries.size())
        throw std::invalid_argument("Invalid index");

    const auto& entry = _entries[index];
    const auto* ptr = _data.data() + entry.dataOffset;
    return stdx::span<const std::byte>(ptr, entry.dataLength);
}

GxEntry SpriteArchive::getGx(uint32_t index) const
{
    const auto& entry = getEntry(index);
    const auto& entryData = getEntryData(index);

    GxEntry gx;
    gx.offset = entryData.data();
    gx.width = entry.width;
    gx.height = entry.height;
    gx.offsetX = entry.offsetX;
    gx.offsetY = entry.offsetY;
    gx.flags = entry.flags;
    gx.zoomOffset = entry.zoomOffset;
    return gx;
}

void SpriteArchive::addEntry(const Entry& entry, stdx::span<const std::byte> data)
{
    _entries.push_back(entry);

    auto& newEntry = _entries.back();
    newEntry.dataOffset = static_cast<uint32_t>(_data.size());
    newEntry.dataLength = static_cast<uint32_t>(data.size());
    _data.insert(_data.end(), data.begin(), data.end());
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
        bw.write(entry.width);
        bw.write(entry.height);
        bw.write(entry.offsetX);
        bw.write(entry.offsetY);
        bw.write(entry.flags);
        bw.write(entry.zoomOffset);
    }

    // Write data
    fs.write(_data.data(), _data.size());
}
