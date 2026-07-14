#include "r3dPCH.h"
#include "r3d.h"

#include "StudioGraphicsShell.h"
#include "StudioRuntimeBridge.h"

#include <Assets/GpuMesh.h>
#include <Assets/MaterialAsset.h>
#include <Assets/MeshAssetBuilder.h>

#include <Graphics/Buffer.h>
#include <Graphics/CommandContext.h>
#include <Graphics/InputLayout.h>
#include <Graphics/PipelineState.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/Shader.h>
#include <Graphics/SwapChain.h>
#include <Graphics/Texture.h>
#include <Graphics/Viewport.h>
#include <GraphicsDX11/D3D11Device.h>
#include <Platform/MessagePump.h>
#include <Platform/Clock.h>
#include <Platform/Window.h>
#include <Runtime/RendererBackend.h>
#include <Runtime/Engine.h>
#include <Math/Matrix4.h>
#include <Math/Vector3.h>

#include <d3dcompiler.h>

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <memory>

extern void RegisterMsgProc(
    bool (*proc)(UINT, WPARAM, LPARAM));
extern void UnregisterMsgProc(
    bool (*proc)(UINT, WPARAM, LPARAM));
extern char __r3dCmdLine[1024];
PCHAR* CommandLineToArgvA(PCHAR commandLine, int* argumentCount);

namespace
{
    using engine::graphics::GraphicsResult;
    using studio::StudioGraphicsShellResult;

    constexpr const char* StudioBindingShaderSource = R"hlsl(
cbuffer FrameConstants : register(b0)
{
    row_major float4x4 worldViewProjection;
    row_major float4x4 world;
    float4 frameData;
};
cbuffer MaterialConstants : register(b0)
{
    float4 baseColorFactor;
    float4 emissiveMetallic;
    float4 roughnessAlpha;
};

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 worldNormal : NORMAL;
    float2 texcoord : TEXCOORD0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(float4(input.position, 1.0f), worldViewProjection);
    output.worldNormal = normalize(mul(float4(input.normal, 0.0f), world).xyz);
    output.texcoord = input.texcoord;
    return output;
}

Texture2D checkerboardTexture : register(t0);
SamplerState checkerboardSampler : register(s0);

float4 PSMain(VertexOutput input) : SV_Target0
{
    float3 lightDirection = normalize(float3(-0.45f, 0.75f, -0.55f));
    float lighting = 0.22f + 0.78f * saturate(dot(input.worldNormal, lightDirection));
    float4 color = checkerboardTexture.Sample(checkerboardSampler, input.texcoord) * baseColorFactor;
    return float4(color.rgb * lighting + emissiveMetallic.rgb, color.a);
}
)hlsl";

    struct alignas(16) StudioFrameConstants final
    {
        float worldViewProjection[16];
        float world[16];
        float frameData[4];
    };

    struct alignas(16) StudioMaterialConstants final
    { float baseColor[4]; float emissiveMetallic[4]; float roughnessAlpha[4]; };

    static_assert(sizeof(StudioFrameConstants) % 16U == 0U);

    class ShaderBlob final
    {
    public:
        ~ShaderBlob() noexcept { Reset(); }
        ShaderBlob() noexcept = default;
        ShaderBlob(const ShaderBlob&) = delete;
        ShaderBlob& operator=(const ShaderBlob&) = delete;

        [[nodiscard]] ID3DBlob* Get() const noexcept { return blob_; }
        [[nodiscard]] ID3DBlob** Put() noexcept
        {
            Reset();
            return &blob_;
        }

    private:
        void Reset() noexcept
        {
            if (blob_ != nullptr)
            {
                blob_->Release();
                blob_ = nullptr;
            }
        }

        ID3DBlob* blob_ = nullptr;
    };

    bool CompileStudioShader(
        const char* const entryPoint,
        const char* const target,
        ShaderBlob& outBytecode) noexcept
    {
        ShaderBlob errors;
        const HRESULT result = D3DCompile(
            StudioBindingShaderSource,
            std::strlen(StudioBindingShaderSource),
            "StudioDX11Bindings.hlsl",
            nullptr,
            nullptr,
            entryPoint,
            target,
            D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS,
            0U,
            outBytecode.Put(),
            errors.Put());
        if (SUCCEEDED(result))
            return true;

        if (errors.Get() != nullptr)
        {
            r3dOutToLog("[Graphics][DX11] Shader compiler error (%s): %.*s\n",
                entryPoint,
                static_cast<int>(errors.Get()->GetBufferSize()),
                static_cast<const char*>(errors.Get()->GetBufferPointer()));
        }
        else
        {
            r3dOutToLog("[Graphics][DX11] Shader compilation failed (%s): HRESULT=0x%08lx\n",
                entryPoint, static_cast<unsigned long>(result));
        }
        return false;
    }

    bool HasCommandLineSwitch(const char* switchName) noexcept
    {
        if (switchName == nullptr || *switchName == '\0')
            return false;

        int argumentCount = 0;
        PCHAR* const arguments = CommandLineToArgvA(
            __r3dCmdLine, &argumentCount);
        if (arguments == nullptr)
            return false;
        bool found = false;
        for (int index = 0; index < argumentCount; ++index)
        {
            if (_stricmp(arguments[index], switchName) == 0)
            {
                found = true;
                break;
            }
        }
        GlobalFree(arguments);
        return found;
    }

    class StudioDX11Shell final
    {
    public:
        bool Initialize(const std::uintptr_t nativeWindow) noexcept
        {
            Shutdown();
            HWND const windowHandle = reinterpret_cast<HWND>(nativeWindow);
            if (windowHandle == nullptr)
                return Fail("invalid Studio HWND", GraphicsResult::InvalidArgument);

            if (!ApplyResizableWindowStyle(windowHandle))
                return Fail("Studio window style", GraphicsResult::BackendFailure);

            window_ = engine::platform::Window(
                engine::platform::NativeWindowHandle::FromValue(nativeWindow));
            const engine::platform::WindowSize size = window_.GetClientSize();
            if (!window_.IsValid() || size.IsEmpty())
                return Fail("invalid Studio HWND or zero client size", GraphicsResult::InvalidArgument);

            engine::graphics::RenderDeviceDesc deviceDesc;
            deviceDesc.backend = engine::graphics::GraphicsBackend::D3D11;
            deviceDesc.enableValidation = true;
            // Development-only fallback validation switch. It is inert unless
            // explicitly supplied and forces failure before device creation.
            if (HasCommandLineSwitch("-dx11shell-fail"))
                deviceDesc.backend = engine::graphics::GraphicsBackend::None;

            GraphicsResult result = device_.Initialize(deviceDesc);
            if (engine::graphics::Failed(result))
                return Fail("device creation", result);

            context_ = device_.GetImmediateCommandContext();
            if (context_ == nullptr || !context_->IsValid())
                return Fail("immediate CommandContext", GraphicsResult::InvalidState);

            engine::graphics::SwapChainDesc swapDesc;
            swapDesc.window = window_.GetNativeHandle();
            swapDesc.width = size.width;
            swapDesc.height = size.height;
            swapDesc.bufferCount = 2;
            swapDesc.presentMode = engine::graphics::PresentMode::VSync;
            result = device_.CreateSwapChain(swapDesc, swapChain_);
            if (engine::graphics::Failed(result))
                return Fail("swap-chain creation", result);

            if (!CreateSizeDependentResources(size.width, size.height))
                return false;
            if (!CreateGeometryResources())
                return false;
            if (!CreateBindingResources())
                return false;

            width_ = size.width;
            height_ = size.height;
            minimized_ = false;
            initialized_ = true;
            r3dOutToLog("[Graphics] Selected DX11 Studio shell backend\n");
            r3dOutToLog("[Graphics][DX11] Device created: featureLevel=0x%04x, debugLayer=%s\n",
                static_cast<unsigned int>(device_.GetFeatureLevel()),
                device_.IsDebugLayerEnabled() ? "enabled" : "disabled");
            r3dOutToLog("[Graphics][DX11] Swap chain, backbuffer and depth buffer created: %ux%u\n",
                width_, height_);
            return true;
        }

        void OnSize(const WPARAM sizeType, const std::uint32_t width,
            const std::uint32_t height) noexcept
        {
            if (!initialized_)
                return;
            if (sizeType == SIZE_MINIMIZED || width == 0 || height == 0)
            {
                if (!minimized_)
                    r3dOutToLog("[Graphics][DX11] Studio window minimized\n");
                minimized_ = true;
                return;
            }

            pendingWidth_ = width;
            pendingHeight_ = height;
            resizePending_ = true;
            if (minimized_)
            {
                r3dOutToLog("[Graphics][DX11] Studio window restored\n");
                resetTimer_ = true;
            }
            minimized_ = false;
        }

        void RequestClose() noexcept
        {
            if (!closeRequested_)
                r3dOutToLog("[Graphics][DX11] Normal user close requested\n");
            closeRequested_ = true;
        }

        [[nodiscard]] bool IsCloseRequested() const noexcept
        {
            return closeRequested_;
        }

        [[nodiscard]] bool ShouldWaitForMessage() const noexcept
        {
            return minimized_ || occluded_;
        }

        [[nodiscard]] bool ConsumeTimerReset() noexcept
        {
            const bool reset = resetTimer_;
            resetTimer_ = false;
            return reset;
        }

        [[nodiscard]] StudioGraphicsShellResult GetFailureResult() const noexcept
        {
            return failureResult_;
        }

        bool RenderFrame(const double deltaSeconds) noexcept
        {
            if (!initialized_)
                return false;
            if (resizePending_ && !Resize(pendingWidth_, pendingHeight_))
                return false;
            if (minimized_)
                return true;
            if (!UpdateFrameConstants(deltaSeconds))
                return false;

            engine::graphics::ClearColor clearColor;
            clearColor.red = 0.035F;
            clearColor.green = 0.055F;
            clearColor.blue = 0.085F;
            clearColor.alpha = 1.0F;

            GraphicsResult result = context_->SetSwapChainRenderTarget(
                *swapChain_, depth_);
            if (engine::graphics::Failed(result))
                return FailFrame("backbuffer/depth bind", result);
            engine::graphics::Viewport viewport;
            viewport.width = static_cast<float>(width_);
            viewport.height = static_cast<float>(height_);
            result = context_->SetViewport(viewport);
            if (engine::graphics::Failed(result))
                return FailFrame("viewport setup", result);
            engine::graphics::ScissorRect scissor;
            scissor.right = static_cast<std::int32_t>(width_);
            scissor.bottom = static_cast<std::int32_t>(height_);
            result = context_->SetScissorRect(scissor);
            if (engine::graphics::Failed(result))
                return FailFrame("scissor setup", result);
            result = context_->ClearSwapChainColor(*swapChain_, clearColor);
            if (engine::graphics::Failed(result))
                return FailFrame("backbuffer clear", result);
            result = context_->ClearDepthStencilTarget(
                depth_, engine::graphics::ClearDepthStencilFlags::Depth |
                engine::graphics::ClearDepthStencilFlags::Stencil, 1.0F, 0);
            if (engine::graphics::Failed(result))
                return FailFrame("depth clear", result);
            if (!DrawGeometry())
                return false;

            engine::graphics::PresentStatus status;
            result = swapChain_->Present(status);
            if (status == engine::graphics::PresentStatus::Occluded)
            {
                if (!occluded_)
                    r3dOutToLog("[Graphics][DX11] Studio window occluded\n");
                occluded_ = true;
                return true;
            }
            if (occluded_)
            {
                r3dOutToLog("[Graphics][DX11] Studio window visible after occlusion\n");
                occluded_ = false;
                resetTimer_ = true;
            }
            if (engine::graphics::Failed(result) ||
                status == engine::graphics::PresentStatus::DeviceLost ||
                status == engine::graphics::PresentStatus::DeviceRemoved)
            {
                r3dOutToLog("[Graphics][DX11] Present failure: result=%s, status=%s\n",
                    engine::graphics::ToString(result), engine::graphics::ToString(status));
                failureResult_ = status == engine::graphics::PresentStatus::DeviceLost
                    ? StudioGraphicsShellResult::DeviceLost
                    : status == engine::graphics::PresentStatus::DeviceRemoved
                        ? StudioGraphicsShellResult::DeviceRemoved
                        : StudioGraphicsShellResult::FrameFailed;
                return false;
            }
            return true;
        }

        void Shutdown() noexcept
        {
            if (context_ != nullptr)
            {
                (void)context_->UnbindConstantBuffers(
                    engine::graphics::ShaderStage::Vertex, 0U, 1U);
                (void)context_->UnbindConstantBuffers(
                    engine::graphics::ShaderStage::Pixel, 0U, 1U);
                (void)context_->UnbindShaderResources(
                    engine::graphics::ShaderStage::Pixel, 0U, 1U);
                (void)context_->UnbindSamplers(
                    engine::graphics::ShaderStage::Pixel, 0U, 1U);
                context_->UnbindGraphicsPipeline();
                context_->UnbindIndexBuffer();
                context_->UnbindRenderTargets();
                context_->ClearState();
                context_->Flush();
            }
            DestroyBindingResources();
            DestroyGeometryResources();
            DestroyDepth();
            swapChain_.reset();
            context_ = nullptr;
            device_.Shutdown();
            RestoreWindowStyle();
            if (initialized_)
                r3dOutToLog("[Graphics][DX11] Studio shell backend shutdown\n");
            initialized_ = false;
            resizePending_ = false;
            minimized_ = false;
            occluded_ = false;
            resetTimer_ = true;
            closeRequested_ = false;
            firstDrawLogged_ = false;
            elapsedSeconds_ = 0.0F;
            meshAsset_.Clear();
            materialAsset_.Clear();
            failureResult_ = StudioGraphicsShellResult::FrameFailed;
            width_ = height_ = 0;
        }

        ~StudioDX11Shell() noexcept { Shutdown(); }

    private:
        bool CreateGeometryResources() noexcept
        {
            ShaderBlob vertexBytecode;
            ShaderBlob pixelBytecode;
            if (!CompileStudioShader("VSMain", "vs_4_0", vertexBytecode) ||
                !CompileStudioShader("PSMain", "ps_4_0", pixelBytecode))
            {
                return Fail("quad shader compilation",
                    GraphicsResult::BackendFailure);
            }
            r3dOutToLog("[Graphics][DX11] Textured quad shader bytecode ready\n");

            engine::graphics::ShaderDesc shaderDesc;
            shaderDesc.stage = engine::graphics::ShaderStage::Vertex;
            shaderDesc.bytecode.data = vertexBytecode.Get()->GetBufferPointer();
            shaderDesc.bytecode.size = vertexBytecode.Get()->GetBufferSize();
            shaderDesc.debugName = "Studio DX11 Textured Quad VS";
            GraphicsResult result = device_.CreateShader(shaderDesc, vertexShader_);
            if (engine::graphics::Failed(result))
                return Fail("quad vertex shader creation", result);
            r3dOutToLog("[Graphics][DX11] Quad vertex shader created\n");

            shaderDesc.stage = engine::graphics::ShaderStage::Pixel;
            shaderDesc.bytecode.data = pixelBytecode.Get()->GetBufferPointer();
            shaderDesc.bytecode.size = pixelBytecode.Get()->GetBufferSize();
            shaderDesc.debugName = "Studio DX11 Textured Quad PS";
            result = device_.CreateShader(shaderDesc, pixelShader_);
            if (engine::graphics::Failed(result))
                return Fail("quad pixel shader creation", result);
            r3dOutToLog("[Graphics][DX11] Quad pixel shader created\n");

            const engine::graphics::VertexElementDesc elements[] = {
                {"POSITION", 0U, engine::graphics::Format::R32G32B32Float,
                    0U, 0U, engine::graphics::VertexInputRate::PerVertex, 0U},
                {"NORMAL", 0U, engine::graphics::Format::R32G32B32Float,
                    0U, 12U, engine::graphics::VertexInputRate::PerVertex, 0U},
                {"TANGENT", 0U, engine::graphics::Format::R32G32B32A32Float,
                    0U, 24U, engine::graphics::VertexInputRate::PerVertex, 0U},
                {"TEXCOORD", 0U, engine::graphics::Format::R32G32Float,
                    0U, 40U, engine::graphics::VertexInputRate::PerVertex, 0U}};
            engine::graphics::InputLayoutDesc inputLayoutDesc;
            inputLayoutDesc.vertexShader = vertexShader_;
            inputLayoutDesc.elements = elements;
            inputLayoutDesc.elementCount = sizeof(elements) / sizeof(elements[0]);
            inputLayoutDesc.debugName = "Studio DX11 Textured Quad Input Layout";
            result = device_.CreateInputLayout(inputLayoutDesc, inputLayout_);
            if (engine::graphics::Failed(result))
                return Fail("quad input layout creation", result);
            r3dOutToLog("[Graphics][DX11] Quad input layout created\n");

            engine::graphics::GraphicsPipelineDesc pipelineDesc;
            pipelineDesc.vertexShader = vertexShader_;
            pipelineDesc.pixelShader = pixelShader_;
            pipelineDesc.inputLayout = inputLayout_;
            pipelineDesc.topology = engine::graphics::PrimitiveTopology::TriangleList;
            pipelineDesc.rasterizer.cullMode = engine::graphics::CullMode::Back;
            pipelineDesc.rasterizer.scissorEnable = true;
            pipelineDesc.blend.renderTargets[0].blendEnable = false;
            pipelineDesc.depthStencil.depthEnable = true;
            pipelineDesc.depthStencil.depthWriteEnable = true;
            pipelineDesc.depthStencil.depthFunction =
                engine::graphics::ComparisonFunction::LessEqual;
            pipelineDesc.debugName = "Studio DX11 Textured Quad Pipeline";
            result = device_.CreateGraphicsPipeline(pipelineDesc, pipeline_);
            if (engine::graphics::Failed(result))
                return Fail("quad pipeline creation", result);
            r3dOutToLog("[Graphics][DX11] Quad pipeline created\n");

            using V = engine::assets::StaticMeshVertex;
            constexpr V vertices[] = {
                {{-1,-1,-1},{0,0,-1},{1,0,0,1},{0,1}},{{-1,1,-1},{0,0,-1},{1,0,0,1},{0,0}},{{1,1,-1},{0,0,-1},{1,0,0,1},{1,0}},{{1,-1,-1},{0,0,-1},{1,0,0,1},{1,1}},
                {{1,-1,1},{0,0,1},{-1,0,0,1},{0,1}},{{1,1,1},{0,0,1},{-1,0,0,1},{0,0}},{{-1,1,1},{0,0,1},{-1,0,0,1},{1,0}},{{-1,-1,1},{0,0,1},{-1,0,0,1},{1,1}},
                {{-1,-1,1},{-1,0,0},{0,0,-1,1},{0,1}},{{-1,1,1},{-1,0,0},{0,0,-1,1},{0,0}},{{-1,1,-1},{-1,0,0},{0,0,-1,1},{1,0}},{{-1,-1,-1},{-1,0,0},{0,0,-1,1},{1,1}},
                {{1,-1,-1},{1,0,0},{0,0,1,1},{0,1}},{{1,1,-1},{1,0,0},{0,0,1,1},{0,0}},{{1,1,1},{1,0,0},{0,0,1,1},{1,0}},{{1,-1,1},{1,0,0},{0,0,1,1},{1,1}},
                {{-1,1,-1},{0,1,0},{1,0,0,1},{0,1}},{{-1,1,1},{0,1,0},{1,0,0,1},{0,0}},{{1,1,1},{0,1,0},{1,0,0,1},{1,0}},{{1,1,-1},{0,1,0},{1,0,0,1},{1,1}},
                {{-1,-1,1},{0,-1,0},{1,0,0,1},{0,1}},{{-1,-1,-1},{0,-1,0},{1,0,0,1},{0,0}},{{1,-1,-1},{0,-1,0},{1,0,0,1},{1,0}},{{1,-1,1},{0,-1,0},{1,0,0,1},{1,1}}};
            constexpr std::uint16_t indices[] = {0,1,2,0,2,3,4,5,6,4,6,7,8,9,10,8,10,11,12,13,14,12,14,15,16,17,18,16,18,19,20,21,22,20,22,23};
            constexpr engine::assets::MeshSubmesh submesh{0U,36U,0,0U};
            const auto assetResult = engine::assets::MeshAssetBuilder::Build(vertices,24U,indices,36U,&submesh,1U,1U,"studio/cube",meshAsset_);
            if (engine::assets::Failed(assetResult)) return Fail("MeshAsset creation", GraphicsResult::InvalidArgument);
            result = gpuMesh_.Upload(device_, meshAsset_);
            if (engine::graphics::Failed(result)) return Fail("GpuMesh upload", result);
            r3dOutToLog("[Graphics][DX11] MeshAsset created: vertices=24 indices=36 submeshes=1 materials=1\n");
            const auto& bounds=meshAsset_.GetBounds();
            r3dOutToLog("[Graphics][DX11] Mesh bounds: min=(%.2f,%.2f,%.2f) max=(%.2f,%.2f,%.2f) radius=%.2f\n",bounds.minimum[0],bounds.minimum[1],bounds.minimum[2],bounds.maximum[0],bounds.maximum[1],bounds.maximum[2],bounds.sphereRadius);
            r3dOutToLog("[Graphics][DX11] GpuMesh uploaded\n");
            return true;
        }

        bool CreateBindingResources() noexcept
        {
            engine::assets::MaterialAssetDesc materialDesc;
            materialDesc.baseColorFactor = {1.0F, 0.95F, 0.9F, 1.0F};
            materialDesc.emissiveFactor = {0.015F, 0.02F, 0.025F};
            materialDesc.sampler.filter = engine::graphics::TextureFilter::Linear;
            materialDesc.sampler.addressU = engine::graphics::TextureAddressMode::Wrap;
            materialDesc.sampler.addressV = engine::graphics::TextureAddressMode::Wrap;
            materialDesc.sampler.addressW = engine::graphics::TextureAddressMode::Wrap;
            materialDesc.debugName = "Studio cube material";
            const auto materialResult = materialAsset_.Initialize(std::move(materialDesc));
            if (engine::assets::Failed(materialResult)) return Fail("MaterialAsset validation", GraphicsResult::InvalidArgument);
            constexpr std::uint32_t checkerboardPixels[16] = {
                0xFF30D8FFU, 0xFF3040D8U, 0xFF30D8FFU, 0xFF3040D8U,
                0xFF3040D8U, 0xFF30D8FFU, 0xFF3040D8U, 0xFF30D8FFU,
                0xFF30D8FFU, 0xFF3040D8U, 0xFF30D8FFU, 0xFF3040D8U,
                0xFF3040D8U, 0xFF30D8FFU, 0xFF3040D8U, 0xFF30D8FFU};
            engine::graphics::TextureDesc textureDesc;
            textureDesc.width = 4U;
            textureDesc.height = 4U;
            textureDesc.format = engine::graphics::Format::R8G8B8A8UNorm;
            textureDesc.usage = engine::graphics::ResourceUsage::Immutable;
            textureDesc.bindFlags = engine::graphics::TextureBindFlags::ShaderResource;
            engine::graphics::TextureSubresourceData textureData;
            textureData.data = reinterpret_cast<const std::byte*>(checkerboardPixels);
            textureData.dataSize = sizeof(checkerboardPixels);
            textureData.rowPitch = 4U * sizeof(std::uint32_t);
            textureData.slicePitch = sizeof(checkerboardPixels);
            GraphicsResult result = device_.CreateTexture(
                textureDesc, &textureData, 1U, checkerboardTexture_);
            if (engine::graphics::Failed(result))
                return Fail("checkerboard texture creation", result);
            r3dOutToLog("[Graphics][DX11] Checkerboard texture created\n");

            engine::graphics::SamplerDesc samplerDesc = materialAsset_.GetDesc().sampler;
            samplerDesc.debugName = "Studio DX11 Checkerboard Sampler";
            result = device_.CreateSampler(samplerDesc, sampler_);
            if (engine::graphics::Failed(result))
                return Fail("checkerboard sampler creation", result);
            r3dOutToLog("[Graphics][DX11] Checkerboard sampler created\n");

            engine::graphics::BufferDesc constantBufferDesc;
            constantBufferDesc.byteSize = sizeof(StudioFrameConstants);
            constantBufferDesc.usage = engine::graphics::ResourceUsage::Dynamic;
            constantBufferDesc.bindFlags = engine::graphics::BufferBindFlags::Constant;
            constantBufferDesc.cpuAccess = engine::graphics::CpuAccessFlags::Write;
            result = device_.CreateBuffer(
                constantBufferDesc, nullptr, constantBuffer_);
            if (engine::graphics::Failed(result))
                return Fail("frame constant buffer creation", result);
            r3dOutToLog("[Graphics][DX11] Dynamic frame constant buffer created\n");
            constantBufferDesc.byteSize = sizeof(StudioMaterialConstants);
            result = device_.CreateBuffer(constantBufferDesc, nullptr, materialConstantBuffer_);
            if (engine::graphics::Failed(result)) return Fail("material constant buffer creation", result);
            const auto& md = materialAsset_.GetDesc();
            StudioMaterialConstants constants{};
            std::memcpy(constants.baseColor, md.baseColorFactor.data(), sizeof(constants.baseColor));
            std::memcpy(constants.emissiveMetallic, md.emissiveFactor.data(), sizeof(float)*3U);
            constants.emissiveMetallic[3]=md.metallicFactor; constants.roughnessAlpha[0]=md.roughnessFactor; constants.roughnessAlpha[1]=md.alphaCutoff;
            result=context_->UpdateBuffer(materialConstantBuffer_,&constants,sizeof(constants));
            if(engine::graphics::Failed(result))return Fail("material constant buffer update",result);
            r3dOutToLog("[Graphics][DX11] MaterialAsset and material resources created\n");
            return true;
        }

        bool UpdateFrameConstants(const double deltaSeconds) noexcept
        {
            if (!constantBuffer_.IsValid() || width_ == 0U || height_ == 0U)
                return FailFrame("frame constant resource validation",
                    GraphicsResult::InvalidState);
            elapsedSeconds_ += static_cast<float>(deltaSeconds);
            if (elapsedSeconds_ > 1000.0F)
                elapsedSeconds_ = std::fmod(elapsedSeconds_, 1000.0F);
            const float aspect = static_cast<float>(width_) /
                static_cast<float>(height_);
            const engine::math::Matrix4 world=engine::math::Matrix4::CreateRotationY(elapsedSeconds_*0.55F)*engine::math::Matrix4::CreateRotationX(elapsedSeconds_*0.31F);
            const engine::math::Matrix4 view=engine::math::Matrix4::CreateTranslation({0.0F,0.0F,4.2F});
            const float yScale=1.0F/std::tan(0.55F),xScale=yScale/aspect,zn=0.1F,zf=100.0F;
            const engine::math::Matrix4 projection{xScale,0,0,0,0,yScale,0,0,0,0,zf/(zf-zn),1,0,0,-zn*zf/(zf-zn),0};
            const engine::math::Matrix4 wvp=world*view*projection;
            StudioFrameConstants constants{};
            std::memcpy(constants.worldViewProjection,wvp.Data(),sizeof(constants.worldViewProjection));
            std::memcpy(constants.world,world.Data(),sizeof(constants.world)); constants.frameData[0]=elapsedSeconds_;
            const GraphicsResult result = context_->UpdateBuffer(
                constantBuffer_, &constants, sizeof(constants));
            if (engine::graphics::Failed(result))
                return FailFrame("frame constant buffer update", result);
            return true;
        }

        void DestroyBindingResources() noexcept
        {
            const bool hadResources = sampler_.IsValid() || checkerboardTexture_.IsValid() || constantBuffer_.IsValid() || materialConstantBuffer_.IsValid();
            if(materialConstantBuffer_.IsValid()){(void)device_.DestroyBuffer(materialConstantBuffer_);materialConstantBuffer_={};}
            if (sampler_.IsValid())
            {
                const GraphicsResult result = device_.DestroySampler(sampler_);
                if (engine::graphics::Failed(result))
                    r3dOutToLog("[Graphics][DX11] Sampler destruction failed: %s\n",
                        engine::graphics::ToString(result));
                sampler_ = {};
            }
            if (checkerboardTexture_.IsValid())
            {
                const GraphicsResult result =
                    device_.DestroyTexture(checkerboardTexture_);
                if (engine::graphics::Failed(result))
                    r3dOutToLog("[Graphics][DX11] Checkerboard destruction failed: %s\n",
                        engine::graphics::ToString(result));
                checkerboardTexture_ = {};
            }
            if (constantBuffer_.IsValid())
            {
                const GraphicsResult result = device_.DestroyBuffer(constantBuffer_);
                if (engine::graphics::Failed(result))
                    r3dOutToLog("[Graphics][DX11] Constant buffer destruction failed: %s\n",
                        engine::graphics::ToString(result));
                constantBuffer_ = {};
            }
            if (hadResources)
                r3dOutToLog("[Graphics][DX11] Material resources released\n");
        }

        bool DrawGeometry() noexcept
        {
            if (!pipeline_.IsValid() || !gpuMesh_.IsValid() || !materialAsset_.IsValid())
            {
                return FailFrame("quad resource validation",
                    GraphicsResult::InvalidState);
            }

            GraphicsResult result = context_->SetGraphicsPipeline(pipeline_);
            if (engine::graphics::Failed(result))
                return FailFrame("quad pipeline bind", result);
            engine::graphics::VertexBufferBinding vertexBinding;
            vertexBinding.buffer = gpuMesh_.GetVertexBuffer();
            vertexBinding.stride = gpuMesh_.GetVertexStride();
            result = context_->SetVertexBuffers(0U, &vertexBinding, 1U);
            if (engine::graphics::Failed(result))
                return FailFrame("quad vertex buffer bind", result);
            engine::graphics::IndexBufferBinding indexBinding;
            indexBinding.buffer = gpuMesh_.GetIndexBuffer();
            result = context_->SetIndexBuffer(indexBinding);
            if (engine::graphics::Failed(result))
                return FailFrame("quad index buffer bind", result);
            result = context_->SetConstantBuffers(
                engine::graphics::ShaderStage::Vertex, 0U, &constantBuffer_, 1U);
            if (engine::graphics::Failed(result))
                return FailFrame("quad constant buffer bind", result);
            result = context_->SetShaderResources(
                engine::graphics::ShaderStage::Pixel, 0U, &checkerboardTexture_, 1U);
            if (engine::graphics::Failed(result))
                return FailFrame("quad texture bind", result);
            result = context_->SetSamplers(
                engine::graphics::ShaderStage::Pixel, 0U, &sampler_, 1U);
            if (engine::graphics::Failed(result))
                return FailFrame("quad sampler bind", result);
            result = context_->SetConstantBuffers(engine::graphics::ShaderStage::Pixel,0U,&materialConstantBuffer_,1U);
            if(engine::graphics::Failed(result))return FailFrame("material constant buffer bind",result);
            for(std::size_t i=0;i<gpuMesh_.GetSubmeshCount();++i){const auto* s=gpuMesh_.GetSubmesh(i);if(!s||s->materialSlot!=0U)return FailFrame("submesh material slot",GraphicsResult::InvalidState);result=context_->DrawIndexed(s->indexCount,s->firstIndex,s->baseVertex);if(engine::graphics::Failed(result))return FailFrame("submesh DrawIndexed",result);}
            if (!firstDrawLogged_)
            {
                r3dOutToLog("[Graphics][DX11] First asset-driven submesh draw completed\n");
                firstDrawLogged_ = true;
            }
            return true;
        }

        void DestroyGeometryResources() noexcept
        {
            const bool hadResources = vertexShader_.IsValid() ||
                pixelShader_.IsValid() || inputLayout_.IsValid() ||
                pipeline_.IsValid() || gpuMesh_.IsValid();
            if(gpuMesh_.IsValid()){const auto result=gpuMesh_.Release(device_);if(engine::graphics::Failed(result))r3dOutToLog("[Graphics][DX11] GpuMesh release failed: %s\n",engine::graphics::ToString(result));else r3dOutToLog("[Graphics][DX11] GpuMesh released\n");}
            if (pipeline_.IsValid())
            {
                const GraphicsResult result = device_.DestroyGraphicsPipeline(pipeline_);
                if (engine::graphics::Failed(result))
                    r3dOutToLog("[Graphics][DX11] Pipeline destruction failed: %s\n",
                        engine::graphics::ToString(result));
                pipeline_ = {};
            }
            if (inputLayout_.IsValid())
            {
                const GraphicsResult result = device_.DestroyInputLayout(inputLayout_);
                if (engine::graphics::Failed(result))
                    r3dOutToLog("[Graphics][DX11] Input layout destruction failed: %s\n",
                        engine::graphics::ToString(result));
                inputLayout_ = {};
            }
            if (pixelShader_.IsValid())
            {
                const GraphicsResult result = device_.DestroyShader(pixelShader_);
                if (engine::graphics::Failed(result))
                    r3dOutToLog("[Graphics][DX11] Pixel shader destruction failed: %s\n",
                        engine::graphics::ToString(result));
                pixelShader_ = {};
            }
            if (vertexShader_.IsValid())
            {
                const GraphicsResult result = device_.DestroyShader(vertexShader_);
                if (engine::graphics::Failed(result))
                    r3dOutToLog("[Graphics][DX11] Vertex shader destruction failed: %s\n",
                        engine::graphics::ToString(result));
                vertexShader_ = {};
            }
            if (hadResources)
                r3dOutToLog("[Graphics][DX11] Quad geometry resources destroyed\n");
        }

        bool ApplyResizableWindowStyle(HWND const windowHandle) noexcept
        {
            SetLastError(ERROR_SUCCESS);
            const LONG_PTR currentStyle =
                GetWindowLongPtr(windowHandle, GWL_STYLE);
            if (currentStyle == 0 && GetLastError() != ERROR_SUCCESS)
            {
                r3dOutToLog("[Graphics][DX11] GetWindowLongPtr failed: error=%lu\n",
                    static_cast<unsigned long>(GetLastError()));
                return false;
            }

            windowHandle_ = windowHandle;
            originalWindowStyle_ = currentStyle;
            const LONG_PTR requiredStyle = currentStyle |
                WS_THICKFRAME | WS_MAXIMIZEBOX;
            if (requiredStyle == currentStyle)
                return true;

            SetLastError(ERROR_SUCCESS);
            const LONG_PTR previousStyle = SetWindowLongPtr(
                windowHandle, GWL_STYLE, requiredStyle);
            if (previousStyle == 0 && GetLastError() != ERROR_SUCCESS)
            {
                r3dOutToLog("[Graphics][DX11] SetWindowLongPtr failed: error=%lu\n",
                    static_cast<unsigned long>(GetLastError()));
                windowHandle_ = nullptr;
                return false;
            }

            styleChanged_ = true;
            if (SetWindowPos(windowHandle, nullptr, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                        SWP_NOACTIVATE | SWP_FRAMECHANGED) == FALSE)
            {
                r3dOutToLog("[Graphics][DX11] Applying window frame failed: error=%lu\n",
                    static_cast<unsigned long>(GetLastError()));
                RestoreWindowStyle();
                return false;
            }
            return true;
        }

        void RestoreWindowStyle() noexcept
        {
            if (!styleChanged_ || windowHandle_ == nullptr)
            {
                windowHandle_ = nullptr;
                styleChanged_ = false;
                return;
            }

            SetLastError(ERROR_SUCCESS);
            const LONG_PTR previousStyle = SetWindowLongPtr(
                windowHandle_, GWL_STYLE, originalWindowStyle_);
            const DWORD styleError = GetLastError();
            if (previousStyle == 0 && styleError != ERROR_SUCCESS)
            {
                r3dOutToLog("[Graphics][DX11] Restoring window style failed: error=%lu\n",
                    static_cast<unsigned long>(styleError));
            }
            else if (SetWindowPos(windowHandle_, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                             SWP_NOACTIVATE | SWP_FRAMECHANGED) == FALSE)
            {
                r3dOutToLog("[Graphics][DX11] Restoring window frame failed: error=%lu\n",
                    static_cast<unsigned long>(GetLastError()));
            }
            windowHandle_ = nullptr;
            styleChanged_ = false;
        }

        bool CreateSizeDependentResources(const std::uint32_t width,
            const std::uint32_t height) noexcept
        {
            if (width == 0 || height == 0)
                return false;
            engine::graphics::TextureDesc depthDesc;
            depthDesc.width = width;
            depthDesc.height = height;
            depthDesc.format = engine::graphics::Format::D24UNormS8UInt;
            depthDesc.bindFlags = engine::graphics::TextureBindFlags::DepthStencil;
            GraphicsResult result = device_.CreateTexture(depthDesc, nullptr, 0, depth_);
            if (engine::graphics::Failed(result))
                return Fail("depth-buffer creation", result);

            engine::graphics::Viewport viewport;
            viewport.width = static_cast<float>(width);
            viewport.height = static_cast<float>(height);
            result = context_->SetViewport(viewport);
            if (engine::graphics::Failed(result))
                return Fail("viewport setup", result);
            engine::graphics::ScissorRect scissor;
            scissor.right = static_cast<std::int32_t>(width);
            scissor.bottom = static_cast<std::int32_t>(height);
            result = context_->SetScissorRect(scissor);
            if (engine::graphics::Failed(result))
                return Fail("scissor setup", result);
            return true;
        }

        bool Resize(const std::uint32_t width, const std::uint32_t height) noexcept
        {
            resizePending_ = false;
            if (width == 0 || height == 0 || (width == width_ && height == height_))
                return true;
            context_->UnbindRenderTargets();
            DestroyDepth();
            GraphicsResult result = swapChain_->Resize(width, height);
            if (engine::graphics::Failed(result))
                return FailFrame("swap-chain resize", result);
            if (!CreateSizeDependentResources(width, height))
                return false;
            width_ = width;
            height_ = height;
            r3dOutToLog("[Graphics][DX11] Studio shell resized: %ux%u\n", width_, height_);
            return true;
        }

        void DestroyDepth() noexcept
        {
            if (depth_.IsValid())
            {
                const GraphicsResult result = device_.DestroyTexture(depth_);
                if (engine::graphics::Failed(result))
                    r3dOutToLog("[Graphics][DX11] Depth destruction failed: %s\n",
                        engine::graphics::ToString(result));
                depth_ = {};
            }
        }

        bool Fail(const char* operation, const GraphicsResult result) noexcept
        {
            r3dOutToLog("[Graphics][DX11] Initialization failed at %s: %s\n",
                operation, engine::graphics::ToString(result));
            Shutdown();
            return false;
        }

        bool FailFrame(const char* operation, const GraphicsResult result) noexcept
        {
            r3dOutToLog("[Graphics][DX11] Frame failure at %s: %s\n",
                operation, engine::graphics::ToString(result));
            failureResult_ = result == GraphicsResult::DeviceLost
                ? StudioGraphicsShellResult::DeviceLost
                : result == GraphicsResult::DeviceRemoved
                    ? StudioGraphicsShellResult::DeviceRemoved
                    : StudioGraphicsShellResult::FrameFailed;
            return false;
        }

        engine::platform::Window window_;
        engine::graphics::d3d11::D3D11Device device_;
        engine::graphics::CommandContext* context_ = nullptr;
        std::unique_ptr<engine::graphics::SwapChain> swapChain_;
        engine::graphics::TextureHandle depth_;
        engine::graphics::ShaderHandle vertexShader_;
        engine::graphics::ShaderHandle pixelShader_;
        engine::graphics::InputLayoutHandle inputLayout_;
        engine::graphics::PipelineStateHandle pipeline_;
        engine::assets::MeshAsset meshAsset_;
        engine::assets::GpuMesh gpuMesh_;
        engine::assets::MaterialAsset materialAsset_;
        engine::graphics::BufferHandle constantBuffer_;
        engine::graphics::BufferHandle materialConstantBuffer_;
        engine::graphics::TextureHandle checkerboardTexture_;
        engine::graphics::SamplerHandle sampler_;
        std::uint32_t width_ = 0, height_ = 0;
        std::uint32_t pendingWidth_ = 0, pendingHeight_ = 0;
        bool initialized_ = false, minimized_ = false, resizePending_ = false;
        bool closeRequested_ = false, occluded_ = false, resetTimer_ = true;
        HWND windowHandle_ = nullptr;
        LONG_PTR originalWindowStyle_ = 0;
        bool styleChanged_ = false;
        bool firstDrawLogged_ = false;
        float elapsedSeconds_ = 0.0F;
        StudioGraphicsShellResult failureResult_ = StudioGraphicsShellResult::FrameFailed;
    };

    StudioDX11Shell* g_activeShell = nullptr;

    bool StudioDX11ShellMsgProc(UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (g_activeShell == nullptr)
            return false;
        if (message == WM_SIZE)
        {
            g_activeShell->OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
        }
        else if (message == WM_CLOSE || message == WM_DESTROY)
        {
            g_activeShell->RequestClose();
            return true;
        }
        return false;
    }
}

namespace studio
{
    bool WantsDX11Shell() noexcept
    {
        return HasCommandLineSwitch("-dx11shell") ||
            HasCommandLineSwitch("/dx11shell") ||
            HasCommandLineSwitch("-dx11shell-fail");
    }

    const char* ToString(const StudioGraphicsShellResult result) noexcept
    {
        switch (result)
        {
        case StudioGraphicsShellResult::NotRequested: return "NotRequested";
        case StudioGraphicsShellResult::Completed: return "Completed";
        case StudioGraphicsShellResult::InitializationFailed: return "InitializationFailed";
        case StudioGraphicsShellResult::RuntimeInitializationFailed: return "RuntimeInitializationFailed";
        case StudioGraphicsShellResult::FrameFailed: return "FrameFailed";
        case StudioGraphicsShellResult::DeviceLost: return "DeviceLost";
        case StudioGraphicsShellResult::DeviceRemoved: return "DeviceRemoved";
        default: return "Unknown";
        }
    }

    StudioGraphicsShellResult RunDX11Shell(
        const std::uintptr_t nativeWindow) noexcept
    {
        if (!WantsDX11Shell())
            return StudioGraphicsShellResult::NotRequested;

        StudioDX11Shell shell;
        if (!shell.Initialize(nativeWindow))
        {
            return StudioGraphicsShellResult::InitializationFailed;
        }

        if (!InitializeStudioRuntimeBridge(engine::runtime::RendererBackend::D3D11))
        {
            r3dOutToLog("[Graphics][DX11] Runtime initialization failed\n");
            shell.Shutdown();
            return StudioGraphicsShellResult::RuntimeInitializationFailed;
        }

        g_activeShell = &shell;
        RegisterMsgProc(StudioDX11ShellMsgProc);
        ShowWindow(reinterpret_cast<HWND>(nativeWindow), SW_SHOW);
        UpdateWindow(reinterpret_cast<HWND>(nativeWindow));

        bool frameSucceeded = true;
        bool quitRequested = false;
        engine::platform::Clock::Tick previousTick = engine::platform::Clock::Now();
        bool timerInitialized = previousTick != 0;
        while (!quitRequested && frameSucceeded)
        {
            if (shell.ShouldWaitForMessage())
            {
                const bool waitSucceeded =
                    engine::platform::MessagePump::WaitForMessage();
                if (!waitSucceeded)
                    r3dOutToLog("[Graphics][DX11] Message wait failed\n");
            }
            const engine::platform::MessagePumpResult messages =
                engine::platform::MessagePump::ProcessPendingMessages();
            quitRequested = messages.quitRequested || shell.IsCloseRequested();
            if (!quitRequested)
            {
                const engine::platform::Clock::Tick currentTick =
                    engine::platform::Clock::Now();
                double deltaSeconds = 0.0;
                if (shell.ConsumeTimerReset())
                    timerInitialized = false;
                if (timerInitialized && currentTick != 0)
                    deltaSeconds = engine::platform::Clock::ElapsedSeconds(
                        previousTick, currentTick);
                if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0)
                    deltaSeconds = 0.0;
                deltaSeconds = (std::min)(deltaSeconds, 0.25);
                previousTick = currentTick;
                timerInitialized = currentTick != 0;
                engine::runtime::Engine* const runtime = TryGetRuntimeEngine();
                const bool beganFrame = runtime != nullptr &&
                    runtime->BeginFrame(deltaSeconds);
                frameSucceeded = beganFrame && shell.RenderFrame(deltaSeconds);
                if (!beganFrame)
                    r3dOutToLog("[Graphics][DX11] Runtime BeginFrame failed\n");
                if (beganFrame && !runtime->EndFrame())
                {
                    r3dOutToLog("[Graphics][DX11] Runtime EndFrame failed\n");
                    frameSucceeded = false;
                }
            }
        }

        UnregisterMsgProc(StudioDX11ShellMsgProc);
        g_activeShell = nullptr;
        ShutdownStudioRuntimeBridge();
        const StudioGraphicsShellResult result = quitRequested
            ? StudioGraphicsShellResult::Completed
            : shell.GetFailureResult();
        if (result == StudioGraphicsShellResult::Completed)
            r3dOutToLog("[Graphics][DX11] Normal user close completed\n");
        shell.Shutdown();
        r3dOutToLog("[Graphics][DX11] Final shell result: %s\n", ToString(result));
        r3dOutToLog("[Graphics][DX11] Shutdown completed\n");
        return result;
    }
}
