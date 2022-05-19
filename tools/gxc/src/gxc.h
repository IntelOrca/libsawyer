#pragma once

#include <optional>
#include <sawyer/Palette.h>
#include <string>
#include <vector>

namespace gxc
{
    namespace ExitCodes
    {
        constexpr int exception = -3;
        constexpr int commandLineError = -2;
        constexpr int commandLineParseError = -1;
        constexpr int ok = 0;
        constexpr int fileError = 1;
        constexpr int invalidArgument = 1;
    }

    enum class CommandLineAction
    {
        build,
        details,
        list,
        exportSingle,
        exportAll,
        merge,
        upgrade,
        help,
        version,
    };

    struct CommandLineOptions
    {
        CommandLineAction action;
        std::string path;
        std::string outputPath;
        std::string manifestPath;
        std::optional<int32_t> idx;
        std::string mode;
        std::string csgHeaderPath;
        std::string csgDataPath;
        std::vector<std::string> inputPath;
        bool quiet{};
    };

    const cs::Palette& GetStandardPalette();
}
