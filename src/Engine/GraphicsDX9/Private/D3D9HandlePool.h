#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace engine::graphics::d3d9::detail
{
    template<typename Handle, typename Entry>
    class HandlePool final
    {
    public:
        HandlePool()
        {
            slots_.emplace_back();
        }

        HandlePool(const HandlePool&) = delete;
        HandlePool& operator=(const HandlePool&) = delete;

        [[nodiscard]] Handle Insert(Entry&& entry)
        {
            std::uint32_t index = 0;

            if (!freeIndices_.empty())
            {
                index = freeIndices_.back();
                freeIndices_.pop_back();
            }
            else
            {
                index = static_cast<std::uint32_t>(slots_.size());
                slots_.emplace_back();
            }

            Slot& slot = slots_[index];
            slot.entry = std::move(entry);
            slot.alive = true;
            ++aliveCount_;

            return Handle::FromParts(index, slot.generation);
        }

        [[nodiscard]] Entry* Get(const Handle handle) noexcept
        {
            if (!handle.IsValid() || handle.Index() >= slots_.size())
            {
                return nullptr;
            }

            Slot& slot = slots_[handle.Index()];
            if (!slot.alive || slot.generation != handle.Generation())
            {
                return nullptr;
            }

            return &slot.entry;
        }

        [[nodiscard]] const Entry* Get(const Handle handle) const noexcept
        {
            if (!handle.IsValid() || handle.Index() >= slots_.size())
            {
                return nullptr;
            }

            const Slot& slot = slots_[handle.Index()];
            if (!slot.alive || slot.generation != handle.Generation())
            {
                return nullptr;
            }

            return &slot.entry;
        }

        [[nodiscard]] bool Remove(const Handle handle) noexcept
        {
            if (!handle.IsValid() || handle.Index() >= slots_.size())
            {
                return false;
            }

            Slot& slot = slots_[handle.Index()];
            if (!slot.alive || slot.generation != handle.Generation())
            {
                return false;
            }

            slot.entry = Entry{};
            slot.alive = false;
            ++slot.generation;
            if (slot.generation == 0)
            {
                slot.generation = 1;
            }

            freeIndices_.push_back(handle.Index());
            --aliveCount_;
            return true;
        }

        template<typename Function>
        void ForEach(Function&& function)
        {
            for (std::size_t index = 1; index < slots_.size(); ++index)
            {
                Slot& slot = slots_[index];
                if (slot.alive)
                {
                    function(slot.entry);
                }
            }
        }

        void Clear() noexcept
        {
            slots_.clear();
            slots_.emplace_back();
            freeIndices_.clear();
            aliveCount_ = 0;
        }

        [[nodiscard]] std::size_t Size() const noexcept
        {
            return aliveCount_;
        }

    private:
        struct Slot final
        {
            Entry entry;
            std::uint32_t generation = 1;
            bool alive = false;
        };

        std::vector<Slot> slots_;
        std::vector<std::uint32_t> freeIndices_;
        std::size_t aliveCount_ = 0;
    };
}
