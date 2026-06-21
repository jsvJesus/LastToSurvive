#pragma once

#include "RENDERING/DX11/RenderDX11Resources.h"
#include "RENDERING/DX11/RenderDX11Platform.h"

class r3dCamera;
class r3dDX11CommonStates;
class r3dDX11DrawContext;
class r3dDX11GBufferResources;
class r3dDX11InputLayout;
class r3dDX11PixelShader;
class r3dDX11ShaderLibrary;
class r3dDX11VertexShader;
class r3dDX11TextureLibrary;
struct r3dDX11WorldRenderStats;
struct ID3D11Device;
struct ID3D11DepthStencilView;

class r3dDX11TerrainPass final
{
public:
	r3dDX11TerrainPass();
	~r3dDX11TerrainPass();

	bool Init(
		ID3D11Device* device,
		r3dDX11DrawContext* drawContext,
		r3dDX11ShaderLibrary* shaderLibrary,
		r3dDX11CommonStates* commonStates,
		r3dDX11TextureLibrary* textureLibrary
	);

	void Shutdown();

	bool RenderGBuffer(
		const r3dCamera& camera,
		r3dDX11GBufferResources& gbuffer,
		const D3DXMATRIX& viewProj,
		r3dDX11WorldRenderStats* stats
	);

	bool RenderDepth(
		const D3DXMATRIX& viewProj,
		ID3D11DepthStencilView* depthStencil,
		int width,
		int height,
		r3dDX11WorldRenderStats* stats
	);

	bool RenderShadow(
		const D3DXMATRIX& viewProj,
		ID3D11DepthStencilView* depthStencil,
		int width,
		int height,
		r3dDX11WorldRenderStats* stats
	);

	bool IsInitialized() const;

private:
	bool CreateShadersAndLayout(ID3D11Device* device);
	bool CreateConstantBuffers(ID3D11Device* device);
	bool BuildTerrainMesh(ID3D11Device* device);
	bool EnsureTerrainMesh(ID3D11Device* device);
	bool DrawTerrain(const D3DXMATRIX& viewProj, bool gbufferMode, r3dDX11WorldRenderStats* stats);

private:
	ID3D11Device* Device = nullptr;
	r3dDX11DrawContext* DrawContext = nullptr;
	r3dDX11ShaderLibrary* ShaderLibrary = nullptr;
	r3dDX11CommonStates* CommonStates = nullptr;
	r3dDX11TextureLibrary* TextureLibrary = nullptr;

	r3dDX11VertexShader* TerrainVS = nullptr;
	r3dDX11PixelShader* TerrainGBufferPS = nullptr;

	r3dDX11InputLayout* InputLayout = nullptr;

	r3dDX11VertexBuffer VertexBuffer;
	r3dDX11IndexBuffer IndexBuffer;

	r3dDX11ConstantBuffer TerrainConstants;

	unsigned int VertexCount = 0;
	unsigned int IndexCount = 0;
	unsigned int TriangleCount = 0;

	int CachedCellCountX = 0;
	int CachedCellCountZ = 0;
	float CachedCellSize = 0.0f;
	float CachedXSize = 0.0f;
	float CachedZSize = 0.0f;

	bool bInitialized = false;
};