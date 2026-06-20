#include "r3dPCH.h"

#include "RENDERING/DX11/RenderDX11Fullscreen.h"

#include "RENDERING/DX11/RenderDX11Draw.h"
#include "RENDERING/DX11/RenderDX11Resources.h"
#include "RENDERING/DX11/RenderDX11States.h"
#include "RENDERING/DX11/ShaderDX11.h"
#include "RENDERING/DX11/RenderDX11Platform.h"

#pragma comment(lib, "d3d11.lib")

namespace
{
	struct FullscreenVertex
	{
		float Position[3];
		float TexCoord[2];
	};
}

r3dDX11FullscreenPass::r3dDX11FullscreenPass()
{
}

r3dDX11FullscreenPass::~r3dDX11FullscreenPass()
{
	Shutdown();
}

bool r3dDX11FullscreenPass::Init(ID3D11Device* device, r3dDX11DrawContext* drawContext, r3dDX11ShaderLibrary* shaderLibrary, r3dDX11CommonStates* commonStates)
{
	if (bInitialized)
		return true;

	if (!device || !drawContext || !shaderLibrary || !commonStates)
		return false;

	DrawContext = drawContext;
	ShaderLibrary = shaderLibrary;
	CommonStates = commonStates;

	if (!CreateShadersAndLayout(device) || !CreateGeometry(device))
	{
		Shutdown();
		return false;
	}

	bInitialized = true;
	return true;
}

void r3dDX11FullscreenPass::Shutdown()
{
	delete VertexBuffer;
	VertexBuffer = nullptr;

	delete InputLayout;
	InputLayout = nullptr;

	CopyPS = nullptr;
	CopyVS = nullptr;
	CommonStates = nullptr;
	ShaderLibrary = nullptr;
	DrawContext = nullptr;
	bInitialized = false;
}

bool r3dDX11FullscreenPass::Copy(
	ID3D11ShaderResourceView* source,
	ID3D11RenderTargetView* destination,
	ID3D11DepthStencilView* depthStencil,
	int width,
	int height
)
{
	if (!bInitialized || !DrawContext || !source || !destination || width <= 0 || height <= 0)
		return false;

	DrawContext->SetRenderTarget(destination, depthStencil);
	DrawContext->SetViewport(width, height);
	DrawContext->SetRasterizerState(CommonStates->GetCullNoneRasterizer());
	DrawContext->SetDepthStencilState(CommonStates->GetDepthDisabledState());
	DrawContext->SetBlendState(CommonStates->GetOpaqueBlendState());
	DrawContext->SetInputLayout(InputLayout);
	DrawContext->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DrawContext->SetVertexBuffer(VertexBuffer);
	DrawContext->SetShaders(CopyVS, CopyPS);
	DrawContext->SetSampler(0, CommonStates->GetLinearClampSampler());
	DrawContext->SetShaderResource(0, source);
	DrawContext->Draw(3);

	ID3D11ShaderResourceView* nullSRV = nullptr;
	DrawContext->SetShaderResource(0, nullSRV);
	return true;
}

bool r3dDX11FullscreenPass::Copy(ID3D11ShaderResourceView* source, r3dDX11RenderTarget& destination)
{
	return Copy(source, destination.GetRTV(), destination.GetDSV(), destination.GetWidth(), destination.GetHeight());
}

bool r3dDX11FullscreenPass::IsInitialized() const
{
	return bInitialized;
}

bool r3dDX11FullscreenPass::CreateShadersAndLayout(ID3D11Device* device)
{
	const int vsIndex = ShaderLibrary->AddVertexShader("VS_FULLSCREEN", "Fullscreen_vs.hls");
	const int psIndex = ShaderLibrary->AddPixelShader("PS_COPY", "copy_ps.hls");
	if (vsIndex < 0 || psIndex < 0)
		return false;

	CopyVS = ShaderLibrary->GetVertexShader(vsIndex);
	CopyPS = ShaderLibrary->GetPixelShader(psIndex);
	if (!CopyVS || !CopyPS || !CopyVS->IsValid() || !CopyPS->IsValid())
		return false;

	const D3D11_INPUT_ELEMENT_DESC inputElements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FullscreenVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FullscreenVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	InputLayout = new r3dDX11InputLayout();
	if (!InputLayout->Create(device, inputElements, static_cast<unsigned int>(_countof(inputElements)), CopyVS->GetBytecode(), CopyVS->GetBytecodeSize(), "DX11.Fullscreen.InputLayout"))
	{
		delete InputLayout;
		InputLayout = nullptr;
		return false;
	}

	return true;
}

bool r3dDX11FullscreenPass::CreateGeometry(ID3D11Device* device)
{
	const FullscreenVertex vertices[] =
	{
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
		{ { -1.0f,  3.0f, 0.0f }, { 0.0f, -1.0f } },
		{ {  3.0f, -1.0f, 0.0f }, { 2.0f, 1.0f } }
	};

	VertexBuffer = new r3dDX11VertexBuffer();
	if (!VertexBuffer->Create(device, sizeof(vertices), sizeof(FullscreenVertex), vertices, R3D_DX11_BUFFER_IMMUTABLE, "DX11.Fullscreen.VertexBuffer"))
	{
		delete VertexBuffer;
		VertexBuffer = nullptr;
		return false;
	}

	return true;
}
