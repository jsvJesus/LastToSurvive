#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11DepthOnlyPass.h"

#include "RENDERING/DX11/RenderDX11Draw.h"
#include "RENDERING/DX11/RenderDX11GBufferResources.h"
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
	MeshConstants.Shutdown();

	delete SkinMeshLayout;
	SkinMeshLayout = nullptr;

	delete MeshLayout;
	MeshLayout = nullptr;

	SkinDepthVS = nullptr;
	DepthVS = nullptr;
	CommonStates = nullptr;
	ShaderLibrary = nullptr;
	DrawContext = nullptr;
	bInitialized = false;
	bSkinnedMode = false;
}

bool r3dDX11DepthOnlyPass::Begin(r3dDX11GBufferResources& gbuffer)
{
	if (!bInitialized || !DrawContext || !gbuffer.IsInitialized())
		return false;

	ID3D11RenderTargetView* const nullRenderTargets[1] = {};
	DrawContext->SetRenderTargets(0, nullRenderTargets, gbuffer.GetDepthStencilView());
	D3D11_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(gbuffer.GetWidth());
	viewport.Height = static_cast<float>(gbuffer.GetHeight());
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	DrawContext->GetContext()->RSSetViewports(1, &viewport);
	DrawContext->ClearDepth(gbuffer.GetDepthStencilView(), 1.0f, 0);

	DrawContext->SetRasterizerState(CommonStates->GetCullBackRasterizer());
	DrawContext->SetDepthStencilState(CommonStates->GetDepthReadWriteState());
	DrawContext->SetBlendState(CommonStates->GetOpaqueBlendState());
	DrawContext->SetInputLayout(MeshLayout);
	DrawContext->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DrawContext->SetShaders(DepthVS, nullptr);
	MeshConstants.BindVS(DrawContext->GetContext(), 0);
	SkinningConstants.BindVS(DrawContext->GetContext(), 1);
	bSkinnedMode = false;
	return true;
}

void r3dDX11DepthOnlyPass::End(r3dDX11GBufferResources&)
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
	DrawContext->SetShaders(skinned ? SkinDepthVS : DepthVS, nullptr);
	bSkinnedMode = skinned;
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
	if (vsIndex < 0 || skinVsIndex < 0)
		return false;

	DepthVS = ShaderLibrary->GetVertexShader(vsIndex);
	SkinDepthVS = ShaderLibrary->GetVertexShader(skinVsIndex);
	if (!DepthVS || !SkinDepthVS || !DepthVS->IsValid() || !SkinDepthVS->IsValid())
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
