#include "r3dPCH.h"

#include "RENDERING/DX11/RenderDX11GBufferPass.h"

#include "RENDERING/DX11/RenderDX11Draw.h"
#include "RENDERING/DX11/RenderDX11GBufferResources.h"
#include "RENDERING/DX11/RenderDX11Material.h"
#include "RENDERING/DX11/RenderDX11States.h"
#include "RENDERING/DX11/RenderDX11VertexLayouts.h"
#include "RENDERING/DX11/ShaderDX11.h"

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

	if (!CreateShadersAndLayout(device) || !MeshConstants.Create(device, sizeof(r3dDX11MeshConstants), "DX11.GBuffer.MeshConstants"))
	{
		Shutdown();
		return false;
	}

	bInitialized = true;
	return true;
}

void r3dDX11GBufferPass::Shutdown()
{
	MeshConstants.Shutdown();

	delete MeshLayout;
	MeshLayout = nullptr;

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

void r3dDX11GBufferPass::SetMaterial(const r3dDX11MaterialTextures& material)
{
	if (bInitialized && DrawContext)
		material.Bind(*DrawContext, 0);
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
	const int psIndex = ShaderLibrary->AddPixelShader("PS_DX11_FILLGBUFFER", "DS_fillbuffer_ps.hls");
	if (vsIndex < 0 || psIndex < 0)
		return false;

	FillVS = ShaderLibrary->GetVertexShader(vsIndex);
	FillPS = ShaderLibrary->GetPixelShader(psIndex);
	if (!FillVS || !FillPS || !FillVS->IsValid() || !FillPS->IsValid())
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

	return true;
}
