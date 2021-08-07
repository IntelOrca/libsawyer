#include "TestData.h"
#include <fstream>
#include <sawyer/FileSystem.hpp>

static fs::path resolveTestDataPath()
{
    std::error_code err;
    if (fs::is_directory("testdata", err))
    {
        return "testdata";
    }
    if (fs::is_directory("../testdata", err))
    {
        return "../testdata";
    }
    throw std::runtime_error("Unable to find testdata directory. Ensure your CWD is correct.");
}

static fs::path resolveTestFile(std::string_view fileName)
{
    auto testDataPath = resolveTestDataPath();
    return testDataPath / fileName;
}

std::string readTestData(std::string_view fileName)
{
    auto inputPath = resolveTestFile(fileName);
    try
    {
        std::ifstream fs;
        fs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fs.open(inputPath, std::ios_base::binary);
        fs.seekg(0, std::ios::end);
        auto length = static_cast<size_t>(fs.tellg());
        fs.seekg(0, std::ios::beg);

        std::string result;
        result.resize(length);
        fs.read(result.data(), length);
        return result;
    }
    catch (...)
    {
        auto absolutePath = fs::absolute(inputPath);
        auto msg = std::string("Unable to open ") + absolutePath.u8string();
        throw std::runtime_error(msg.c_str());
    }
}
