#include "Graphics/GraphicsBackend.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/InputLayout.h"
#include "Graphics/PipelineState.h"
#include "Graphics/RenderDevice.h"
#include "Graphics/Sampler.h"
#include "Graphics/Shader.h"
#include "Graphics/SwapChain.h"
#include "Graphics/Texture.h"
#include "GraphicsDX11/D3D11Context.h"
#include "GraphicsDX11/D3D11Device.h"
#include "Platform/Window.h"

#include <d3dcompiler.h>
#include <windows.h>

#include <cstdio>
#include <cstring>
#include <memory>

namespace
{
    class Blob final
    {
    public:
        Blob() noexcept = default;

        ~Blob() noexcept
        {
            Reset();
        }

        Blob(const Blob&) = delete;
        Blob& operator=(const Blob&) = delete;

        [[nodiscard]] ID3DBlob* Get() const noexcept
        {
            return value_;
        }

        [[nodiscard]] ID3DBlob** Put() noexcept
        {
            Reset();
            return &value_;
        }

        void Reset() noexcept
        {
            if (value_ != nullptr)
            {
                value_->Release();
                value_ = nullptr;
            }
        }

    private:
        ID3DBlob* value_ = nullptr;
    };

    [[nodiscard]] bool Check(
        const bool condition,
        const char* const message) noexcept
    {
        if (condition)
        {
            return true;
        }

        std::fprintf(stderr, "FAILED: %s\n", message);
        return false;
    }

    [[nodiscard]] bool CompileShader(
        const char* const source,
        const char* const entryPoint,
        const char* const target,
        Blob& outBytecode) noexcept
    {
        Blob errors;

        const HRESULT result = D3DCompile(
            source,
            std::strlen(source),
            "LTS.Graphics.DX11.Tests.hlsl",
            nullptr,
            nullptr,
            entryPoint,
            target,
            D3DCOMPILE_ENABLE_STRICTNESS |
                D3DCOMPILE_WARNINGS_ARE_ERRORS,
            0U,
            outBytecode.Put(),
            errors.Put());

        if (SUCCEEDED(result))
        {
            return true;
        }

        if (errors.Get() != nullptr)
        {
            std::fprintf(
                stderr,
                "%.*s\n",
                static_cast<int>(errors.Get()->GetBufferSize()),
                static_cast<const char*>(
                    errors.Get()->GetBufferPointer()));
        }

        return false;
    }
}

int main()
{
    using namespace engine::graphics;
    using engine::graphics::d3d11::D3D11Context;
    using engine::graphics::d3d11::D3D11Device;

    constexpr const char* shaderSource = R"hlsl(
cbuffer FrameConstants : register(b0)
{
    row_major float4x4 transform;
};

struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(float4(input.position, 1.0f), transform);
    output.color = input.color;
    output.texcoord = input.texcoord;
    return output;
}

Texture2D sourceTexture : register(t0);
SamplerState sourceSampler : register(s0);

float4 PSMain(VertexOutput input) : SV_Target0
{
    return sourceTexture.Sample(sourceSampler, input.texcoord) * input.color;
}
)hlsl";

    Blob vertexBytecode;
    Blob pixelBytecode;

    if (!Check(
            CompileShader(
                shaderSource,
                "VSMain",
                "vs_4_0",
                vertexBytecode),
            "compile vertex shader"))
    {
        return 1;
    }

    if (!Check(
            CompileShader(
                shaderSource,
                "PSMain",
                "ps_4_0",
                pixelBytecode),
            "compile pixel shader"))
    {
        return 1;
    }

    D3D11Device device;
    RenderDeviceDesc deviceDesc;
    deviceDesc.backend = GraphicsBackend::D3D11;
    deviceDesc.enableValidation = false;
    deviceDesc.enableDebugMarkers = true;

    if (!Check(
            Succeeded(device.Initialize(deviceDesc)),
            "initialize D3D11 device"))
    {
        return 1;
    }

    ShaderDesc vertexShaderDesc;
    vertexShaderDesc.stage = ShaderStage::Vertex;
    vertexShaderDesc.bytecode.data =
        vertexBytecode.Get()->GetBufferPointer();
    vertexShaderDesc.bytecode.size =
        vertexBytecode.Get()->GetBufferSize();
    vertexShaderDesc.debugName = "LTS DX11 Test VS";

    ShaderHandle vertexShader;

    if (!Check(
            Succeeded(
                device.CreateShader(
                    vertexShaderDesc,
                    vertexShader)),
            "create vertex shader"))
    {
        return 1;
    }

    ShaderDesc pixelShaderDesc;
    pixelShaderDesc.stage = ShaderStage::Pixel;
    pixelShaderDesc.bytecode.data =
        pixelBytecode.Get()->GetBufferPointer();
    pixelShaderDesc.bytecode.size =
        pixelBytecode.Get()->GetBufferSize();
    pixelShaderDesc.debugName = "LTS DX11 Test PS";

    ShaderHandle pixelShader;

    if (!Check(
            Succeeded(
                device.CreateShader(
                    pixelShaderDesc,
                    pixelShader)),
            "create pixel shader"))
    {
        return 1;
    }

    if (!Check(
            device.GetNativeShader(vertexShader) != nullptr &&
                device.GetNativeShader(pixelShader) != nullptr &&
                device.GetShaderCount() == 2U,
            "native shader handles"))
    {
        return 1;
    }

    const VertexElementDesc elements[] = {
        {
            "POSITION",
            0U,
            Format::R32G32B32Float,
            0U,
            0U,
            VertexInputRate::PerVertex,
            0U
        },
        {
            "COLOR",
            0U,
            Format::R32G32B32A32Float,
            0U,
            12U,
            VertexInputRate::PerVertex,
            0U
        },
        {
            "TEXCOORD",
            0U,
            Format::R32G32Float,
            0U,
            28U,
            VertexInputRate::PerVertex,
            0U
        }
    };

    InputLayoutDesc inputLayoutDesc;
    inputLayoutDesc.vertexShader = vertexShader;
    inputLayoutDesc.elements = elements;
    inputLayoutDesc.elementCount =
        sizeof(elements) / sizeof(elements[0]);
    inputLayoutDesc.debugName = "LTS DX11 Test Input Layout";

    InputLayoutHandle inputLayout;

    if (!Check(
            Succeeded(
                device.CreateInputLayout(
                    inputLayoutDesc,
                    inputLayout)),
            "create input layout"))
    {
        return 1;
    }

    if (!Check(
            device.GetNativeInputLayout(inputLayout) != nullptr &&
                device.GetInputLayoutCount() == 1U,
            "native input layout"))
    {
        return 1;
    }

    InputLayoutDesc wrongStageLayout = inputLayoutDesc;
    wrongStageLayout.vertexShader = pixelShader;
    InputLayoutHandle rejectedLayout;

    if (!Check(
            device.CreateInputLayout(
                wrongStageLayout,
                rejectedLayout) == GraphicsResult::InvalidArgument &&
                !rejectedLayout.IsValid(),
            "reject pixel shader input layout"))
    {
        return 1;
    }

    GraphicsPipelineDesc pipelineDesc;
    pipelineDesc.vertexShader = vertexShader;
    pipelineDesc.pixelShader = pixelShader;
    pipelineDesc.inputLayout = inputLayout;
    pipelineDesc.topology = PrimitiveTopology::TriangleList;
    pipelineDesc.debugName = "LTS DX11 Test Pipeline";

    PipelineStateHandle pipeline;

    if (!Check(
            Succeeded(
                device.CreateGraphicsPipeline(
                    pipelineDesc,
                    pipeline)),
            "create graphics pipeline"))
    {
        return 1;
    }

    if (!Check(
            device.GetGraphicsPipelineCount() == 1U,
            "graphics pipeline count"))
    {
        return 1;
    }

    // The pipeline owns native COM references and must remain bindable after
    // the source handles have been removed from the registry.
    if (!Check(
            Succeeded(device.DestroyInputLayout(inputLayout)) &&
                Succeeded(device.DestroyShader(pixelShader)) &&
                Succeeded(device.DestroyShader(vertexShader)),
            "destroy source pipeline resources"))
    {
        return 1;
    }

    if (!Check(
            device.GetInputLayoutCount() == 0U &&
                device.GetShaderCount() == 0U,
            "source registry resources released"))
    {
        return 1;
    }

    D3D11Context* const context = device.GetImmediateContext();

    if (!Check(
            context != nullptr && context->IsValid(),
            "get immediate context"))
    {
        return 1;
    }

    if (!Check(
            Succeeded(context->BindGraphicsPipeline(pipeline)),
            "bind graphics pipeline"))
    {
        return 1;
    }

    context->UnbindGraphicsPipeline();

    if (!Check(
            Succeeded(device.DestroyGraphicsPipeline(pipeline)) &&
                device.GetGraphicsPipelineCount() == 0U,
            "destroy graphics pipeline"))
    {
        return 1;
    }

    TextureHandle invalidTexture;
    if (!Check(
            context->BindGraphicsPipeline(pipeline) ==
                GraphicsResult::NotFound,
            "reject stale pipeline handle"))
    {
        return 1;
    }

    HWND const testWindow = CreateWindowExW(
        0, L"STATIC", L"LTS DX11 Tests", WS_OVERLAPPED,
        0, 0, 320, 240, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!Check(testWindow != nullptr, "create test window"))
    {
        return 1;
    }

    SwapChainDesc swapChainDesc;
    swapChainDesc.window = engine::platform::NativeWindowHandle::FromValue(
        reinterpret_cast<std::uintptr_t>(testWindow));
    swapChainDesc.width = 320;
    swapChainDesc.height = 240;
    swapChainDesc.presentMode = PresentMode::Immediate;
    std::unique_ptr<SwapChain> swapChain;
    if (!Check(
            Succeeded(device.CreateSwapChain(swapChainDesc, swapChain)) &&
                swapChain != nullptr,
            "create test swap chain"))
    {
        DestroyWindow(testWindow);
        return 1;
    }

    TextureDesc depthDesc;
    depthDesc.width = 320;
    depthDesc.height = 240;
    depthDesc.format = Format::D24UNormS8UInt;
    depthDesc.bindFlags = TextureBindFlags::DepthStencil;
    TextureHandle depth;
    if (!Check(
            Succeeded(device.CreateTexture(depthDesc, nullptr, 0U, depth)),
            "create depth target"))
    {
        DestroyWindow(testWindow);
        return 1;
    }

    TextureDesc colorDesc;
    colorDesc.width = 320;
    colorDesc.height = 240;
    colorDesc.format = Format::R8G8B8A8UNorm;
    colorDesc.bindFlags = TextureBindFlags::RenderTarget;
    TextureHandle color;
    if (!Check(
            Succeeded(device.CreateTexture(colorDesc, nullptr, 0U, color)),
            "create non-depth target"))
    {
        DestroyWindow(testWindow);
        return 1;
    }

    if (!Check(
            context->SetSwapChainRenderTarget(*swapChain) ==
                GraphicsResult::Success &&
            context->SetSwapChainRenderTarget(*swapChain, TextureHandle{}) ==
                GraphicsResult::InvalidArgument &&
            context->SetSwapChainRenderTarget(*swapChain, color) ==
                GraphicsResult::InvalidArgument,
            "validate invalid and non-depth DSV handles"))
    {
        return 1;
    }

    const TextureHandle staleDepth = depth;
    if (!Check(
            Succeeded(context->SetSwapChainRenderTarget(*swapChain, depth)) &&
                Succeeded(context->ClearSwapChainColor(*swapChain, ClearColor{})) &&
                Succeeded(context->ClearDepthStencilTarget(
                    depth, ClearDepthStencilFlags::Depth |
                        ClearDepthStencilFlags::Stencil, 1.0F, 0U)),
            "bind and clear swap-chain RTV with valid DSV"))
    {
        return 1;
    }

    context->ClearState();
    if (!Check(
            Succeeded(context->SetSwapChainRenderTarget(*swapChain, depth)),
            "bind after ClearState"))
    {
        return 1;
    }
    context->UnbindRenderTargets();
    if (!Check(
            Succeeded(device.DestroyTexture(depth)) &&
                context->SetSwapChainRenderTarget(*swapChain, staleDepth) ==
                    GraphicsResult::NotFound,
            "reject stale destroyed depth handle"))
    {
        return 1;
    }

    depthDesc.width = 400;
    depthDesc.height = 300;
    if (!Check(
            Succeeded(swapChain->Resize(400U, 300U)) &&
                Succeeded(device.CreateTexture(depthDesc, nullptr, 0U, depth)) &&
                Succeeded(context->SetSwapChainRenderTarget(*swapChain, depth)),
            "bind recreated depth after swap-chain resize"))
    {
        return 1;
    }

    ShaderHandle drawVertexShader;
    ShaderHandle drawPixelShader;
    InputLayoutHandle drawInputLayout;
    PipelineStateHandle drawPipeline;
    if (!Check(
            Succeeded(device.CreateShader(vertexShaderDesc, drawVertexShader)) &&
                Succeeded(device.CreateShader(pixelShaderDesc, drawPixelShader)),
            "create draw shaders"))
    {
        return 1;
    }
    inputLayoutDesc.vertexShader = drawVertexShader;
    if (!Check(
            Succeeded(device.CreateInputLayout(inputLayoutDesc, drawInputLayout)),
            "create draw input layout"))
    {
        return 1;
    }
    pipelineDesc.vertexShader = drawVertexShader;
    pipelineDesc.pixelShader = drawPixelShader;
    pipelineDesc.inputLayout = drawInputLayout;
    pipelineDesc.rasterizer.cullMode = CullMode::None;
    if (!Check(
            Succeeded(device.CreateGraphicsPipeline(pipelineDesc, drawPipeline)),
            "create draw pipeline"))
    {
        return 1;
    }

    constexpr float drawVertices[] = {
        0.0F, 0.5F, 0.5F, 1.0F, 0.0F, 0.0F, 1.0F, 0.5F, 0.0F,
        -0.5F, -0.5F, 0.5F, 0.0F, 1.0F, 0.0F, 1.0F, 0.0F, 1.0F,
        0.5F, -0.5F, 0.5F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F, 1.0F};
    constexpr std::uint16_t drawIndices[] = {0U, 1U, 2U};
    BufferDesc vertexBufferDesc;
    vertexBufferDesc.byteSize = sizeof(drawVertices);
    vertexBufferDesc.stride = 9U * sizeof(float);
    vertexBufferDesc.usage = ResourceUsage::Immutable;
    vertexBufferDesc.bindFlags = BufferBindFlags::Vertex;
    BufferInitialData vertexBufferData;
    vertexBufferData.data = reinterpret_cast<const std::byte*>(drawVertices);
    vertexBufferData.dataSize = sizeof(drawVertices);
    BufferHandle drawVertexBuffer;
    BufferDesc indexBufferDesc;
    indexBufferDesc.byteSize = sizeof(drawIndices);
    indexBufferDesc.stride = sizeof(std::uint16_t);
    indexBufferDesc.usage = ResourceUsage::Immutable;
    indexBufferDesc.bindFlags = BufferBindFlags::Index;
    indexBufferDesc.indexFormat = IndexFormat::UInt16;
    BufferInitialData indexBufferData;
    indexBufferData.data = reinterpret_cast<const std::byte*>(drawIndices);
    indexBufferData.dataSize = sizeof(drawIndices);
    BufferHandle drawIndexBuffer;
    if (!Check(
            Succeeded(device.CreateBuffer(
                vertexBufferDesc, &vertexBufferData, drawVertexBuffer)) &&
                Succeeded(device.CreateBuffer(
                    indexBufferDesc, &indexBufferData, drawIndexBuffer)),
            "create draw vertex and index buffers"))
    {
        return 1;
    }

    SamplerDesc invalidSamplerDesc;
    invalidSamplerDesc.maximumAnisotropy = 0U;
    SamplerHandle rejectedSampler;
    SamplerDesc samplerDesc;
    samplerDesc.filter = TextureFilter::Linear;
    samplerDesc.addressU = TextureAddressMode::Wrap;
    samplerDesc.addressV = TextureAddressMode::Wrap;
    samplerDesc.addressW = TextureAddressMode::Wrap;
    SamplerHandle drawSampler;
    if (!Check(
            device.CreateSampler(invalidSamplerDesc, rejectedSampler) ==
                GraphicsResult::InvalidArgument &&
                !rejectedSampler.IsValid() &&
                Succeeded(device.CreateSampler(samplerDesc, drawSampler)) &&
                device.GetSamplerCount() == 1U,
            "validate and create sampler"))
    {
        return 1;
    }

    constexpr std::uint32_t texturePixels[4] = {
        0xFFFFFFFFU, 0xFF0000FFU, 0xFF00FF00U, 0xFFFF0000U};
    TextureDesc shaderTextureDesc;
    shaderTextureDesc.width = 2U;
    shaderTextureDesc.height = 2U;
    shaderTextureDesc.format = Format::R8G8B8A8UNorm;
    shaderTextureDesc.usage = ResourceUsage::Immutable;
    shaderTextureDesc.bindFlags = TextureBindFlags::ShaderResource;
    TextureSubresourceData shaderTextureData;
    shaderTextureData.data = reinterpret_cast<const std::byte*>(texturePixels);
    shaderTextureData.dataSize = sizeof(texturePixels);
    shaderTextureData.rowPitch = 2U * sizeof(std::uint32_t);
    shaderTextureData.slicePitch = sizeof(texturePixels);
    TextureHandle shaderTexture;
    if (!Check(
            Succeeded(device.CreateTexture(
                shaderTextureDesc, &shaderTextureData, 1U, shaderTexture)),
            "create shader resource texture"))
    {
        return 1;
    }

    alignas(16) constexpr float identityConstants[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};
    BufferDesc wrongConstantDesc;
    wrongConstantDesc.byteSize = 20U;
    wrongConstantDesc.bindFlags = BufferBindFlags::Constant;
    BufferHandle rejectedConstant;
    BufferDesc dynamicConstantDesc;
    dynamicConstantDesc.byteSize = sizeof(identityConstants);
    dynamicConstantDesc.usage = ResourceUsage::Dynamic;
    dynamicConstantDesc.bindFlags = BufferBindFlags::Constant;
    dynamicConstantDesc.cpuAccess = CpuAccessFlags::Write;
    BufferHandle dynamicConstant;
    BufferDesc immutableConstantDesc;
    immutableConstantDesc.byteSize = sizeof(identityConstants);
    immutableConstantDesc.usage = ResourceUsage::Immutable;
    immutableConstantDesc.bindFlags = BufferBindFlags::Constant;
    BufferInitialData constantInitialData;
    constantInitialData.data = reinterpret_cast<const std::byte*>(identityConstants);
    constantInitialData.dataSize = sizeof(identityConstants);
    BufferHandle immutableConstant;
    if (!Check(
            device.CreateBuffer(wrongConstantDesc, nullptr, rejectedConstant) ==
                GraphicsResult::InvalidArgument &&
                Succeeded(device.CreateBuffer(
                    dynamicConstantDesc, nullptr, dynamicConstant)) &&
                Succeeded(device.CreateBuffer(
                    immutableConstantDesc, &constantInitialData,
                    immutableConstant)),
            "validate and create constant buffers"))
    {
        return 1;
    }

    if (!Check(
            Succeeded(context->UpdateBuffer(dynamicConstant,
                identityConstants, sizeof(identityConstants))) &&
                context->UpdateBuffer(dynamicConstant, nullptr,
                    sizeof(identityConstants)) == GraphicsResult::InvalidArgument &&
                context->UpdateBuffer(dynamicConstant, identityConstants,
                    sizeof(identityConstants) + 16U) ==
                    GraphicsResult::InvalidArgument &&
                context->UpdateBuffer(immutableConstant, identityConstants,
                    sizeof(identityConstants)) == GraphicsResult::InvalidArgument,
            "validate dynamic constant buffer updates"))
    {
        return 1;
    }

    if (!Check(
            context->SetShaderResources(
                ShaderStage::Pixel, 0U, nullptr, 0U) == GraphicsResult::Success &&
                context->SetShaderResources(
                    ShaderStage::Pixel, 0U, nullptr, 1U) ==
                    GraphicsResult::InvalidArgument &&
                context->SetShaderResources(
                    ShaderStage::Pixel, 0U, &invalidTexture, 1U) ==
                    GraphicsResult::InvalidArgument &&
                context->SetShaderResources(
                    ShaderStage::Pixel, 0U, &color, 1U) ==
                    GraphicsResult::InvalidArgument &&
                context->SetShaderResources(
                    ShaderStage::Unknown, 0U, &shaderTexture, 1U) ==
                    GraphicsResult::Unsupported &&
                context->SetShaderResources(
                    ShaderStage::Pixel,
                    static_cast<std::uint32_t>(MaxShaderResourceSlots),
                    &shaderTexture, 1U) == GraphicsResult::InvalidArgument,
            "validate shader resource bindings"))
    {
        return 1;
    }

    SamplerHandle invalidSampler;
    BufferHandle invalidBuffer;
    if (!Check(
            context->UpdateBuffer(invalidBuffer, identityConstants,
                sizeof(identityConstants)) == GraphicsResult::InvalidArgument &&
            context->SetSamplers(
                ShaderStage::Pixel, 0U, &invalidSampler, 1U) ==
                    GraphicsResult::InvalidArgument &&
                context->SetConstantBuffers(
                    ShaderStage::Vertex, 0U, &invalidBuffer, 1U) ==
                    GraphicsResult::InvalidArgument &&
                context->SetConstantBuffers(
                    ShaderStage::Vertex, 0U, &drawVertexBuffer, 1U) ==
                    GraphicsResult::InvalidArgument &&
                context->SetSamplers(
                    ShaderStage::Unknown, 0U, &drawSampler, 1U) ==
                    GraphicsResult::Unsupported &&
                context->SetSamplers(
                    ShaderStage::Pixel,
                    static_cast<std::uint32_t>(MaxSamplerSlots),
                    &drawSampler, 1U) == GraphicsResult::InvalidArgument &&
                context->SetConstantBuffers(
                    ShaderStage::Unknown, 0U, &dynamicConstant, 1U) ==
                    GraphicsResult::Unsupported &&
                context->SetConstantBuffers(
                    ShaderStage::Vertex,
                    static_cast<std::uint32_t>(MaxConstantBufferSlots),
                    &dynamicConstant, 1U) == GraphicsResult::InvalidArgument,
            "reject invalid sampler and constant buffer bindings"))
    {
        return 1;
    }

    VertexBufferBinding vertexBinding;
    vertexBinding.buffer = drawVertexBuffer;
    vertexBinding.stride = vertexBufferDesc.stride;
    IndexBufferBinding indexBinding;
    indexBinding.buffer = drawIndexBuffer;
    PresentStatus presentStatus = PresentStatus::Failed;
    if (!Check(
            Succeeded(context->SetSwapChainRenderTarget(*swapChain, depth)) &&
                Succeeded(context->SetGraphicsPipeline(drawPipeline)) &&
                Succeeded(context->SetVertexBuffers(0U, &vertexBinding, 1U)) &&
                Succeeded(context->SetIndexBuffer(indexBinding)) &&
                Succeeded(context->SetConstantBuffers(
                    ShaderStage::Vertex, 0U, &dynamicConstant, 1U)) &&
                Succeeded(context->SetConstantBuffers(
                    ShaderStage::Pixel, 0U, &dynamicConstant, 1U)) &&
                Succeeded(context->SetShaderResources(
                    ShaderStage::Pixel, 0U, &shaderTexture, 1U)) &&
                Succeeded(context->SetSamplers(
                    ShaderStage::Pixel, 0U, &drawSampler, 1U)) &&
                Succeeded(context->DrawIndexed(3U, 0U, 0)) &&
                Succeeded(swapChain->Present(presentStatus)),
            "complete neutral indexed draw path"))
    {
        return 1;
    }
    if (!Check(
            Succeeded(context->UnbindConstantBuffers(
                ShaderStage::Vertex, 0U, 1U)) &&
                Succeeded(context->UnbindConstantBuffers(
                    ShaderStage::Pixel, 0U, 1U)) &&
                Succeeded(context->UnbindShaderResources(
                    ShaderStage::Pixel, 0U, 1U)) &&
                Succeeded(context->UnbindSamplers(
                    ShaderStage::Pixel, 0U, 1U)),
            "unbind draw resources"))
    {
        return 1;
    }
    context->Flush();
    context->ClearState();

    const SamplerHandle staleSampler = drawSampler;
    const TextureHandle staleShaderTexture = shaderTexture;
    const BufferHandle staleConstant = dynamicConstant;
    if (!Check(
            Succeeded(device.DestroySampler(drawSampler)) &&
                Succeeded(device.DestroyTexture(shaderTexture)) &&
                Succeeded(device.DestroyBuffer(dynamicConstant)) &&
                Succeeded(device.DestroyBuffer(immutableConstant)) &&
                device.GetSamplerCount() == 0U &&
                device.DestroySampler(staleSampler) == GraphicsResult::NotFound &&
                context->SetSamplers(
                    ShaderStage::Pixel, 0U, &staleSampler, 1U) ==
                    GraphicsResult::NotFound &&
                context->SetShaderResources(
                    ShaderStage::Pixel, 0U, &staleShaderTexture, 1U) ==
                    GraphicsResult::NotFound &&
                context->SetConstantBuffers(
                    ShaderStage::Vertex, 0U, &staleConstant, 1U) ==
                    GraphicsResult::NotFound,
            "destroy bindings and reject stale handles"))
    {
        return 1;
    }

    const BufferHandle staleVertexBuffer = drawVertexBuffer;
    const PipelineStateHandle staleDrawPipeline = drawPipeline;
    if (!Check(
            Succeeded(device.DestroyBuffer(drawIndexBuffer)) &&
                Succeeded(device.DestroyBuffer(drawVertexBuffer)) &&
                Succeeded(device.DestroyGraphicsPipeline(drawPipeline)) &&
                Succeeded(device.DestroyInputLayout(drawInputLayout)) &&
                Succeeded(device.DestroyShader(drawPixelShader)) &&
                Succeeded(device.DestroyShader(drawVertexShader)),
            "destroy draw resources in ownership order"))
    {
        return 1;
    }
    vertexBinding.buffer = staleVertexBuffer;
    if (!Check(
            context->SetVertexBuffers(0U, &vertexBinding, 1U) ==
                GraphicsResult::NotFound &&
                context->SetGraphicsPipeline(staleDrawPipeline) ==
                    GraphicsResult::NotFound,
            "reject stale draw buffer and pipeline handles"))
    {
        return 1;
    }

    context->UnbindRenderTargets();
    if (!Check(
            Succeeded(device.DestroyTexture(depth)) &&
                Succeeded(device.DestroyTexture(color)),
            "destroy test render targets"))
    {
        return 1;
    }
    swapChain.reset();
    DestroyWindow(testWindow);

    device.Shutdown();
    device.Shutdown();

    if (!Check(
            device.GetState() == DeviceState::Stopped,
            "repeated shutdown D3D11 device"))
    {
        return 1;
    }

    std::puts("LTS.Graphics.DX11 resource binding tests passed");
    return 0;
}
