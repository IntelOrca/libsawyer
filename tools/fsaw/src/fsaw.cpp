#include <cstdio>
#include <sawyer/SawyerStream.h>
#include <vector>

using namespace cs;

enum class S5Type : uint8_t
{
    savedGame = 0,
    scenario = 1,
    landscape = 3,
};

enum S5Flags : uint8_t
{
    isRaw = 1 << 0,
    isDump = 1 << 1,
    hasSaveDetails = 1 << 3,
};

#pragma pack(push, 1)
struct S5Header
{
    S5Type type;
    uint8_t flags;
    uint16_t numPackedObjects;
    uint32_t version;
    uint32_t magic;
    std::byte padding[20];
};
#pragma pack(pop)
static_assert(sizeof(S5Header) == 0x20);

struct DataBlob
{
    bool isChunk{};
    std::vector<uint8_t> data;

    template<typename T>
    DataBlob(bool isc, const T& value)
    {
        isChunk = isc;
        auto* ptr = reinterpret_cast<const uint8_t*>(&value);
        data = std::vector<uint8_t>(ptr, ptr + sizeof(value));
    }

    DataBlob(bool isc, const stdx::span<uint8_t const>& value)
    {
        isChunk = isc;
        data = std::vector<uint8_t>(value.begin(), value.end());
    }
};

static std::string toString(S5Type type)
{
    switch (type)
    {
        case S5Type::savedGame: return "Saved Game";
        case S5Type::scenario: return "Scenario";
        case S5Type::landscape: return "Landscape";
        default: return "Unknown (" + std::to_string(static_cast<int32_t>(type)) + ")";
    }
}

int main(int argc, const char** argv)
{
    if (argc <= 1)
    {
        std::printf(
            "usage: fsaw [-d] [-o <file>] <file>\n"
            "options:\n"
            "    -d     Decode all chunks\n"
            "    -o     Specify ouput path\n");
        return 1;
    }

    bool decodeAllChunks{};
    std::string outputPath;
    std::string inputPath;
    for (int i = 1; i < argc; i++)
    {
        auto arg = std::string_view(argv[i]);
        if (arg == "-d")
        {
            decodeAllChunks = true;
        }
        else if (arg == "-o")
        {
            i++;
            if (i >= argc)
            {
                std::fprintf(stderr, "No output path specified.\n");
                return 1;
            }
            outputPath = std::string(argv[i]);
        }
        else if (i + 1 < argc)
        {
            std::fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
        else
        {
            inputPath = std::string(arg);
            if (outputPath.empty())
                outputPath = inputPath;
        }
    }

    try
    {
        SawyerStreamReader reader(fs::u8path(inputPath));

        S5Header header;
        reader.readChunk(&header, sizeof(header));

        if (decodeAllChunks)
        {
            // Read in chunks
            std::vector<DataBlob> blobs;
            blobs.emplace_back(true, header);
            if (header.type == S5Type::landscape)
            {
                blobs.emplace_back(true, reader.readChunk());
            }
            if (header.flags & S5Flags::hasSaveDetails)
            {
                blobs.emplace_back(true, reader.readChunk());
            }
            for (size_t i = 0; i < header.numPackedObjects; i++)
            {
                uint8_t objHeader[16];
                reader.read(objHeader, sizeof(objHeader));
                auto objData = reader.readChunk();

                blobs.emplace_back(false, stdx::make_span(objHeader));
                blobs.emplace_back(true, objData);
            }
            blobs.emplace_back(true, reader.readChunk());

            if (header.type == S5Type::scenario)
            {
                blobs.emplace_back(true, reader.readChunk());
                blobs.emplace_back(true, reader.readChunk());
                blobs.emplace_back(true, reader.readChunk());
            }
            else
            {
                blobs.emplace_back(true, reader.readChunk());
            }

            blobs.emplace_back(true, reader.readChunk());

            // Write out chunks
            try
            {
                SawyerStreamWriter writer(fs::u8path(outputPath));
                for (const auto& blob : blobs)
                {
                    if (blob.isChunk)
                        writer.writeChunk(SawyerEncoding::uncompressed, blob.data.data(), blob.data.size());
                    else
                        writer.write(blob.data.data(), blob.data.size());
                }
                writer.writeChecksum();
                writer.close();
            }
            catch (...)
            {
                std::fprintf(stderr, "Unable to write %s\n", outputPath.c_str());
                return 2;
            }
        }
        else
        {
            std::printf("Type: %s\n", toString(header.type).c_str());
            std::printf("Flags: %u\n", header.flags);
            std::printf("Packed objects: %u\n", header.numPackedObjects);
            std::printf("Version: %u\n", header.version);
            std::printf("Magic: 0x%.8X\n", header.magic);

            auto valid = reader.validateChecksum();
            if (valid)
            {
                std::printf("Checksum: \033[0;32mvalid\033[0m\n");
            }
            else
            {
                std::printf("Checksum: \033[0;31minvalid\033[0m\n");
            }
        }
        return 0;
    }
    catch (...)
    {
        std::fprintf(stderr, "Unable to read %s\n", inputPath.c_str());
        return 2;
    }
}
