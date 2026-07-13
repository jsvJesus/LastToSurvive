#include "Assets/AssetRegistry.h"

#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::assets
{
    namespace
    {
        [[nodiscard]] std::uint32_t NextGeneration(
            const std::uint32_t generation) noexcept
        {
            std::uint32_t next = generation + 1U;

            if (next == 0U)
            {
                next = 1U;
            }

            return next;
        }
    }

    class AssetRegistry::Impl final
    {
    public:
        struct Slot final
        {
            AssetMetadata metadata;
            AssetState state = AssetState::Unloaded;

            std::uint32_t generation = 1U;
            bool alive = false;
        };

        std::vector<Slot> slots;

        std::unordered_map<
            AssetId::ValueType,
            std::uint32_t> idToIndex;

        std::unordered_map<
            std::string,
            std::uint32_t> pathToIndex;

        std::size_t aliveCount = 0U;

        [[nodiscard]] Slot* Resolve(
            const AssetHandle handle) noexcept
        {
            if (!handle.IsValid())
            {
                return nullptr;
            }

            const std::uint32_t index =
                handle.Index();

            if (index >= slots.size())
            {
                return nullptr;
            }

            Slot& slot = slots[index];

            if (
                !slot.alive ||
                slot.generation != handle.Generation()
            )
            {
                return nullptr;
            }

            return &slot;
        }

        [[nodiscard]] const Slot* Resolve(
            const AssetHandle handle) const noexcept
        {
            if (!handle.IsValid())
            {
                return nullptr;
            }

            const std::uint32_t index =
                handle.Index();

            if (index >= slots.size())
            {
                return nullptr;
            }

            const Slot& slot = slots[index];

            if (
                !slot.alive ||
                slot.generation != handle.Generation()
            )
            {
                return nullptr;
            }

            return &slot;
        }

        [[nodiscard]] AssetHandle MakeHandle(
            const std::uint32_t index) const noexcept
        {
            if (index == 0U || index >= slots.size())
            {
                return AssetHandle{};
            }

            const Slot& slot = slots[index];

            if (!slot.alive)
            {
                return AssetHandle{};
            }

            return AssetHandle::FromParts(
                index,
                slot.generation);
        }
    };

    AssetRegistry::AssetRegistry() noexcept
    {
        try
        {
            impl_ = std::make_unique<Impl>();

            // Slot 0 всегда invalid.
            impl_->slots.emplace_back();
        }
        catch (...)
        {
            impl_.reset();
        }
    }

    AssetRegistry::~AssetRegistry() noexcept = default;

    AssetResult AssetRegistry::Register(
        const AssetMetadata& metadata,
        AssetHandle& outHandle) noexcept
    {
        outHandle = AssetHandle{};

        if (!impl_)
        {
            return AssetResult::OutOfMemory;
        }

        if (!metadata.IsValid())
        {
            return AssetResult::InvalidMetadata;
        }

        const auto pathIterator =
            impl_->pathToIndex.find(
                metadata.path.String());

        if (pathIterator != impl_->pathToIndex.end())
        {
            outHandle =
                impl_->MakeHandle(pathIterator->second);

            return AssetResult::AlreadyExists;
        }

        const auto idIterator =
            impl_->idToIndex.find(metadata.id.Value());

        if (idIterator != impl_->idToIndex.end())
        {
            // Одинаковый hash, но другой path.
            return AssetResult::IdCollision;
        }

        std::uint32_t slotIndex = 0U;
        bool createdNewSlot = false;

        for (
            std::uint32_t index = 1U;
            index < impl_->slots.size();
            ++index
        )
        {
            if (!impl_->slots[index].alive)
            {
                slotIndex = index;
                break;
            }
        }

        try
        {
            if (slotIndex == 0U)
            {
                impl_->slots.emplace_back();

                slotIndex =
                    static_cast<std::uint32_t>(
                        impl_->slots.size() - 1U);

                createdNewSlot = true;
            }

            Impl::Slot& slot =
                impl_->slots[slotIndex];

            slot.metadata = metadata;
            slot.state = AssetState::Unloaded;

            const auto idInsert =
                impl_->idToIndex.emplace(
                    metadata.id.Value(),
                    slotIndex);

            if (!idInsert.second)
            {
                slot.metadata = AssetMetadata{};

                if (createdNewSlot)
                {
                    impl_->slots.pop_back();
                }

                return AssetResult::IdCollision;
            }

            try
            {
                const auto pathInsert =
                    impl_->pathToIndex.emplace(
                        metadata.path.String(),
                        slotIndex);

                if (!pathInsert.second)
                {
                    impl_->idToIndex.erase(
                        metadata.id.Value());

                    slot.metadata = AssetMetadata{};

                    if (createdNewSlot)
                    {
                        impl_->slots.pop_back();
                    }

                    return AssetResult::AlreadyExists;
                }
            }
            catch (...)
            {
                impl_->idToIndex.erase(
                    metadata.id.Value());

                slot.metadata = AssetMetadata{};

                if (createdNewSlot)
                {
                    impl_->slots.pop_back();
                }

                throw;
            }

            slot.alive = true;
            ++impl_->aliveCount;

            outHandle =
                AssetHandle::FromParts(
                    slotIndex,
                    slot.generation);

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

    AssetResult AssetRegistry::Unregister(
        const AssetHandle handle) noexcept
    {
        if (!impl_)
        {
            return AssetResult::InvalidState;
        }

        Impl::Slot* slot =
            impl_->Resolve(handle);

        if (!slot)
        {
            return handle.IsValid()
                ? AssetResult::StaleHandle
                : AssetResult::InvalidArgument;
        }

        impl_->idToIndex.erase(
            slot->metadata.id.Value());

        impl_->pathToIndex.erase(
            slot->metadata.path.String());

        slot->metadata = AssetMetadata{};
        slot->state = AssetState::Unloaded;
        slot->alive = false;

        slot->generation =
            NextGeneration(slot->generation);

        if (impl_->aliveCount > 0U)
        {
            --impl_->aliveCount;
        }

        return AssetResult::Success;
    }

    AssetResult AssetRegistry::FindById(
        const AssetId id,
        AssetHandle& outHandle) const noexcept
    {
        outHandle = AssetHandle{};

        if (!impl_ || !id.IsValid())
        {
            return AssetResult::InvalidArgument;
        }

        const auto iterator =
            impl_->idToIndex.find(id.Value());

        if (iterator == impl_->idToIndex.end())
        {
            return AssetResult::NotFound;
        }

        outHandle =
            impl_->MakeHandle(iterator->second);

        return outHandle.IsValid()
            ? AssetResult::Success
            : AssetResult::InternalError;
    }

    AssetResult AssetRegistry::FindByPath(
        const AssetPath& path,
        AssetHandle& outHandle) const noexcept
    {
        outHandle = AssetHandle{};

        if (!impl_ || !path.IsValid())
        {
            return AssetResult::InvalidArgument;
        }

        const auto iterator =
            impl_->pathToIndex.find(path.String());

        if (iterator == impl_->pathToIndex.end())
        {
            return AssetResult::NotFound;
        }

        outHandle =
            impl_->MakeHandle(iterator->second);

        return outHandle.IsValid()
            ? AssetResult::Success
            : AssetResult::InternalError;
    }

    AssetResult AssetRegistry::GetMetadata(
        const AssetHandle handle,
        AssetMetadata& outMetadata) const noexcept
    {
        outMetadata = AssetMetadata{};

        if (!impl_)
        {
            return AssetResult::InvalidState;
        }

        const Impl::Slot* slot =
            impl_->Resolve(handle);

        if (!slot)
        {
            return handle.IsValid()
                ? AssetResult::StaleHandle
                : AssetResult::InvalidArgument;
        }

        try
        {
            outMetadata = slot->metadata;
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

    AssetResult AssetRegistry::GetState(
        const AssetHandle handle,
        AssetState& outState) const noexcept
    {
        outState = AssetState::Unloaded;

        if (!impl_)
        {
            return AssetResult::InvalidState;
        }

        const Impl::Slot* slot =
            impl_->Resolve(handle);

        if (!slot)
        {
            return handle.IsValid()
                ? AssetResult::StaleHandle
                : AssetResult::InvalidArgument;
        }

        outState = slot->state;
        return AssetResult::Success;
    }

    AssetResult AssetRegistry::SetState(
        const AssetHandle handle,
        const AssetState state) noexcept
    {
        if (!impl_)
        {
            return AssetResult::InvalidState;
        }

        Impl::Slot* slot =
            impl_->Resolve(handle);

        if (!slot)
        {
            return handle.IsValid()
                ? AssetResult::StaleHandle
                : AssetResult::InvalidArgument;
        }

        slot->state = state;
        return AssetResult::Success;
    }

    bool AssetRegistry::Contains(
        const AssetHandle handle) const noexcept
    {
        return
            impl_ &&
            impl_->Resolve(handle) != nullptr;
    }

    std::size_t AssetRegistry::GetCount() const noexcept
    {
        return impl_
            ? impl_->aliveCount
            : 0U;
    }

    void AssetRegistry::Clear() noexcept
    {
        if (!impl_)
        {
            return;
        }

        impl_->idToIndex.clear();
        impl_->pathToIndex.clear();

        for (
            std::size_t index = 1U;
            index < impl_->slots.size();
            ++index
        )
        {
            Impl::Slot& slot =
                impl_->slots[index];

            slot.metadata = AssetMetadata{};
            slot.state = AssetState::Unloaded;
            slot.alive = false;

            slot.generation =
                NextGeneration(slot.generation);
        }

        impl_->aliveCount = 0U;
    }
}