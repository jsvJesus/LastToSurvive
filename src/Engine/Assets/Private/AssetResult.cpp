#include "Assets/AssetResult.h"

namespace engine::assets
{
    const char* ToString(
        const AssetResult result) noexcept
    {
        switch (result)
        {
        case AssetResult::Success:
            return "Success";

        case AssetResult::InvalidArgument:
            return "InvalidArgument";

        case AssetResult::InvalidPath:
            return "InvalidPath";

        case AssetResult::InvalidMetadata:
            return "InvalidMetadata";

        case AssetResult::InvalidState:
            return "InvalidState";

        case AssetResult::AlreadyExists:
            return "AlreadyExists";

        case AssetResult::NotFound:
            return "NotFound";

        case AssetResult::StaleHandle:
            return "StaleHandle";

        case AssetResult::IdCollision:
            return "IdCollision";

        case AssetResult::UnsupportedFormat:
            return "UnsupportedFormat";

        case AssetResult::UnsupportedFeature:
            return "UnsupportedFeature";

        case AssetResult::CorruptData:
            return "CorruptData";

        case AssetResult::IoError:
            return "IoError";

        case AssetResult::FileTooLarge:
            return "FileTooLarge";

        case AssetResult::TypeMismatch:
            return "TypeMismatch";

        case AssetResult::ReferenceOverflow:
            return "ReferenceOverflow";

        case AssetResult::OutOfMemory:
            return "OutOfMemory";

        case AssetResult::InternalError:
            return "InternalError";

        default:
            return "Unknown";
        }
    }
}
