#pragma once

#include "FileSystem.hpp"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <vector>

namespace cs
{
    namespace StreamFlags
    {
        constexpr uint8_t read = 1;
        constexpr uint8_t write = 2;
    }

    class Stream
    {
    public:
        virtual ~Stream() {}
        virtual uint64_t getLength() const { throwInvalidOperation(); }
        virtual uint64_t getPosition() const { throwInvalidOperation(); }
        virtual void setPosition(uint64_t position) { throwInvalidOperation(); }
        virtual void read(void* buffer, size_t len) { throwInvalidOperation(); }
        virtual void write(const void* buffer, size_t len) { throwInvalidOperation(); }

        void seek(int64_t pos);

    private:
        [[noreturn]] static void throwInvalidOperation();
    };

    class BinaryStream final : public Stream
    {
    private:
        const void* _data{};
        size_t _index{};
        size_t _len{};

    public:
        BinaryStream(const void* data, size_t len);
        uint64_t getLength() const override;
        uint64_t getPosition() const override;
        void setPosition(uint64_t position) override;
        void read(void* buffer, size_t len) override;
    };

    class MemoryStream final : public Stream
    {
    private:
        std::vector<std::byte> _data{};
        size_t _index{};

        void ensureLength(size_t len);

    public:
        const void* data() const;
        void* data();
        uint64_t getLength() const override;
        uint64_t getPosition() const override;
        void setPosition(uint64_t position) override;
        void read(void* buffer, size_t len) override;
        void write(const void* buffer, size_t len) override;
    };

    class FileStream final : public Stream
    {
    private:
        std::fstream _fstream;
        bool _reading{};
        bool _writing{};

    public:
        FileStream(const fs::path path, uint8_t flags);
        uint64_t getLength() const override;
        uint64_t getPosition() const override;
        void setPosition(uint64_t position) override;
        void read(void* buffer, size_t len) override;
        void write(const void* buffer, size_t len) override;
    };
}
