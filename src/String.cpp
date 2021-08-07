#include "String.h"
#include <cctype>
#include <cstring>
#include <limits>

#ifdef _WIN32
#include <windows.h>
#undef max
#else
#include <codecvt>
#include <errno.h>
#include <iconv.h>
#include <iostream>
#include <locale>
#endif

namespace cs
{
    bool equals(std::string_view a, std::string_view b, bool ignoreCase)
    {
        return ignoreCase ? iequals(a, b) : a == b;
    }

    bool iequals(std::string_view a, std::string_view b)
    {
        if (a.size() != b.size())
        {
            return false;
        }
        for (size_t i = 0; i < a.size(); i++)
        {
            if (std::tolower(a[i]) != std::tolower(b[i]))
            {
                return false;
            }
        }
        return true;
    }

    bool startsWith(std::string_view s, std::string_view value, bool ignoreCase)
    {
        if (s.size() >= value.size())
        {
            auto substr = s.substr(0, value.size());
            return equals(substr, value, ignoreCase);
        }
        return false;
    }

    bool endsWith(std::string_view s, std::string_view value, bool ignoreCase)
    {
        if (s.size() >= value.size())
        {
            auto substr = s.substr(s.size() - value.size());
            return equals(substr, value, ignoreCase);
        }
        return false;
    }

    size_t strcpy(char* dest, const char* src, size_t size)
    {
        size_t src_len = std::strlen(src);

        if (src_len < size)
        {
            std::memcpy(dest, src, src_len + 1);
        }
        else
        {
            std::memcpy(dest, src, size);
            dest[size - 1] = '\0';
        }

        return src_len;
    }

    size_t strcat(char* dest, const char* src, size_t size)
    {
        size_t src_len = std::strlen(src);

        if (size == 0)
        {
            return src_len;
        }

        // this lambda is essentially a reimplementation of strnlen, which isn't standard
        size_t dest_len = [=] {
            auto dest_end = reinterpret_cast<const char*>(std::memchr(dest, '\0', size));
            if (dest_end != nullptr)
            {
                return static_cast<size_t>(dest_end - dest);
            }
            else
            {
                return size;
            }
        }();

        if (dest_len < size)
        {
            size_t copy_count = std::min<size_t>((size - dest_len) - 1, src_len);
            char* copy_ptr = (dest + dest_len);

            std::memcpy(copy_ptr, src, copy_count);
            copy_ptr[copy_count] = '\0';
        }

        return (dest_len + src_len);
    }

    static std::string_view skipDigits(std::string_view s)
    {
        while (!s.empty() && (s[0] == '.' || s[0] == ',' || isdigit(s[0])))
        {
            s = s.substr(1);
        }
        return s;
    }

    static std::pair<int32_t, std::string_view> parseNextNumber(std::string_view s)
    {
        int32_t num = 0;
        while (!s.empty())
        {
            if (s[0] != '.' && s[0] != ',')
            {
                if (!isdigit(s[0]))
                    break;

                auto newResult = num * 10;
                if (newResult / 10 != num)
                {
                    return { std::numeric_limits<int32_t>::max(), skipDigits(s) };
                }
                num = newResult + (s[0] - '0');
                if (num < newResult)
                {
                    return { std::numeric_limits<int32_t>::max(), skipDigits(s) };
                }
            }
            s = s.substr(1);
        }
        return { num, s };
    }

    int32_t logicalcmp(std::string_view s1, std::string_view s2)
    {
        for (;;)
        {
            if (s2.empty())
                return !s1.empty();
            else if (s1.empty())
                return -1;
            else if (!(isdigit(s1[0]) && isdigit(s2[0])))
            {
                if (toupper(s1[0]) != toupper(s2[0]))
                    return toupper(s1[0]) - toupper(s2[0]);
                else
                {
                    s1 = s1.substr(1);
                    s2 = s2.substr(1);
                }
            }
            else
            {
                // TODO: Replace with std::from_chars when all compilers support std::from_chars
                auto [n1, s1n] = parseNextNumber(s1);
                auto [n2, s2n] = parseNextNumber(s2);
                if (n1 > n2)
                    return 1;
                else if (n1 < n2)
                    return -1;
                s1 = s1n;
                s2 = s2n;
            }
        }
    }

    std::string toUtf8(std::wstring_view src)
    {
#ifdef _WIN32
        auto srcLen = static_cast<int>(src.size());
        int sizeReq = WideCharToMultiByte(CP_UTF8, 0, src.data(), srcLen, nullptr, 0, nullptr, nullptr);
        auto result = std::string(sizeReq, 0);
        WideCharToMultiByte(CP_UTF8, 0, src.data(), srcLen, result.data(), sizeReq, nullptr, nullptr);
        return result;
#else
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        return converter.to_bytes(src.data());
#endif
    }

    std::wstring toUtf16(std::string_view src)
    {
#ifdef _WIN32
        auto srcLen = static_cast<int>(src.size());
        int sizeReq = MultiByteToWideChar(CP_UTF8, 0, src.data(), srcLen, nullptr, 0);
        auto result = std::wstring(sizeReq, 0);
        MultiByteToWideChar(CP_UTF8, 0, src.data(), srcLen, result.data(), sizeReq);
        return result;
#else
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
        return converter.from_bytes(src.data());
#endif
    }
}
