#include "spritec.h"
#include "CommandLine.h"
#include <cstdio>
#include <iostream>
#include <optional>
#include <sawyer/Image.h>
#include <sawyer/Palette.h>
#include <sawyer/SawyerStream.h>
#include <sawyer/Stream.h>
#include <string>
#include <string_view>

using namespace cs;
using namespace spritec;

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

static std::optional<GxFile> readGxFileOrError(std::string_view path)
{
    try
    {
        return loadGxFile(fs::u8path(path));
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "Unable to read gx file: %s\n", e.what());
        return {};
    }
}

int runDetails(const CommandLineOptions& options)
{
    auto gxFile = readGxFileOrError(options.path);
    if (!gxFile)
    {
        return ExitCodes::fileError;
    }

    std::printf("gx file:\n");
    std::printf("    numEntries: %d\n", gxFile->header.numEntries);
    std::printf("    dataSize: %d\n\n", gxFile->header.dataSize);

    if (!options.idx)
    {
        std::cerr << "index not specified" << std::endl;
        return ExitCodes::invalidArgument;
    }

    auto idx = *options.idx;
    std::printf("entry %d:\n", idx);
    if (idx >= 0 && idx < gxFile->entries.size())
    {
        const auto& entry = gxFile->entries[idx];
        auto offset = static_cast<uint32_t>(reinterpret_cast<uint8_t*>(entry.offset) - gxFile->data.get());
        auto szFlags = stringifyFlags(entry.flags);

        std::printf("    width: %d\n", entry.width);
        std::printf("    height: %d\n", entry.height);
        std::printf("    offset X: %d\n", entry.offsetX);
        std::printf("    offset Y: %d\n", entry.offsetY);
        std::printf("    zoom offset: %d\n", entry.zoomOffset);
        std::printf("    flags: 0x%02X (%s)\n", entry.flags, szFlags.c_str());
        std::printf("    data offset Y: 0x%08X\n", offset);

        auto bufferLen = gxFile->header.dataSize - (static_cast<uint8_t*>(entry.offset) - gxFile->data.get());
        auto [valid, dataSize] = entry.calculateDataSize(bufferLen);
        std::printf("    data length: %lld (%s)\n", dataSize, valid ? "valid" : "invalid");
        return ExitCodes::ok;
    }
    else
    {
        std::fprintf(stderr, "    invalid entry index\n");
        return ExitCodes::invalidArgument;
    }
}

static std::string buildManifest(const GxFile& gxFile)
{
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
    return sb;
}

static void exportManifest(const GxFile& gxFile, const fs::path& manifestPath)
{
    auto manifest = buildManifest(gxFile);
    FileStream fs(manifestPath, StreamFlags::write);
    fs.write(manifest.data(), manifest.size());
}

static void exportImage(const GxEntry& entry, const Palette& palette, const fs::path& imageFilename)
{
    Image image;
    image.width = entry.width;
    image.height = entry.height;
    image.depth = 8;
    image.palette = std::make_unique<Palette>(palette);

    auto src8 = static_cast<uint8_t*>(entry.offset);
    image.pixels = std::vector<uint8_t>(src8, src8 + (entry.width * entry.height));
    image.stride = entry.width;

    FileStream pngfs(imageFilename, StreamFlags::write);
    image.WriteToPng(pngfs);
}

int runExportAll(const CommandLineOptions& options)
{
    auto gxFile = readGxFileOrError(options.path);
    if (!gxFile)
    {
        return ExitCodes::fileError;
    }

    auto outputDirectory = fs::u8path(options.outputPath);
    if (fs::is_directory(outputDirectory) || fs::create_directories(outputDirectory))
    {
        exportManifest(*gxFile, outputDirectory / "manifest.json");

        auto& palette = GetStandardPalette();
        for (size_t i = 0; i < gxFile->header.numEntries; i++)
        {
            const auto& entry = gxFile->entries[i];
            if (entry.flags & GxFlags::rle)
                continue;

            char filename[32]{};
            std::snprintf(filename, sizeof(filename), "%05lld.png", i);
            if (!options.quiet)
            {
                printf("Writing %s...\n", filename);
            }

            auto imageFilename = outputDirectory / filename;
            exportImage(entry, palette, imageFilename);
        }
        return ExitCodes::ok;
    }
    else
    {
        std::cerr << "Unable to create or access output directory" << std::endl;
        return ExitCodes::fileError;
    }
}

std::optional<CommandLineOptions> parseCommandLine(int argc, const char** argv)
{
    auto parser = CommandLineParser(argc, argv)
                      .registerOption("-q")
                      .registerOption("--help", "-h")
                      .registerOption("--version");
    if (!parser.parse())
    {
        std::cerr << parser.getErrorMessage() << std::endl;
        return {};
    }

    CommandLineOptions options;
    if (parser.hasOption("--version"))
    {
        options.action = CommandLineAction::version;
    }
    else if (parser.hasOption("--help") || parser.hasOption("-h"))
    {
        options.action = CommandLineAction::help;
    }
    else
    {
        auto firstArg = parser.getArg(0);
        if (firstArg == "details")
        {
            options.action = CommandLineAction::details;
            options.path = parser.getArg(1);
            options.idx = parser.getArg<int32_t>(2);
        }
        else if (firstArg == "exportall")
        {
            options.action = CommandLineAction::exportall;
            options.path = parser.getArg(1);
            options.outputPath = parser.getArg(2);
        }
        else
        {
            options.action = CommandLineAction::help;
        }
    }

    options.quiet = parser.hasOption("-q");

    return options;
}

static std::string getVersionInfo()
{
    return "1.0";
}

static void printVersion()
{
    std::cout << getVersionInfo() << std::endl;
}

static void printHelp()
{
    std::cout << "gx Sprite Archive Tool " << getVersionInfo() << std::endl;
    std::cout << std::endl;
    std::cout << "usage: spritec append    <gx_file> <input> [x_offset y_offset]" << std::endl;
    std::cout << "               build     <gx_file> <json_path>" << std::endl;
    std::cout << "               create    <gx_file>" << std::endl;
    std::cout << "               details   <gx_file> [idx]" << std::endl;
    std::cout << "               export    <gx_file> [idx] <output_file>" << std::endl;
    std::cout << "               exportall <gx_file> <output_directory>" << std::endl;
    std::cout << "options:" << std::endl;
    std::cout << "           -q     Quiet" << std::endl;
    std::cout << "--help     -h     Print help" << std::endl;
    std::cout << "--version         Print version" << std::endl;
}

int main(int argc, const char** argv)
{
    try
    {
        auto options = parseCommandLine(argc, argv);
        if (!options)
        {
            return ExitCodes::commandLineParseError;
        }

        switch (options->action)
        {
            case CommandLineAction::details:
                runDetails(*options);
                break;
            case CommandLineAction::exportall:
                runExportAll(*options);
                break;
            default:
                printHelp();
                return ExitCodes::commandLineError;
        }
    }
    catch (std::exception& e)
    {
        fprintf(stderr, "An error occurred: %s\n", e.what());
        return ExitCodes::exception;
    }
}
