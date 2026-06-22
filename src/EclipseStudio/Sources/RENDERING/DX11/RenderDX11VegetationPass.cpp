#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11VegetationPass.h"

#include "RENDERING/DX11/RenderDX11Draw.h"
#include "RENDERING/DX11/RenderDX11Material.h"
#include "RENDERING/DX11/RenderDX11MeshRenderData.h"
#include "RENDERING/DX11/RenderDX11States.h"
#include "RENDERING/DX11/RenderDX11VertexLayouts.h"
#include "RENDERING/DX11/ShaderDX11.h"

#include <cstring>

namespace
{
	int ConvertDepthBiasToD24Units(float depthBias)
	{
		const double scaledBias = static_cast<double>(depthBias) * 16777216.0;
		if (scaledBias > 2147483647.0)
			return 2147483647;
		if (scaledBias < -2147483648.0)
			return static_cast<int>(-2147483647 - 1);
		return static_cast<int>(scaledBias);
	}
}

r3dDX11VegetationPass::r3dDX11VegetationPass()
{
}

r3dDX11VegetationPass::~r3dDX11VegetationPass()
{
	Shutdown();
}

bool r3dDX11VegetationPass::Init(ID3D11Device* device, r3dDX11DrawContext* drawContext, r3dDX11ShaderLibrary* shaderLibrary, r3dDX11CommonStates* commonStates)
{
	if (bInitialized)
		return true;

	if (!device || !drawContext || !shaderLibrary || !commonStates)
		return false;

	Device = device;
	DrawContext = drawContext;
	ShaderLibrary = shaderLibrary;
	CommonStates = commonStates;

	if (!CreateShadersAndLayouts(device) ||
		!MeshConstants.Create(device, sizeof(r3dDX11MeshConstants), "DX11.Vegetation.MeshConstants") ||
		!MaterialConstants.Create(device, sizeof(r3dDX11MaterialConstants), "DX11.Vegetation.MaterialConstants") ||
		!WindConstants.Create(device, sizeof(r3dDX11VegetationWindConstants), "DX11.Vegetation.WindConstants"))
	{
		Shutdown();
		return false;
	}

	bInitialized = true;
	return true;
}

void r3dDX11VegetationPass::Shutdown()
{
	r3dDX11::SafeRelease(BiasedCullNoneRasterizer);
	r3dDX11::SafeRelease(BiasedCullBackRasterizer);

	InstanceBuffer.Shutdown();
	WindConstants.Shutdown();
	MaterialConstants.Shutdown();
	MeshConstants.Shutdown();

	delete VegetationBendingLayout;
	VegetationBendingLayout = nullptr;

	delete VegetationLayout;
	VegetationLayout = nullptr;

	AlphaTestPS = nullptr;
	VegetationPS = nullptr;
	VegetationBendingVS = nullptr;
	VegetationVS = nullptr;
	CommonStates = nullptr;
	ShaderLibrary = nullptr;
	DrawContext = nullptr;
	Device = nullptr;
	InstanceCapacity = 0;
	CurrentDepthBias = 0.0f;
	CurrentSlopeScaledDepthBias = 0.0f;
	bUseDepthBias = false;
	bInitialized = false;
	bBendingMode = false;
	bDepthMode = false;
}

bool r3dDX11VegetationPass::BeginGBuffer()
{
	if (!bInitialized || !DrawContext)
		return false;

	if (!SetDepthBias(0.0f, 0.0f))
		return false;

	DrawContext->SetDepthStencilState(CommonStates->GetDepthReadWriteState());
	DrawContext->SetBlendState(CommonStates->GetOpaqueBlendState());
	DrawContext->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DrawContext->SetSampler(0, CommonStates->GetLinearWrapSampler());
	MeshConstants.BindVS(DrawContext->GetContext(), 0);
	WindConstants.BindVS(DrawContext->GetContext(), 2);
	MaterialConstants.BindPS(DrawContext->GetContext(), 0);
	bBendingMode = true;
	bDepthMode = true;
	ApplyLayoutAndShaders(false, false);
	return true;
}

bool r3dDX11VegetationPass::BeginDepth()
{
	if (!bInitialized || !DrawContext)
		return false;

	if (!SetDepthBias(0.0f, 0.0f))
		return false;

	DrawContext->SetDepthStencilState(CommonStates->GetDepthReadWriteState());
	DrawContext->SetBlendState(CommonStates->GetOpaqueBlendState());
	DrawContext->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DrawContext->SetSampler(0, CommonStates->GetLinearWrapSampler());
	MeshConstants.BindVS(DrawContext->GetContext(), 0);
	WindConstants.BindVS(DrawContext->GetContext(), 2);
	MaterialConstants.BindPS(DrawContext->GetContext(), 0);
	bBendingMode = true;
	bDepthMode = false;
	ApplyLayoutAndShaders(false, true);
	return true;
}

bool r3dDX11VegetationPass::SetDepthBias(float depthBias, float slopeScaledDepthBias)
{
	if (!bInitialized || !Device || !DrawContext || !CommonStates)
		return false;

	if (depthBias == 0.0f && slopeScaledDepthBias == 0.0f)
	{
		CurrentDepthBias = 0.0f;
		CurrentSlopeScaledDepthBias = 0.0f;
		bUseDepthBias = false;
		return ApplyRasterizerState(true);
	}

	if (
		!bUseDepthBias ||
		CurrentDepthBias != depthBias ||
		CurrentSlopeScaledDepthBias != slopeScaledDepthBias
	)
	{
		if (!CreateBiasedRasterizers(depthBias, slopeScaledDepthBias))
			return false;

		CurrentDepthBias = depthBias;
		CurrentSlopeScaledDepthBias = slopeScaledDepthBias;
		bUseDepthBias = true;
	}

	return ApplyRasterizerState(true);
}

void r3dDX11VegetationPass::End()
{
	if (!DrawContext)
		return;

	DrawContext->SetVertexBuffer(nullptr, 1);
	bBendingMode = true;
	bDepthMode = true;
}

bool r3dDX11VegetationPass::SetWindConstants(const r3dDX11VegetationWindConstants& constants)
{
	if (!bInitialized || !DrawContext)
		return false;

	return WindConstants.Update(DrawContext->GetContext(), &constants, sizeof(constants));
}

bool r3dDX11VegetationPass::DrawBatch(
	r3dDX11MeshRenderData& renderData,
	unsigned int batchIndex,
	const D3DXMATRIX& viewProj,
	const r3dDX11VegetationInstance* instances,
	unsigned int instanceCount,
	bool depthOnly
)
{
	if (!bInitialized || !Device || !DrawContext || !instances || instanceCount == 0 || !renderData.IsValid())
		return false;

	r3dDX11MeshResource& meshResource = renderData.GetMeshResource();
	const r3dDX11MeshBatch* batch = meshResource.GetBatch(batchIndex);
	const r3dDX11MaterialTextures* sourceMaterial = renderData.GetMaterialTextures(batchIndex);
	if (!batch || !sourceMaterial)
		return false;

	if (!EnsureInstanceCapacity(Device, instanceCount))
		return false;

	if (!InstanceBuffer.Update(DrawContext->GetContext(), instances, sizeof(instances[0]) * instanceCount))
		return false;

	r3dDX11MaterialTextures material = *sourceMaterial;
	material.SetDomain(R3D_DX11_MATERIAL_VEGETATION);
	material.SetDrawInGBuffer(true);
	material.SetAlphaCut(true);
	material.SetDoubleSided(true);
	material.SetTransparent(false);
	material.SetSkipDraw(false);

	r3dDX11MaterialConstants materialConstants = material.BuildConstants();
	if (!MaterialConstants.Update(DrawContext->GetContext(), &materialConstants, sizeof(materialConstants)))
		return false;

	material.Bind(*DrawContext, 0);

	r3dDX11MeshConstants meshConstants = {};
	memcpy(meshConstants.WorldViewProj, &viewProj, sizeof(meshConstants.WorldViewProj));
	D3DXMATRIX identity;
	D3DXMatrixIdentity(&identity);
	memcpy(meshConstants.World, &identity, sizeof(meshConstants.World));
	const float* positionScale = meshResource.GetPositionScale();
	const float* texcoordScale = meshResource.GetTexcoordScale();
	meshConstants.PositionScale[0] = positionScale[0];
	meshConstants.PositionScale[1] = positionScale[1];
	meshConstants.PositionScale[2] = positionScale[2];
	meshConstants.PositionScale[3] = 0.0f;
	meshConstants.TexcoordScale[0] = texcoordScale[0];
	meshConstants.TexcoordScale[1] = texcoordScale[1];
	meshConstants.TexcoordScale[2] = 0.0f;
	meshConstants.TexcoordScale[3] = 0.0f;
	if (!MeshConstants.Update(DrawContext->GetContext(), &meshConstants, sizeof(meshConstants)))
		return false;

	ApplyLayoutAndShaders(meshResource.IsBending(), depthOnly);
	meshResource.DrawBatchInstanced(*DrawContext, InstanceBuffer, batchIndex, instanceCount);
	return true;
}

bool r3dDX11VegetationPass::IsInitialized() const
{
	return bInitialized;
}

bool r3dDX11VegetationPass::CreateShadersAndLayouts(ID3D11Device* device)
{
	const int vegetationVsIndex = ShaderLibrary->AddVertexShader("VS_DX11_VEGETATION", "DS_vegetation_vs.hls");
	const int vegetationBendingVsIndex = ShaderLibrary->AddVertexShader("VS_DX11_VEGETATION_BENDING", "DS_vegetation_bending_vs.hls");
	const int vegetationPsIndex = ShaderLibrary->AddPixelShader("PS_DX11_VEGETATION", "DS_vegetation_ps.hls");
	const int alphaPsIndex = ShaderLibrary->AddPixelShader("PS_DX11_VEGETATION_DEPTH_ALPHA_TEST", "DS_depthonly_alpha_ps.hls");
	if (vegetationVsIndex < 0 || vegetationBendingVsIndex < 0 || vegetationPsIndex < 0 || alphaPsIndex < 0)
		return false;

	VegetationVS = ShaderLibrary->GetVertexShader(vegetationVsIndex);
	VegetationBendingVS = ShaderLibrary->GetVertexShader(vegetationBendingVsIndex);
	VegetationPS = ShaderLibrary->GetPixelShader(vegetationPsIndex);
	AlphaTestPS = ShaderLibrary->GetPixelShader(alphaPsIndex);
	if (!VegetationVS || !VegetationBendingVS || !VegetationPS || !AlphaTestPS ||
		!VegetationVS->IsValid() || !VegetationBendingVS->IsValid() || !VegetationPS->IsValid() || !AlphaTestPS->IsValid())
	{
		return false;
	}

	unsigned int layoutCount = 0;
	const D3D11_INPUT_ELEMENT_DESC* layout = r3dDX11VertexLayouts::InstancedMesh(&layoutCount);
	VegetationLayout = new r3dDX11InputLayout();
	if (!VegetationLayout->Create(device, layout, layoutCount, VegetationVS->GetBytecode(), VegetationVS->GetBytecodeSize(), "DX11.Vegetation.Layout"))
		return false;

	layout = r3dDX11VertexLayouts::InstancedBendingMesh(&layoutCount);
	VegetationBendingLayout = new r3dDX11InputLayout();
	if (!VegetationBendingLayout->Create(device, layout, layoutCount, VegetationBendingVS->GetBytecode(), VegetationBendingVS->GetBytecodeSize(), "DX11.Vegetation.BendingLayout"))
		return false;

	return true;
}

bool r3dDX11VegetationPass::EnsureInstanceCapacity(ID3D11Device* device, unsigned int instanceCount)
{
	if (InstanceBuffer.IsValid() && InstanceCapacity >= instanceCount)
		return true;

	unsigned int newCapacity = InstanceCapacity ? InstanceCapacity : 512;
	while (newCapacity < instanceCount)
		newCapacity *= 2;

	if (!InstanceBuffer.Create(device, sizeof(r3dDX11VegetationInstance) * newCapacity, sizeof(r3dDX11VegetationInstance), nullptr, R3D_DX11_BUFFER_DYNAMIC, "DX11.Vegetation.InstanceVB"))
	{
		InstanceCapacity = 0;
		return false;
	}

	InstanceCapacity = newCapacity;
	return true;
}

bool r3dDX11VegetationPass::ApplyRasterizerState(bool doubleSided)
{
	if (!DrawContext || !CommonStates)
		return false;

	ID3D11RasterizerState* state = nullptr;
	if (bUseDepthBias)
		state = doubleSided ? BiasedCullNoneRasterizer : BiasedCullBackRasterizer;
	else
		state = doubleSided ? CommonStates->GetCullNoneRasterizer() : CommonStates->GetCullBackRasterizer();

	if (!state)
		return false;

	DrawContext->SetRasterizerState(state);
	return true;
}

bool r3dDX11VegetationPass::CreateBiasedRasterizers(float depthBias, float slopeScaledDepthBias)
{
	if (!Device)
		return false;

	r3dDX11::SafeRelease(BiasedCullNoneRasterizer);
	r3dDX11::SafeRelease(BiasedCullBackRasterizer);

	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthClipEnable = TRUE;
	rasterDesc.DepthBias = ConvertDepthBiasToD24Units(depthBias);
	rasterDesc.SlopeScaledDepthBias = slopeScaledDepthBias;

	if (FAILED(Device->CreateRasterizerState(&rasterDesc, &BiasedCullBackRasterizer)))
	{
		r3dDX11::SafeRelease(BiasedCullBackRasterizer);
		return false;
	}

	rasterDesc.CullMode = D3D11_CULL_NONE;
	if (FAILED(Device->CreateRasterizerState(&rasterDesc, &BiasedCullNoneRasterizer)))
	{
		r3dDX11::SafeRelease(BiasedCullNoneRasterizer);
		r3dDX11::SafeRelease(BiasedCullBackRasterizer);
		return false;
	}

	r3dDX11::SetDebugName(BiasedCullBackRasterizer, "DX11.Vegetation.BiasedCullBack");
	r3dDX11::SetDebugName(BiasedCullNoneRasterizer, "DX11.Vegetation.BiasedCullNone");
	return true;
}

void r3dDX11VegetationPass::ApplyLayoutAndShaders(bool bending, bool depthOnly)
{
	if (!DrawContext)
		return;

	const bool layoutChanged = bBendingMode != bending;
	const bool shaderChanged = layoutChanged || bDepthMode != depthOnly;

	if (layoutChanged)
	{
		DrawContext->SetInputLayout(bending ? VegetationBendingLayout : VegetationLayout);
	}

	if (shaderChanged)
	{
		DrawContext->SetShaders(bending ? VegetationBendingVS : VegetationVS, depthOnly ? AlphaTestPS : VegetationPS);
	}

	bBendingMode = bending;
	bDepthMode = depthOnly;
}
