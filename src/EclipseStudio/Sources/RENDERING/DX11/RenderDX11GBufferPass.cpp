#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11GBufferPass.h"

#include "RENDERING/DX11/RenderDX11Draw.h"
#include "RENDERING/DX11/RenderDX11GBufferResources.h"
#include "RENDERING/DX11/RenderDX11Material.h"
#include "RENDERING/DX11/RenderDX11States.h"
#include "RENDERING/DX11/RenderDX11VertexLayouts.h"
#include "RENDERING/DX11/ShaderDX11.h"
#include "r3dSkeleton.h"

r3dDX11GBufferPass::r3dDX11GBufferPass()
{
}

r3dDX11GBufferPass::~r3dDX11GBufferPass()
{
	Shutdown();
}

bool r3dDX11GBufferPass::Init(ID3D11Device* device, r3dDX11DrawContext* drawContext, r3dDX11ShaderLibrary* shaderLibrary, r3dDX11CommonStates* commonStates)
{
	if (bInitialized)
		return true;

	if (!device || !drawContext || !shaderLibrary || !commonStates)
		return false;

	DrawContext = drawContext;
	ShaderLibrary = shaderLibrary;
	CommonStates = commonStates;

	if (!CreateShadersAndLayout(device) ||
		!MeshConstants.Create(device, sizeof(r3dDX11MeshConstants), "DX11.GBuffer.MeshConstants") ||
		!MaterialConstants.Create(device, sizeof(r3dDX11MaterialConstants), "DX11.GBuffer.MaterialConstants") ||
		!SkinningConstants.Create(device, sizeof(r3dDX11SkinningConstants), "DX11.GBuffer.SkinningConstants"))
	{
		Shutdown();
		return false;
	}

	bInitialized = true;
	return true;
}

void r3dDX11GBufferPass::Shutdown()
{
	SkinningConstants.Shutdown();
	MaterialConstants.Shutdown();
	MeshConstants.Shutdown();

	delete SkinMeshLayout;
	SkinMeshLayout = nullptr;

	delete MeshLayout;
	MeshLayout = nullptr;

	SkinFillVS = nullptr;
	FillPS = nullptr;
	FillVS = nullptr;
	CommonStates = nullptr;
	ShaderLibrary = nullptr;
	DrawContext = nullptr;
	bInitialized = false;
}

bool r3dDX11GBufferPass::Begin(r3dDX11GBufferResources& gbuffer)
{
	if (!bInitialized || !DrawContext || !gbuffer.IsInitialized())
		return false;

	gbuffer.BeginGBuffer();

	DrawContext->SetRasterizerState(CommonStates->GetCullBackRasterizer());
	DrawContext->SetDepthStencilState(CommonStates->GetDepthReadWriteState());
	DrawContext->SetBlendState(CommonStates->GetOpaqueBlendState());
	DrawContext->SetInputLayout(MeshLayout);
	DrawContext->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DrawContext->SetShaders(FillVS, FillPS);
	DrawContext->SetSampler(0, CommonStates->GetLinearWrapSampler());
	MeshConstants.BindVS(DrawContext->GetContext(), 0);
	SkinningConstants.BindVS(DrawContext->GetContext(), 1);
	MaterialConstants.BindPS(DrawContext->GetContext(), 0);
	bSkinnedMode = false;
	return true;
}

void r3dDX11GBufferPass::End(r3dDX11GBufferResources& gbuffer)
{
	gbuffer.EndGBuffer();
}

bool r3dDX11GBufferPass::SetMeshConstants(const r3dDX11MeshConstants& constants)
{
	if (!bInitialized || !DrawContext)
		return false;

	return MeshConstants.Update(DrawContext->GetContext(), &constants, sizeof(constants));
}

bool r3dDX11GBufferPass::SetSkinningBones(const r3dSkeleton* skeleton)
{
	if (!bInitialized || !DrawContext)
		return false;

	r3dDX11SkinningConstants constants = {};
	const unsigned int maxBones = static_cast<unsigned int>(_countof(constants.BoneMatrices));
	unsigned int boneCount = 0;
	if (skeleton && skeleton->bLoaded)
	{
		boneCount = static_cast<unsigned int>(skeleton->GetNumBones());
		if (boneCount > maxBones)
			boneCount = maxBones;
	}

	for (unsigned int i = 0; i < maxBones; ++i)
	{
		D3DXMATRIX bone;
		if (i < boneCount)
			bone = skeleton->Bones[i].CurrentTM;
		else
			D3DXMatrixIdentity(&bone);

		memcpy(constants.BoneMatrices[i], &bone, sizeof(constants.BoneMatrices[i]));
	}

	if (!SkinningConstants.Update(DrawContext->GetContext(), &constants, sizeof(constants)))
		return false;

	SkinningConstants.BindVS(DrawContext->GetContext(), 1);
	return true;
}

void r3dDX11GBufferPass::SetSkinnedMeshMode(bool skinned)
{
	if (!bInitialized || !DrawContext || bSkinnedMode == skinned)
		return;

	DrawContext->SetInputLayout(skinned ? SkinMeshLayout : MeshLayout);
	DrawContext->SetShaders(skinned ? SkinFillVS : FillVS, FillPS);
	bSkinnedMode = skinned;
}

bool r3dDX11GBufferPass::SetMaterial(const r3dDX11MaterialTextures& material, unsigned int objectColorPacked)
{
	if (!bInitialized || !DrawContext || !material.ShouldDrawInGBuffer())
		return false;

	const r3dDX11MaterialConstants constants = material.BuildConstants(objectColorPacked);
	if (!MaterialConstants.Update(DrawContext->GetContext(), &constants, sizeof(constants)))
		return false;

	MaterialConstants.BindPS(DrawContext->GetContext(), 0);
	DrawContext->SetRasterizerState(material.IsDoubleSided() ? CommonStates->GetCullNoneRasterizer() : CommonStates->GetCullBackRasterizer());
	material.Bind(*DrawContext, 0);
	return true;
}

void r3dDX11GBufferPass::DrawMesh(r3dDX11VertexBuffer& vertexBuffer, r3dDX11IndexBuffer& indexBuffer, unsigned int indexCount, unsigned int startIndex, int baseVertex)
{
	if (!bInitialized || indexCount == 0)
		return;

	DrawContext->SetVertexBuffer(&vertexBuffer);
	DrawContext->SetIndexBuffer(&indexBuffer);
	DrawContext->DrawIndexed(indexCount, startIndex, baseVertex);
}

bool r3dDX11GBufferPass::IsInitialized() const
{
	return bInitialized;
}

bool r3dDX11GBufferPass::CreateShadersAndLayout(ID3D11Device* device)
{
	const int vsIndex = ShaderLibrary->AddVertexShader("VS_DX11_FILLGBUFFER", "DS_fillbuffer_vs.hls");
	const int skinVsIndex = ShaderLibrary->AddVertexShader("VS_DX11_SKIN_FILLGBUFFER", "DS_fillbuffer_skin_vs.hls");
	const int psIndex = ShaderLibrary->AddPixelShader("PS_DX11_FILLGBUFFER", "DS_fillbuffer_ps.hls");
	if (vsIndex < 0 || skinVsIndex < 0 || psIndex < 0)
		return false;

	FillVS = ShaderLibrary->GetVertexShader(vsIndex);
	SkinFillVS = ShaderLibrary->GetVertexShader(skinVsIndex);
	FillPS = ShaderLibrary->GetPixelShader(psIndex);
	if (!FillVS || !SkinFillVS || !FillPS || !FillVS->IsValid() || !SkinFillVS->IsValid() || !FillPS->IsValid())
		return false;

	unsigned int layoutCount = 0;
	const D3D11_INPUT_ELEMENT_DESC* layout = r3dDX11VertexLayouts::Mesh(&layoutCount);

	MeshLayout = new r3dDX11InputLayout();
	if (!MeshLayout->Create(device, layout, layoutCount, FillVS->GetBytecode(), FillVS->GetBytecodeSize(), "DX11.GBuffer.MeshLayout"))
	{
		delete MeshLayout;
		MeshLayout = nullptr;
		return false;
	}

	layoutCount = 0;
	layout = r3dDX11VertexLayouts::SkinnedMesh(&layoutCount);

	SkinMeshLayout = new r3dDX11InputLayout();
	if (!SkinMeshLayout->Create(device, layout, layoutCount, SkinFillVS->GetBytecode(), SkinFillVS->GetBytecodeSize(), "DX11.GBuffer.SkinMeshLayout"))
	{
		delete SkinMeshLayout;
		SkinMeshLayout = nullptr;
		return false;
	}

	return true;
}
