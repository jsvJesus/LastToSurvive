#pragma once

#include "Editor/EditorCameraController.h"
#include "Editor/EditorSceneDocument.h"

#include <cstddef>

namespace lts::editor
{
    class EditorStaticMeshRenderer;
    class EditorScenePicker final
    {
    public:
        EditorScenePicker() = delete;

        [[nodiscard]]
        static bool Pick(
            const EditorSceneDocument& document,
            const EditorPickRay& ray,
            std::size_t& outEntityIndex,
            float& outDistance,
            const EditorStaticMeshRenderer* meshRenderer = nullptr) noexcept;
    };
}
