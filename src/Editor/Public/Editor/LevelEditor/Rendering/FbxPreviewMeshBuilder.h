#pragma once

#include <Assets/AssetResult.h>
#include <Assets/MeshAsset.h>

#include <filesystem>
#include <string>
#include <vector>

namespace lts::editor
{
    class FbxPreviewMeshBuilder final
    {
    public:
        FbxPreviewMeshBuilder() = delete;

        [[nodiscard]]
        static engine::assets::AssetResult Build(
            const std::filesystem::path& sourcePath,
            engine::assets::MeshAsset& output,
            std::wstring& error,
            std::vector<std::wstring>* warnings =
                nullptr) noexcept;
    };
}