#include "r3dPCH.h"
#include "r3d.h"

#include "r3dRendererConfig.h"

#if LTS_STUDIO_DX11

#include <D3D11.h>
#include <DXGI.h>

#include <stdio.h>

#include "GameCommon.h"
#include "rendering/DX11/RenderDX11.h"

bool RenderDX11_Init();
void RenderDX11_Shutdown();
bool RenderDX11_IsReady();

bool RenderDX11_RenderWorld(
	const WorldDX11FrameDesc& Desc
);

namespace
{
	ID3D11Device*			gDX11Device = 0;
	ID3D11DeviceContext*	gDX11Context = 0;

	ID3D11Texture2D*		gDX11ColorTexture = 0;
	ID3D11Texture2D*		gDX11DepthTexture = 0;

	ID3D11RenderTargetView*	gDX11ColorRTV = 0;
	ID3D11DepthStencilView*	gDX11DepthDSV = 0;

	D3D11_VIEWPORT			gDX11Viewport = {};

	ID3D11DepthStencilState* gDX11DepthWriteLessEqual = 0;
	ID3D11DepthStencilState* gDX11DepthReadLessEqual = 0;
	ID3D11DepthStencilState* gDX11DepthDisabled = 0;

	ID3D11RasterizerState*	gDX11RasterSolidBackCull = 0;
	ID3D11RasterizerState*	gDX11RasterSolidNoCull = 0;

	ID3D11BlendState*		gDX11BlendOpaque = 0;
	ID3D11BlendState*		gDX11BlendAlpha = 0;

	ID3D11SamplerState*		gDX11SamplerLinearWrap = 0;
	ID3D11SamplerState*		gDX11SamplerLinearClamp = 0;

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

	void RenderDX11_ReleaseStates()
	{
		RenderDX11_SafeRelease(gDX11SamplerLinearClamp);
		RenderDX11_SafeRelease(gDX11SamplerLinearWrap);

		RenderDX11_SafeRelease(gDX11BlendAlpha);
		RenderDX11_SafeRelease(gDX11BlendOpaque);

		RenderDX11_SafeRelease(gDX11RasterSolidNoCull);
		RenderDX11_SafeRelease(gDX11RasterSolidBackCull);

		RenderDX11_SafeRelease(gDX11DepthDisabled);
		RenderDX11_SafeRelease(gDX11DepthReadLessEqual);
		RenderDX11_SafeRelease(gDX11DepthWriteLessEqual);
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

	bool RenderDX11_CreateDepthStates()
	{
		D3D11_DEPTH_STENCIL_DESC Desc = {};
		Desc.DepthEnable = TRUE;
		Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		Desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		Desc.StencilEnable = FALSE;

		HRESULT Hr =
			gDX11Device->CreateDepthStencilState(
				&Desc,
				&gDX11DepthWriteLessEqual
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create depth write state failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

		Hr =
			gDX11Device->CreateDepthStencilState(
				&Desc,
				&gDX11DepthReadLessEqual
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create depth read state failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		Desc.DepthEnable = FALSE;
		Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		Desc.DepthFunc = D3D11_COMPARISON_ALWAYS;

		Hr =
			gDX11Device->CreateDepthStencilState(
				&Desc,
				&gDX11DepthDisabled
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create depth disabled state failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		return true;
	}

	bool RenderDX11_CreateRasterizerStates()
	{
		D3D11_RASTERIZER_DESC Desc = {};
		Desc.FillMode = D3D11_FILL_SOLID;
		Desc.CullMode = D3D11_CULL_BACK;
		Desc.FrontCounterClockwise = FALSE;
		Desc.DepthBias = 0;
		Desc.DepthBiasClamp = 0.0f;
		Desc.SlopeScaledDepthBias = 0.0f;
		Desc.DepthClipEnable = TRUE;
		Desc.ScissorEnable = FALSE;
		Desc.MultisampleEnable = FALSE;
		Desc.AntialiasedLineEnable = FALSE;

		HRESULT Hr =
			gDX11Device->CreateRasterizerState(
				&Desc,
				&gDX11RasterSolidBackCull
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create raster back-cull state failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		Desc.CullMode = D3D11_CULL_NONE;

		Hr =
			gDX11Device->CreateRasterizerState(
				&Desc,
				&gDX11RasterSolidNoCull
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create raster no-cull state failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		return true;
	}

	bool RenderDX11_CreateBlendStates()
	{
		D3D11_BLEND_DESC Desc = {};
		Desc.AlphaToCoverageEnable = FALSE;
		Desc.IndependentBlendEnable = FALSE;

		Desc.RenderTarget[0].BlendEnable = FALSE;
		Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		Desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		Desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		Desc.RenderTarget[0].RenderTargetWriteMask =
			D3D11_COLOR_WRITE_ENABLE_ALL;

		HRESULT Hr =
			gDX11Device->CreateBlendState(
				&Desc,
				&gDX11BlendOpaque
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create opaque blend state failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		Desc.RenderTarget[0].BlendEnable = TRUE;
		Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		Desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		Desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		Desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		Desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

		Hr =
			gDX11Device->CreateBlendState(
				&Desc,
				&gDX11BlendAlpha
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create alpha blend state failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		return true;
	}

	bool RenderDX11_CreateSamplerStates()
	{
		D3D11_SAMPLER_DESC Desc = {};
		Desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		Desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		Desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		Desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		Desc.MipLODBias = 0.0f;
		Desc.MaxAnisotropy = 1;
		Desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		Desc.BorderColor[0] = 0.0f;
		Desc.BorderColor[1] = 0.0f;
		Desc.BorderColor[2] = 0.0f;
		Desc.BorderColor[3] = 0.0f;
		Desc.MinLOD = 0.0f;
		Desc.MaxLOD = D3D11_FLOAT32_MAX;

		HRESULT Hr =
			gDX11Device->CreateSamplerState(
				&Desc,
				&gDX11SamplerLinearWrap
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create linear wrap sampler failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		Desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		Desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		Desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

		Hr =
			gDX11Device->CreateSamplerState(
				&Desc,
				&gDX11SamplerLinearClamp
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create linear clamp sampler failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		return true;
	}

	bool RenderDX11_CreateStates()
	{
		RenderDX11_ReleaseStates();

		if (!RenderDX11_CreateDepthStates())
		{
			RenderDX11_ReleaseStates();
			return false;
		}

		if (!RenderDX11_CreateRasterizerStates())
		{
			RenderDX11_ReleaseStates();
			return false;
		}

		if (!RenderDX11_CreateBlendStates())
		{
			RenderDX11_ReleaseStates();
			return false;
		}

		if (!RenderDX11_CreateSamplerStates())
		{
			RenderDX11_ReleaseStates();
			return false;
		}

		OutputDebugStringA(
			"[RenderDX11] Render states created\n"
		);

		return true;
	}

	void RenderDX11_ApplyDefaultStates()
	{
		if (!gDX11Context)
			return;

		gDX11Context->OMSetDepthStencilState(
			gDX11DepthWriteLessEqual,
			0
		);

		const float BlendFactor[4] =
		{
			0.0f,
			0.0f,
			0.0f,
			0.0f
		};

		gDX11Context->OMSetBlendState(
			gDX11BlendOpaque,
			BlendFactor,
			0xffffffff
		);

		gDX11Context->RSSetState(
			gDX11RasterSolidBackCull
		);

		ID3D11SamplerState* Samplers[1] =
		{
			gDX11SamplerLinearWrap
		};

		gDX11Context->PSSetSamplers(
			0,
			1,
			Samplers
		);
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

		RenderDX11_ApplyDefaultStates();
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

	if (!RenderDX11_CreateStates())
	{
		OutputDebugStringA(
			"[RenderDX11] Failed to create render states\n"
		);

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

	RenderDX11_ReleaseStates();

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