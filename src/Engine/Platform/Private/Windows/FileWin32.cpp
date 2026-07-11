#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include "Platform/File.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace engine::platform
{
    namespace
    {
        [[nodiscard]] HANDLE ToNativeFile(
            const std::uintptr_t nativeHandle) noexcept
        {
            return reinterpret_cast<HANDLE>(
                nativeHandle);
        }

        [[nodiscard]] std::uintptr_t FromNativeFile(
            const HANDLE handle) noexcept
        {
            return reinterpret_cast<std::uintptr_t>(
                handle);
        }

        [[nodiscard]] DWORD TranslateAccess(
            const FileAccess access) noexcept
        {
            switch (access)
            {
                case FileAccess::Read:
                    return GENERIC_READ;

                case FileAccess::Write:
                    return GENERIC_WRITE;

                case FileAccess::ReadWrite:
                    return GENERIC_READ | GENERIC_WRITE;

                default:
                    return 0;
            }
        }

        [[nodiscard]] DWORD TranslateCreation(
            const FileCreation creation) noexcept
        {
            switch (creation)
            {
                case FileCreation::OpenExisting:
                    return OPEN_EXISTING;

                case FileCreation::CreateAlways:
                    return CREATE_ALWAYS;

                case FileCreation::OpenAlways:
                    return OPEN_ALWAYS;

                case FileCreation::CreateNew:
                    return CREATE_NEW;

                case FileCreation::TruncateExisting:
                    return TRUNCATE_EXISTING;

                default:
                    return 0;
            }
        }

        [[nodiscard]] DWORD TranslateSeekOrigin(
            const FileSeekOrigin origin) noexcept
        {
            switch (origin)
            {
                case FileSeekOrigin::Begin:
                    return FILE_BEGIN;

                case FileSeekOrigin::Current:
                    return FILE_CURRENT;

                case FileSeekOrigin::End:
                    return FILE_END;

                default:
                    return FILE_BEGIN;
            }
        }
    }

    File::File(
        const Path& path,
        const FileAccess access,
        const FileCreation creation)
    {
        Open(
            path,
            access,
            creation);
    }

    File::~File() noexcept
    {
        Close();
    }

    File::File(File&& other) noexcept
        : nativeHandle_(
              std::exchange(
                  other.nativeHandle_,
                  InvalidNativeHandle)),
          path_(
              std::move(other.path_)),
          lastErrorCode_(
              std::exchange(
                  other.lastErrorCode_,
                  ERROR_SUCCESS))
    {
    }

    File& File::operator=(File&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Close();

        nativeHandle_ =
            std::exchange(
                other.nativeHandle_,
                InvalidNativeHandle);

        path_ =
            std::move(other.path_);

        lastErrorCode_ =
            std::exchange(
                other.lastErrorCode_,
                ERROR_SUCCESS);

        return *this;
    }

    bool File::Open(
        const Path& path,
        const FileAccess access,
        const FileCreation creation)
    {
        Close();

        if (path.empty())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_PARAMETER);

            return false;
        }

        const DWORD desiredAccess =
            TranslateAccess(access);

        const DWORD creationDisposition =
            TranslateCreation(creation);

        if (desiredAccess == 0 ||
            creationDisposition == 0)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_PARAMETER);

            return false;
        }

        constexpr DWORD shareMode =
            FILE_SHARE_READ |
            FILE_SHARE_WRITE |
            FILE_SHARE_DELETE;

        const HANDLE handle =
            ::CreateFileW(
                path.c_str(),
                desiredAccess,
                shareMode,
                nullptr,
                creationDisposition,
                FILE_ATTRIBUTE_NORMAL,
                nullptr);

        if (handle == INVALID_HANDLE_VALUE)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return false;
        }

        nativeHandle_ =
            FromNativeFile(handle);

        path_ = path;
        lastErrorCode_ = ERROR_SUCCESS;

        return true;
    }

    void File::Close() noexcept
    {
        if (!IsOpen())
        {
            path_.clear();
            lastErrorCode_ = ERROR_SUCCESS;
            return;
        }

        const HANDLE handle =
            ToNativeFile(nativeHandle_);

        nativeHandle_ = InvalidNativeHandle;
        path_.clear();

        if (::CloseHandle(handle) == FALSE)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return;
        }

        lastErrorCode_ = ERROR_SUCCESS;
    }

    bool File::IsOpen() const noexcept
    {
        return nativeHandle_ != InvalidNativeHandle;
    }

    File::operator bool() const noexcept
    {
        return IsOpen();
    }

    FileIoResult File::Read(
        void* destination,
        const std::size_t byteCount) noexcept
    {
        if (!IsOpen())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_HANDLE);

            return {};
        }

        if (byteCount == 0)
        {
            lastErrorCode_ = ERROR_SUCCESS;

            return
            {
                0,
                true
            };
        }

        if (destination == nullptr)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_PARAMETER);

            return {};
        }

        auto* output =
            static_cast<unsigned char*>(
                destination);

        std::size_t totalBytesRead = 0;

        constexpr std::size_t maximumChunkSize =
            static_cast<std::size_t>(
                std::numeric_limits<DWORD>::max());

        while (totalBytesRead < byteCount)
        {
            const std::size_t remainingBytes =
                byteCount - totalBytesRead;

            const DWORD chunkSize =
                static_cast<DWORD>(
                    std::min(
                        remainingBytes,
                        maximumChunkSize));

            DWORD bytesRead = 0;

            const BOOL result =
                ::ReadFile(
                    ToNativeFile(nativeHandle_),
                    output + totalBytesRead,
                    chunkSize,
                    &bytesRead,
                    nullptr);

            if (result == FALSE)
            {
                lastErrorCode_ =
                    static_cast<std::uint32_t>(
                        ::GetLastError());

                return
                {
                    totalBytesRead,
                    false
                };
            }

            totalBytesRead +=
                static_cast<std::size_t>(
                    bytesRead);

            if (bytesRead == 0)
            {
                break;
            }
        }

        lastErrorCode_ = ERROR_SUCCESS;

        return
        {
            totalBytesRead,
            true
        };
    }

    FileIoResult File::Write(
        const void* source,
        const std::size_t byteCount) noexcept
    {
        if (!IsOpen())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_HANDLE);

            return {};
        }

        if (byteCount == 0)
        {
            lastErrorCode_ = ERROR_SUCCESS;

            return
            {
                0,
                true
            };
        }

        if (source == nullptr)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_PARAMETER);

            return {};
        }

        const auto* input =
            static_cast<const unsigned char*>(
                source);

        std::size_t totalBytesWritten = 0;

        constexpr std::size_t maximumChunkSize =
            static_cast<std::size_t>(
                std::numeric_limits<DWORD>::max());

        while (totalBytesWritten < byteCount)
        {
            const std::size_t remainingBytes =
                byteCount - totalBytesWritten;

            const DWORD chunkSize =
                static_cast<DWORD>(
                    std::min(
                        remainingBytes,
                        maximumChunkSize));

            DWORD bytesWritten = 0;

            const BOOL result =
                ::WriteFile(
                    ToNativeFile(nativeHandle_),
                    input + totalBytesWritten,
                    chunkSize,
                    &bytesWritten,
                    nullptr);

            if (result == FALSE)
            {
                lastErrorCode_ =
                    static_cast<std::uint32_t>(
                        ::GetLastError());

                return
                {
                    totalBytesWritten,
                    false
                };
            }

            if (bytesWritten == 0)
            {
                lastErrorCode_ =
                    static_cast<std::uint32_t>(
                        ERROR_WRITE_FAULT);

                return
                {
                    totalBytesWritten,
                    false
                };
            }

            totalBytesWritten +=
                static_cast<std::size_t>(
                    bytesWritten);
        }

        lastErrorCode_ = ERROR_SUCCESS;

        return
        {
            totalBytesWritten,
            true
        };
    }

    bool File::Seek(
        const std::int64_t offset,
        const FileSeekOrigin origin) noexcept
    {
        if (!IsOpen())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_HANDLE);

            return false;
        }

        LARGE_INTEGER distance{};
        distance.QuadPart = offset;

        const BOOL result =
            ::SetFilePointerEx(
                ToNativeFile(nativeHandle_),
                distance,
                nullptr,
                TranslateSeekOrigin(origin));

        if (result == FALSE)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return false;
        }

        lastErrorCode_ = ERROR_SUCCESS;
        return true;
    }

    std::optional<std::uint64_t>
        File::GetPosition() const noexcept
    {
        if (!IsOpen())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_HANDLE);

            return std::nullopt;
        }

        LARGE_INTEGER distance{};
        LARGE_INTEGER position{};

        const BOOL result =
            ::SetFilePointerEx(
                ToNativeFile(nativeHandle_),
                distance,
                &position,
                FILE_CURRENT);

        if (result == FALSE ||
            position.QuadPart < 0)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return std::nullopt;
        }

        lastErrorCode_ = ERROR_SUCCESS;

        return static_cast<std::uint64_t>(
            position.QuadPart);
    }

    std::optional<std::uint64_t>
        File::GetSize() const noexcept
    {
        if (!IsOpen())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_HANDLE);

            return std::nullopt;
        }

        LARGE_INTEGER size{};

        if (::GetFileSizeEx(
                ToNativeFile(nativeHandle_),
                &size) == FALSE ||
            size.QuadPart < 0)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return std::nullopt;
        }

        lastErrorCode_ = ERROR_SUCCESS;

        return static_cast<std::uint64_t>(
            size.QuadPart);
    }

    bool File::Flush() noexcept
    {
        if (!IsOpen())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_HANDLE);

            return false;
        }

        if (::FlushFileBuffers(
                ToNativeFile(nativeHandle_)) == FALSE)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return false;
        }

        lastErrorCode_ = ERROR_SUCCESS;
        return true;
    }

    const Path& File::GetPath() const noexcept
    {
        return path_;
    }

    std::uint32_t File::GetLastErrorCode() const noexcept
    {
        return lastErrorCode_;
    }
}