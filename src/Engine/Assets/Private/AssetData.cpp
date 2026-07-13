#include "Assets/AssetData.h"

#include <new>

namespace engine::assets
{
    AssetResult AssetData::Resize(
        const std::size_t size) noexcept
    {
        try
        {
            bytes_.resize(size);
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

    void AssetData::Clear() noexcept
    {
        bytes_.clear();
    }

    bool AssetData::IsEmpty() const noexcept
    {
        return bytes_.empty();
    }

    std::size_t AssetData::GetSize() const noexcept
    {
        return bytes_.size();
    }

    std::byte* AssetData::GetData() noexcept
    {
        return bytes_.data();
    }

    const std::byte* AssetData::GetData() const noexcept
    {
        return bytes_.data();
    }
}