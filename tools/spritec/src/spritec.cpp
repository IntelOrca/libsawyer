#include "spritec.h"
#include "CommandLine.h"
#include "SpriteArchive.h"
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

static void convertRleToBmp(const GxEntry& entry, void* dst)
{
    assert(entry.flags & GxFlags::rle);

    auto startAddress = static_cast<const uint8_t*>(entry.offset);
    auto dst8 = static_cast<uint8_t*>(dst);
    for (int32_t y = 0; y < entry.height; y++)
    {
        // Set source pointer to the start of the row
        auto rowOffset8 = startAddress + (y * 2);
        uint16_t rowOffset = rowOffset8[0] | (rowOffset8[1] << 8);
        auto src8 = startAddress + rowOffset;

        // Read row offset
        int32_t x = 0;
        auto endOfRow = false;
        do
        {
            // Get run length, end flag and x position
            auto code = *src8++;
            auto len = code & GxRleRowLengthMask;
            endOfRow = (code & GxRleRowEndFlag) != 0;
            x = *src8++;

            // Copy pixel run to destination buffer
            std::memcpy(dst8 + x, src8, len);
            src8 += len;
        } while (!endOfRow);

        // Move destination pointer to next row
        dst8 += entry.width;
    }
}

static void convertPaletteToBmp(const GxEntry& entry, void* dst)
{
    auto src8 = static_cast<const uint8_t*>(entry.offset);
    auto dst8 = static_cast<uint8_t*>(dst);
    for (int32_t x = 0; x < entry.width; x++)
    {
        dst8[0] = src8[2];
        dst8[1] = src8[1];
        dst8[2] = src8[0];
        dst8[3] = 0xFF;
        src8 += 3;
        dst8 += 4;
    }
}

static std::string stringifyFlags(uint16_t flags)
{
    std::string result;
    if (flags & GxFlags::bmp)
        result += "bmp|";
    if (flags & GxFlags::rle)
        result += "rle|";
    if (flags & GxFlags::isPalette)
        result += "isPalette|";
    if (flags & GxFlags::hasZoom)
        result += "hasZoom|";
    if (flags & GxFlags::noZoom)
        result += "noZoom|";
    if (result.size() >= 1)
        result.resize(result.size() - 1);
    return result;
}

static std::optional<SpriteArchive> readGxFileOrError(std::string_view path)
{
    try
    {
        return SpriteArchive::fromFile(fs::u8path(path));
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "Unable to read gx file: %s\n", e.what());
        return {};
    }
}

int runDetails(const CommandLineOptions& options)
{
    auto archive = readGxFileOrError(options.path);
    if (!archive)
    {
        return ExitCodes::fileError;
    }

    std::printf("gx file:\n");
    std::printf("    numEntries: %d\n", archive->getNumEntries());
    std::printf("    dataSize: %d\n\n", archive->getDataSize());

    if (!options.idx)
    {
        return ExitCodes::ok;
    }

    auto idx = *options.idx;
    auto numEntries = archive->getNumEntries();
    if (idx >= 0 && static_cast<uint32_t>(idx) < numEntries)
    {
        std::printf("entry %d:\n", idx);
        auto entry = archive->getEntry(idx);
        auto szFlags = stringifyFlags(entry.flags);

        std::printf("    width: %d\n", entry.width);
        std::printf("    height: %d\n", entry.height);
        std::printf("    offset X: %d\n", entry.offsetX);
        std::printf("    offset Y: %d\n", entry.offsetY);
        std::printf("    zoom offset: %d\n", entry.zoomOffset);
        std::printf("    flags: 0x%02X (%s)\n", entry.flags, szFlags.c_str());
        std::printf("    data offset: 0x%08X\n", entry.dataOffset);
        std::printf("    data length: %u\n", entry.dataLength);

        auto gx = archive->getGx(idx);
        auto [valid, dataSize] = gx.calculateDataSize(entry.dataLength);
        std::printf("    calculated data length: %lld (%s)\n", dataSize, valid ? "valid" : "invalid");
        return ExitCodes::ok;
    }
    else
    {
        std::fprintf(stderr, "    invalid entry index\n");
        return ExitCodes::invalidArgument;
    }
}

int runList(const CommandLineOptions& options)
{
    auto archive = readGxFileOrError(options.path);
    if (!archive)
    {
        return ExitCodes::fileError;
    }

    std::printf("index offset     length width height offsetX offsetY zoomOffset flags\n");
    auto numEntries = archive->getNumEntries();
    for (uint32_t i = 0; i < numEntries; i++)
    {
        const auto& entry = archive->getEntry(i);
        auto szFlags = stringifyFlags(entry.flags);
        std::printf("%05d 0x%08X %6u %5d %6d %7d %7d %10d %s\n", i, entry.dataOffset, entry.dataLength, entry.width, entry.height, entry.offsetX, entry.offsetY, entry.zoomOffset, szFlags.c_str());
    }
    return ExitCodes::ok;
}

static std::string buildManifest(const SpriteArchive& archive)
{
    std::string sb;
    sb.append("[\n");
    auto numEntries = archive.getNumEntries();
    for (uint32_t i = 0; i < numEntries; i++)
    {
        auto& entry = archive.getEntry(i);
        sb.append("    {\n");

        char filename[32];
        std::snprintf(filename, sizeof(filename), "%05d.png", i);
        sb.append("        \"path\": \"");
        sb.append(filename);
        sb.append("\",\n");

        if (entry.flags & GxFlags::isPalette)
        {
            sb.append("        \"format\": \"palette\",\n");
        }
        else
        {
            if (entry.flags & GxFlags::rle)
            {
                sb.append("        \"format\": \"rle\",\n");
            }
            else
            {
                sb.append("        \"format\": \"bmp\",\n");
            }
            sb.append("        \"palette\": \"keep\",\n");
        }

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
        sb.erase(sb.size() - 2, 2);
        sb.append("\n    },\n");
    }
    sb.erase(sb.size() - 2, 2);
    sb.append("\n]\n");
    return sb;
}

static void exportManifest(const SpriteArchive& archive, const fs::path& manifestPath)
{
    auto manifest = buildManifest(archive);
    FileStream fs(manifestPath, StreamFlags::write);
    fs.write(manifest.data(), manifest.size());
}

static void exportImage(const GxEntry& entry, const Palette& palette, const fs::path& imageFilename)
{
    Image image;
    if (entry.flags & GxFlags::isPalette)
    {
        image.width = entry.width;
        image.height = 1;
        image.depth = 32;
        image.pixels = std::vector<uint8_t>(entry.width * 4);
        image.stride = entry.width * 4;
        convertPaletteToBmp(entry, image.pixels.data());
    }
    else
    {
        image.width = entry.width;
        image.height = entry.height;
        image.palette = std::make_unique<Palette>(palette);
        image.depth = 8;
        image.pixels = std::vector<uint8_t>(entry.width * entry.height);
        image.stride = entry.width;
        if (entry.flags & GxFlags::rle)
        {
            convertRleToBmp(entry, image.pixels.data());
        }
        else
        {
            std::memcpy(image.pixels.data(), entry.offset, image.pixels.size());
        }
    }

    FileStream pngfs(imageFilename, StreamFlags::write);
    image.WriteToPng(pngfs);
}

int runExport(const CommandLineOptions& options)
{
    auto archive = readGxFileOrError(options.path);
    if (!archive)
    {
        return ExitCodes::fileError;
    }

    if (!options.idx)
    {
        std::cerr << "index not specified" << std::endl;
        return ExitCodes::invalidArgument;
    }

    auto idx = *options.idx;
    auto numEntries = archive->getNumEntries();
    if (idx >= 0 && static_cast<uint32_t>(idx) < numEntries)
    {
        const auto gx = archive->getGx(idx);
        auto& palette = GetStandardPalette();
        auto imageFilename = fs::u8path(options.outputPath);
        exportImage(gx, palette, imageFilename);
        return ExitCodes::ok;
    }
    else
    {
        std::fprintf(stderr, "    invalid entry index\n");
        return ExitCodes::invalidArgument;
    }
}

int runExportAll(const CommandLineOptions& options)
{
    auto archive = readGxFileOrError(options.path);
    if (!archive)
    {
        return ExitCodes::fileError;
    }

    auto outputDirectory = fs::u8path(options.outputPath);
    if (fs::is_directory(outputDirectory) || fs::create_directories(outputDirectory))
    {
        exportManifest(*archive, outputDirectory / "manifest.json");

        auto& palette = GetStandardPalette();
        auto numEntries = archive->getNumEntries();
        for (uint32_t i = 0; i < numEntries; i++)
        {
            const auto gx = archive->getGx(i);

            char filename[32]{};
            std::snprintf(filename, sizeof(filename), "%05d.png", i);
            if (!options.quiet)
            {
                printf("Writing %s...\n", filename);
            }

            auto imageFilename = outputDirectory / filename;
            exportImage(gx, palette, imageFilename);
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
        else if (firstArg == "list")
        {
            options.action = CommandLineAction::list;
            options.path = parser.getArg(1);
        }
        else if (firstArg == "export")
        {
            options.action = CommandLineAction::exportSingle;
            options.path = parser.getArg(1);
            options.idx = parser.getArg<int32_t>(2);
            options.outputPath = parser.getArg(3);
        }
        else if (firstArg == "exportall")
        {
            options.action = CommandLineAction::exportAll;
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
    std::cout << "               list      <gx_file>" << std::endl;
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
            case CommandLineAction::list:
                runList(*options);
                break;
            case CommandLineAction::exportSingle:
                runExport(*options);
                break;
            case CommandLineAction::exportAll:
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
