#pragma once
#include "Assets/MeshAsset.h"
#include "Graphics/GraphicsBackend.h"
#include <cstddef>
#include <string>
namespace engine::renderer
{
    // Read-only resource metadata exposed without native pointers. Ownership,
    // GPU handles and mutation remain inside StaticModelRenderer.
    struct StaticModelRenderResourceInfo final
    {
        engine::graphics::GraphicsBackend backend=engine::graphics::GraphicsBackend::None;
        engine::assets::MeshBounds bounds;
        std::size_t materialCount=0U;
        std::string debugName;
        [[nodiscard]] bool IsValid()const noexcept{return backend!=engine::graphics::GraphicsBackend::None&&bounds.IsValid()&&materialCount!=0U;}
    };
}
