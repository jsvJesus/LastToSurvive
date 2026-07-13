#pragma once

#include <cstdint>

namespace engine::assets
{
    enum class AssetResult : std::uint8_t
    {
        Success = 0,

        InvalidArgument,
        InvalidPath,
        InvalidMetadata,
        InvalidState,

        AlreadyExists,
        NotFound,
        StaleHandle,
        IdCollision,

        UnsupportedFormat,
        CorruptData,

        IoError,
        FileTooLarge,

        TypeMismatch,
        ReferenceOverflow,
        
        OutOfMemory,
        InternalError
    };

    [[nodiscard]] constexpr bool Succeeded(
        const AssetResult result) noexcept
    {
        return result == AssetResult::Success;
    }

    [[nodiscard]] constexpr bool Failed(
        const AssetResult result) noexcept
    {
        return !Succeeded(result);
    }

    [[nodiscard]] const char* ToString(
        AssetResult result) noexcept;
}