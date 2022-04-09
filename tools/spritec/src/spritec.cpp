#include <cstdio>
#include <sawyer/SawyerStream.h>
#include <sawyer/Stream.h>
#include <string>
#include <string_view>

using namespace cs;

namespace cs
{
    namespace GxFlags
    {
        constexpr uint16_t bmp = 1 << 0;       // Image data is encoded as raw pixels
        constexpr uint16_t rle = 1 << 2;       // Image data is encoded using run length encoding
        constexpr uint16_t isPalette = 1 << 3; // Image data is a sequence of palette entries R8G8B8
        constexpr uint16_t hasZoom = 1 << 4;   // Use a different sprite for higher zoom levels
        constexpr uint16_t noZoom = 1 << 5;    // Does not get drawn at higher zoom levels (only zoom 0)
    }

    struct GxHeader
    {
        uint32_t numEntries{};
        uint32_t dataSize{};
    };

    struct GxEntry
    {
        void* offset{};
        int16_t width{};
        int16_t height{};
        int16_t offsetX{};
        int16_t offsetY{};
        uint16_t flags{};
        uint16_t zoomOffset{};
    };

    struct GxFile
    {
        GxHeader header;
        std::vector<GxEntry> entries;
        std::unique_ptr<uint8_t[]> data;
    };
}

class BinaryReader
{
private:
    Stream* _stream{};

public:
    BinaryReader(Stream& stream)
        : _stream(&stream)
    {
    }

    template<typename T>
    T read()
    {
        T buffer;
        _stream->read(&buffer, sizeof(T));
        return buffer;
    }
};

static GxFile loadGxFile(const fs::path& path)
{
    FileStream fs(path, StreamFlags::read);
    BinaryReader br(fs);

    GxFile file;
    file.header.numEntries = br.read<uint32_t>();
    file.header.dataSize = br.read<uint32_t>();

    file.data = std::make_unique<uint8_t[]>(file.header.dataSize);

    file.entries.resize(file.header.numEntries);
    for (size_t i = 0; i < file.entries.size(); i++)
    {
        auto& entry = file.entries[i];
        entry.offset = file.data.get() + static_cast<uintptr_t>(br.read<uint32_t>());
        entry.width = br.read<uint16_t>();
        entry.height = br.read<uint16_t>();
        entry.offsetX = br.read<uint16_t>();
        entry.offsetY = br.read<uint16_t>();
        entry.flags = br.read<uint16_t>();
        entry.zoomOffset = br.read<uint16_t>();
    }

    fs.read(file.data.get(), file.header.dataSize);

    return file;
}

static int showHelp()
{
    std::printf(
        "usage: spritec append    <gx_file> <input> [x_offset y_offset]\n"
        "               build     <gx_file> <json_path>\n"
        "               create    <gx_file>\n"
        "               details   <gx_file> [idx]\n"
        "               export    <gx_file> [idx] <output_file>\n"
        "               exportall <gx_file> <output_directory>\n"
        "options:\n"
        "    -q     Quiet\n");
    return 1;
}

static std::string stringifyFlags(uint16_t flags)
{
    std::string result;
    if (flags & GxFlags::bmp)
        result += "bmp, ";
    if (flags & GxFlags::rle)
        result += "rle, ";
    if (flags & GxFlags::isPalette)
        result += "isPalette, ";
    if (flags & GxFlags::hasZoom)
        result += "hasZoom, ";
    if (flags & GxFlags::noZoom)
        result += "noZoom, ";
    if (result.size() >= 2)
        result.resize(result.size() - 2);
    return result;
}

int main(int argc, const char** argv)
{
    if (argc <= 2)
    {
        return showHelp();
    }

    auto command = std::string_view(argv[1]);
    if (command == "details")
    {
        if (argc <= 3)
        {
            return showHelp();
        }

        auto gxPath = std::string_view(argv[2]);
        GxFile gxFile;
        try
        {
            gxFile = loadGxFile(fs::u8path(gxPath));
        }
        catch (const std::exception& e)
        {
            std::fprintf(stderr, "Unable to read gx file: %s\n", e.what());
            return -1;
        }

        std::printf("gx file:\n");
        std::printf("    numEntries: %d\n", gxFile.header.numEntries);
        std::printf("    dataSize: %d\n\n", gxFile.header.dataSize);

        auto idx = std::stoi(argv[3]);
        std::printf("entry %d:\n", idx);
        if (idx >= 0 && idx < gxFile.entries.size())
        {
            const auto& entry = gxFile.entries[idx];
            auto offset = static_cast<uint32_t>(reinterpret_cast<uint8_t*>(entry.offset) - gxFile.data.get());
            auto szFlags = stringifyFlags(entry.flags);

            std::printf("    width: %d\n", entry.width);
            std::printf("    height: %d\n", entry.height);
            std::printf("    offset X: %d\n", entry.offsetX);
            std::printf("    offset Y: %d\n", entry.offsetY);
            std::printf("    zoom offset: %d\n", entry.zoomOffset);
            std::printf("    flags: 0x%02X (%s)\n", entry.flags, szFlags.c_str());
            std::printf("    data offset Y: 0x%08X\n", offset);
        }
        else
        {
            std::fprintf(stderr, "    invalid entry index\n");
            return -1;
        }
    }
    return 0;
}
