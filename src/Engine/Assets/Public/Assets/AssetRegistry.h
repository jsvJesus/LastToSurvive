#include "Assets/AssetHandle.h"
#include "Assets/AssetId.h"
#include "Assets/AssetMetadata.h"
#include "Assets/AssetPath.h"
#include "Assets/AssetResult.h"
#include "Assets/AssetState.h"

#include <cstddef>
#include <memory>

namespace engine::assets
{
    // Первый registry работает на owning/main thread.
    // Thread-safe access будет добавлен вместе с async loading.
    class AssetRegistry final
    {
    public:
        AssetRegistry() noexcept;
        ~AssetRegistry() noexcept;

        AssetRegistry(const AssetRegistry&) = delete;
        AssetRegistry& operator=(const AssetRegistry&) = delete;

        AssetRegistry(AssetRegistry&&) = delete;
        AssetRegistry& operator=(AssetRegistry&&) = delete;

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

        [[nodiscard]] AssetResult GetMetadata(
            AssetHandle handle,
            AssetMetadata& outMetadata) const noexcept;

        [[nodiscard]] AssetResult GetState(
            AssetHandle handle,
            AssetState& outState) const noexcept;

        [[nodiscard]] AssetResult SetState(
            AssetHandle handle,
            AssetState state) noexcept;

        [[nodiscard]] bool Contains(
            AssetHandle handle) const noexcept;

        [[nodiscard]] std::size_t GetCount() const noexcept;

        void Clear() noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}