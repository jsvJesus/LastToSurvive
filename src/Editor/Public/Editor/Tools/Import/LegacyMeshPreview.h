#pragma once

#include "Editor/Tools/Import/LegacyAnimationReader.h"
#include "Editor/Tools/Import/LegacyMaterialReader.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace lts::editor
{
    class LegacyMeshPreview final
    {
    public:
        LegacyMeshPreview();
        ~LegacyMeshPreview() noexcept;

        LegacyMeshPreview(
            const LegacyMeshPreview&) = delete;

        LegacyMeshPreview& operator=(
            const LegacyMeshPreview&) = delete;

        void Initialize(
            ID3D11Device* device,
            ID3D11DeviceContext* context) noexcept;

        void Shutdown() noexcept;
        void Reset() noexcept;

        void Frame(
            const LegacyMeshData& mesh) noexcept;

        void Draw(
            const LegacyMeshData& mesh,
            const LegacySkeletonData* skeleton,
            const LegacyWeightData* weights,
            const LegacyMaterialSet* materials,
            const LegacyAnimationPose* animationPose,
            float width,
            float height,
            bool showSkeleton,
            bool wireframe) noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;

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