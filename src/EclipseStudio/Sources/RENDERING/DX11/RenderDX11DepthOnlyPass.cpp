#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11DepthOnlyPass.h"

#include "RENDERING/DX11/RenderDX11Draw.h"
#include "RENDERING/DX11/RenderDX11GBufferResources.h"
#include "RENDERING/DX11/RenderDX11Material.h"
#include "RENDERING/DX11/RenderDX11States.h"
#include "RENDERING/DX11/RenderDX11VertexLayouts.h"
#include "RENDERING/DX11/ShaderDX11.h"
#include "r3dSkeleton.h"

r3dDX11DepthOnlyPass::r3dDX11DepthOnlyPass()
{
}

r3dDX11DepthOnlyPass::~r3dDX11DepthOnlyPass()
{
	Shutdown();
}

bool r3dDX11DepthOnlyPass::Init(ID3D11Device* device, r3dDX11DrawContext* drawContext, r3dDX11ShaderLibrary* shaderLibrary, r3dDX11CommonStates* commonStates)
{
	if (bInitialized)
		return true;

	if (!device || !drawContext || !shaderLibrary || !commonStates)
		return false;

	DrawContext = drawContext;
	ShaderLibrary = shaderLibrary;
	CommonStates = commonStates;

	if (!CreateShadersAndLayout(device) ||
		!MeshConstants.Create(device, sizeof(r3dDX11MeshConstants), "DX11.DepthOnly.MeshConstants") ||
		!MaterialConstants.Create(device, sizeof(r3dDX11MaterialConstants), "DX11.DepthOnly.MaterialConstants") ||
		!SkinningConstants.Create(device, sizeof(r3dDX11SkinningConstants), "DX11.DepthOnly.SkinningConstants"))
	{
		Shutdown();
		return false;
	}

	bInitialized = true;
	return true;
}

void r3dDX11DepthOnlyPass::Shutdown()
{
	SkinningConstants.Shutdown();
	MaterialConstants.Shutdown();
	MeshConstants.Shutdown();

	delete SkinMeshLayout;
	SkinMeshLayout = nullptr;

	delete MeshLayout;
	MeshLayout = nullptr;

	SkinDepthVS = nullptr;
	AlphaTestPS = nullptr;
	DepthVS = nullptr;
	CommonStates = nullptr;
	ShaderLibrary = nullptr;
	DrawContext = nullptr;
	bInitialized = false;
	bSkinnedMode = false;
	bAlphaTestMode = false;
}

bool r3dDX11DepthOnlyPass::Begin(r3dDX11GBufferResources& gbuffer, bool clearDepth)
{
	if (!bInitialized || !DrawContext || !gbuffer.IsInitialized())
		return false;

	return BeginDepthTarget(
		gbuffer.GetDepthStencilView(),
		gbuffer.GetWidth(),
		gbuffer.GetHeight(),
		clearDepth
	);
}

bool r3dDX11DepthOnlyPass::BeginDepthTarget(ID3D11DepthStencilView* depthStencilView, int width, int height, bool clearDepth)
{
	if (!bInitialized || !DrawContext || !depthStencilView || width <= 0 || height <= 0)
		return false;

	ID3D11RenderTargetView* const nullRenderTargets[1] = {};
	DrawContext->SetRenderTargets(0, nullRenderTargets, depthStencilView);
	D3D11_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	DrawContext->GetContext()->RSSetViewports(1, &viewport);
	if (clearDepth)
		DrawContext->ClearDepth(depthStencilView, 1.0f, 0);

	DrawContext->SetRasterizerState(CommonStates->GetCullBackRasterizer());
	DrawContext->SetDepthStencilState(CommonStates->GetDepthReadWriteState());
	DrawContext->SetBlendState(CommonStates->GetOpaqueBlendState());
	DrawContext->SetInputLayout(MeshLayout);
	DrawContext->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DrawContext->SetShaders(DepthVS, nullptr);
	DrawContext->SetSampler(0, CommonStates->GetLinearWrapSampler());
	MeshConstants.BindVS(DrawContext->GetContext(), 0);
	SkinningConstants.BindVS(DrawContext->GetContext(), 1);
	MaterialConstants.BindPS(DrawContext->GetContext(), 0);
	bSkinnedMode = false;
	bAlphaTestMode = false;
	return true;
}

void r3dDX11DepthOnlyPass::End(r3dDX11GBufferResources&)
{
	if (!DrawContext)
		return;

	ID3D11RenderTargetView* const nullRenderTargets[1] = {};
	DrawContext->SetRenderTargets(0, nullRenderTargets, nullptr);
}

void r3dDX11DepthOnlyPass::EndDepthTarget()
{
	if (!DrawContext)
		return;

	ID3D11RenderTargetView* const nullRenderTargets[1] = {};
	DrawContext->SetRenderTargets(0, nullRenderTargets, nullptr);
}

bool r3dDX11DepthOnlyPass::SetMeshConstants(const r3dDX11MeshConstants& constants)
{
	if (!bInitialized || !DrawContext)
		return false;

	return MeshConstants.Update(DrawContext->GetContext(), &constants, sizeof(constants));
}

bool r3dDX11DepthOnlyPass::SetSkinningBones(const r3dSkeleton* skeleton)
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

void r3dDX11DepthOnlyPass::SetSkinnedMeshMode(bool skinned)
{
	if (!bInitialized || !DrawContext || bSkinnedMode == skinned)
		return;

	DrawContext->SetInputLayout(skinned ? SkinMeshLayout : MeshLayout);
	bSkinnedMode = skinned;
	ApplyShaders();
}

bool r3dDX11DepthOnlyPass::SetMaterial(const r3dDX11MaterialTextures& material)
{
	if (!bInitialized || !DrawContext || material.IsTransparent() || material.IsSkipDraw())
		return false;

	const r3dDX11MaterialConstants constants = material.BuildConstants();
	if (!MaterialConstants.Update(DrawContext->GetContext(), &constants, sizeof(constants)))
		return false;

	MaterialConstants.BindPS(DrawContext->GetContext(), 0);
	DrawContext->SetRasterizerState(material.IsDoubleSided() ? CommonStates->GetCullNoneRasterizer() : CommonStates->GetCullBackRasterizer());
	if (material.IsAlphaCut())
		material.Bind(*DrawContext, 0);

	const bool alphaTestMode = material.IsAlphaCut();
	if (bAlphaTestMode != alphaTestMode)
	{
		bAlphaTestMode = alphaTestMode;
		ApplyShaders();
	}

	return true;
}

void r3dDX11DepthOnlyPass::DrawMesh(r3dDX11VertexBuffer& vertexBuffer, r3dDX11IndexBuffer& indexBuffer, unsigned int indexCount, unsigned int startIndex, int baseVertex)
{
	if (!bInitialized || indexCount == 0)
		return;

	DrawContext->SetVertexBuffer(&vertexBuffer);
	DrawContext->SetIndexBuffer(&indexBuffer);
	DrawContext->DrawIndexed(indexCount, startIndex, baseVertex);
}

bool r3dDX11DepthOnlyPass::IsInitialized() const
{
	return bInitialized;
}

bool r3dDX11DepthOnlyPass::CreateShadersAndLayout(ID3D11Device* device)
{
	const int vsIndex = ShaderLibrary->AddVertexShader("VS_DX11_DEPTH_ONLY", "DS_depthonly_vs.hls");
	const int skinVsIndex = ShaderLibrary->AddVertexShader("VS_DX11_SKIN_DEPTH_ONLY", "DS_depthonly_skin_vs.hls");
	const int alphaPsIndex = ShaderLibrary->AddPixelShader("PS_DX11_DEPTH_ALPHA_TEST", "DS_depthonly_alpha_ps.hls");
	if (vsIndex < 0 || skinVsIndex < 0 || alphaPsIndex < 0)
		return false;

	DepthVS = ShaderLibrary->GetVertexShader(vsIndex);
	SkinDepthVS = ShaderLibrary->GetVertexShader(skinVsIndex);
	AlphaTestPS = ShaderLibrary->GetPixelShader(alphaPsIndex);
	if (!DepthVS || !SkinDepthVS || !AlphaTestPS || !DepthVS->IsValid() || !SkinDepthVS->IsValid() || !AlphaTestPS->IsValid())
		return false;

	unsigned int layoutCount = 0;
	const D3D11_INPUT_ELEMENT_DESC* layout = r3dDX11VertexLayouts::Mesh(&layoutCount);

	MeshLayout = new r3dDX11InputLayout();
	if (!MeshLayout->Create(device, layout, layoutCount, DepthVS->GetBytecode(), DepthVS->GetBytecodeSize(), "DX11.DepthOnly.MeshLayout"))
	{
		delete MeshLayout;
		MeshLayout = nullptr;
		return false;
	}

	layoutCount = 0;
	layout = r3dDX11VertexLayouts::SkinnedMesh(&layoutCount);

	SkinMeshLayout = new r3dDX11InputLayout();
	if (!SkinMeshLayout->Create(device, layout, layoutCount, SkinDepthVS->GetBytecode(), SkinDepthVS->GetBytecodeSize(), "DX11.DepthOnly.SkinMeshLayout"))
	{
		delete SkinMeshLayout;
		SkinMeshLayout = nullptr;
		return false;
	}

	return true;
}

void r3dDX11DepthOnlyPass::ApplyShaders()
{
	if (!DrawContext)
		return;

	DrawContext->SetShaders(bSkinnedMode ? SkinDepthVS : DepthVS, bAlphaTestMode ? AlphaTestPS : nullptr);
}
