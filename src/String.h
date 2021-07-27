#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace cs
{
    bool equals(std::string_view a, std::string_view b, bool ignoreCase = false);
    bool iequals(std::string_view a, std::string_view b);
    bool startsWith(std::string_view s, std::string_view value, bool ignoreCase = false);
    bool endsWith(std::string_view s, std::string_view value, bool ignoreCase = false);

    /**
     * Case-insensitive logical string comparison function, useful for ordering
     * strings containing numbers in a meaningful way.
     * @returns -1, 0, or 1 depending on the order / equality of s1 and s2.
     */
    int32_t logicalcmp(std::string_view s1, std::string_view s2);

    /**
     * Decent version of strncpy which ensures that dest is null-terminated.
     * @returns the new length of dest in bytes.
     */
    size_t strcpy(char* dest, const char* src, size_t size);

    template<size_t N>
    size_t strcpy(char (&dest)[N], const char* src)
    {
        return strlcpy(dest, src, N);
    }

    /**
     * Decent version of strncat which ensures that dest is null-terminated.
     * @returns the new length of dest in bytes.
     */
    size_t strcat(char* dest, const char* src, size_t size);

    template<size_t N>
    size_t strcat(char (&dest)[N], const char* src)
    {
        return strcat(dest, src, N);
    }

    template<size_t N, typename... Args>
    int sprintf(char (&dest)[N], const char* fmt, Args&&... args)
    {
        return std::snprintf(dest, N, fmt, std::forward<Args>(args)...);
    }

    /**
     * Converts the given wide char string to a UTF-8 string.
     * On Windows, this can be used to convert from UTF-16 to UTF-8 which is
     * particularly useful when dealing with Win32 APIs.
     */
    std::string toUtf8(std::wstring_view src);

    /**
     * Converts the given UTF-8 string to a UTF-16 string.
     * On Windows, this can be used to convert from UTF-8 to UTF-16 which is
     * particularly useful when dealing with Win32 APIs.
     */
    std::wstring toUtf16(std::string_view src);
}
