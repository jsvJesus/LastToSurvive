#include "Assets/AssetType.h"

namespace engine::assets
{
    const char* ToString(
        const AssetType type) noexcept
    {
        switch (type)
        {
        case AssetType::Unknown:
            return "Unknown";

        case AssetType::Texture:
            return "Texture";

        case AssetType::Mesh:
            return "Mesh";

        case AssetType::Material:
            return "Material";

        case AssetType::Shader:
            return "Shader";

        case AssetType::Animation:
            return "Animation";

        case AssetType::Effect:
            return "Effect";

        case AssetType::Audio:
            return "Audio";

        case AssetType::Data:
            return "Data";

        case AssetType::StaticModel:
            return "StaticModel";
            
        case AssetType::SkeletalMesh:
            return "SkeletalMesh";

        case AssetType::Skeleton:
            return "Skeleton";

        default:
            return "Unknown";
        }
    }
}
