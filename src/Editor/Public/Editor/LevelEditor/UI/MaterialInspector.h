#pragma once

#include <Assets/MaterialAsset.h>

#include <filesystem>
#include <string>
#include <vector>

namespace lts::editor
{
    class StaticMeshRenderer;

    class MaterialInspector final
    {
    public:
        void Draw(
            const std::wstring& meshAssetPath,
            StaticMeshRenderer& renderer) noexcept;

        void Reset() noexcept;

    private:
        struct MaterialSlot final
        {
            std::filesystem::path file;

            engine::assets::MaterialAssetDesc original;
            engine::assets::MaterialAssetDesc edited;

            std::string message;

            bool dirty = false;
            bool error = false;
        };

        [[nodiscard]]
        bool Reload(
            const std::wstring& meshAssetPath) noexcept;

        [[nodiscard]]
        bool SaveMaterial(
            std::size_t slotIndex,
            StaticMeshRenderer& renderer) noexcept;

        [[nodiscard]]
        bool SelectTexture(
            std::size_t slotIndex,
            std::optional<engine::assets::AssetPath>&
                texture) noexcept;

        void DrawTexture(
            const char* label,
            int controlId,
            std::size_t slotIndex,
            std::optional<engine::assets::AssetPath>& texture,
            StaticMeshRenderer& renderer) noexcept;

        std::wstring meshAssetPath_;

        std::filesystem::path gameRoot_;
        std::filesystem::path meshFile_;
        std::filesystem::path textureDirectory_;

        std::vector<MaterialSlot> slots_;

        std::string status_;
        bool statusIsError_ = false;
    };
}