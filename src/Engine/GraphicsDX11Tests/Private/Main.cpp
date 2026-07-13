#include "Graphics/GraphicsBackend.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/InputLayout.h"
#include "Graphics/PipelineState.h"
#include "Graphics/RenderDevice.h"
#include "Graphics/Shader.h"
#include "GraphicsDX11/D3D11Context.h"
#include "GraphicsDX11/D3D11Device.h"

#include <d3dcompiler.h>

#include <cstdio>
#include <cstring>

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

    device.Shutdown();

    if (!Check(
            device.GetState() == DeviceState::Stopped,
            "shutdown D3D11 device"))
    {
        return 1;
    }

    std::puts("LTS.Graphics.DX11 shader pipeline tests passed");
    return 0;
}
