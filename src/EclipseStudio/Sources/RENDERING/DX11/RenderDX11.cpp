#include "r3dPCH.h"
#include "r3d.h"

#include "r3dRendererConfig.h"

#if LTS_STUDIO_DX11

#include <D3D11.h>
#include <DXGI.h>
#include <D3Dcompiler.h>
#include <D3DX11.h>

#include <stdio.h>

#include "GameCommon.h"
#include "rendering/World/WorldDX11.h"

namespace
{
	ID3D11Device*			gDX11Device = 0;
	ID3D11DeviceContext*	gDX11Context = 0;

	ID3D11Texture2D*		gDX11ColorTexture = 0;
	ID3D11Texture2D*		gDX11DepthTexture = 0;

	ID3D11RenderTargetView*	gDX11ColorRTV = 0;
	ID3D11DepthStencilView*	gDX11DepthDSV = 0;

	D3D11_VIEWPORT			gDX11Viewport = {};

	int						gDX11FrameWidth = 0;
	int						gDX11FrameHeight = 0;

	D3D_FEATURE_LEVEL		gDX11FeatureLevel = D3D_FEATURE_LEVEL_10_0;

	bool					gDX11Initialized = false;

	template <typename T>
	void RenderDX11_SafeRelease(T*& Ptr)
	{
		if (Ptr)
		{
			Ptr->Release();
			Ptr = 0;
		}
	}

	const char* RenderDX11_FeatureLevelToString(
		D3D_FEATURE_LEVEL FeatureLevel
	)
	{
		switch (FeatureLevel)
		{
		case D3D_FEATURE_LEVEL_11_0:
			return "11_0";

		case D3D_FEATURE_LEVEL_10_1:
			return "10_1";

		case D3D_FEATURE_LEVEL_10_0:
			return "10_0";

		default:
			return "unknown";
		}
	}

	int RenderDX11_ClampSize(int Value)
	{
		if (Value < 1)
			return 1;

		if (Value > 16384)
			return 16384;

		return Value;
	}

	void RenderDX11_ReleaseFrameTargets()
	{
		if (gDX11Context)
		{
			ID3D11RenderTargetView* NullRTV[1] =
			{
				0
			};

			gDX11Context->OMSetRenderTargets(
				1,
				NullRTV,
				0
			);
		}

		RenderDX11_SafeRelease(gDX11ColorRTV);
		RenderDX11_SafeRelease(gDX11DepthDSV);

		RenderDX11_SafeRelease(gDX11ColorTexture);
		RenderDX11_SafeRelease(gDX11DepthTexture);

		gDX11FrameWidth = 0;
		gDX11FrameHeight = 0;

		gDX11Viewport = D3D11_VIEWPORT();
	}

	bool RenderDX11_CreateColorTarget(
		int Width,
		int Height
	)
	{
		D3D11_TEXTURE2D_DESC TextureDesc = {};
		TextureDesc.Width = static_cast<UINT>(Width);
		TextureDesc.Height = static_cast<UINT>(Height);
		TextureDesc.MipLevels = 1;
		TextureDesc.ArraySize = 1;
		TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		TextureDesc.SampleDesc.Count = 1;
		TextureDesc.SampleDesc.Quality = 0;
		TextureDesc.Usage = D3D11_USAGE_DEFAULT;
		TextureDesc.BindFlags =
			D3D11_BIND_RENDER_TARGET |
			D3D11_BIND_SHADER_RESOURCE;

		HRESULT Hr =
			gDX11Device->CreateTexture2D(
				&TextureDesc,
				0,
				&gDX11ColorTexture
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create color texture failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		Hr =
			gDX11Device->CreateRenderTargetView(
				gDX11ColorTexture,
				0,
				&gDX11ColorRTV
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create color RTV failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		return true;
	}

	bool RenderDX11_CreateDepthTarget(
		int Width,
		int Height
	)
	{
		D3D11_TEXTURE2D_DESC TextureDesc = {};
		TextureDesc.Width = static_cast<UINT>(Width);
		TextureDesc.Height = static_cast<UINT>(Height);
		TextureDesc.MipLevels = 1;
		TextureDesc.ArraySize = 1;
		TextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		TextureDesc.SampleDesc.Count = 1;
		TextureDesc.SampleDesc.Quality = 0;
		TextureDesc.Usage = D3D11_USAGE_DEFAULT;
		TextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		HRESULT Hr =
			gDX11Device->CreateTexture2D(
				&TextureDesc,
				0,
				&gDX11DepthTexture
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create depth texture failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		DSVDesc.Texture2D.MipSlice = 0;

		Hr =
			gDX11Device->CreateDepthStencilView(
				gDX11DepthTexture,
				&DSVDesc,
				&gDX11DepthDSV
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create depth DSV failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		return true;
	}

	bool RenderDX11_EnsureFrameTargets(
		const WorldDX11FrameDesc& Desc
	)
	{
		const int Width =
			RenderDX11_ClampSize(Desc.Width);

		const int Height =
			RenderDX11_ClampSize(Desc.Height);

		if (
			gDX11ColorTexture &&
			gDX11DepthTexture &&
			gDX11ColorRTV &&
			gDX11DepthDSV &&
			gDX11FrameWidth == Width &&
			gDX11FrameHeight == Height
		)
		{
			return true;
		}

		RenderDX11_ReleaseFrameTargets();

		if (!RenderDX11_CreateColorTarget(Width, Height))
		{
			RenderDX11_ReleaseFrameTargets();
			return false;
		}

		if (!RenderDX11_CreateDepthTarget(Width, Height))
		{
			RenderDX11_ReleaseFrameTargets();
			return false;
		}

		gDX11Viewport.TopLeftX = 0.0f;
		gDX11Viewport.TopLeftY = 0.0f;
		gDX11Viewport.Width = static_cast<float>(Width);
		gDX11Viewport.Height = static_cast<float>(Height);
		gDX11Viewport.MinDepth = 0.0f;
		gDX11Viewport.MaxDepth = 1.0f;

		gDX11FrameWidth = Width;
		gDX11FrameHeight = Height;

		char Text[256] = {};
		sprintf_s(
			Text,
			"[RenderDX11] Frame targets ready %dx%d\n",
			Width,
			Height
		);

		OutputDebugStringA(Text);

		return true;
	}

	void RenderDX11_BindFrameTargets()
	{
		ID3D11RenderTargetView* RTViews[1] =
		{
			gDX11ColorRTV
		};

		gDX11Context->OMSetRenderTargets(
			1,
			RTViews,
			gDX11DepthDSV
		);

		gDX11Context->RSSetViewports(
			1,
			&gDX11Viewport
		);
	}

	void RenderDX11_ClearFrameTargets()
	{
		const float ClearColor[4] =
		{
			0.02f,
			0.04f,
			0.06f,
			1.0f
		};

		gDX11Context->ClearRenderTargetView(
			gDX11ColorRTV,
			ClearColor
		);

		gDX11Context->ClearDepthStencilView(
			gDX11DepthDSV,
			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
			1.0f,
			0
		);
	}

	void RenderDX11_UnbindFrameTargets()
	{
		ID3D11RenderTargetView* NullRTV[1] =
		{
			0
		};

		gDX11Context->OMSetRenderTargets(
			1,
			NullRTV,
			0
		);
	}
}

#include "DrawWorldDX11.hpp"

bool RenderDX11_Init()
{
	if (gDX11Initialized)
		return true;

	const D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	UINT Flags = 0;

#if defined(_DEBUG)
	Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT Hr =
		D3D11CreateDevice(
			0,
			D3D_DRIVER_TYPE_HARDWARE,
			0,
			Flags,
			FeatureLevels,
			_countof(FeatureLevels),
			D3D11_SDK_VERSION,
			&gDX11Device,
			&gDX11FeatureLevel,
			&gDX11Context
		);

#if defined(_DEBUG)
	if (FAILED(Hr) && (Flags & D3D11_CREATE_DEVICE_DEBUG))
	{
		OutputDebugStringA(
			"[RenderDX11] Debug layer failed. Retrying without debug layer.\n"
		);

		Flags &= ~D3D11_CREATE_DEVICE_DEBUG;

		Hr =
			D3D11CreateDevice(
				0,
				D3D_DRIVER_TYPE_HARDWARE,
				0,
				Flags,
				FeatureLevels,
				_countof(FeatureLevels),
				D3D11_SDK_VERSION,
				&gDX11Device,
				&gDX11FeatureLevel,
				&gDX11Context
			);
	}
#endif

	if (FAILED(Hr))
	{
		char Text[256] = {};
		sprintf_s(
			Text,
			"[RenderDX11] D3D11CreateDevice failed. HRESULT=0x%08X\n",
			static_cast<unsigned int>(Hr)
		);

		OutputDebugStringA(Text);

		RenderDX11_Shutdown();
		return false;
	}

	gDX11Initialized = true;

	char Text[256] = {};
	sprintf_s(
		Text,
		"[RenderDX11] Initialized. FeatureLevel=%s\n",
		RenderDX11_FeatureLevelToString(gDX11FeatureLevel)
	);

	OutputDebugStringA(Text);

	return true;
}

void RenderDX11_Shutdown()
{
	RenderDX11_ReleaseFrameTargets();

	if (gDX11Context)
	{
		gDX11Context->ClearState();
		gDX11Context->Flush();
	}

	RenderDX11_SafeRelease(gDX11Context);
	RenderDX11_SafeRelease(gDX11Device);

	gDX11FeatureLevel = D3D_FEATURE_LEVEL_10_0;
	gDX11Initialized = false;

	OutputDebugStringA(
		"[RenderDX11] Shutdown\n"
	);
}

bool RenderDX11_IsReady()
{
	return
		gDX11Initialized &&
		gDX11Device != 0 &&
		gDX11Context != 0;
}

bool RenderDX11_RenderWorld(
	const WorldDX11FrameDesc& Desc
)
{
	if (!RenderDX11_IsReady())
	{
		if (!RenderDX11_Init())
			return false;
	}

	if (!RenderDX11_EnsureFrameTargets(Desc))
	{
		OutputDebugStringA(
			"[RenderDX11] Render skipped: frame targets failed\n"
		);

		return false;
	}

	RenderDX11_BindFrameTargets();

	RenderDX11_ClearFrameTargets();

	if (!DrawWorldDX11_BeginFrame(Desc))
	{
		RenderDX11_UnbindFrameTargets();
		return false;
	}

	if (!DrawWorldDX11_DepthPrepass(Desc))
	{
		RenderDX11_UnbindFrameTargets();
		return false;
	}

	if (!DrawWorldDX11_FillGBuffer(Desc))
	{
		RenderDX11_UnbindFrameTargets();
		return false;
	}

	if (!DrawWorldDX11_Lighting(Desc))
	{
		RenderDX11_UnbindFrameTargets();
		return false;
	}

	if (!DrawWorldDX11_EndFrame(Desc))
	{
		RenderDX11_UnbindFrameTargets();
		return false;
	}

	RenderDX11_UnbindFrameTargets();

	OutputDebugStringA(
		"[RenderDX11] World path executed. Falling back to DX9 world.\n"
	);

	// Пока возвращаем false.
	// Это важно: старый DX9 RenderDeferredScene1() продолжит рисовать мир.
	return false;
}

#endif // LTS_STUDIO_DX11