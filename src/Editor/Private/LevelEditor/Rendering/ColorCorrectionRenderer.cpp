#include "Editor/LevelEditor/Rendering/ColorCorrectionRenderer.h"
#include "Editor/LevelEditor/Rendering/ShaderCompiler.h"

#include <Core/Log.h>

#include <Graphics/Buffer.h>
#include <Graphics/CommandContext.h>
#include <Graphics/PipelineState.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/Sampler.h>
#include <Graphics/Shader.h>

#include <DirectXMath.h>
#include <wrl/client.h>

#include <algorithm>
#include <cmath>
#include <new>
#include <string>

namespace lts::editor
{
    namespace
    {
        struct alignas(16) ColorCorrectionConstants final
        {
            DirectX::XMFLOAT4 exposureContrastSaturationGamma;
            DirectX::XMFLOAT4 vibranceTemperatureTintFilmic;
            DirectX::XMFLOAT4 liftGainSharpenVignette;
            DirectX::XMFLOAT4 bloomParameters;
            DirectX::XMFLOAT4 texelSizeAndVignette;
            DirectX::XMFLOAT4 colorFilterEnabled;
        };

        static_assert(sizeof(ColorCorrectionConstants) % 16U == 0U);

        void LogFailure(
            const char* const operation,
            const engine::graphics::GraphicsResult result) noexcept
        {
            std::string message = operation != nullptr
                ? operation
                : "Color correction operation";
            message += " failed: ";
            message += engine::graphics::ToString(result);
            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                "LTS.Editor.ColorCorrection",
                message);
        }
    }

    class ColorCorrectionRenderer::Impl final
    {
    public:
        [[nodiscard]] bool Initialize(
            engine::graphics::RenderDevice& device) noexcept
        {
            if (initialized_)
            {
                return true;
            }

            Microsoft::WRL::ComPtr<ID3DBlob> vertexBytecode;
            Microsoft::WRL::ComPtr<ID3DBlob> pixelBytecode;
            if (!CompileEditorShaderFile(
                    L"ColorCorrection.hlsl",
                    "VSMain",
                    "vs_5_0",
                    "LTS.Editor.ColorCorrection",
                    vertexBytecode) ||
                !CompileEditorShaderFile(
                    L"ColorCorrection.hlsl",
                    "PSMain",
                    "ps_5_0",
                    "LTS.Editor.ColorCorrection",
                    pixelBytecode))
            {
                return false;
            }

            engine::graphics::ShaderDesc shaderDescription;
            shaderDescription.stage = engine::graphics::ShaderStage::Vertex;
            shaderDescription.bytecode.data = vertexBytecode->GetBufferPointer();
            shaderDescription.bytecode.size = vertexBytecode->GetBufferSize();
            shaderDescription.debugName = "EditorColorCorrection.VertexShader";
            auto result = device.CreateShader(shaderDescription, vertexShader_);
            if (engine::graphics::Failed(result))
            {
                LogFailure("Create color correction vertex shader", result);
                Shutdown(device);
                return false;
            }

            shaderDescription.stage = engine::graphics::ShaderStage::Pixel;
            shaderDescription.bytecode.data = pixelBytecode->GetBufferPointer();
            shaderDescription.bytecode.size = pixelBytecode->GetBufferSize();
            shaderDescription.debugName = "EditorColorCorrection.PixelShader";
            result = device.CreateShader(shaderDescription, pixelShader_);
            if (engine::graphics::Failed(result))
            {
                LogFailure("Create color correction pixel shader", result);
                Shutdown(device);
                return false;
            }

            engine::graphics::BufferDesc constantBufferDescription;
            constantBufferDescription.byteSize = sizeof(ColorCorrectionConstants);
            constantBufferDescription.usage =
                engine::graphics::ResourceUsage::Default;
            constantBufferDescription.bindFlags =
                engine::graphics::BufferBindFlags::Constant;
            result = device.CreateBuffer(
                constantBufferDescription,
                nullptr,
                constantBuffer_);
            if (engine::graphics::Failed(result))
            {
                LogFailure("Create color correction constants", result);
                Shutdown(device);
                return false;
            }

            engine::graphics::SamplerDesc samplerDescription;
            samplerDescription.filter = engine::graphics::TextureFilter::Linear;
            samplerDescription.addressU =
                engine::graphics::TextureAddressMode::Clamp;
            samplerDescription.addressV =
                engine::graphics::TextureAddressMode::Clamp;
            samplerDescription.addressW =
                engine::graphics::TextureAddressMode::Clamp;
            samplerDescription.debugName = "EditorColorCorrection.Sampler";
            result = device.CreateSampler(samplerDescription, sampler_);
            if (engine::graphics::Failed(result))
            {
                LogFailure("Create color correction sampler", result);
                Shutdown(device);
                return false;
            }

            engine::graphics::GraphicsPipelineDesc pipelineDescription;
            pipelineDescription.vertexShader = vertexShader_;
            pipelineDescription.pixelShader = pixelShader_;
            pipelineDescription.topology =
                engine::graphics::PrimitiveTopology::TriangleList;
            pipelineDescription.rasterizer.cullMode =
                engine::graphics::CullMode::None;
            pipelineDescription.depthStencil.depthEnable = false;
            pipelineDescription.depthStencil.depthWriteEnable = false;
            pipelineDescription.debugName = "EditorColorCorrection.Pipeline";
            result = device.CreateGraphicsPipeline(
                pipelineDescription,
                pipeline_);
            if (engine::graphics::Failed(result))
            {
                LogFailure("Create color correction pipeline", result);
                Shutdown(device);
                return false;
            }

            initialized_ = true;
            engine::core::GetLogger().Write(
                engine::core::LogLevel::Information,
                "LTS.Editor.ColorCorrection",
                "DX11 color correction renderer initialized.");
            return true;
        }

        void Shutdown(engine::graphics::RenderDevice& device) noexcept
        {
            initialized_ = false;
            if (pipeline_.IsValid())
            {
                static_cast<void>(device.DestroyGraphicsPipeline(pipeline_));
                pipeline_ = {};
            }
            if (sampler_.IsValid())
            {
                static_cast<void>(device.DestroySampler(sampler_));
                sampler_ = {};
            }
            if (constantBuffer_.IsValid())
            {
                static_cast<void>(device.DestroyBuffer(constantBuffer_));
                constantBuffer_ = {};
            }
            if (pixelShader_.IsValid())
            {
                static_cast<void>(device.DestroyShader(pixelShader_));
                pixelShader_ = {};
            }
            if (vertexShader_.IsValid())
            {
                static_cast<void>(device.DestroyShader(vertexShader_));
                vertexShader_ = {};
            }
        }

        [[nodiscard]] engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const engine::graphics::TextureHandle source,
            const std::uint32_t width,
            const std::uint32_t height,
            const ColorCorrectionSettings& settings) noexcept
        {
            if (!initialized_ || !source.IsValid() || width == 0U || height == 0U)
            {
                return engine::graphics::GraphicsResult::InvalidState;
            }

            ColorCorrectionConstants constants{};
            constants.exposureContrastSaturationGamma = {
                (std::clamp)(settings.exposure, -5.0F, 5.0F),
                (std::clamp)(settings.contrast, 0.0F, 3.0F),
                (std::clamp)(settings.saturation, 0.0F, 3.0F),
                (std::clamp)(settings.gamma, 0.1F, 4.0F)};
            constants.vibranceTemperatureTintFilmic = {
                (std::clamp)(settings.vibrance, -1.0F, 1.0F),
                (std::clamp)(settings.temperature, -1.0F, 1.0F),
                (std::clamp)(settings.tint, -1.0F, 1.0F),
                (std::clamp)(settings.filmicStrength, 0.0F, 1.0F)};
            constants.liftGainSharpenVignette = {
                (std::clamp)(settings.lift, -0.5F, 0.5F),
                (std::clamp)(settings.gain, 0.0F, 3.0F),
                (std::clamp)(settings.sharpen, 0.0F, 2.0F),
                (std::clamp)(settings.vignette, 0.0F, 1.0F)};
            constants.bloomParameters = {
                (std::clamp)(settings.bloomStrength, 0.0F, 2.0F),
                (std::clamp)(settings.bloomThreshold, 0.0F, 1.0F),
                (std::clamp)(settings.bloomRadius, 0.5F, 8.0F),
                0.0F};
            constants.texelSizeAndVignette = {
                1.0F / static_cast<float>(width),
                1.0F / static_cast<float>(height),
                (std::clamp)(settings.vignetteSoftness, 0.05F, 1.0F),
                0.0F};
            constants.colorFilterEnabled = {
                (std::clamp)(settings.colorFilter[0], 0.0F, 2.0F),
                (std::clamp)(settings.colorFilter[1], 0.0F, 2.0F),
                (std::clamp)(settings.colorFilter[2], 0.0F, 2.0F),
                settings.enabled ? 1.0F : 0.0F};

            auto result = context.SetGraphicsPipeline(pipeline_);
            if (engine::graphics::Failed(result)) return result;
            result = context.UpdateBuffer(
                constantBuffer_,
                &constants,
                sizeof(constants));
            if (engine::graphics::Failed(result)) return result;
            result = context.SetConstantBuffers(
                engine::graphics::ShaderStage::Pixel,
                0U,
                &constantBuffer_,
                1U);
            if (engine::graphics::Failed(result)) return result;
            result = context.SetShaderResources(
                engine::graphics::ShaderStage::Pixel,
                0U,
                &source,
                1U);
            if (engine::graphics::Failed(result)) return result;
            result = context.SetSamplers(
                engine::graphics::ShaderStage::Pixel,
                0U,
                &sampler_,
                1U);
            if (engine::graphics::Failed(result)) return result;

            result = context.Draw(3U, 0U);

            static_cast<void>(context.UnbindShaderResources(
                engine::graphics::ShaderStage::Pixel, 0U, 1U));
            static_cast<void>(context.UnbindSamplers(
                engine::graphics::ShaderStage::Pixel, 0U, 1U));
            static_cast<void>(context.UnbindConstantBuffers(
                engine::graphics::ShaderStage::Pixel, 0U, 1U));
            context.UnbindGraphicsPipeline();
            return result;
        }

    private:
        engine::graphics::ShaderHandle vertexShader_;
        engine::graphics::ShaderHandle pixelShader_;
        engine::graphics::BufferHandle constantBuffer_;
        engine::graphics::SamplerHandle sampler_;
        engine::graphics::PipelineStateHandle pipeline_;
        bool initialized_ = false;
    };

    ColorCorrectionRenderer::ColorCorrectionRenderer() noexcept = default;
    ColorCorrectionRenderer::~ColorCorrectionRenderer() noexcept = default;

    bool ColorCorrectionRenderer::Initialize(
        engine::graphics::RenderDevice& device) noexcept
    {
        if (impl_ != nullptr) return true;
        try
        {
            impl_ = std::make_unique<Impl>();
        }
        catch (...)
        {
            return false;
        }
        if (!impl_->Initialize(device))
        {
            impl_.reset();
            return false;
        }
        return true;
    }

    void ColorCorrectionRenderer::Shutdown(
        engine::graphics::RenderDevice& device) noexcept
    {
        if (impl_ == nullptr) return;
        impl_->Shutdown(device);
        impl_.reset();
    }

    engine::graphics::GraphicsResult ColorCorrectionRenderer::Render(
        engine::graphics::CommandContext& context,
        const engine::graphics::TextureHandle source,
        const std::uint32_t width,
        const std::uint32_t height,
        const ColorCorrectionSettings& settings) noexcept
    {
        return impl_ != nullptr
            ? impl_->Render(context, source, width, height, settings)
            : engine::graphics::GraphicsResult::InvalidState;
    }
}
