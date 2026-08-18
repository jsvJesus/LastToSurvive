#pragma once

#include "Assets/AssetResult.h"
#include "Graphics/Shader.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace engine::assets
{
    struct ShaderAssetDesc final
    {
        engine::graphics::ShaderStage stage = engine::graphics::ShaderStage::Unknown;
        std::vector<std::byte> bytecode;
        std::string targetProfile;
        std::string entryPoint;
        std::uint64_t sourceHash = 0U;
        std::string debugName;
    };

    class ShaderAsset final
    {
    public:
        static constexpr std::size_t MaximumBytecodeSize = 16U * 1024U * 1024U;
        static constexpr std::size_t MaximumMetadataLength = 1024U;
        [[nodiscard]] AssetResult Initialize(ShaderAssetDesc desc) noexcept;
        void Clear() noexcept;
        [[nodiscard]] bool IsValid() const noexcept;
        [[nodiscard]] engine::graphics::ShaderStage GetStage() const noexcept { return desc_.stage; }
        [[nodiscard]] const std::byte* GetBytecode() const noexcept { return desc_.bytecode.data(); }
        [[nodiscard]] std::size_t GetBytecodeSize() const noexcept { return desc_.bytecode.size(); }
        [[nodiscard]] const std::string& GetTargetProfile() const noexcept { return desc_.targetProfile; }
        [[nodiscard]] const std::string& GetEntryPoint() const noexcept { return desc_.entryPoint; }
        [[nodiscard]] std::uint64_t GetSourceHash() const noexcept { return desc_.sourceHash; }
        [[nodiscard]] const std::string& GetDebugName() const noexcept { return desc_.debugName; }
    private:
        ShaderAssetDesc desc_;
        bool initialized_ = false;
    };
}
