#include "Assets/FileAssetSource.h"

#include "Platform/File.h"

#include <filesystem>
#include <limits>
#include <new>
#include <utility>

namespace engine::assets
{
    AssetResult FileAssetSource::Initialize(
        const engine::platform::Path& rootDirectory,
        const std::uint64_t maximumAssetSize) noexcept
    {
        Shutdown();

        if (
            rootDirectory.empty() ||
            maximumAssetSize == 0U
        )
        {
            return AssetResult::InvalidArgument;
        }

        try
        {
            engine::platform::Path absoluteRoot =
                engine::platform::MakeAbsolute(
                    rootDirectory);

            if (absoluteRoot.empty())
            {
                return AssetResult::InvalidPath;
            }

            absoluteRoot =
                engine::platform::NormalizePath(
                    absoluteRoot);

            if (
                !engine::platform::PathExists(
                    absoluteRoot) ||
                !engine::platform::IsDirectory(
                    absoluteRoot)
            )
            {
                return AssetResult::NotFound;
            }

            rootDirectory_ = std::move(absoluteRoot);
            maximumAssetSize_ = maximumAssetSize;
            initialized_ = true;

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

    void FileAssetSource::Shutdown() noexcept
    {
        rootDirectory_.clear();

        maximumAssetSize_ =
            DefaultMaximumAssetSize;

        initialized_ = false;
    }

    bool FileAssetSource::IsInitialized() const noexcept
    {
        return initialized_;
    }

    AssetResult FileAssetSource::ResolvePath(
        const AssetPath& assetPath,
        engine::platform::Path& outPath) const noexcept
    {
        outPath.clear();

        if (!initialized_)
        {
            return AssetResult::InvalidState;
        }

        if (!assetPath.IsValid())
        {
            return AssetResult::InvalidPath;
        }

        try
        {
            const engine::platform::Path relativePath =
                std::filesystem::u8path(
                    assetPath.String());

            if (
                relativePath.empty() ||
                relativePath.is_absolute() ||
                relativePath.has_root_name() ||
                relativePath.has_root_directory()
            )
            {
                return AssetResult::InvalidPath;
            }

            const engine::platform::Path resolvedPath =
                engine::platform::NormalizePath(
                    rootDirectory_ / relativePath);

            if (resolvedPath.empty())
            {
                return AssetResult::InvalidPath;
            }

            const engine::platform::Path pathFromRoot =
                resolvedPath.lexically_relative(
                    rootDirectory_);

            if (
                pathFromRoot.empty() ||
                pathFromRoot.is_absolute()
            )
            {
                return AssetResult::InvalidPath;
            }

            for (const auto& component : pathFromRoot)
            {
                if (component == "..")
                {
                    return AssetResult::InvalidPath;
                }
            }

            outPath = resolvedPath;

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

    AssetResult FileAssetSource::Read(
        const AssetPath& path,
        AssetData& outData) noexcept
    {
        outData.Clear();

        engine::platform::Path resolvedPath;

        const AssetResult resolveResult =
            ResolvePath(path, resolvedPath);

        if (Failed(resolveResult))
        {
            return resolveResult;
        }

        if (
            !engine::platform::PathExists(resolvedPath) ||
            engine::platform::IsDirectory(resolvedPath)
        )
        {
            return AssetResult::NotFound;
        }

        try
        {
            engine::platform::File file;

            if (
                !file.Open(
                    resolvedPath,
                    engine::platform::FileAccess::Read,
                    engine::platform::FileCreation::OpenExisting)
            )
            {
                return AssetResult::IoError;
            }

            const auto fileSizeResult =
                file.GetSize();

            if (!fileSizeResult.has_value())
            {
                return AssetResult::IoError;
            }

            const std::uint64_t fileSize =
                fileSizeResult.value();

            if (
                fileSize > maximumAssetSize_ ||
                fileSize >
                    static_cast<std::uint64_t>(
                        std::numeric_limits<
                            std::size_t>::max())
            )
            {
                return AssetResult::FileTooLarge;
            }

            const std::size_t allocationSize =
                static_cast<std::size_t>(
                    fileSize);

            const AssetResult resizeResult =
                outData.Resize(allocationSize);

            if (Failed(resizeResult))
            {
                return resizeResult;
            }

            if (allocationSize == 0U)
            {
                return AssetResult::Success;
            }

            const engine::platform::FileIoResult readResult =
                file.Read(
                    outData.GetData(),
                    allocationSize);

            if (
                !readResult.success ||
                readResult.bytesTransferred !=
                    allocationSize
            )
            {
                outData.Clear();
                return AssetResult::IoError;
            }

            return AssetResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            outData.Clear();
            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            outData.Clear();
            return AssetResult::InternalError;
        }
    }

    bool FileAssetSource::Exists(
        const AssetPath& path) const noexcept
    {
        engine::platform::Path resolvedPath;

        if (
            Failed(
                ResolvePath(
                    path,
                    resolvedPath))
        )
        {
            return false;
        }

        return
            engine::platform::PathExists(
                resolvedPath) &&
            !engine::platform::IsDirectory(
                resolvedPath);
    }

    const engine::platform::Path&
    FileAssetSource::GetRootDirectory() const noexcept
    {
        return rootDirectory_;
    }

    std::uint64_t
    FileAssetSource::GetMaximumAssetSize() const noexcept
    {
        return maximumAssetSize_;
    }
}