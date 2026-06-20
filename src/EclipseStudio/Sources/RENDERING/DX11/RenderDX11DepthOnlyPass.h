#pragma once

#include "RENDERING/DX11/RenderDX11GBufferPass.h"

class r3dDX11CommonStates;
class r3dDX11DrawContext;
class r3dDX11GBufferResources;
class r3dDX11InputLayout;
class r3dDX11IndexBuffer;
class r3dDX11ShaderLibrary;
class r3dDX11MaterialTextures;
class r3dDX11PixelShader;
class r3dDX11VertexBuffer;
class r3dDX11VertexShader;
class r3dSkeleton;
struct ID3D11Device;
struct ID3D11DepthStencilView;

class r3dDX11DepthOnlyPass final
{
public:
	r3dDX11DepthOnlyPass();
	~r3dDX11DepthOnlyPass();

	bool Init(ID3D11Device* device, r3dDX11DrawContext* drawContext, r3dDX11ShaderLibrary* shaderLibrary, r3dDX11CommonStates* commonStates);
	void Shutdown();

	bool Begin(r3dDX11GBufferResources& gbuffer, bool clearDepth = true);
	bool BeginDepthTarget(ID3D11DepthStencilView* depthStencilView, int width, int height, bool clearDepth = true);
	void End(r3dDX11GBufferResources& gbuffer);
	void EndDepthTarget();
	bool SetMeshConstants(const r3dDX11MeshConstants& constants);
	bool SetSkinningBones(const r3dSkeleton* skeleton);
	void SetSkinnedMeshMode(bool skinned);
	bool SetMaterial(const r3dDX11MaterialTextures& material);
	void DrawMesh(r3dDX11VertexBuffer& vertexBuffer, r3dDX11IndexBuffer& indexBuffer, unsigned int indexCount, unsigned int startIndex = 0, int baseVertex = 0);

	bool IsInitialized() const;

private:
	bool CreateShadersAndLayout(ID3D11Device* device);
	void ApplyShaders();

private:
	r3dDX11DrawContext* DrawContext = nullptr;
	r3dDX11ShaderLibrary* ShaderLibrary = nullptr;
	r3dDX11CommonStates* CommonStates = nullptr;
	r3dDX11VertexShader* DepthVS = nullptr;
	r3dDX11VertexShader* SkinDepthVS = nullptr;
	r3dDX11PixelShader* AlphaTestPS = nullptr;
	r3dDX11InputLayout* MeshLayout = nullptr;
	r3dDX11InputLayout* SkinMeshLayout = nullptr;
	r3dDX11ConstantBuffer MeshConstants;
	r3dDX11ConstantBuffer MaterialConstants;
	r3dDX11ConstantBuffer SkinningConstants;
	bool bInitialized = false;
	bool bSkinnedMode = false;
	bool bAlphaTestMode = false;
};
