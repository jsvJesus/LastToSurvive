#pragma once
#include "Assets/AssetLoaderRegistry.h"
#include "Assets/AssetManager.h"
#include "Assets/AssetPath.h"
#include "Assets/AssetResult.h"
#include "Assets/MeshAsset.h"
#include "Assets/ShaderAsset.h"
#include "Graphics/CommandContext.h"
#include "Graphics/GraphicsBackend.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/RenderDevice.h"
#include "Math/Matrix4.h"
#include "Math/Vector4.h"
#include "Renderer/RenderView.h"
#include <cstddef>
#include <cstdint>
#include <memory>
namespace engine::renderer
{
    struct StaticModelRenderHandle final
    {
        std::uint32_t index=0U,generation=0U;
        [[nodiscard]] constexpr bool IsValid()const noexcept{return generation!=0U;}
        friend constexpr bool operator==(const StaticModelRenderHandle&a,const StaticModelRenderHandle&b)noexcept{return a.index==b.index&&a.generation==b.generation;}
    };
    struct StaticModelInstance final
    {
        StaticModelRenderHandle model;
        engine::math::Matrix4 world;
        engine::math::Vector4 tint{1,1,1,1};
        bool visible=true;
        std::uint32_t objectId=0U;
        [[nodiscard]] bool IsValid()const noexcept;
    };
    struct StaticModelRenderStats final
    {
        std::size_t submittedInstances=0,acceptedInstances=0,rejectedInvalidInstances=0,drawCalls=0,triangles=0;
        std::size_t opaqueDraws=0,maskDraws=0,blendDraws=0,pipelineChanges=0,materialChanges=0,meshChanges=0;
        std::size_t uniqueModelResources=0,uniqueGpuTextures=0,reusedTextureBindings=0;
    };
    class StaticModelRenderer final
    {
    public:
        StaticModelRenderer()noexcept;~StaticModelRenderer()noexcept;
        StaticModelRenderer(const StaticModelRenderer&)=delete;StaticModelRenderer&operator=(const StaticModelRenderer&)=delete;
        [[nodiscard]] engine::graphics::GraphicsResult Initialize(engine::graphics::RenderDevice&,engine::graphics::CommandContext&,const engine::assets::ShaderAsset&,const engine::assets::ShaderAsset&)noexcept;
        void Shutdown()noexcept;[[nodiscard]] bool IsInitialized()const noexcept;
        [[nodiscard]] engine::assets::AssetResult CreateModel(engine::assets::AssetManager&,engine::assets::AssetLoaderRegistry&,const engine::assets::AssetPath&,StaticModelRenderHandle&)noexcept;
        [[nodiscard]] engine::assets::AssetResult DestroyModel(StaticModelRenderHandle)noexcept;
        [[nodiscard]] bool IsModelValid(StaticModelRenderHandle)const noexcept;
        [[nodiscard]] engine::assets::AssetResult GetModelBounds(StaticModelRenderHandle,engine::assets::MeshBounds&)const noexcept;
        [[nodiscard]] engine::graphics::GraphicsResult Render(const RenderView&,const StaticModelInstance*,std::size_t,StaticModelRenderStats&)noexcept;
        void Unbind()noexcept;
    private:class Impl;std::unique_ptr<Impl> impl_;
    };
}
