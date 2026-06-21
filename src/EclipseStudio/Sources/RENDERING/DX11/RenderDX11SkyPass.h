#pragma once

#include "RENDERING/DX11/RenderDX11Resources.h"
#include "RENDERING/DX11/RenderDX11Platform.h"

class r3dCamera;
class r3dDX11CommonStates;
class r3dDX11DrawContext;
class r3dDX11GBufferResources;
class r3dDX11InputLayout;
class r3dDX11PixelShader;
class r3dDX11RenderTarget;
class r3dDX11ShaderLibrary;
class r3dDX11VertexBuffer;
class r3dDX11VertexShader;

class r3dDX11SkyPass final
{
public:
    r3dDX11SkyPass();
    ~r3dDX11SkyPass();

    bool Init(
        ID3D11Device* device,
        r3dDX11DrawContext* drawContext,
        r3dDX11ShaderLibrary* shaderLibrary,
        r3dDX11CommonStates* commonStates
    );

    void Shutdown();

    bool Render(
        const r3dCamera& camera,
        r3dDX11GBufferResources& gbuffer,
        r3dDX11RenderTarget& sceneColor
    );

    bool IsInitialized() const;

private:
    bool CreateShadersAndLayout(ID3D11Device* device);
    bool CreateGeometry(ID3D11Device* device);
    bool CreateConstantBuffers(ID3D11Device* device);

private:
    r3dDX11DrawContext* DrawContext = nullptr;
    r3dDX11ShaderLibrary* ShaderLibrary = nullptr;
    r3dDX11CommonStates* CommonStates = nullptr;

    r3dDX11VertexShader* SkyVS = nullptr;
    r3dDX11PixelShader* SkyPS = nullptr;

    r3dDX11InputLayout* InputLayout = nullptr;
    r3dDX11VertexBuffer* VertexBuffer = nullptr;

    r3dDX11ConstantBuffer SkyConstants;

    bool bInitialized = false;
};