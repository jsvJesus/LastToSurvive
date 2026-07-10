#include "r3dPCH.h"
#include "r3d.h"

#include "rendering/DX11/RenderDX11Core.h"
#include "rendering/DX11/RenderDX11FrameTargets.h"

#if LTS_STUDIO_DX11

#include <stdio.h>

namespace
{
	template <typename T>
	void RenderDX11FrameTargets_SafeRelease(
		T*& Object
	)
	{
		if (!Object)
			return;

		Object->Release();
		Object = 0;
	}

	void RenderDX11FrameTargets_Log(
		const char* Text
	)
	{
		if (!Text)
			return;

		OutputDebugStringA(Text);
		r3dOutToLog("%s", Text);
	}
}

RenderDX11FrameTargets::RenderDX11FrameTargets()
	: GBufferColorTexture_(0)
	, GBufferNormalTexture_(0)
	, GBufferDepthLinearTexture_(0)
	, GBufferAuxTexture_(0)
	, SceneColorTexture_(0)
	, FinalColorTexture_(0)
	, DepthTexture_(0)
	, SmokeReadbackTexture_(0)
	, GBufferColorRTV_(0)
	, GBufferNormalRTV_(0)
	, GBufferDepthLinearRTV_(0)
	, GBufferAuxRTV_(0)
	, SceneColorRTV_(0)
	, FinalColorRTV_(0)
	, DepthDSV_(0)
	, GBufferColorSRV_(0)
	, GBufferNormalSRV_(0)
	, GBufferDepthLinearSRV_(0)
	, SceneColorSRV_(0)
	, FrameWidth_(0)
	, FrameHeight_(0)
	, FailureLogged_(false)
{
	Viewport_ = D3D11_VIEWPORT();
}

int RenderDX11FrameTargets::ClampSize(
	int Value
)
{
	if (Value < 1)
		return 1;

	if (Value > 16384)
		return 16384;

	return Value;
}

void RenderDX11FrameTargets::LogFailureOnce(
	const char* Stage,
	HRESULT Result
)
{
	if (FailureLogged_)
		return;

	FailureLogged_ = true;

	char Text[512] = {};

	sprintf_s(
		Text,
		"[DX11][FrameTargets] %s failed. HRESULT=0x%08X\n",
		Stage ? Stage : "Unknown operation",
		static_cast<unsigned int>(Result)
	);

	RenderDX11FrameTargets_Log(Text);
}

bool RenderDX11FrameTargets::CreateRenderTarget(
	int Width,
	int Height,
	DXGI_FORMAT Format,
	const char* DebugName,
	ID3D11Texture2D** OutTexture,
	ID3D11RenderTargetView** OutRTV,
	ID3D11ShaderResourceView** OutSRV
)
{
	if (!OutTexture || !OutRTV)
		return false;

	ID3D11Device* Device =
		RenderDX11_GetCore().GetDevice();

	if (!Device)
		return false;

	*OutTexture = 0;
	*OutRTV = 0;

	if (OutSRV)
		*OutSRV = 0;

	D3D11_TEXTURE2D_DESC TextureDesc = {};

	TextureDesc.Width =
		static_cast<UINT>(Width);

	TextureDesc.Height =
		static_cast<UINT>(Height);

	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = Format;

	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;

	TextureDesc.Usage =
		D3D11_USAGE_DEFAULT;

	TextureDesc.BindFlags =
		D3D11_BIND_RENDER_TARGET |
		D3D11_BIND_SHADER_RESOURCE;

	HRESULT Result =
		Device->CreateTexture2D(
			&TextureDesc,
			0,
			OutTexture
		);

	if (FAILED(Result))
	{
		char Stage[256] = {};

		sprintf_s(
			Stage,
			"Create %s texture",
			DebugName ? DebugName : "render target"
		);

		LogFailureOnce(Stage, Result);
		return false;
	}

	Result =
		Device->CreateRenderTargetView(
			*OutTexture,
			0,
			OutRTV
		);

	if (FAILED(Result))
	{
		char Stage[256] = {};

		sprintf_s(
			Stage,
			"Create %s RTV",
			DebugName ? DebugName : "render target"
		);

		LogFailureOnce(Stage, Result);
		return false;
	}

	if (OutSRV)
	{
		Result =
			Device->CreateShaderResourceView(
				*OutTexture,
				0,
				OutSRV
			);

		if (FAILED(Result))
		{
			char Stage[256] = {};

			sprintf_s(
				Stage,
				"Create %s SRV",
				DebugName ? DebugName : "render target"
			);

			LogFailureOnce(Stage, Result);
			return false;
		}
	}

	return true;
}

bool RenderDX11FrameTargets::CreateDepthTarget(
	int Width,
	int Height
)
{
	ID3D11Device* Device =
		RenderDX11_GetCore().GetDevice();

	if (!Device)
		return false;

	D3D11_TEXTURE2D_DESC TextureDesc = {};

	TextureDesc.Width =
		static_cast<UINT>(Width);

	TextureDesc.Height =
		static_cast<UINT>(Height);

	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;

	TextureDesc.Format =
		DXGI_FORMAT_D24_UNORM_S8_UINT;

	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;

	TextureDesc.Usage =
		D3D11_USAGE_DEFAULT;

	TextureDesc.BindFlags =
		D3D11_BIND_DEPTH_STENCIL;

	HRESULT Result =
		Device->CreateTexture2D(
			&TextureDesc,
			0,
			&DepthTexture_
		);

	if (FAILED(Result))
	{
		LogFailureOnce(
			"Create depth texture",
			Result
		);

		return false;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};

	DSVDesc.Format =
		DXGI_FORMAT_D24_UNORM_S8_UINT;

	DSVDesc.ViewDimension =
		D3D11_DSV_DIMENSION_TEXTURE2D;

	DSVDesc.Texture2D.MipSlice = 0;

	Result =
		Device->CreateDepthStencilView(
			DepthTexture_,
			&DSVDesc,
			&DepthDSV_
		);

	if (FAILED(Result))
	{
		LogFailureOnce(
			"Create depth DSV",
			Result
		);

		return false;
	}

	return true;
}

bool RenderDX11FrameTargets::CreateSmokeReadbackTarget(
	int Width,
	int Height
)
{
	ID3D11Device* Device =
		RenderDX11_GetCore().GetDevice();

	if (!Device)
		return false;

	D3D11_TEXTURE2D_DESC TextureDesc = {};

	TextureDesc.Width =
		static_cast<UINT>(Width);

	TextureDesc.Height =
		static_cast<UINT>(Height);

	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;

	TextureDesc.Format =
		DXGI_FORMAT_R8G8B8A8_UNORM;

	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.SampleDesc.Quality = 0;

	TextureDesc.Usage =
		D3D11_USAGE_STAGING;

	TextureDesc.BindFlags = 0;

	TextureDesc.CPUAccessFlags =
		D3D11_CPU_ACCESS_READ;

	TextureDesc.MiscFlags = 0;

	const HRESULT Result =
		Device->CreateTexture2D(
			&TextureDesc,
			0,
			&SmokeReadbackTexture_
		);

	if (FAILED(Result))
	{
		LogFailureOnce(
			"Create smoke readback texture",
			Result
		);

		return false;
	}

	return true;
}

bool RenderDX11FrameTargets::IsReady(
	bool SmokeReadbackRequired
) const
{
	if (
		!GBufferColorTexture_ ||
		!GBufferNormalTexture_ ||
		!GBufferDepthLinearTexture_ ||
		!GBufferAuxTexture_ ||
		!SceneColorTexture_ ||
		!FinalColorTexture_ ||
		!DepthTexture_ ||
		!GBufferColorRTV_ ||
		!GBufferNormalRTV_ ||
		!GBufferDepthLinearRTV_ ||
		!GBufferAuxRTV_ ||
		!SceneColorRTV_ ||
		!FinalColorRTV_ ||
		!DepthDSV_ ||
		!GBufferColorSRV_ ||
		!GBufferNormalSRV_ ||
		!GBufferDepthLinearSRV_ ||
		!SceneColorSRV_ ||
		FrameWidth_ <= 0 ||
		FrameHeight_ <= 0
	)
	{
		return false;
	}

	if (
		SmokeReadbackRequired &&
		!SmokeReadbackTexture_
	)
	{
		return false;
	}

	return true;
}

bool RenderDX11FrameTargets::Ensure(
	int Width,
	int Height,
	bool CreateSmokeReadback
)
{
	Width = ClampSize(Width);
	Height = ClampSize(Height);

	if (
		FrameWidth_ == Width &&
		FrameHeight_ == Height &&
		IsReady(CreateSmokeReadback)
	)
	{
		return true;
	}

	Release();

	if (!RenderDX11_GetCore().IsReady())
	{
		LogFailureOnce(
			"Ensure called without initialized DX11 core",
			E_FAIL
		);

		return false;
	}

	if (!CreateRenderTarget(
		Width,
		Height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		"GBufferColor",
		&GBufferColorTexture_,
		&GBufferColorRTV_,
		&GBufferColorSRV_
	))
	{
		Release();
		return false;
	}

	if (!CreateRenderTarget(
		Width,
		Height,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		"GBufferNormal",
		&GBufferNormalTexture_,
		&GBufferNormalRTV_,
		&GBufferNormalSRV_
	))
	{
		Release();
		return false;
	}

	if (!CreateRenderTarget(
		Width,
		Height,
		DXGI_FORMAT_R32_FLOAT,
		"GBufferDepthLinear",
		&GBufferDepthLinearTexture_,
		&GBufferDepthLinearRTV_,
		&GBufferDepthLinearSRV_
	))
	{
		Release();
		return false;
	}

	if (!CreateRenderTarget(
		Width,
		Height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		"GBufferAux",
		&GBufferAuxTexture_,
		&GBufferAuxRTV_,
		0
	))
	{
		Release();
		return false;
	}

	if (!CreateRenderTarget(
		Width,
		Height,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		"SceneColor",
		&SceneColorTexture_,
		&SceneColorRTV_,
		&SceneColorSRV_
	))
	{
		Release();
		return false;
	}

	if (!CreateRenderTarget(
		Width,
		Height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		"FinalColor",
		&FinalColorTexture_,
		&FinalColorRTV_,
		0
	))
	{
		Release();
		return false;
	}

	if (!CreateDepthTarget(
		Width,
		Height
	))
	{
		Release();
		return false;
	}

	if (
		CreateSmokeReadback &&
		!CreateSmokeReadbackTarget(
			Width,
			Height
		)
	)
	{
		Release();
		return false;
	}

	Viewport_.TopLeftX = 0.0f;
	Viewport_.TopLeftY = 0.0f;

	Viewport_.Width =
		static_cast<float>(Width);

	Viewport_.Height =
		static_cast<float>(Height);

	Viewport_.MinDepth = 0.0f;
	Viewport_.MaxDepth = 1.0f;

	FrameWidth_ = Width;
	FrameHeight_ = Height;

	FailureLogged_ = false;

	char Text[256] = {};

	sprintf_s(
		Text,
		"[DX11][FrameTargets] Ready %dx%d\n",
		Width,
		Height
	);

	RenderDX11FrameTargets_Log(Text);

	return true;
}

void RenderDX11FrameTargets::BindGBuffer()
{
	ID3D11DeviceContext* Context =
		RenderDX11_GetCore().GetContext();

	if (
		!Context ||
		!IsReady(false)
	)
	{
		return;
	}

	ID3D11RenderTargetView* RenderTargets[4] =
	{
		GBufferColorRTV_,
		GBufferNormalRTV_,
		GBufferDepthLinearRTV_,
		GBufferAuxRTV_
	};

	Context->OMSetRenderTargets(
		4,
		RenderTargets,
		DepthDSV_
	);

	Context->RSSetViewports(
		1,
		&Viewport_
	);
}

void RenderDX11FrameTargets::Clear()
{
	ID3D11DeviceContext* Context =
		RenderDX11_GetCore().GetContext();

	if (
		!Context ||
		!IsReady(false)
	)
	{
		return;
	}

	const float ClearColorAlbedo[4] =
	{
		0.02f,
		0.04f,
		0.06f,
		1.0f
	};

	const float ClearNormal[4] =
	{
		0.5f,
		0.5f,
		1.0f,
		1.0f
	};

	const float ClearDepthLinear[4] =
	{
		1.0f,
		0.0f,
		0.0f,
		0.0f
	};

	const float ClearAux[4] =
	{
		0.0f,
		0.0f,
		0.0f,
		0.0f
	};

	Context->ClearRenderTargetView(
		GBufferColorRTV_,
		ClearColorAlbedo
	);

	Context->ClearRenderTargetView(
		SceneColorRTV_,
		ClearColorAlbedo
	);

	Context->ClearRenderTargetView(
		FinalColorRTV_,
		ClearColorAlbedo
	);

	Context->ClearRenderTargetView(
		GBufferNormalRTV_,
		ClearNormal
	);

	Context->ClearRenderTargetView(
		GBufferDepthLinearRTV_,
		ClearDepthLinear
	);

	Context->ClearRenderTargetView(
		GBufferAuxRTV_,
		ClearAux
	);

	Context->ClearDepthStencilView(
		DepthDSV_,
		D3D11_CLEAR_DEPTH |
		D3D11_CLEAR_STENCIL,
		1.0f,
		0
	);
}

void RenderDX11FrameTargets::Unbind()
{
	ID3D11DeviceContext* Context =
		RenderDX11_GetCore().GetContext();

	if (!Context)
		return;

	ID3D11RenderTargetView* NullRTV[4] =
	{
		0,
		0,
		0,
		0
	};

	Context->OMSetRenderTargets(
		4,
		NullRTV,
		0
	);

	ID3D11ShaderResourceView* NullSRV[8] =
	{
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0
	};

	Context->PSSetShaderResources(
		0,
		8,
		NullSRV
	);
}

void RenderDX11FrameTargets::Release()
{
	Unbind();

	RenderDX11FrameTargets_SafeRelease(
		GBufferAuxRTV_
	);

	RenderDX11FrameTargets_SafeRelease(
		GBufferDepthLinearRTV_
	);

	RenderDX11FrameTargets_SafeRelease(
		GBufferNormalRTV_
	);

	RenderDX11FrameTargets_SafeRelease(
		GBufferColorRTV_
	);

	RenderDX11FrameTargets_SafeRelease(
		SceneColorRTV_
	);

	RenderDX11FrameTargets_SafeRelease(
		FinalColorRTV_
	);

	RenderDX11FrameTargets_SafeRelease(
		DepthDSV_
	);

	RenderDX11FrameTargets_SafeRelease(
		SceneColorSRV_
	);

	RenderDX11FrameTargets_SafeRelease(
		GBufferDepthLinearSRV_
	);

	RenderDX11FrameTargets_SafeRelease(
		GBufferNormalSRV_
	);

	RenderDX11FrameTargets_SafeRelease(
		GBufferColorSRV_
	);

	RenderDX11FrameTargets_SafeRelease(
		SmokeReadbackTexture_
	);

	RenderDX11FrameTargets_SafeRelease(
		GBufferAuxTexture_
	);

	RenderDX11FrameTargets_SafeRelease(
		GBufferDepthLinearTexture_
	);

	RenderDX11FrameTargets_SafeRelease(
		GBufferNormalTexture_
	);

	RenderDX11FrameTargets_SafeRelease(
		GBufferColorTexture_
	);

	RenderDX11FrameTargets_SafeRelease(
		SceneColorTexture_
	);

	RenderDX11FrameTargets_SafeRelease(
		FinalColorTexture_
	);

	RenderDX11FrameTargets_SafeRelease(
		DepthTexture_
	);

	Viewport_ = D3D11_VIEWPORT();

	FrameWidth_ = 0;
	FrameHeight_ = 0;
}

RenderDX11FrameTargets& RenderDX11_GetFrameTargets()
{
	static RenderDX11FrameTargets FrameTargets;
	return FrameTargets;
}

#endif // LTS_STUDIO_DX11