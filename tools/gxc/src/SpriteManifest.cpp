#include "SpriteManifest.h"
#include "SpriteArchive.h"
#include <nlohmann/json.hpp>
#include <sawyer/Stream.h>

using namespace cs;
using namespace gxc;

struct Range
{
    int32_t Lower{};
    int32_t Upper{};

    Range() = default;

    Range(int32_t single)
        : Lower(single)
        , Upper(single)
    {
    }

    Range(int32_t lower, int32_t upper)
        : Lower(lower)
        , Upper(upper)
    {
    }

    int32_t getLength() const
    {
        return Upper - Lower + 1;
    }
};

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

static std::optional<int32_t> parseInteger(std::string_view s)
{
    if (s.empty())
        return std::nullopt;
    auto neg = 1;
    if (s[0] == '-')
    {
        s = s.substr(1);
        neg = -1;
    }
    int64_t num = 0;
    for (auto ch : s)
    {
        if (ch >= '0' && ch <= '9')
        {
            num *= 10;
            num += (ch - '0');
        }
        else
        {
            return std::nullopt;
        }
    }
    num *= neg;
    if (num < std::numeric_limits<int32_t>::min() || num > std::numeric_limits<int32_t>::max())
    {
        return std::nullopt;
    }
    return static_cast<int32_t>(num);
}

static std::optional<Range> parseRange(std::string_view s)
{
    if (s.size() < 3)
        return std::nullopt;
    if (s[0] != '[' || s[s.size() - 1] != ']')
        return std::nullopt;
    s = s.substr(1, s.size() - 2);

    auto middleIndex = s.find("..");
    if (middleIndex == std::string_view::npos)
    {
        // E.g. [0]
        auto left = parseInteger(s);
        if (left)
        {
            return Range(*left);
        }
    }
    else
    {
        // E.g. [0..2]
        auto left = parseInteger(s.substr(0, middleIndex));
        auto right = parseInteger(s.substr(middleIndex + 2));
        if (left && right)
        {
            return Range(*left, *right);
        }
    }
    return std::nullopt;
}

static SpriteManifest::Entry parseEntry(const nlohmann::json& jEntry)
{
    SpriteManifest::Entry result;
    if (jEntry.is_string())
    {
        auto value = jEntry.get<std::string>();
        if (value.empty())
        {
            result.format = SpriteManifest::Format::empty;
            result.count = 1;
        }
        else
        {
            if (value[0] == '$')
            {
                auto range = parseRange(value.substr(1));
                if (!range)
                    throw std::runtime_error("Invalid range: " + value);

                result.format = SpriteManifest::Format::empty;
                result.count = range->getLength();
            }
            else
            {
                throw std::runtime_error("Only \"\" or \"$[#..#]\" syntax supported.");
            }
        }
    }
    else
    {
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
    }
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

    auto text = FileStream::readAllText(path);
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

std::string SpriteManifest::buildManifest(const SpriteArchive& archive)
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
            sb.append("        \"x\": ");
            sb.append(std::to_string(entry.offsetX));
            sb.append(",\n");
        }
        if (entry.offsetY != 0)
        {
            sb.append("        \"y\": ");
            sb.append(std::to_string(entry.offsetY));
            sb.append(",\n");
        }
        if ((entry.flags & GxFlags::hasZoom) && entry.zoomOffset != 0)
        {
            sb.append("        \"zoom\": ");
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
