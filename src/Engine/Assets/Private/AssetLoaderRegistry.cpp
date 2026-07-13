#include "Assets/AssetLoaderRegistry.h"

#include <cstdint>
#include <new>
#include <unordered_map>

namespace engine::assets
{
    namespace
    {
        using LoaderKey = std::uint8_t;

        [[nodiscard]] constexpr LoaderKey ToKey(
            const AssetType type) noexcept
        {
            return static_cast<LoaderKey>(type);
        }
    }

    class AssetLoaderRegistry::Impl final
    {
    public:
        std::unordered_map<LoaderKey, AssetLoader*> loaders;
    };

    AssetLoaderRegistry::AssetLoaderRegistry() noexcept
    {
        try
        {
            impl_ = std::make_unique<Impl>();
        }
        catch (...)
        {
            impl_.reset();
        }
    }

    AssetLoaderRegistry::~AssetLoaderRegistry() noexcept = default;

    AssetResult AssetLoaderRegistry::Register(
        AssetLoader& loader) noexcept
    {
        if (!impl_)
        {
            return AssetResult::OutOfMemory;
        }

        const AssetType type = loader.GetAssetType();

        if (type == AssetType::Unknown)
        {
            return AssetResult::InvalidArgument;
        }

        try
        {
            const auto result = impl_->loaders.emplace(
                ToKey(type),
                &loader);

            return result.second
                ? AssetResult::Success
                : AssetResult::AlreadyExists;
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

    AssetResult AssetLoaderRegistry::Unregister(
        const AssetType type,
        const AssetLoader* const expectedLoader) noexcept
    {
        if (!impl_)
        {
            return AssetResult::InvalidState;
        }

        if (type == AssetType::Unknown)
        {
            return AssetResult::InvalidArgument;
        }

        const auto iterator = impl_->loaders.find(ToKey(type));

        if (iterator == impl_->loaders.end())
        {
            return AssetResult::NotFound;
        }

        if (
            expectedLoader != nullptr &&
            iterator->second != expectedLoader)
        {
            return AssetResult::TypeMismatch;
        }

        impl_->loaders.erase(iterator);
        return AssetResult::Success;
    }

    AssetLoader* AssetLoaderRegistry::Find(
        const AssetType type) noexcept
    {
        if (!impl_ || type == AssetType::Unknown)
        {
            return nullptr;
        }

        const auto iterator = impl_->loaders.find(ToKey(type));

        return iterator != impl_->loaders.end()
            ? iterator->second
            : nullptr;
    }

    const AssetLoader* AssetLoaderRegistry::Find(
        const AssetType type) const noexcept
    {
        if (!impl_ || type == AssetType::Unknown)
        {
            return nullptr;
        }

        const auto iterator = impl_->loaders.find(ToKey(type));

        return iterator != impl_->loaders.end()
            ? iterator->second
            : nullptr;
    }

    AssetResult AssetLoaderRegistry::Load(
        const AssetMetadata& metadata,
        const AssetData& source,
        std::unique_ptr<LoadedAsset>& outAsset) const noexcept
    {
        outAsset.reset();

        if (!impl_)
        {
            return AssetResult::InvalidState;
        }

        if (!metadata.IsValid())
        {
            return AssetResult::InvalidMetadata;
        }

        const AssetLoader* const loader = Find(metadata.type);

        if (loader == nullptr)
        {
            return AssetResult::NotFound;
        }

        const AssetResult result = loader->Load(
            metadata,
            source,
            outAsset);

        if (Failed(result))
        {
            outAsset.reset();
            return result;
        }

        if (
            !outAsset ||
            outAsset->GetType() != metadata.type)
        {
            outAsset.reset();
            return AssetResult::InternalError;
        }

        return AssetResult::Success;
    }

    bool AssetLoaderRegistry::Contains(
        const AssetType type) const noexcept
    {
        return Find(type) != nullptr;
    }

    std::size_t AssetLoaderRegistry::GetCount() const noexcept
    {
        return impl_ ? impl_->loaders.size() : 0U;
    }

    void AssetLoaderRegistry::Clear() noexcept
    {
        if (impl_)
        {
            impl_->loaders.clear();
        }
    }
}
