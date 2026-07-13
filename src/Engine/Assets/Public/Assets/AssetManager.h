#pragma once

#include "Assets/AssetData.h"
#include "Assets/AssetHandle.h"
#include "Assets/AssetId.h"
#include "Assets/AssetMetadata.h"
#include "Assets/AssetPath.h"
#include "Assets/AssetRegistry.h"
#include "Assets/AssetResult.h"
#include "Assets/AssetSource.h"
#include "Assets/AssetState.h"

#include <cstddef>
#include <memory>

namespace engine::assets
{
    // Первый AssetManager работает синхронно и должен использоваться
    // с owning/main thread.
    //
    // AssetSource передаётся как non-owning reference и должен жить
    // дольше AssetManager либо до вызова Shutdown().
    class AssetManager final
    {
    public:
        AssetManager() noexcept;
        ~AssetManager() noexcept;

        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;

        AssetManager(AssetManager&&) = delete;
        AssetManager& operator=(AssetManager&&) = delete;

        [[nodiscard]] AssetResult Initialize(
            AssetSource& source) noexcept;

        void Shutdown() noexcept;

        [[nodiscard]] bool IsInitialized() const noexcept;

        [[nodiscard]] AssetResult Register(
            const AssetMetadata& metadata,
            AssetHandle& outHandle) noexcept;

        [[nodiscard]] AssetResult Unregister(
            AssetHandle handle) noexcept;

        [[nodiscard]] AssetResult FindById(
            AssetId id,
            AssetHandle& outHandle) const noexcept;

        [[nodiscard]] AssetResult FindByPath(
            const AssetPath& path,
            AssetHandle& outHandle) const noexcept;

        [[nodiscard]] AssetResult Load(
            AssetHandle handle) noexcept;

        [[nodiscard]] AssetResult LoadById(
            AssetId id,
            AssetHandle& outHandle) noexcept;

        [[nodiscard]] AssetResult LoadByPath(
            const AssetPath& path,
            AssetHandle& outHandle) noexcept;

        [[nodiscard]] AssetResult Reload(
            AssetHandle handle) noexcept;

        [[nodiscard]] AssetResult Unload(
            AssetHandle handle) noexcept;

        [[nodiscard]] AssetResult GetData(
            AssetHandle handle,
            const AssetData*& outData) const noexcept;

        [[nodiscard]] AssetResult GetMetadata(
            AssetHandle handle,
            AssetMetadata& outMetadata) const noexcept;

        [[nodiscard]] AssetResult GetState(
            AssetHandle handle,
            AssetState& outState) const noexcept;

        [[nodiscard]] bool IsLoaded(
            AssetHandle handle) const noexcept;

        [[nodiscard]] std::size_t
            GetRegisteredCount() const noexcept;

        [[nodiscard]] std::size_t
            GetLoadedCount() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}