#pragma once

#include "Editor/Tools/Character/CharacterPreviewRenderer.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace engine::graphics
{
    class CommandContext;
    class RenderDevice;
}

namespace lts::editor
{
    enum class CharacterModuleType : std::uint8_t
    {
        Head = 0,
        Body,
        Legs,
        Shoes,
        Hands,
        Count
    };

    enum class CharacterArmorType : std::uint8_t
    {
        Helmet = 0,
        Mask,
        EyeWear,
        Gloves,
        Armor,
        Backpack,
        Count
    };

    enum class CharacterWeaponSlot : std::uint8_t
    {
        Primary = 0,
        Secondary,
        Count
    };

    struct CharacterVector3 final
    {
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
    };

    struct CharacterTransform final
    {
        CharacterVector3 position;
        CharacterVector3 rotation;
        CharacterVector3 scale
        {
            1.0F,
            1.0F,
            1.0F
        };
    };

    struct CharacterMeshSlot final
    {
        std::filesystem::path meshFile;
        bool visible = true;
    };

    struct CharacterArmorSlot final
    {
        std::filesystem::path meshFile;
        std::string attachmentBone;
        CharacterTransform localTransform;

        /*
         * true:
         * mesh использует body.skeleton и общую skinning palette.
         *
         * false:
         * mesh является жёстким attachment к attachmentBone.
         */
        bool skinned = false;
        bool visible = true;
    };

    struct CharacterWeaponIk final
    {
        bool enabled = true;

        /*
         * Используется для оружия без IK:
         * holster, спина, пояс и другие attachment-позиции.
         */
        std::string attachmentBone = "hand_r";

        /*
         * Основная рука.
         * Оружие позиционируется относительно этой кости.
         */
        std::string rightHandBone = "hand_r";

        /*
         * Цепочка IK вспомогательной руки.
         */
        std::string leftUpperArmBone = "upperarm_l";
        std::string leftLowerArmBone = "lowerarm_l";
        std::string leftHandBone = "hand_l";

        /*
         * Дополнительное смещение pole position локтя.
         * Значение задаётся в пространстве персонажа.
         */
        CharacterVector3 leftElbowPoleOffset;

        /*
         * Поправка положения оружия относительно правой руки.
         */
        CharacterTransform weaponTransform;

        /*
         * Положение основной рукояти в локальном пространстве оружия.
         */
        CharacterTransform rightHandTransform;

        /*
         * Положение цевья в локальном пространстве оружия.
         */
        CharacterTransform leftHandTransform;
    };

    struct CharacterWeapon final
    {
        std::filesystem::path meshFile;
        CharacterWeaponIk ik;
        bool visible = true;
    };

    struct CharacterDefinition final
    {
        std::filesystem::path sourceFile;
        std::filesystem::path bodySkeletonFile;

        std::array<
            CharacterMeshSlot,
            static_cast<std::size_t>(
                CharacterModuleType::Count)>
            modules;

        std::array<
            CharacterArmorSlot,
            static_cast<std::size_t>(
                CharacterArmorType::Count)>
            armor;

        std::array<
            CharacterWeapon,
            static_cast<std::size_t>(
                CharacterWeaponSlot::Count)>
            weapons;
    };

    class CharacterEditor final
    {
    public:
        CharacterEditor() noexcept;

        void SetOpen(bool open) noexcept;

        [[nodiscard]]
        bool IsOpen() const noexcept;

        void Draw(engine::graphics::RenderDevice& device, engine::graphics::CommandContext& context) noexcept;
        void Shutdown(engine::graphics::RenderDevice& device) noexcept;

    private:
        enum class InspectorSection : std::uint8_t
        {
            Character = 0,
            Armor,
            Weapons,
            Skeleton,
            Validation
        };

        void DrawToolbar() noexcept;
        void DrawContent(engine::graphics::RenderDevice& device, engine::graphics::CommandContext& context) noexcept;
        void DrawHierarchy() noexcept;
        void DrawInspector() noexcept;
        void DrawPreview(engine::graphics::RenderDevice& device, engine::graphics::CommandContext& context) noexcept;
        void DrawStatusBar() noexcept;

        void DrawCharacterInspector() noexcept;
        void DrawArmorInspector() noexcept;
        void DrawWeaponInspector() noexcept;
        void DrawSkeletonInspector() noexcept;
        void DrawValidationInspector() noexcept;

        void DrawMeshSlot(
            const char* label,
            CharacterMeshSlot& slot) noexcept;

        void DrawArmorSlot(
            const char* label,
            CharacterArmorSlot& slot,
            const char* defaultBone) noexcept;

        void DrawWeaponSlot(
            const char* label,
            CharacterWeapon& weapon) noexcept;

        void DrawTransform(
            const char* identifier,
            CharacterTransform& transform) noexcept;

        void NewCharacter() noexcept;
        void OpenCharacter() noexcept;
        void SaveCharacter() noexcept;
        void SaveCharacterAs() noexcept;

        void SelectBodySkeleton() noexcept;

        void SelectMeshFile(
            std::filesystem::path& destination) noexcept;

        void ValidateCharacter() noexcept;

        [[nodiscard]]
        bool SerializeCharacter(
            const std::filesystem::path& file) noexcept;

        [[nodiscard]]
        bool DeserializeCharacter(
            const std::filesystem::path& file) noexcept;

        [[nodiscard]]
        bool IsDirty() const noexcept;

        void MarkDirty() noexcept;

        void SetStatus(
            std::string message,
            bool error = false) noexcept;

        bool open_ = false;
        bool dirty_ = false;
        bool statusIsError_ = false;

        InspectorSection selectedSection_ =
            InspectorSection::Character;

        CharacterModuleType selectedModule_ =
            CharacterModuleType::Body;

        CharacterArmorType selectedArmor_ =
            CharacterArmorType::Armor;

        CharacterWeaponSlot selectedWeapon_ =
            CharacterWeaponSlot::Primary;

        CharacterDefinition character_;

        std::vector<std::string> validationErrors_;
        std::vector<std::string> validationWarnings_;

        CharacterPreviewRenderer previewRenderer_;
        std::vector<CharacterPreviewDebugBone> previewDebugBones_;

        bool previewInitialized_ = false;
        bool previewDirty_ = true;

        std::string status_ = "Character Editor ready.";

        float previewYaw_ = 180.0F;
        float previewPitch_ = 0.0F;
        float previewDistance_ = 3.0F;

        bool showSkeleton_ = false;
        bool showBones_ = false;
        bool showSockets_ = true;
        bool showGrid_ = true;
    };
}