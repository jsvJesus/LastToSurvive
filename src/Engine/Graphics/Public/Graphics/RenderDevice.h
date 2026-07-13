#pragma once

#include "Graphics/Buffer.h"
#include "Graphics/GraphicsBackend.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/InputLayout.h"
#include "Graphics/PipelineState.h"
#include "Graphics/ResourceHandle.h"
#include "Graphics/Shader.h"
#include "Graphics/SwapChain.h"
#include "Graphics/Texture.h"

#include <cstddef>
#include <memory>

namespace engine::graphics
{
    struct RenderDeviceDesc final
    {
        GraphicsBackend backend = GraphicsBackend::None;
        bool enableValidation = true;
        bool enableDebugMarkers = true;

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return backend != GraphicsBackend::None;
        }
    };

    class RenderDevice
    {
    public:
        virtual ~RenderDevice() noexcept = default;

        RenderDevice(const RenderDevice&) = delete;
        RenderDevice& operator=(const RenderDevice&) = delete;

        RenderDevice(RenderDevice&&) = delete;
        RenderDevice& operator=(RenderDevice&&) = delete;

        [[nodiscard]] virtual GraphicsBackend GetBackend() const noexcept = 0;

        [[nodiscard]] virtual DeviceState GetState() const noexcept = 0;

        [[nodiscard]] virtual GraphicsResult Initialize(
            const RenderDeviceDesc& desc) noexcept = 0;

        virtual void Shutdown() noexcept = 0;

        [[nodiscard]] virtual GraphicsResult CreateSwapChain(
            const SwapChainDesc& desc,
            std::unique_ptr<SwapChain>& outSwapChain) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult CreateTexture(
            const TextureDesc& desc,
            const TextureSubresourceData* initialData,
            std::size_t initialDataCount,
            TextureHandle& outTexture) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult DestroyTexture(
            TextureHandle texture) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult CreateBuffer(
            const BufferDesc& desc,
            const BufferInitialData* initialData,
            BufferHandle& outBuffer) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult DestroyBuffer(
            BufferHandle buffer) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult CreateShader(
            const ShaderDesc& desc,
            ShaderHandle& outShader) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult DestroyShader(
            ShaderHandle shader) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult CreateInputLayout(
            const InputLayoutDesc& desc,
            InputLayoutHandle& outInputLayout) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult DestroyInputLayout(
            InputLayoutHandle inputLayout) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult CreateGraphicsPipeline(
            const GraphicsPipelineDesc& desc,
            PipelineStateHandle& outPipeline) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult DestroyGraphicsPipeline(
            PipelineStateHandle pipeline) noexcept = 0;

        [[nodiscard]] bool IsReady() const noexcept
        {
            return GetState() == DeviceState::Ready;
        }

    protected:
        RenderDevice() noexcept = default;
    };
}
