#include "CommandLine.h"
#include <string>

using namespace cs;

std::vector<std::string> CommandLineParser::splitArgs(const char* args)
{
    std::vector<std::string> arguments;
    arguments.push_back("");
    auto ch = args;
    auto arg = ch;
    auto inQuotes = false;
    while (*ch != '\0')
    {
        if (*ch == ' ' && !inQuotes)
        {
            if (ch != arg)
            {
                arguments.emplace_back(arg, ch - arg);
            }
            arg = ch + 1;
        }
        else if (*ch == '"' && ch == arg)
        {
            inQuotes = true;
            arg = ch + 1;
        }
        else if (*ch == '"' && (*(ch + 1) == ' ' || *(ch + 1) == '\0'))
        {
            arguments.emplace_back(arg, ch - arg);
            arg = ch + 1;
            inQuotes = false;
        }
        ch++;
    }
    if (*arg != '\0')
        arguments.emplace_back(arg);
    return arguments;
}

bool CommandLineParser::isLongOption(std::string_view arg)
{
    if (arg.size() >= 3 && arg[0] == '-' && arg[1] == '-')
        return true;
    return false;
}

bool CommandLineParser::isShortOption(std::string_view arg)
{
    if (arg.size() >= 2 && arg[0] == '-' && arg[1] != '-')
        return true;
    return false;
}

bool CommandLineParser::isOptionTerminator(std::string_view arg)
{
    return arg == "--";
}

bool CommandLineParser::isArg(std::string_view arg)
{
    return !isLongOption(arg) && !isShortOption(arg) && !isOptionTerminator(arg);
}

std::optional<size_t> CommandLineParser::getOptionArgCount(std::string_view arg)
{
    for (const auto& r : _registered)
    {
        if (std::get<0>(r) == arg)
        {
            return std::get<1>(r);
        }
    }
    return {};
}

void CommandLineParser::setOption(std::string_view arg)
{
    for (auto& r : _values)
    {
        if (std::get<0>(r) == arg)
        {
            return;
        }
    }
    _values.emplace_back(arg, std::vector<std::string_view>());
}

void CommandLineParser::appendValue(std::string_view arg, std::string_view value)
{
    for (auto& r : _values)
    {
        if (std::get<0>(r) == arg)
        {
            auto& v = std::get<1>(r);
            v.push_back(value);
            return;
        }
    }
    _values.emplace_back(arg, std::vector<std::string_view>{ value });
}

bool CommandLineParser::appendValues(std::string_view arg, size_t count, size_t& i)
{
    for (size_t j = 0; j < count; j++)
    {
        i++;
        if (i < _args.size())
        {
            auto& nextArg = _args[i];
            if (isArg(nextArg))
            {
                appendValue(arg, nextArg);
            }
            else
            {
                setArgumentCountError(arg, count);
                return false;
            }
        }
        else
        {
            setArgumentCountError(arg, count);
            return false;
        }
    }
    return true;
}

void CommandLineParser::setArgumentUnknownError(std::string_view arg)
{
    _errorMessage = std::string("Unknown option: ") + std::string(arg);
}

void CommandLineParser::setArgumentCountError(std::string_view arg, size_t count)
{
    if (count == 1)
    {
        _errorMessage = std::string("Expected ") + "1 argument for " + std::string(arg);
    }
    else
    {
        _errorMessage = std::string("Expected ") + std::to_string(count) + " arguments for " + std::string(arg);
    }
}

CommandLineParser::CommandLineParser(int argc, const char** argv)
{
    _args.resize(argc - 1);
    for (int i = 1; i < argc; i++)
    {
        _args[i - 1] = argv[i];
    }
}

CommandLineParser& CommandLineParser::registerOption(std::string_view option, size_t count)
{
    _registered.emplace_back(option, count);
    return *this;
}

CommandLineParser& CommandLineParser::registerOption(std::string_view a, std::string_view b, size_t count)
{
    _registered.emplace_back(a, count);
    _registered.emplace_back(b, count);
    return *this;
}

bool CommandLineParser::parse()
{
    auto noMoreOptions = false;
    for (size_t i = 0; i < _args.size(); i++)
    {
        auto& arg = _args[i];
        if (!noMoreOptions && isLongOption(arg))
        {
            auto count = getOptionArgCount(arg);
            if (count)
            {
                if (*count == 0)
                {
                    setOption(arg);
                }
                else if (!appendValues(arg, *count, i))
                {
                    return false;
                }
            }
            else
            {
                setArgumentUnknownError(arg);
                return false;
            }
        }
        else if (!noMoreOptions && isShortOption(arg))
        {
            for (auto c : arg.substr(1))
            {
                auto count = getOptionArgCount(arg);
                if (count)
                {
                    if (*count == 0)
                    {
                        setOption(arg);
                    }
                    else if (!appendValues(arg, *count, i))
                    {
                        return false;
                    }
                }
                else
                {
                    setArgumentUnknownError(std::string_view(&c, 1));
                    return false;
                }
            }
        }
        else if (!noMoreOptions && arg == "--")
        {
            noMoreOptions = true;
        }
        else
        {
            appendValue("", arg);
        }
    }
    return true;
}

std::string_view CommandLineParser::getErrorMessage() const
{
    return _errorMessage;
}

bool CommandLineParser::hasOption(std::string_view name) const
{
    for (auto& r : _values)
    {
        if (std::get<0>(r) == name)
        {
            return true;
        }
    }
    return false;
}

const std::vector<std::string_view>* CommandLineParser::getArgs() const
{
    return getArgs("");
}

const std::vector<std::string_view>* CommandLineParser::getArgs(std::string_view name) const
{
    for (auto& r : _values)
    {
        if (std::get<0>(r) == name)
        {
            return &std::get<1>(r);
        }
    }
    return nullptr;
}

std::string_view CommandLineParser::getArg(std::string_view name, size_t index) const
{
    const auto* args = getArgs(name);
    if (args == nullptr || index >= args->size())
        return {};
    return (*args)[index];
}

std::string_view CommandLineParser::getArg(size_t index) const
{
    return getArg("", index);
}

template<>
std::optional<int32_t> CommandLineParser::getArg(std::string_view name, size_t index) const
{
    auto arg = getArg(name, index);
    if (arg.size() != 0)
    {
        return std::stoi(std::string(arg));
    }
    return {};
}
