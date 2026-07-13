#include "Assets/AssetPath.h"

#include <cctype>
#include <new>
#include <string>

namespace engine::assets
{
    namespace
    {
        [[nodiscard]] bool IsSeparator(
            const char character) noexcept
        {
            return
                character == '/' ||
                character == '\\';
        }

        [[nodiscard]] bool IsControlCharacter(
            const unsigned char character) noexcept
        {
            return
                character < 32U ||
                character == 127U;
        }

        [[nodiscard]] char NormalizeAsciiCharacter(
            const char character) noexcept
        {
            const unsigned char value =
                static_cast<unsigned char>(character);

            if (value >= 'A' && value <= 'Z')
            {
                return static_cast<char>(
                    value - 'A' + 'a');
            }

            return character;
        }

        [[nodiscard]] bool IsDrivePath(
            const std::string_view source) noexcept
        {
            if (source.size() < 2U)
            {
                return false;
            }

            const unsigned char first =
                static_cast<unsigned char>(source[0]);

            return
                std::isalpha(first) != 0 &&
                source[1] == ':';
        }
    }

    AssetResult AssetPath::TryCreate(
        const std::string_view source,
        AssetPath& outPath) noexcept
    {
        outPath.Clear();

        if (
            source.empty() ||
            source.size() > MaximumLength
        )
        {
            return AssetResult::InvalidPath;
        }

        if (
            IsSeparator(source.front()) ||
            IsDrivePath(source)
        )
        {
            return AssetResult::InvalidPath;
        }

        try
        {
            std::string normalized;
            normalized.reserve(source.size());

            std::string segment;
            segment.reserve(source.size());

            const auto flushSegment =
                [&normalized, &segment]() -> AssetResult
            {
                if (segment.empty() || segment == ".")
                {
                    segment.clear();
                    return AssetResult::Success;
                }

                if (segment == "..")
                {
                    return AssetResult::InvalidPath;
                }

                if (!normalized.empty())
                {
                    normalized.push_back('/');
                }

                normalized.append(segment);
                segment.clear();

                return AssetResult::Success;
            };

            for (const char character : source)
            {
                const unsigned char value =
                    static_cast<unsigned char>(character);

                if (
                    character == '\0' ||
                    IsControlCharacter(value) ||
                    character == ':'
                )
                {
                    return AssetResult::InvalidPath;
                }

                if (IsSeparator(character))
                {
                    const AssetResult result =
                        flushSegment();

                    if (Failed(result))
                    {
                        return result;
                    }

                    continue;
                }

                segment.push_back(
                    NormalizeAsciiCharacter(character));
            }

            const AssetResult result =
                flushSegment();

            if (Failed(result) || normalized.empty())
            {
                return AssetResult::InvalidPath;
            }

            if (normalized.size() > MaximumLength)
            {
                return AssetResult::InvalidPath;
            }

            outPath.value_ = std::move(normalized);

            return AssetResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            return AssetResult::InternalError;
        }
    }

    bool AssetPath::IsValid() const noexcept
    {
        return !value_.empty();
    }

    const std::string& AssetPath::String() const noexcept
    {
        return value_;
    }

    std::string_view AssetPath::View() const noexcept
    {
        return value_;
    }

    AssetId AssetPath::GetId() const noexcept
    {
        return AssetId::FromNormalizedPath(value_);
    }

    void AssetPath::Clear() noexcept
    {
        value_.clear();
    }
}