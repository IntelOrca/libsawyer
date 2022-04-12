#include <optional>
#include <string>

namespace spritec
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
        details,
        exportall,
        help,
        version,
    };

    struct CommandLineOptions
    {
        CommandLineAction action;
        std::string path;
        std::string outputPath;
        std::optional<int32_t> idx;
        bool quiet{};
    };
}