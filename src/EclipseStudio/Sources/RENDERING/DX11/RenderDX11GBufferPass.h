#pragma once

#include "RENDERING/DX11/RenderDX11Resources.h"

class r3dDX11CommonStates;
class r3dDX11DrawContext;
class r3dDX11GBufferResources;
class r3dDX11InputLayout;
class r3dDX11IndexBuffer;
class r3dDX11PixelShader;
class r3dDX11ShaderLibrary;
class r3dDX11VertexBuffer;
class r3dDX11VertexShader;
struct ID3D11Device;

struct r3dDX11MeshConstants
{
	float WorldViewProj[16];
	float World[16];
};

class r3dDX11GBufferPass final
{
public:
	r3dDX11GBufferPass();
	~r3dDX11GBufferPass();

	bool Init(ID3D11Device* device, r3dDX11DrawContext* drawContext, r3dDX11ShaderLibrary* shaderLibrary, r3dDX11CommonStates* commonStates);
	void Shutdown();

	bool Begin(r3dDX11GBufferResources& gbuffer);
	void End(r3dDX11GBufferResources& gbuffer);
	bool SetMeshConstants(const r3dDX11MeshConstants& constants);
	void DrawMesh(r3dDX11VertexBuffer& vertexBuffer, r3dDX11IndexBuffer& indexBuffer, unsigned int indexCount, unsigned int startIndex = 0, int baseVertex = 0);

	bool IsInitialized() const;

private:
	bool CreateShadersAndLayout(ID3D11Device* device);

private:
	r3dDX11DrawContext* DrawContext = nullptr;
	r3dDX11ShaderLibrary* ShaderLibrary = nullptr;
	r3dDX11CommonStates* CommonStates = nullptr;
	r3dDX11VertexShader* FillVS = nullptr;
	r3dDX11PixelShader* FillPS = nullptr;
	r3dDX11InputLayout* MeshLayout = nullptr;
	r3dDX11ConstantBuffer MeshConstants;
	bool bInitialized = false;
};
