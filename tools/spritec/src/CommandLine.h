#include <optional>
#include <string_view>
#include <tuple>
#include <vector>

namespace cs
{
    class CommandLineParser
    {
    private:
        std::vector<std::string_view> _args;
        std::vector<std::pair<std::string_view, size_t>> _registered;
        std::vector<std::pair<std::string_view, std::vector<std::string_view>>> _values;
        std::string _errorMessage;

        static std::vector<std::string> splitArgs(const char* args);
        static bool isLongOption(std::string_view arg);
        static bool isShortOption(std::string_view arg);
        static bool isOptionTerminator(std::string_view arg);
        static bool isArg(std::string_view arg);

        std::optional<size_t> getOptionArgCount(std::string_view arg);
        void setOption(std::string_view arg);
        void appendValue(std::string_view arg, std::string_view value);
        bool appendValues(std::string_view arg, size_t count, size_t& i);
        void setArgumentUnknownError(std::string_view arg);
        void setArgumentCountError(std::string_view arg, size_t count);

    public:
        CommandLineParser(int argc, const char** argv);

        CommandLineParser& registerOption(std::string_view option, size_t count = 0);
        CommandLineParser& registerOption(std::string_view a, std::string_view b, size_t count = 0);
        bool parse();

        std::string_view getErrorMessage() const;
        bool hasOption(std::string_view name) const;
        const std::vector<std::string_view>* getArgs(std::string_view name) const;
        std::string_view getArg(std::string_view name, size_t index = 0) const;
        std::string_view getArg(size_t index) const;

        template<typename int32_t>
        std::optional<int32_t> getArg(std::string_view name, size_t index = 0) const;

        template<typename T>
        std::optional<T> getArg(size_t index) const
        {
            return getArg<T>("", index);
        }
    };
}
