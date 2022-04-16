// Code that will go into libsawyer
#include "external.h"
#include <limits>
#include <optional>
#include <sawyer/Stream.h>

using namespace cs;

size_t GxEntry::calculateDataSize() const
{
    auto [success, size] = calculateDataSize(std::numeric_limits<size_t>::max());
    return size;
}

bool GxEntry::validateData(size_t bufferLen) const
{
    auto [success, size] = calculateDataSize(bufferLen);
    return success;
}

std::pair<bool, size_t> GxEntry::calculateDataSize(size_t bufferLen) const
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

std::pair<bool, size_t> GxEntry::calculateRleSize(size_t bufferLen) const
{
    BinaryStream bs(offset, bufferLen);
    BinaryReader br(bs);

    std::optional<bool> bufferWasLargeEnough;

    // Find the row with the largest offset
    auto highestRowOffset = 0;
    for (size_t i = 0; i < height; i++)
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

void GxEncoder::encodeRle(const ImageBuffer8& input, Stream& stream)
{
    const auto* src = input.data;

    // Reserve space for row offsets
    auto rowOffsetBegin = stream.getPosition();
    stream.seek(input.height * 2);

    for (auto y = 0; y < input.height; y++)
    {
        auto rowPosition = stream.getPosition();
        auto rowOffset = static_cast<uint16_t>(rowPosition - rowOffsetBegin);

        // Write row offset in table
        stream.setPosition(rowOffsetBegin + (y * 2));
        stream.write(&rowOffset, sizeof(rowOffset));
        stream.setPosition(rowPosition);

        // Write RLE codes
        RLECode currentCode;
        for (auto x = 0; x < input.width; x++)
        {
            auto paletteIndex = *src++;
            if (paletteIndex == 0)
            {
                // Transparent
                if (currentCode.NumPixels != 0)
                {
                    pushRun(stream, currentCode);
                }
                currentCode.OffsetX++;
            }
            else
            {
                // Not transparent
                currentCode.NumPixels++;
            }
            if (currentCode.NumPixels == GxRleRowLengthMask)
            {
                pushRun(stream, currentCode);
            }
        }
        currentCode.NumPixels |= GxRleRowEndFlag;
        pushRun(stream, currentCode);
    }
}

void GxEncoder::pushRun(Stream& stream, RLECode& currentCode)
{
    stream.write(&currentCode.NumPixels, 1);
    stream.write(&currentCode.OffsetX, 1);
    stream.write(currentCode.Pixels, currentCode.NumPixels);
    currentCode = {};
}
