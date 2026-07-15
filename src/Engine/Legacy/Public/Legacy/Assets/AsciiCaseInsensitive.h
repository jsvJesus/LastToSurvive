#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace engine::legacy::assets
{
    [[nodiscard]] constexpr char LowerAscii(const char value) noexcept
    {
        return value >= 'A' && value <= 'Z' ? static_cast<char>(value - 'A' + 'a') : value;
    }

    struct AsciiCaseInsensitiveEqual final
    {
        [[nodiscard]] bool operator()(const std::string_view left, const std::string_view right) const noexcept
        {
            if (left.size() != right.size()) return false;
            for (std::size_t index = 0U; index < left.size(); ++index)
                if (LowerAscii(left[index]) != LowerAscii(right[index])) return false;
            return true;
        }
        [[nodiscard]] bool operator()(const std::string& left, const std::string& right) const noexcept
        { return (*this)(std::string_view(left), std::string_view(right)); }
    };

    struct AsciiCaseInsensitiveHash final
    {
        [[nodiscard]] std::size_t operator()(const std::string_view value) const noexcept
        {
            std::size_t hash = sizeof(std::size_t) == 8U ? static_cast<std::size_t>(1469598103934665603ULL) : 2166136261U;
            const std::size_t prime = sizeof(std::size_t) == 8U ? static_cast<std::size_t>(1099511628211ULL) : 16777619U;
            for (const char character : value) { hash ^= static_cast<unsigned char>(LowerAscii(character)); hash *= prime; }
            return hash;
        }
        [[nodiscard]] std::size_t operator()(const std::string& value) const noexcept { return (*this)(std::string_view(value)); }
    };
}
