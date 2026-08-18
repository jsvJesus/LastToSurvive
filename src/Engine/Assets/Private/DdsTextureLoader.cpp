#include "Assets/DdsTextureLoader.h"

#include <new>
#include <utility>

namespace engine::assets
{
    AssetResult DdsTextureLoader::Load(
        const AssetMetadata& metadata,
        const AssetData& source,
        std::unique_ptr<LoadedAsset>& outAsset) noexcept
    {
        outAsset.reset();

        if (!metadata.IsValid())
        {
            return AssetResult::InvalidMetadata;
        }

        if (metadata.type != AssetType::Texture)
        {
            return AssetResult::TypeMismatch;
        }

        TextureAsset textureAsset;

        const AssetResult decodeResult =
            DdsTextureDecoder::Decode(
                source,
                options_,
                textureAsset);

        if (Failed(decodeResult))
        {
            return decodeResult;
        }

        if (!textureAsset.IsValid())
        {
            return AssetResult::CorruptData;
        }

        try
        {
            outAsset = std::make_unique<TextureLoadedAsset>(
                std::move(textureAsset));
        }
        catch (const std::bad_alloc&)
        {
            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            return AssetResult::InternalError;
        }

        return outAsset
            ? AssetResult::Success
            : AssetResult::InternalError;
    }
}
