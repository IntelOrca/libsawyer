#include "SpriteManifest.h"
#include <nlohmann/json.hpp>
#include <sawyer/Stream.h>

using namespace cs;

std::string readAllText(const fs::path& path)
{
    std::string result;

    FileStream fs(path, StreamFlags::read);
    if (fs.getLength() >= 3)
    {
        // Read BOM
        uint8_t bom[3]{};
        fs.read(bom, sizeof(bom));
        if (!(bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF))
        {
            result.append(reinterpret_cast<char*>(bom), sizeof(bom));
        }
    }

    auto remainingLen = fs.getLength() - fs.getPosition();
    auto start = result.size();
    result.resize(start + remainingLen);
    fs.read(result.data() + start, remainingLen);
    return result;
}

static SpriteManifest::Format parseFormat(const nlohmann::json& jFormat)
{
    auto value = jFormat.get<std::string>();
    if (value == "bmp" || value == "raw")
        return SpriteManifest::Format::bmp;
    else if (value == "rle")
        return SpriteManifest::Format::rle;
    else if (value == "palette")
        return SpriteManifest::Format::palette;
    else
        throw std::runtime_error("Unknown format");
}

static SpriteManifest::PaletteKind parsePalette(const nlohmann::json& jFormat)
{
    auto value = jFormat.get<std::string>();
    if (value == "keep")
        return SpriteManifest::PaletteKind::keep;
    else
        throw std::runtime_error("Unknown palette kind");
}

static SpriteManifest::Entry parseEntry(const nlohmann::json& jEntry)
{
    SpriteManifest::Entry result;
    result.path = jEntry.at("path").get<std::string>();
    if (jEntry.contains("format"))
        result.format = parseFormat(jEntry["format"]);
    if (jEntry.contains("palette"))
        result.palette = parsePalette(jEntry["palette"]);
    if (jEntry.contains("x"))
        result.offsetX = jEntry["x"];
    else if (jEntry.contains("x_offset"))
        result.offsetX = jEntry["x_offset"];
    if (jEntry.contains("y"))
        result.offsetY = jEntry["y"];
    else if (jEntry.contains("y_offset"))
        result.offsetY = jEntry["y_offset"];
    else if (jEntry.contains("zoom"))
        result.zoomOffset = jEntry["zoom"];
    if (jEntry.contains("srcX"))
        result.srcX = jEntry["srcX"];
    if (jEntry.contains("srcY"))
        result.srcY = jEntry["srcY"];
    if (jEntry.contains("srcWidth"))
        result.srcWidth = jEntry["srcWidth"];
    if (jEntry.contains("srcHeight"))
        result.srcHeight = jEntry["srcHeight"];
    return result;
}

static fs::path updatePath(const fs::path& workingDirectory, const fs::path& path)
{
    if (path.is_absolute())
        return path;
    return (workingDirectory / path).make_preferred();
}

SpriteManifest SpriteManifest::fromFile(const fs::path& path)
{
    auto workingDirectory = path.parent_path();

    auto text = readAllText(path);
    auto jRoot = nlohmann::json::parse(text);
    if (jRoot.is_array())
    {
        SpriteManifest manifest;
        manifest.entries.reserve(jRoot.size());
        for (const auto& jEntry : jRoot)
        {
            auto entry = parseEntry(jEntry);
            entry.path = updatePath(workingDirectory, entry.path);
            manifest.entries.push_back(entry);
        }
        return manifest;
    }
    else
    {
        throw std::runtime_error("Invalid manifest");
    }
}
