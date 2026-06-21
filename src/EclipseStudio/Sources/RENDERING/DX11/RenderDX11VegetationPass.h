#pragma once

#include "RENDERING/DX11/RenderDX11GBufferPass.h"
#include "RENDERING/DX11/RenderDX11Resources.h"

class r3dDX11CommonStates;
class r3dDX11DrawContext;
class r3dDX11InputLayout;
class r3dDX11MeshRenderData;
class r3dDX11PixelShader;
class r3dDX11ShaderLibrary;
class r3dDX11VertexShader;
struct ID3D11Device;

struct r3dDX11VegetationInstance
{
	float PositionAngle[4];
	float ScaleRandom[4];
	float Anim[4];
	float Options[4];
};

struct r3dDX11VegetationWindConstants
{
	float Params[5][4];
};

class r3dDX11VegetationPass final
{
public:
	r3dDX11VegetationPass();
	~r3dDX11VegetationPass();

	bool Init(ID3D11Device* device, r3dDX11DrawContext* drawContext, r3dDX11ShaderLibrary* shaderLibrary, r3dDX11CommonStates* commonStates);
	void Shutdown();

	bool BeginGBuffer();
	bool BeginDepth();
	void End();

	bool SetWindConstants(const r3dDX11VegetationWindConstants& constants);
	bool DrawBatch(
		r3dDX11MeshRenderData& renderData,
		unsigned int batchIndex,
		const D3DXMATRIX& viewProj,
		const r3dDX11VegetationInstance* instances,
		unsigned int instanceCount,
		bool depthOnly
	);

	bool IsInitialized() const;

private:
	bool CreateShadersAndLayouts(ID3D11Device* device);
	bool EnsureInstanceCapacity(ID3D11Device* device, unsigned int instanceCount);
	void ApplyLayoutAndShaders(bool bending, bool depthOnly);

private:
	ID3D11Device* Device = nullptr;
	r3dDX11DrawContext* DrawContext = nullptr;
	r3dDX11ShaderLibrary* ShaderLibrary = nullptr;
	r3dDX11CommonStates* CommonStates = nullptr;
	r3dDX11VertexShader* VegetationVS = nullptr;
	r3dDX11VertexShader* VegetationBendingVS = nullptr;
	r3dDX11PixelShader* VegetationPS = nullptr;
	r3dDX11PixelShader* AlphaTestPS = nullptr;
	r3dDX11InputLayout* VegetationLayout = nullptr;
	r3dDX11InputLayout* VegetationBendingLayout = nullptr;
	r3dDX11ConstantBuffer MeshConstants;
	r3dDX11ConstantBuffer MaterialConstants;
	r3dDX11ConstantBuffer WindConstants;
	r3dDX11VertexBuffer InstanceBuffer;
	unsigned int InstanceCapacity = 0;
	bool bInitialized = false;
	bool bBendingMode = false;
	bool bDepthMode = false;
};
