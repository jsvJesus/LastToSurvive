#pragma once

#include "Platform/Path.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace engine::platform
{
    enum class FileAccess : std::uint8_t
    {
        Read,
        Write,
        ReadWrite
    };

    enum class FileCreation : std::uint8_t
    {
        OpenExisting,
        CreateAlways,
        OpenAlways,
        CreateNew,
        TruncateExisting
    };

    enum class FileSeekOrigin : std::uint8_t
    {
        Begin,
        Current,
        End
    };

    struct FileIoResult final
    {
        std::size_t bytesTransferred = 0;
        bool success = false;

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return success;
        }
    };

    class File final
    {
    public:
        File() noexcept = default;

        explicit File(
            const Path& path,
            FileAccess access = FileAccess::Read,
            FileCreation creation = FileCreation::OpenExisting);

        ~File() noexcept;

        File(const File&) = delete;
        File& operator=(const File&) = delete;

        File(File&& other) noexcept;
        File& operator=(File&& other) noexcept;

        [[nodiscard]] bool Open(
            const Path& path,
            FileAccess access = FileAccess::Read,
            FileCreation creation = FileCreation::OpenExisting);

        void Close() noexcept;

        [[nodiscard]] bool IsOpen() const noexcept;

        [[nodiscard]] explicit operator bool() const noexcept;

        [[nodiscard]] FileIoResult Read(
            void* destination,
            std::size_t byteCount) noexcept;

        [[nodiscard]] FileIoResult Write(
            const void* source,
            std::size_t byteCount) noexcept;

        [[nodiscard]] bool Seek(
            std::int64_t offset,
            FileSeekOrigin origin) noexcept;

        [[nodiscard]] std::optional<std::uint64_t>
            GetPosition() const noexcept;

        [[nodiscard]] std::optional<std::uint64_t>
            GetSize() const noexcept;

        [[nodiscard]] bool Flush() noexcept;

        [[nodiscard]] const Path& GetPath() const noexcept;

        [[nodiscard]] std::uint32_t
            GetLastErrorCode() const noexcept;

    private:
        static constexpr std::uintptr_t InvalidNativeHandle =
            ~std::uintptr_t{0};

        std::uintptr_t nativeHandle_ = InvalidNativeHandle;
        Path path_;
        mutable std::uint32_t lastErrorCode_ = 0;
    };
}