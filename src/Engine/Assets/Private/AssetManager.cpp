#include "Assets/AssetManager.h"

#include <new>
#include <unordered_map>
#include <utility>

namespace engine::assets
{
    namespace
    {
        void SetStateBestEffort(
            AssetRegistry& registry,
            const AssetHandle handle,
            const AssetState state) noexcept
        {
            const AssetResult result =
                registry.SetState(handle, state);

            (void)result;
        }

        [[nodiscard]] bool IsBusyState(
            const AssetState state) noexcept
        {
            switch (state)
            {
            case AssetState::Queued:
            case AssetState::Loading:
            case AssetState::Reloading:
            case AssetState::Unloading:
                return true;

            default:
                return false;
            }
        }
    }

    class AssetManager::Impl final
    {
    public:
        using CacheKey = AssetHandle::ValueType;

        AssetSource* source = nullptr;

        AssetRegistry registry;

        std::unordered_map<
            CacheKey,
            AssetData> loadedAssets;

        bool initialized = false;
    };

    AssetManager::AssetManager() noexcept
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

    AssetManager::~AssetManager() noexcept
    {
        Shutdown();
    }

    AssetResult AssetManager::Initialize(
        AssetSource& source) noexcept
    {
        if (!impl_)
        {
            return AssetResult::OutOfMemory;
        }

        if (impl_->initialized)
        {
            return AssetResult::InvalidState;
        }

        impl_->loadedAssets.clear();
        impl_->registry.Clear();

        impl_->source = &source;
        impl_->initialized = true;

        return AssetResult::Success;
    }

    void AssetManager::Shutdown() noexcept
    {
        if (!impl_)
        {
            return;
        }

        impl_->loadedAssets.clear();
        impl_->registry.Clear();

        impl_->source = nullptr;
        impl_->initialized = false;
    }

    bool AssetManager::IsInitialized() const noexcept
    {
        return
            impl_ != nullptr &&
            impl_->initialized &&
            impl_->source != nullptr;
    }

    AssetResult AssetManager::Register(
        const AssetMetadata& metadata,
        AssetHandle& outHandle) noexcept
    {
        outHandle = AssetHandle{};

        if (!IsInitialized())
        {
            return AssetResult::InvalidState;
        }

        return impl_->registry.Register(
            metadata,
            outHandle);
    }

    AssetResult AssetManager::Unregister(
        const AssetHandle handle) noexcept
    {
        if (!IsInitialized())
        {
            return AssetResult::InvalidState;
        }

        AssetState state = AssetState::Unloaded;

        const AssetResult stateResult =
            impl_->registry.GetState(
                handle,
                state);

        if (Failed(stateResult))
        {
            return stateResult;
        }

        if (state != AssetState::Unloaded)
        {
            const AssetResult unloadResult =
                Unload(handle);

            if (Failed(unloadResult))
            {
                return unloadResult;
            }
        }

        impl_->loadedAssets.erase(
            handle.Value());

        return impl_->registry.Unregister(
            handle);
    }

    AssetResult AssetManager::FindById(
        const AssetId id,
        AssetHandle& outHandle) const noexcept
    {
        outHandle = AssetHandle{};

        if (!IsInitialized())
        {
            return AssetResult::InvalidState;
        }

        return impl_->registry.FindById(
            id,
            outHandle);
    }

    AssetResult AssetManager::FindByPath(
        const AssetPath& path,
        AssetHandle& outHandle) const noexcept
    {
        outHandle = AssetHandle{};

        if (!IsInitialized())
        {
            return AssetResult::InvalidState;
        }

        return impl_->registry.FindByPath(
            path,
            outHandle);
    }

    AssetResult AssetManager::Load(
        const AssetHandle handle) noexcept
    {
        if (!IsInitialized())
        {
            return AssetResult::InvalidState;
        }

        AssetState currentState =
            AssetState::Unloaded;

        AssetResult result =
            impl_->registry.GetState(
                handle,
                currentState);

        if (Failed(result))
        {
            return result;
        }

        const Impl::CacheKey cacheKey =
            handle.Value();

        if (currentState == AssetState::Ready)
        {
            return
                impl_->loadedAssets.find(cacheKey) !=
                    impl_->loadedAssets.end()
                ? AssetResult::Success
                : AssetResult::InternalError;
        }

        if (IsBusyState(currentState))
        {
            return AssetResult::InvalidState;
        }

        AssetMetadata metadata;

        result = impl_->registry.GetMetadata(
            handle,
            metadata);

        if (Failed(result))
        {
            return result;
        }

        // Удаляем возможные остатки после предыдущего failed load.
        impl_->loadedAssets.erase(cacheKey);

        result = impl_->registry.SetState(
            handle,
            AssetState::Loading);

        if (Failed(result))
        {
            return result;
        }

        AssetData loadedData;

        result = impl_->source->Read(
            metadata.path,
            loadedData);

        if (Failed(result))
        {
            SetStateBestEffort(
                impl_->registry,
                handle,
                AssetState::Failed);

            return result;
        }

        try
        {
            const auto insertResult =
                impl_->loadedAssets.emplace(
                    cacheKey,
                    std::move(loadedData));

            if (!insertResult.second)
            {
                SetStateBestEffort(
                    impl_->registry,
                    handle,
                    AssetState::Failed);

                return AssetResult::InternalError;
            }
        }
        catch (const std::bad_alloc&)
        {
            SetStateBestEffort(
                impl_->registry,
                handle,
                AssetState::Failed);

            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            SetStateBestEffort(
                impl_->registry,
                handle,
                AssetState::Failed);

            return AssetResult::InternalError;
        }

        result = impl_->registry.SetState(
            handle,
            AssetState::Ready);

        if (Failed(result))
        {
            impl_->loadedAssets.erase(cacheKey);

            SetStateBestEffort(
                impl_->registry,
                handle,
                AssetState::Failed);

            return result;
        }

        return AssetResult::Success;
    }

    AssetResult AssetManager::LoadById(
        const AssetId id,
        AssetHandle& outHandle) noexcept
    {
        outHandle = AssetHandle{};

        const AssetResult findResult =
            FindById(
                id,
                outHandle);

        if (Failed(findResult))
        {
            return findResult;
        }

        return Load(outHandle);
    }

    AssetResult AssetManager::LoadByPath(
        const AssetPath& path,
        AssetHandle& outHandle) noexcept
    {
        outHandle = AssetHandle{};

        const AssetResult findResult =
            FindByPath(
                path,
                outHandle);

        if (Failed(findResult))
        {
            return findResult;
        }

        return Load(outHandle);
    }

    AssetResult AssetManager::Reload(
        const AssetHandle handle) noexcept
    {
        if (!IsInitialized())
        {
            return AssetResult::InvalidState;
        }

        AssetState currentState =
            AssetState::Unloaded;

        AssetResult result =
            impl_->registry.GetState(
                handle,
                currentState);

        if (Failed(result))
        {
            return result;
        }

        if (
            currentState == AssetState::Unloaded ||
            currentState == AssetState::Failed
        )
        {
            return Load(handle);
        }

        if (currentState != AssetState::Ready)
        {
            return AssetResult::InvalidState;
        }

        const Impl::CacheKey cacheKey =
            handle.Value();

        auto cacheIterator =
            impl_->loadedAssets.find(cacheKey);

        if (
            cacheIterator ==
            impl_->loadedAssets.end()
        )
        {
            SetStateBestEffort(
                impl_->registry,
                handle,
                AssetState::Failed);

            return AssetResult::InternalError;
        }

        AssetMetadata metadata;

        result = impl_->registry.GetMetadata(
            handle,
            metadata);

        if (Failed(result))
        {
            return result;
        }

        result = impl_->registry.SetState(
            handle,
            AssetState::Reloading);

        if (Failed(result))
        {
            return result;
        }

        AssetData replacementData;

        result = impl_->source->Read(
            metadata.path,
            replacementData);

        if (Failed(result))
        {
            // Старые данные остаются рабочими.
            SetStateBestEffort(
                impl_->registry,
                handle,
                AssetState::Ready);

            return result;
        }

        cacheIterator->second.Swap(
            replacementData);

        result = impl_->registry.SetState(
            handle,
            AssetState::Ready);

        if (Failed(result))
        {
            // Возвращаем предыдущие данные.
            cacheIterator->second.Swap(
                replacementData);

            return result;
        }

        return AssetResult::Success;
    }

    AssetResult AssetManager::Unload(
        const AssetHandle handle) noexcept
    {
        if (!IsInitialized())
        {
            return AssetResult::InvalidState;
        }

        AssetState currentState =
            AssetState::Unloaded;

        AssetResult result =
            impl_->registry.GetState(
                handle,
                currentState);

        if (Failed(result))
        {
            return result;
        }

        const Impl::CacheKey cacheKey =
            handle.Value();

        if (currentState == AssetState::Unloaded)
        {
            impl_->loadedAssets.erase(cacheKey);
            return AssetResult::Success;
        }

        if (IsBusyState(currentState))
        {
            return AssetResult::InvalidState;
        }

        result = impl_->registry.SetState(
            handle,
            AssetState::Unloading);

        if (Failed(result))
        {
            return result;
        }

        impl_->loadedAssets.erase(cacheKey);

        result = impl_->registry.SetState(
            handle,
            AssetState::Unloaded);

        if (Failed(result))
        {
            SetStateBestEffort(
                impl_->registry,
                handle,
                AssetState::Failed);

            return result;
        }

        return AssetResult::Success;
    }

    AssetResult AssetManager::GetData(
        const AssetHandle handle,
        const AssetData*& outData) const noexcept
    {
        outData = nullptr;

        if (!IsInitialized())
        {
            return AssetResult::InvalidState;
        }

        AssetState state =
            AssetState::Unloaded;

        const AssetResult stateResult =
            impl_->registry.GetState(
                handle,
                state);

        if (Failed(stateResult))
        {
            return stateResult;
        }

        if (state != AssetState::Ready)
        {
            return AssetResult::InvalidState;
        }

        const auto iterator =
            impl_->loadedAssets.find(
                handle.Value());

        if (
            iterator ==
            impl_->loadedAssets.end()
        )
        {
            return AssetResult::InternalError;
        }

        outData = &iterator->second;

        return AssetResult::Success;
    }

    AssetResult AssetManager::GetMetadata(
        const AssetHandle handle,
        AssetMetadata& outMetadata) const noexcept
    {
        outMetadata = AssetMetadata{};

        if (!IsInitialized())
        {
            return AssetResult::InvalidState;
        }

        return impl_->registry.GetMetadata(
            handle,
            outMetadata);
    }

    AssetResult AssetManager::GetState(
        const AssetHandle handle,
        AssetState& outState) const noexcept
    {
        outState = AssetState::Unloaded;

        if (!IsInitialized())
        {
            return AssetResult::InvalidState;
        }

        return impl_->registry.GetState(
            handle,
            outState);
    }

    bool AssetManager::IsLoaded(
        const AssetHandle handle) const noexcept
    {
        if (!IsInitialized())
        {
            return false;
        }

        AssetState state =
            AssetState::Unloaded;

        if (
            Failed(
                impl_->registry.GetState(
                    handle,
                    state))
        )
        {
            return false;
        }

        return
            state == AssetState::Ready &&
            impl_->loadedAssets.find(
                handle.Value()) !=
                    impl_->loadedAssets.end();
    }

    std::size_t
    AssetManager::GetRegisteredCount() const noexcept
    {
        return IsInitialized()
            ? impl_->registry.GetCount()
            : 0U;
    }

    std::size_t
    AssetManager::GetLoadedCount() const noexcept
    {
        return IsInitialized()
            ? impl_->loadedAssets.size()
            : 0U;
    }
}