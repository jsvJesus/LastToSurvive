#include "Graphics/GraphicsBackend.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/InputLayout.h"
#include "Graphics/PipelineState.h"
#include "Graphics/RenderDevice.h"
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
struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR0;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    output.position = float4(input.position, 1.0f);
    output.color = input.color;
    return output;
}

float4 PSMain(VertexOutput input) : SV_Target0
{
    return input.color;
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
        0.0F, 0.5F, 0.5F, 1.0F, 0.0F, 0.0F, 1.0F,
        -0.5F, -0.5F, 0.5F, 0.0F, 1.0F, 0.0F, 1.0F,
        0.5F, -0.5F, 0.5F, 0.0F, 0.0F, 1.0F, 1.0F};
    constexpr std::uint16_t drawIndices[] = {0U, 1U, 2U};
    BufferDesc vertexBufferDesc;
    vertexBufferDesc.byteSize = sizeof(drawVertices);
    vertexBufferDesc.stride = 7U * sizeof(float);
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
                Succeeded(context->DrawIndexed(3U, 0U, 0)) &&
                Succeeded(swapChain->Present(presentStatus)),
            "complete neutral indexed draw path"))
    {
        return 1;
    }
    context->Flush();
    context->ClearState();

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

    std::puts("LTS.Graphics.DX11 indexed draw tests passed");
    return 0;
}
