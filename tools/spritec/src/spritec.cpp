#include <cstdio>
#include <optional>
#include <sawyer/Image.h>
#include <sawyer/Palette.h>
#include <sawyer/SawyerStream.h>
#include <sawyer/Stream.h>
#include <string>
#include <string_view>

using namespace cs;

const Palette& GetStandardPalette();

namespace cs
{
    class BinaryReader final
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

        template<typename T>
        std::optional<T> tryRead()
        {
            auto remainingLen = _stream->getLength() - _stream->getPosition();
            if (remainingLen >= sizeof(T))
            {
                return read<T>();
            }
            return std::nullopt;
        }

        bool trySeek(int64_t len)
        {
            auto actualLen = getSafeSeekAmount(len);
            if (actualLen != 0)
            {
                if (actualLen == len)
                {
                    _stream->seek(actualLen);
                    return true;
                }
                else
                {
                    return false;
                }
            }
            return true;
        }

        bool seekSafe(int64_t len)
        {
            auto actualLen = getSafeSeekAmount(len);
            if (actualLen != 0)
            {
                _stream->seek(actualLen);
                return actualLen == len;
            }
            return true;
        }

    private:
        int64_t getSafeSeekAmount(int64_t len)
        {
            if (len == 0)
            {
                return 0;
            }
            else if (len < 0)
            {
                return -static_cast<int64_t>(std::max<uint64_t>(_stream->getPosition(), -len));
            }
            else
            {
                auto remainingLen = _stream->getLength() - _stream->getPosition();
                return std::min<uint64_t>(len, remainingLen);
            }
        }
    };

    namespace GxFlags
    {
        constexpr uint16_t bmp = 1 << 0;       // Image data is encoded as raw pixels
        constexpr uint16_t rle = 1 << 2;       // Image data is encoded using run length encoding
        constexpr uint16_t isPalette = 1 << 3; // Image data is a sequence of palette entries R8G8B8
        constexpr uint16_t hasZoom = 1 << 4;   // Use a different sprite for higher zoom levels
        constexpr uint16_t noZoom = 1 << 5;    // Does not get drawn at higher zoom levels (only zoom 0)
    }

    constexpr uint8_t GxRleRowLengthMask = 0x7F;
    constexpr uint8_t GxRleRowEndFlag = 0x80;

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

        /**
         * Calculates the expected length of the entry's data based on the width, height and flags.
         * For RLE, the data needs to be read in order to calculate the size, this may not be safe.
         */
        size_t calculateDataSize() const
        {
            auto [success, size] = calculateDataSize(std::numeric_limits<size_t>::max());
            return size;
        }

        bool validateData(size_t bufferLen) const
        {
            auto [success, size] = calculateDataSize(bufferLen);
            return success;
        }

        std::pair<bool, size_t> calculateDataSize(size_t bufferLen) const
        {
            if (offset == nullptr)
            {
                return std::make_pair(true, 0);
            }

            if (flags & GxFlags::isPalette)
            {
                auto len = width * 3;
                return std::make_pair(bufferLen >= len, len);
            }

            if (!(flags & GxFlags::rle))
            {
                auto len = width * height;
                return std::make_pair(bufferLen >= len, len);
            }

            return calculateRleSize(bufferLen);
        }

    private:
        /**
         * Calculates the size of RLE data by scanning the RLE data. If it reaches the end of the buffer,
         * the size of the data read so far is returned.
         * A boolean is returned to indicate whether the buffer was large enough for the RLE data.
         */
        std::pair<bool, size_t> calculateRleSize(size_t bufferLen) const
        {
            BinaryStream bs(offset, bufferLen);
            BinaryReader br(bs);

            std::optional<bool> bufferWasLargeEnough;

            // Find the row with the largest offset
            auto highestRowOffset = 0;
            for (size_t i = 0; i < height - 1; i++)
            {
                auto rowOffset = br.tryRead<uint16_t>();
                if (rowOffset)
                {
                    if (*rowOffset > highestRowOffset)
                        highestRowOffset = *rowOffset;
                }
                else
                {
                    bufferWasLargeEnough = false;
                }
            }

            // Read the elements of the row with the largest offset (usually the last row)
            bs.setPosition(0);
            if (br.seekSafe(highestRowOffset))
            {
                auto endOfRow = false;
                do
                {
                    // read num pixels
                    auto chunk0 = br.tryRead<uint8_t>();
                    if (!chunk0)
                        break;

                    // skip x
                    if (!br.trySeek(1))
                        break;

                    // skip pixels
                    if (!br.trySeek(*chunk0 & GxRleRowLengthMask))
                        break;

                    endOfRow = (*chunk0 & GxRleRowEndFlag) != 0;
                } while (!endOfRow);
                bufferWasLargeEnough = endOfRow;
            }
            return std::make_pair(bufferWasLargeEnough.value_or(false), bs.getPosition());
        }
    };

    struct GxFile
    {
        GxHeader header;
        std::vector<GxEntry> entries;
        std::unique_ptr<uint8_t[]> data;
    };
}

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

            auto bufferLen = gxFile.header.dataSize - (static_cast<uint8_t*>(entry.offset) - gxFile.data.get());
            auto [valid, dataSize] = entry.calculateDataSize(bufferLen);
            std::printf("    data length: %lld (%s)\n", dataSize, valid ? "valid" : "invalid");
        }
        else
        {
            std::fprintf(stderr, "    invalid entry index\n");
            return -1;
        }
    }
    else if (command == "exportall")
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

        auto outputDirectory = fs::u8path(argv[3]);
        if (fs::is_directory(outputDirectory) || fs::create_directories(outputDirectory))
        {
            auto manifestPath = outputDirectory / "manifest.json";

            std::string sb;
            sb.append("[\n");
            for (size_t i = 0; i < gxFile.header.numEntries; i++)
            {
                auto& entry = gxFile.entries[i];
                sb.append("    {\n");

                char filename[32];
                std::snprintf(filename, sizeof(filename), "%05lld.png", i);
                sb.append("        \"path\": \"");
                sb.append(filename);
                sb.append("\",\n");

                if (entry.offsetX != 0)
                {
                    sb.append("        \"x_offset\": ");
                    sb.append(std::to_string(entry.offsetX));
                    sb.append(",\n");
                }
                if (entry.offsetY != 0)
                {
                    sb.append("        \"y_offset\": ");
                    sb.append(std::to_string(entry.offsetY));
                    sb.append(",\n");
                }
                if ((entry.flags & GxFlags::hasZoom) && entry.zoomOffset != 0)
                {
                    sb.append("        \"zoomOffset\": ");
                    sb.append(std::to_string(entry.zoomOffset));
                    sb.append(",\n");
                }
                if (!(entry.flags & GxFlags::rle))
                {
                    sb.append("        \"forceBmp\": true,\n");
                }
                sb.append("        \"palette\": \"keep\"\n");

                sb.append("    },\n");
            }
            sb.erase(sb.size() - 2, 2);
            sb.append("\n]\n");

            FileStream fs(manifestPath, StreamFlags::write);
            fs.write(sb.data(), sb.size());
        }
    }
    return 0;
}
