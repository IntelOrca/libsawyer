#pragma once

#include "Stream.h"
#include <cstring>
#include <stdexcept>

using namespace cs;

void Stream::seek(int64_t pos)
{
    setPosition(getPosition() + pos);
}

void Stream::throwInvalidOperation() { throw std::runtime_error("Invalid operation"); }

BinaryStream::BinaryStream(const void* data, size_t len)
    : _data(data)
    , _len(len)
{
}

uint64_t BinaryStream::getLength() const
{
    return _len;
}

uint64_t BinaryStream::getPosition() const
{
    return _index;
}

void BinaryStream::setPosition(uint64_t position)
{
    if (position > _len)
        throw std::out_of_range("Position too large");
    _index = static_cast<size_t>(position);
}

void BinaryStream::read(void* buffer, size_t len)
{
    auto maxReadLen = _len - _index;
    if (len > maxReadLen)
        throw std::runtime_error("Failed to read data");
    std::memcpy(buffer, reinterpret_cast<const void*>(reinterpret_cast<size_t>(_data) + _index), len);
    _index += len;
}

void MemoryStream::ensureLength(size_t len)
{
    if (_data.size() < len)
    {
        _data.resize(len);
    }
}

const void* MemoryStream::data() const
{
    return _data.data();
}

void* MemoryStream::data()
{
    return _data.data();
}

uint64_t MemoryStream::getLength() const
{
    return _data.size();
}

uint64_t MemoryStream::getPosition() const
{
    return _index;
}

void MemoryStream::setPosition(uint64_t position)
{
    _index = static_cast<size_t>(position);
}

void MemoryStream::read(void* buffer, size_t len)
{
    auto maxReadLen = _data.size() - _index;
    if (len > maxReadLen)
        throw std::runtime_error("Failed to read data");
    std::memcpy(buffer, reinterpret_cast<const void*>(reinterpret_cast<size_t>(_data.data()) + _index), len);
    _index += len;
}

void MemoryStream::write(const void* buffer, size_t len)
{
    if (len != 0)
    {
        ensureLength(_index + len);
        std::memcpy(reinterpret_cast<void*>(reinterpret_cast<size_t>(_data.data()) + _index), buffer, len);
        _index += len;
    }
}

FileStream::FileStream(const fs::path path, uint8_t flags)
{
    if (flags & StreamFlags::write)
    {
        _fstream.open(path, std::ios::out | std::ios::binary);
        if (!_fstream.is_open())
        {
            throw std::runtime_error("Failed to open '" + path.u8string() + "' for writing");
        }
        _reading = true;
        _writing = true;
    }
    else
    {
        _fstream.open(path, std::ios::in | std::ios::binary);
        if (!_fstream.is_open())
        {
            throw std::runtime_error("Failed to open '" + path.u8string() + "' for reading");
        }
        _reading = true;
    }
}

uint64_t FileStream::getLength() const
{
    auto* fs = const_cast<std::fstream*>(&_fstream);
    if (_writing)
    {
        auto backup = fs->tellp();
        fs->seekp(0, std::ios_base::end);
        auto len = fs->tellp();
        fs->seekp(backup);
        return len;
    }
    else
    {
        auto backup = fs->tellg();
        fs->seekg(0, std::ios_base::end);
        auto len = fs->tellg();
        fs->seekg(backup);
        return len;
    }
}

uint64_t FileStream::getPosition() const
{
    auto* fs = const_cast<std::fstream*>(&_fstream);
    if (_writing)
    {
        return static_cast<uint64_t>(fs->tellp());
    }
    else
    {
        return static_cast<uint64_t>(fs->tellg());
    }
}

void FileStream::setPosition(uint64_t position)
{
    if (_reading)
        _fstream.seekg(position);
    if (_writing)
        _fstream.seekp(position);
}

void FileStream::read(void* buffer, size_t len)
{
    _fstream.read(static_cast<char*>(buffer), len);
    if (_fstream.fail())
        throw std::runtime_error("Failed to read data");
}

void FileStream::write(const void* buffer, size_t len)
{
    if (len != 0)
    {
        _fstream.write(static_cast<const char*>(buffer), len);
        if (_fstream.fail())
            throw std::runtime_error("Failed to write data");
    }
}

BinaryReader::BinaryReader(Stream& stream)
    : _stream(&stream)
{
}

bool BinaryReader::trySeek(int64_t len)
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

bool BinaryReader::seekSafe(int64_t len)
{
    auto actualLen = getSafeSeekAmount(len);
    if (actualLen != 0)
    {
        _stream->seek(actualLen);
        return actualLen == len;
    }
    return true;
}

int64_t BinaryReader::getSafeSeekAmount(int64_t len)
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

BinaryWriter::BinaryWriter(Stream& stream)
    : _stream(&stream)
{
}
