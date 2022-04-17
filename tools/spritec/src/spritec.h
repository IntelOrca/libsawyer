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
        build,
        details,
        list,
        exportSingle,
        exportAll,
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
        bool quiet{};
    };
}
