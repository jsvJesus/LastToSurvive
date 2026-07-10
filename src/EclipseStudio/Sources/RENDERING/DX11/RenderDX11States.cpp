#include "r3dPCH.h"
#include "r3d.h"

#include "rendering/DX11/RenderDX11Core.h"
#include "rendering/DX11/RenderDX11States.h"

#if LTS_STUDIO_DX11

#include <stdio.h>

namespace
{
	template <typename T>
	void RenderDX11States_SafeRelease(
		T*& Object
	)
	{
		if (!Object)
			return;

		Object->Release();
		Object = 0;
	}

	void RenderDX11States_Log(
		const char* Text
	)
	{
		if (!Text)
			return;

		OutputDebugStringA(Text);
		r3dOutToLog("%s", Text);
	}
}

RenderDX11States::RenderDX11States()
	: DepthWriteLessEqual_(0)
	, DepthReadLessEqual_(0)
	, DepthDisabled_(0)
	, RasterSolidBackCull_(0)
	, RasterSolidNoCull_(0)
	, BlendOpaque_(0)
	, BlendAlpha_(0)
	, SamplerLinearWrap_(0)
	, SamplerLinearClamp_(0)
	, SunGlareBorderSampler_(0)
{
}

void RenderDX11States::LogFailure(
	const char* Stage,
	HRESULT Result
)
{
	char Text[512] = {};

	sprintf_s(
		Text,
		"[DX11][States] %s failed. HRESULT=0x%08X\n",
		Stage ? Stage : "Unknown operation",
		static_cast<unsigned int>(Result)
	);

	RenderDX11States_Log(Text);
}

bool RenderDX11States::CreateDepthStates()
{
	ID3D11Device* Device =
		RenderDX11_GetCore().GetDevice();

	if (!Device)
		return false;

	D3D11_DEPTH_STENCIL_DESC Desc = {};

	Desc.DepthEnable = TRUE;
	Desc.DepthWriteMask =
		D3D11_DEPTH_WRITE_MASK_ALL;

	Desc.DepthFunc =
		D3D11_COMPARISON_LESS_EQUAL;

	Desc.StencilEnable = FALSE;

	HRESULT Result =
		Device->CreateDepthStencilState(
			&Desc,
			&DepthWriteLessEqual_
		);

	if (FAILED(Result))
	{
		LogFailure(
			"Create depth-write-less-equal state",
			Result
		);

		return false;
	}

	Desc.DepthWriteMask =
		D3D11_DEPTH_WRITE_MASK_ZERO;

	Result =
		Device->CreateDepthStencilState(
			&Desc,
			&DepthReadLessEqual_
		);

	if (FAILED(Result))
	{
		LogFailure(
			"Create depth-read-less-equal state",
			Result
		);

		return false;
	}

	Desc.DepthEnable = FALSE;

	Desc.DepthWriteMask =
		D3D11_DEPTH_WRITE_MASK_ZERO;

	Desc.DepthFunc =
		D3D11_COMPARISON_ALWAYS;

	Result =
		Device->CreateDepthStencilState(
			&Desc,
			&DepthDisabled_
		);

	if (FAILED(Result))
	{
		LogFailure(
			"Create depth-disabled state",
			Result
		);

		return false;
	}

	return true;
}

bool RenderDX11States::CreateRasterizerStates()
{
	ID3D11Device* Device =
		RenderDX11_GetCore().GetDevice();

	if (!Device)
		return false;

	D3D11_RASTERIZER_DESC Desc = {};

	Desc.FillMode =
		D3D11_FILL_SOLID;

	Desc.CullMode =
		D3D11_CULL_BACK;

	Desc.FrontCounterClockwise = FALSE;

	Desc.DepthBias = 0;
	Desc.DepthBiasClamp = 0.0f;
	Desc.SlopeScaledDepthBias = 0.0f;

	Desc.DepthClipEnable = TRUE;
	Desc.ScissorEnable = FALSE;
	Desc.MultisampleEnable = FALSE;
	Desc.AntialiasedLineEnable = FALSE;

	HRESULT Result =
		Device->CreateRasterizerState(
			&Desc,
			&RasterSolidBackCull_
		);

	if (FAILED(Result))
	{
		LogFailure(
			"Create solid-back-cull rasterizer state",
			Result
		);

		return false;
	}

	Desc.CullMode =
		D3D11_CULL_NONE;

	Result =
		Device->CreateRasterizerState(
			&Desc,
			&RasterSolidNoCull_
		);

	if (FAILED(Result))
	{
		LogFailure(
			"Create solid-no-cull rasterizer state",
			Result
		);

		return false;
	}

	return true;
}

bool RenderDX11States::CreateBlendStates()
{
	ID3D11Device* Device =
		RenderDX11_GetCore().GetDevice();

	if (!Device)
		return false;

	D3D11_BLEND_DESC Desc = {};

	Desc.AlphaToCoverageEnable = FALSE;
	Desc.IndependentBlendEnable = FALSE;

	D3D11_RENDER_TARGET_BLEND_DESC& Target =
		Desc.RenderTarget[0];

	Target.BlendEnable = FALSE;

	Target.SrcBlend =
		D3D11_BLEND_ONE;

	Target.DestBlend =
		D3D11_BLEND_ZERO;

	Target.BlendOp =
		D3D11_BLEND_OP_ADD;

	Target.SrcBlendAlpha =
		D3D11_BLEND_ONE;

	Target.DestBlendAlpha =
		D3D11_BLEND_ZERO;

	Target.BlendOpAlpha =
		D3D11_BLEND_OP_ADD;

	Target.RenderTargetWriteMask =
		D3D11_COLOR_WRITE_ENABLE_ALL;

	HRESULT Result =
		Device->CreateBlendState(
			&Desc,
			&BlendOpaque_
		);

	if (FAILED(Result))
	{
		LogFailure(
			"Create opaque blend state",
			Result
		);

		return false;
	}

	Target.BlendEnable = TRUE;

	Target.SrcBlend =
		D3D11_BLEND_SRC_ALPHA;

	Target.DestBlend =
		D3D11_BLEND_INV_SRC_ALPHA;

	Target.BlendOp =
		D3D11_BLEND_OP_ADD;

	Target.SrcBlendAlpha =
		D3D11_BLEND_ONE;

	Target.DestBlendAlpha =
		D3D11_BLEND_INV_SRC_ALPHA;

	Target.BlendOpAlpha =
		D3D11_BLEND_OP_ADD;

	Result =
		Device->CreateBlendState(
			&Desc,
			&BlendAlpha_
		);

	if (FAILED(Result))
	{
		LogFailure(
			"Create alpha blend state",
			Result
		);

		return false;
	}

	return true;
}

bool RenderDX11States::CreateSamplerStates()
{
	ID3D11Device* Device =
		RenderDX11_GetCore().GetDevice();

	if (!Device)
		return false;

	D3D11_SAMPLER_DESC Desc = {};

	Desc.Filter =
		D3D11_FILTER_ANISOTROPIC;

	Desc.AddressU =
		D3D11_TEXTURE_ADDRESS_WRAP;

	Desc.AddressV =
		D3D11_TEXTURE_ADDRESS_WRAP;

	Desc.AddressW =
		D3D11_TEXTURE_ADDRESS_WRAP;

	Desc.MipLODBias = 0.0f;
	Desc.MaxAnisotropy = 8;

	Desc.ComparisonFunc =
		D3D11_COMPARISON_NEVER;

	Desc.BorderColor[0] = 0.0f;
	Desc.BorderColor[1] = 0.0f;
	Desc.BorderColor[2] = 0.0f;
	Desc.BorderColor[3] = 0.0f;

	Desc.MinLOD = 0.0f;
	Desc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT Result =
		Device->CreateSamplerState(
			&Desc,
			&SamplerLinearWrap_
		);

	if (FAILED(Result))
	{
		LogFailure(
			"Create anisotropic-wrap sampler",
			Result
		);

		return false;
	}

	Desc.AddressU =
		D3D11_TEXTURE_ADDRESS_CLAMP;

	Desc.AddressV =
		D3D11_TEXTURE_ADDRESS_CLAMP;

	Desc.AddressW =
		D3D11_TEXTURE_ADDRESS_CLAMP;

	Result =
		Device->CreateSamplerState(
			&Desc,
			&SamplerLinearClamp_
		);

	if (FAILED(Result))
	{
		LogFailure(
			"Create anisotropic-clamp sampler",
			Result
		);

		return false;
	}

	Desc.Filter =
		D3D11_FILTER_MIN_MAG_MIP_LINEAR;

	Desc.AddressU =
		D3D11_TEXTURE_ADDRESS_BORDER;

	Desc.AddressV =
		D3D11_TEXTURE_ADDRESS_BORDER;

	Desc.AddressW =
		D3D11_TEXTURE_ADDRESS_BORDER;

	Desc.MipLODBias = 0.0f;
	Desc.MaxAnisotropy = 1;

	Desc.ComparisonFunc =
		D3D11_COMPARISON_NEVER;

	Desc.BorderColor[0] = 0.0f;
	Desc.BorderColor[1] = 0.0f;
	Desc.BorderColor[2] = 0.0f;
	Desc.BorderColor[3] = 0.0f;

	Desc.MinLOD = 0.0f;
	Desc.MaxLOD = D3D11_FLOAT32_MAX;

	Result =
		Device->CreateSamplerState(
			&Desc,
			&SunGlareBorderSampler_
		);

	if (FAILED(Result))
	{
		LogFailure(
			"Create SunGlare border sampler",
			Result
		);

		return false;
	}

	return true;
}

bool RenderDX11States::Initialize()
{
	if (IsReady())
		return true;

	Shutdown();

	if (!RenderDX11_GetCore().IsReady())
	{
		RenderDX11States_Log(
			"[DX11][States] Cannot initialize without DX11 core\n"
		);

		return false;
	}

	if (!CreateDepthStates())
	{
		Shutdown();
		return false;
	}

	if (!CreateRasterizerStates())
	{
		Shutdown();
		return false;
	}

	if (!CreateBlendStates())
	{
		Shutdown();
		return false;
	}

	if (!CreateSamplerStates())
	{
		Shutdown();
		return false;
	}

	RenderDX11States_Log(
		"[DX11][States] Render states created\n"
	);

	return true;
}

void RenderDX11States::Shutdown()
{
	RenderDX11States_SafeRelease(
		SunGlareBorderSampler_
	);

	RenderDX11States_SafeRelease(
		SamplerLinearClamp_
	);

	RenderDX11States_SafeRelease(
		SamplerLinearWrap_
	);

	RenderDX11States_SafeRelease(
		BlendAlpha_
	);

	RenderDX11States_SafeRelease(
		BlendOpaque_
	);

	RenderDX11States_SafeRelease(
		RasterSolidNoCull_
	);

	RenderDX11States_SafeRelease(
		RasterSolidBackCull_
	);

	RenderDX11States_SafeRelease(
		DepthDisabled_
	);

	RenderDX11States_SafeRelease(
		DepthReadLessEqual_
	);

	RenderDX11States_SafeRelease(
		DepthWriteLessEqual_
	);
}

bool RenderDX11States::IsReady() const
{
	return
		DepthWriteLessEqual_ != 0 &&
		DepthReadLessEqual_ != 0 &&
		DepthDisabled_ != 0 &&
		RasterSolidBackCull_ != 0 &&
		RasterSolidNoCull_ != 0 &&
		BlendOpaque_ != 0 &&
		BlendAlpha_ != 0 &&
		SamplerLinearWrap_ != 0 &&
		SamplerLinearClamp_ != 0 &&
		SunGlareBorderSampler_ != 0;
}

void RenderDX11States::ApplyDefaults()
{
	ID3D11DeviceContext* Context =
		RenderDX11_GetCore().GetContext();

	if (
		!Context ||
		!IsReady()
	)
	{
		return;
	}

	Context->OMSetDepthStencilState(
		DepthWriteLessEqual_,
		0
	);

	const float BlendFactor[4] =
	{
		0.0f,
		0.0f,
		0.0f,
		0.0f
	};

	Context->OMSetBlendState(
		BlendOpaque_,
		BlendFactor,
		0xffffffff
	);

	Context->RSSetState(
		RasterSolidBackCull_
	);

	ID3D11SamplerState* Samplers[1] =
	{
		SamplerLinearWrap_
	};

	Context->PSSetSamplers(
		0,
		1,
		Samplers
	);
}

RenderDX11States& RenderDX11_GetStates()
{
	static RenderDX11States States;
	return States;
}

#endif // LTS_STUDIO_DX11