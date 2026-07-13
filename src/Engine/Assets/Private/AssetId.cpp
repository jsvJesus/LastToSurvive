#include "Assets/AssetId.h"

namespace engine::assets
{
    AssetId AssetId::FromNormalizedPath(
        const std::string_view normalizedPath) noexcept
    {
        if (normalizedPath.empty())
        {
            return AssetId{};
        }

        // FNV-1a 64-bit.
        constexpr ValueType offsetBasis =
            14695981039346656037ULL;

        constexpr ValueType prime =
            1099511628211ULL;

        ValueType hash = offsetBasis;

        for (const char character : normalizedPath)
        {
            hash ^=
                static_cast<ValueType>(
                    static_cast<unsigned char>(
                        character));

            hash *= prime;
        }

        // Ноль зарезервирован под invalid AssetId.
        if (hash == 0U)
        {
            hash = 1U;
        }

        return AssetId::FromValue(hash);
    }
}