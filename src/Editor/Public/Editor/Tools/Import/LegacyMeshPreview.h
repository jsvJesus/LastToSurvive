#pragma once

#include "Editor/Tools/Import/LegacyMaterialReader.h"

#include <array>
#include <cstddef>
#include <filesystem>

namespace lts::editor
{
    class LegacyMeshPreview final
    {
    public:
        void Reset() noexcept;

        void Frame(
            const LegacyMeshData& mesh) noexcept;

        void Draw(
            const LegacyMeshData& mesh,
            const LegacySkeletonData* skeleton,
            const LegacyMaterialSet* materials,
            float width,
            float height,
            bool showSkeleton,
            bool wireframe) noexcept;

    private:
        std::filesystem::path framedSource_;

        std::array<float, 3U> target_
        {
            0.0F,
            0.0F,
            0.0F
        };

        std::size_t framedVertexCount_ = 0U;

        float yaw_ = 0.65F;
        float pitch_ = 0.25F;
        float distance_ = 3.0F;
        float radius_ = 1.0F;

        bool framed_ = false;
    };
}