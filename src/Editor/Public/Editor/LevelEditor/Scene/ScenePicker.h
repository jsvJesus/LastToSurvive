#pragma once

#include "Editor/LevelEditor/Viewport/CameraController.h"
#include "Editor/LevelEditor/Scene/SceneDocument.h"

#include <cstddef>

namespace lts::editor
{
    class StaticMeshRenderer;
    class ScenePicker final
    {
    public:
        ScenePicker() = delete;

        [[nodiscard]]
        static bool Pick(
            const SceneDocument& document,
            const EditorPickRay& ray,
            std::size_t& outEntityIndex,
            float& outDistance,
            const StaticMeshRenderer* meshRenderer = nullptr,
            bool includeEditorRoads = true) noexcept;
    };
}