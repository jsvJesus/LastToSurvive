#include "r3dPCH.h"

#include "RENDERING/DX11/RenderDX11States.h"

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
}

r3dDX11CommonStates::r3dDX11CommonStates()
{
}

r3dDX11CommonStates::~r3dDX11CommonStates()
{
	Shutdown();
}

bool r3dDX11CommonStates::Init(ID3D11Device* device)
{
	if (bInitialized)
		return true;

	if (!device)
		return false;

	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	if (FAILED(device->CreateSamplerState(&samplerDesc, &LinearClampSampler)))
	{
		Shutdown();
		return false;
	}

	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	if (FAILED(device->CreateSamplerState(&samplerDesc, &LinearWrapSampler)))
	{
		Shutdown();
		return false;
	}

	D3D11_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	if (FAILED(device->CreateBlendState(&blendDesc, &OpaqueBlendState)))
	{
		Shutdown();
		return false;
	}

	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	if (FAILED(device->CreateBlendState(&blendDesc, &AlphaBlendState)))
	{
		Shutdown();
		return false;
	}

	D3D11_DEPTH_STENCIL_DESC depthDesc{};
	depthDesc.DepthEnable = FALSE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	if (FAILED(device->CreateDepthStencilState(&depthDesc, &DepthDisabledState)))
	{
		Shutdown();
		return false;
	}

	depthDesc.DepthEnable = TRUE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	if (FAILED(device->CreateDepthStencilState(&depthDesc, &DepthReadWriteState)))
	{
		Shutdown();
		return false;
	}

	D3D11_RASTERIZER_DESC rasterDesc{};
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthClipEnable = TRUE;
	if (FAILED(device->CreateRasterizerState(&rasterDesc, &CullBackRasterizer)))
	{
		Shutdown();
		return false;
	}

	rasterDesc.CullMode = D3D11_CULL_NONE;
	if (FAILED(device->CreateRasterizerState(&rasterDesc, &CullNoneRasterizer)))
	{
		Shutdown();
		return false;
	}

	bInitialized = true;
	return true;
}

void r3dDX11CommonStates::Shutdown()
{
	SafeReleaseDX11(CullNoneRasterizer);
	SafeReleaseDX11(CullBackRasterizer);
	SafeReleaseDX11(DepthReadWriteState);
	SafeReleaseDX11(DepthDisabledState);
	SafeReleaseDX11(AlphaBlendState);
	SafeReleaseDX11(OpaqueBlendState);
	SafeReleaseDX11(LinearWrapSampler);
	SafeReleaseDX11(LinearClampSampler);
	bInitialized = false;
}

ID3D11SamplerState* r3dDX11CommonStates::GetLinearClampSampler() const
{
	return LinearClampSampler;
}

ID3D11SamplerState* r3dDX11CommonStates::GetLinearWrapSampler() const
{
	return LinearWrapSampler;
}

ID3D11BlendState* r3dDX11CommonStates::GetOpaqueBlendState() const
{
	return OpaqueBlendState;
}

ID3D11BlendState* r3dDX11CommonStates::GetAlphaBlendState() const
{
	return AlphaBlendState;
}

ID3D11DepthStencilState* r3dDX11CommonStates::GetDepthDisabledState() const
{
	return DepthDisabledState;
}

ID3D11DepthStencilState* r3dDX11CommonStates::GetDepthReadWriteState() const
{
	return DepthReadWriteState;
}

ID3D11RasterizerState* r3dDX11CommonStates::GetCullBackRasterizer() const
{
	return CullBackRasterizer;
}

ID3D11RasterizerState* r3dDX11CommonStates::GetCullNoneRasterizer() const
{
	return CullNoneRasterizer;
}

bool r3dDX11CommonStates::IsInitialized() const
{
	return bInitialized;
}
