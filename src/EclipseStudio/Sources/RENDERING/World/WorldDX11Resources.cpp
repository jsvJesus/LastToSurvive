#include "r3dPCH.h"
#include "r3d.h"

#include "rendering/World/WorldDX11Device.h"
#include "rendering/World/WorldDX11Resources.h"

#if LTS_STUDIO_DX11

#include <stdio.h>

namespace
{
	ID3D11Texture2D* GColorTexture = 0;
	ID3D11Texture2D* GDepthTexture = 0;

	ID3D11RenderTargetView* GColorRTV = 0;
	ID3D11DepthStencilView* GDepthDSV = 0;

	D3D11_VIEWPORT GViewport = {};

	WorldDX11ResourcesState GState = {};

	template <typename T>
	void SafeReleaseDX11(T*& Ptr)
	{
		if (Ptr)
		{
			Ptr->Release();
			Ptr = 0;
		}
	}

	int ClampSize(int Value)
	{
		if (Value < 1)
			return 1;

		if (Value > 16384)
			return 16384;

		return Value;
	}

	DXGI_FORMAT NormalizeDepthFormat(DXGI_FORMAT Format)
	{
		if (Format == DXGI_FORMAT_UNKNOWN)
			return DXGI_FORMAT_D24_UNORM_S8_UINT;

		return Format;
	}

	DXGI_FORMAT NormalizeColorFormat(DXGI_FORMAT Format)
	{
		if (Format == DXGI_FORMAT_UNKNOWN)
			return DXGI_FORMAT_R8G8B8A8_UNORM;

		return Format;
	}

	void ResetState()
	{
		GState.Width = 0;
		GState.Height = 0;
		GState.ColorFormat = DXGI_FORMAT_UNKNOWN;
		GState.DepthFormat = DXGI_FORMAT_UNKNOWN;
		GState.bInitialized = false;

		GViewport = D3D11_VIEWPORT();
	}

	void ReleaseResources()
	{
		ID3D11DeviceContext* Context =
			WorldDX11Device_GetContext();

		if (Context)
		{
			ID3D11RenderTargetView* NullRTV[1] = { 0 };
			Context->OMSetRenderTargets(
				1,
				NullRTV,
				0
			);
		}

		SafeReleaseDX11(GColorRTV);
		SafeReleaseDX11(GDepthDSV);

		SafeReleaseDX11(GColorTexture);
		SafeReleaseDX11(GDepthTexture);

		ResetState();
	}

	bool CreateColorTarget(
		ID3D11Device* Device,
		int Width,
		int Height,
		DXGI_FORMAT Format
	)
	{
		D3D11_TEXTURE2D_DESC TextureDesc = {};
		TextureDesc.Width = static_cast<UINT>(Width);
		TextureDesc.Height = static_cast<UINT>(Height);
		TextureDesc.MipLevels = 1;
		TextureDesc.ArraySize = 1;
		TextureDesc.Format = Format;
		TextureDesc.SampleDesc.Count = 1;
		TextureDesc.SampleDesc.Quality = 0;
		TextureDesc.Usage = D3D11_USAGE_DEFAULT;
		TextureDesc.BindFlags =
			D3D11_BIND_RENDER_TARGET |
			D3D11_BIND_SHADER_RESOURCE;

		HRESULT Hr =
			Device->CreateTexture2D(
				&TextureDesc,
				0,
				&GColorTexture
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[WorldDX11Resources] Create color texture failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		Hr =
			Device->CreateRenderTargetView(
				GColorTexture,
				0,
				&GColorRTV
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[WorldDX11Resources] Create color RTV failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		return true;
	}

	bool CreateDepthTarget(
		ID3D11Device* Device,
		int Width,
		int Height,
		DXGI_FORMAT Format
	)
	{
		D3D11_TEXTURE2D_DESC TextureDesc = {};
		TextureDesc.Width = static_cast<UINT>(Width);
		TextureDesc.Height = static_cast<UINT>(Height);
		TextureDesc.MipLevels = 1;
		TextureDesc.ArraySize = 1;
		TextureDesc.Format = Format;
		TextureDesc.SampleDesc.Count = 1;
		TextureDesc.SampleDesc.Quality = 0;
		TextureDesc.Usage = D3D11_USAGE_DEFAULT;
		TextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		HRESULT Hr =
			Device->CreateTexture2D(
				&TextureDesc,
				0,
				&GDepthTexture
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[WorldDX11Resources] Create depth texture failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format = Format;
		DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		DSVDesc.Texture2D.MipSlice = 0;

		Hr =
			Device->CreateDepthStencilView(
				GDepthTexture,
				&DSVDesc,
				&GDepthDSV
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[WorldDX11Resources] Create depth DSV failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		return true;
	}
}

bool WorldDX11Resources_Init(
	const WorldDX11ResourcesDesc& Desc
)
{
	return WorldDX11Resources_Resize(Desc);
}

void WorldDX11Resources_Shutdown()
{
	ReleaseResources();

	OutputDebugStringA(
		"[WorldDX11Resources] Shutdown\n"
	);
}

bool WorldDX11Resources_Resize(
	const WorldDX11ResourcesDesc& Desc
)
{
	if (!WorldDX11Device_IsInitialized())
	{
		OutputDebugStringA(
			"[WorldDX11Resources] Resize failed: device is not initialized\n"
		);

		return false;
	}

	const int Width =
		ClampSize(Desc.Width);

	const int Height =
		ClampSize(Desc.Height);

	const DXGI_FORMAT ColorFormat =
		NormalizeColorFormat(Desc.ColorFormat);

	const DXGI_FORMAT DepthFormat =
		NormalizeDepthFormat(Desc.DepthFormat);

	if (
		GState.bInitialized &&
		GState.Width == Width &&
		GState.Height == Height &&
		GState.ColorFormat == ColorFormat &&
		GState.DepthFormat == DepthFormat
	)
	{
		return true;
	}

	ReleaseResources();

	ID3D11Device* Device =
		WorldDX11Device_GetDevice();

	if (!Device)
	{
		OutputDebugStringA(
			"[WorldDX11Resources] Resize failed: null D3D11 device\n"
		);

		return false;
	}

	if (!CreateColorTarget(
		Device,
		Width,
		Height,
		ColorFormat
	))
	{
		ReleaseResources();
		return false;
	}

	if (!CreateDepthTarget(
		Device,
		Width,
		Height,
		DepthFormat
	))
	{
		ReleaseResources();
		return false;
	}

	GViewport.TopLeftX = 0.0f;
	GViewport.TopLeftY = 0.0f;
	GViewport.Width = static_cast<float>(Width);
	GViewport.Height = static_cast<float>(Height);
	GViewport.MinDepth = 0.0f;
	GViewport.MaxDepth = 1.0f;

	GState.Width = Width;
	GState.Height = Height;
	GState.ColorFormat = ColorFormat;
	GState.DepthFormat = DepthFormat;
	GState.bInitialized = true;

	char Text[256] = {};
	sprintf_s(
		Text,
		"[WorldDX11Resources] Ready %dx%d\n",
		Width,
		Height
	);

	OutputDebugStringA(Text);

	return true;
}

bool WorldDX11Resources_IsInitialized()
{
	return
		GState.bInitialized &&
		GColorTexture &&
		GDepthTexture &&
		GColorRTV &&
		GDepthDSV;
}

ID3D11Texture2D* WorldDX11Resources_GetColorTexture()
{
	return GColorTexture;
}

ID3D11Texture2D* WorldDX11Resources_GetDepthTexture()
{
	return GDepthTexture;
}

ID3D11RenderTargetView* WorldDX11Resources_GetColorRTV()
{
	return GColorRTV;
}

ID3D11DepthStencilView* WorldDX11Resources_GetDepthDSV()
{
	return GDepthDSV;
}

const D3D11_VIEWPORT& WorldDX11Resources_GetViewport()
{
	return GViewport;
}

const WorldDX11ResourcesState& WorldDX11Resources_GetState()
{
	return GState;
}

void WorldDX11Resources_Bind()
{
	if (!WorldDX11Resources_IsInitialized())
		return;

	ID3D11DeviceContext* Context =
		WorldDX11Device_GetContext();

	if (!Context)
		return;

	ID3D11RenderTargetView* RTViews[1] =
	{
		GColorRTV
	};

	Context->OMSetRenderTargets(
		1,
		RTViews,
		GDepthDSV
	);

	Context->RSSetViewports(
		1,
		&GViewport
	);
}

void WorldDX11Resources_Clear(
	float R,
	float G,
	float B,
	float A
)
{
	if (!WorldDX11Resources_IsInitialized())
		return;

	ID3D11DeviceContext* Context =
		WorldDX11Device_GetContext();

	if (!Context)
		return;

	const float ClearColor[4] =
	{
		R,
		G,
		B,
		A
	};

	Context->ClearRenderTargetView(
		GColorRTV,
		ClearColor
	);

	Context->ClearDepthStencilView(
		GDepthDSV,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0
	);
}

#endif // LTS_STUDIO_DX11