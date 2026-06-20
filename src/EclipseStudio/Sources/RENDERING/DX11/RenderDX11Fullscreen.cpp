#include "r3dPCH.h"

#include "RENDERING/DX11/RenderDX11Fullscreen.h"

#include "RENDERING/DX11/RenderDX11Resources.h"
#include "RENDERING/DX11/ShaderDX11.h"

#include <d3d11_1.h>

#pragma comment(lib, "d3d11.lib")

namespace
{
	template <typename T>
	void SafeReleaseDX11(T*& value)
	{
		if (value)
		{
			value->Release();
			value = nullptr;
		}
	}

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

bool r3dDX11FullscreenPass::Init(ID3D11Device* device, ID3D11DeviceContext* context)
{
	if (bInitialized)
		return true;

	if (!device || !context)
		return false;

	Context = context;

	if (!CreateShaders(device) || !CreateGeometry(device) || !CreateStates(device))
	{
		Shutdown();
		return false;
	}

	bInitialized = true;
	return true;
}

void r3dDX11FullscreenPass::Shutdown()
{
	SafeReleaseDX11(BlendState);
	SafeReleaseDX11(DepthStencilState);
	SafeReleaseDX11(RasterizerState);
	SafeReleaseDX11(SamplerState);
	SafeReleaseDX11(VertexBuffer);
	SafeReleaseDX11(InputLayout);
	SafeReleaseDX11(CopyPS);
	SafeReleaseDX11(CopyVS);

	Context = nullptr;
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
	if (!bInitialized || !Context || !source || !destination || width <= 0 || height <= 0)
		return false;

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	const UINT stride = sizeof(FullscreenVertex);
	const UINT offset = 0;
	const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	Context->OMSetRenderTargets(1, &destination, depthStencil);
	Context->RSSetViewports(1, &viewport);
	Context->RSSetState(RasterizerState);
	Context->OMSetDepthStencilState(DepthStencilState, 0);
	Context->OMSetBlendState(BlendState, blendFactor, 0xffffffff);

	Context->IASetInputLayout(InputLayout);
	Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Context->IASetVertexBuffers(0, 1, &VertexBuffer, &stride, &offset);

	Context->VSSetShader(CopyVS, nullptr, 0);
	Context->PSSetShader(CopyPS, nullptr, 0);
	Context->PSSetSamplers(0, 1, &SamplerState);
	Context->PSSetShaderResources(0, 1, &source);

	Context->Draw(3, 0);

	ID3D11ShaderResourceView* nullSRV = nullptr;
	Context->PSSetShaderResources(0, 1, &nullSRV);
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

bool r3dDX11FullscreenPass::CreateShaders(ID3D11Device* device)
{
	r3dDX11ShaderCompiler compiler;

	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;

	if (!compiler.CompileVertexShader("Fullscreen_vs.hls", "main", nullptr, &vsBlob))
		return false;

	if (!compiler.CompilePixelShader("copy_ps.hls", "main", nullptr, &psBlob))
	{
		SafeReleaseDX11(vsBlob);
		return false;
	}

	HRESULT result = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &CopyVS);
	if (FAILED(result) || !CopyVS)
	{
		SafeReleaseDX11(psBlob);
		SafeReleaseDX11(vsBlob);
		return false;
	}

	result = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &CopyPS);
	if (FAILED(result) || !CopyPS)
	{
		SafeReleaseDX11(psBlob);
		SafeReleaseDX11(vsBlob);
		return false;
	}

	const D3D11_INPUT_ELEMENT_DESC inputElements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FullscreenVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FullscreenVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	result = device->CreateInputLayout(
		inputElements,
		static_cast<UINT>(_countof(inputElements)),
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		&InputLayout
	);

	SafeReleaseDX11(psBlob);
	SafeReleaseDX11(vsBlob);

	return SUCCEEDED(result) && InputLayout != nullptr;
}

bool r3dDX11FullscreenPass::CreateGeometry(ID3D11Device* device)
{
	const FullscreenVertex vertices[] =
	{
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
		{ { -1.0f,  3.0f, 0.0f }, { 0.0f, -1.0f } },
		{ {  3.0f, -1.0f, 0.0f }, { 2.0f, 1.0f } }
	};

	D3D11_BUFFER_DESC desc{};
	desc.ByteWidth = sizeof(vertices);
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA init{};
	init.pSysMem = vertices;

	HRESULT result = device->CreateBuffer(&desc, &init, &VertexBuffer);
	return SUCCEEDED(result) && VertexBuffer != nullptr;
}

bool r3dDX11FullscreenPass::CreateStates(ID3D11Device* device)
{
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT result = device->CreateSamplerState(&samplerDesc, &SamplerState);
	if (FAILED(result) || !SamplerState)
		return false;

	D3D11_RASTERIZER_DESC rasterDesc{};
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthClipEnable = TRUE;

	result = device->CreateRasterizerState(&rasterDesc, &RasterizerState);
	if (FAILED(result) || !RasterizerState)
		return false;

	D3D11_DEPTH_STENCIL_DESC depthDesc{};
	depthDesc.DepthEnable = FALSE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

	result = device->CreateDepthStencilState(&depthDesc, &DepthStencilState);
	if (FAILED(result) || !DepthStencilState)
		return false;

	D3D11_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	result = device->CreateBlendState(&blendDesc, &BlendState);
	return SUCCEEDED(result) && BlendState != nullptr;
}

