#pragma once

#include "Editor/EditorSceneDocument.h"

#include <Platform/Window.h>

namespace lts::editor
{
    class EditorTransformController final
    {
    public:
        EditorTransformController() noexcept = default;
        ~EditorTransformController() noexcept = default;

        EditorTransformController(const EditorTransformController&) = delete;
        EditorTransformController& operator=(const EditorTransformController&) = delete;

        void SetViewportWindow(
            engine::platform::NativeWindowHandle window) noexcept;

        [[nodiscard]]
        bool Update(
            EditorSceneDocument& document,
            double deltaSeconds) noexcept;

    private:
        engine::platform::NativeWindowHandle viewportWindow_;
    };
}