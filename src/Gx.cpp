#include "Gx.h"
#include <cstring>
#include <limits>
#include <optional>

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

void GxEntry::convertToBmp(Stream& stream) const
{
    auto bufferLen = width * height;
    auto buffer = std::make_unique<uint8_t[]>(bufferLen);
    convertToBmp(buffer.get());
    stream.write(buffer.get(), bufferLen);
}

void GxEntry::convertToBmp(void* dst) const
{
    if (flags & GxFlags::rle)
    {
        convertRleToBmp(dst);
    }
    else if (flags & GxFlags::isPalette)
    {
        throw std::runtime_error("Palette can not be converted to bmp");
    }
    else
    {
        std::memcpy(dst, offset, width * height);
    }
}

void GxEntry::convertRleToBmp(void* dst) const
{
    auto startAddress = static_cast<const uint8_t*>(offset);
    auto dst8 = static_cast<uint8_t*>(dst);
    for (int32_t y = 0; y < height; y++)
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
        dst8 += width;
    }
}

bool GxEncoder::isWorthUsingRle(const ImageBuffer8& input)
{
    const auto* src = input.data;
    uint32_t averageTransparentRun = 0;
    uint32_t transparentRuns = 0;
    for (uint16_t y = 0; y < input.height; y++)
    {
        uint32_t transparentRun = 0;
        for (uint16_t x = 0; x < input.width; x++)
        {
            auto c = *src++;
            if (c == 0)
            {
                transparentRun++;
            }
            else if (transparentRun != 0)
            {
                averageTransparentRun += transparentRun;
                transparentRuns++;
                transparentRun = 0;
            }
        }
        if (transparentRun != 0)
        {
            averageTransparentRun += transparentRun;
            transparentRuns++;
        }
    }
    if (transparentRuns > 0)
    {
        averageTransparentRun /= transparentRuns;
    }

    // If on average all the transparent runs are above 4
    return averageTransparentRun >= 4 && transparentRuns > 4;
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
            }
            else
            {
                // Not transparent
                if (currentCode.NumPixels == 0)
                {
                    currentCode.OffsetX = x;
                    currentCode.Pixels = src - 1;
                }
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
    stream.write(currentCode.Pixels, currentCode.NumPixels & GxRleRowLengthMask);
    currentCode = {};
}
