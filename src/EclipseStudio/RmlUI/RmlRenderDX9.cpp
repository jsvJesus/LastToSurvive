#include "r3dPCH.h"
#include "r3d.h"

#include "RmlRenderDX9.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Types.h>

#include <windows.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cstring>
#include <type_traits>

#ifdef SetViewport
#undef SetViewport
#endif

#ifdef SetTransform
#undef SetTransform
#endif

#ifdef SetFVF
#undef SetFVF
#endif

#ifdef SetRenderState
#undef SetRenderState
#endif

#ifdef SetTextureStageState
#undef SetTextureStageState
#endif

#ifdef SetSamplerState
#undef SetSamplerState
#endif

#ifdef SetScissorRect
#undef SetScissorRect
#endif

#ifdef SetStreamSource
#undef SetStreamSource
#endif

#ifdef SetIndices
#undef SetIndices
#endif

#ifdef SetTexture
#undef SetTexture
#endif

#ifdef DrawIndexedPrimitive
#undef DrawIndexedPrimitive
#endif

#ifdef CreateStateBlock
#undef CreateStateBlock
#endif

#ifdef CreateVertexBuffer
#undef CreateVertexBuffer
#endif

#ifdef CreateIndexBuffer
#undef CreateIndexBuffer
#endif

#ifdef D3DRS_CULLMODE
#undef D3DRS_CULLMODE
#endif

#ifdef D3DRS_SCISSORTESTENABLE
#undef D3DRS_SCISSORTESTENABLE
#endif

#ifdef D3DRS_ALPHATESTENABLE
#undef D3DRS_ALPHATESTENABLE
#endif

#ifdef D3DRS_ALPHABLENDENABLE
#undef D3DRS_ALPHABLENDENABLE
#endif

#ifdef D3DRS_SRCBLEND
#undef D3DRS_SRCBLEND
#endif

#ifdef D3DRS_DESTBLEND
#undef D3DRS_DESTBLEND
#endif

#ifdef D3DRS_BLENDOP
#undef D3DRS_BLENDOP
#endif

#ifdef D3DRS_ZENABLE
#undef D3DRS_ZENABLE
#endif

#ifdef D3DRS_ZWRITEENABLE
#undef D3DRS_ZWRITEENABLE
#endif

#ifdef D3DRS_LIGHTING
#undef D3DRS_LIGHTING
#endif

#ifdef CreateTexture
#undef CreateTexture
#endif

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

static const D3DRENDERSTATETYPE RML_D3DRS_ZENABLE = (D3DRENDERSTATETYPE)7;
static const D3DRENDERSTATETYPE RML_D3DRS_ZWRITEENABLE = (D3DRENDERSTATETYPE)14;
static const D3DRENDERSTATETYPE RML_D3DRS_ALPHATESTENABLE = (D3DRENDERSTATETYPE)15;
static const D3DRENDERSTATETYPE RML_D3DRS_SRCBLEND = (D3DRENDERSTATETYPE)19;
static const D3DRENDERSTATETYPE RML_D3DRS_DESTBLEND = (D3DRENDERSTATETYPE)20;
static const D3DRENDERSTATETYPE RML_D3DRS_CULLMODE = (D3DRENDERSTATETYPE)22;
static const D3DRENDERSTATETYPE RML_D3DRS_ALPHABLENDENABLE = (D3DRENDERSTATETYPE)27;
static const D3DRENDERSTATETYPE RML_D3DRS_LIGHTING = (D3DRENDERSTATETYPE)137;
static const D3DRENDERSTATETYPE RML_D3DRS_SCISSORTESTENABLE = (D3DRENDERSTATETYPE)174;
static const D3DRENDERSTATETYPE RML_D3DRS_BLENDOP = (D3DRENDERSTATETYPE)171;
static const D3DRENDERSTATETYPE RML_D3DRS_STENCILENABLE = static_cast<D3DRENDERSTATETYPE>(52);
static const D3DRENDERSTATETYPE RML_D3DRS_STENCILFAIL = static_cast<D3DRENDERSTATETYPE>(53);
static const D3DRENDERSTATETYPE RML_D3DRS_STENCILZFAIL = static_cast<D3DRENDERSTATETYPE>(54);
static const D3DRENDERSTATETYPE RML_D3DRS_STENCILPASS = static_cast<D3DRENDERSTATETYPE>(55);
static const D3DRENDERSTATETYPE RML_D3DRS_STENCILFUNC = static_cast<D3DRENDERSTATETYPE>(56);
static const D3DRENDERSTATETYPE RML_D3DRS_STENCILREF = static_cast<D3DRENDERSTATETYPE>(57);
static const D3DRENDERSTATETYPE RML_D3DRS_STENCILMASK = static_cast<D3DRENDERSTATETYPE>(58);
static const D3DRENDERSTATETYPE RML_D3DRS_STENCILWRITEMASK = static_cast<D3DRENDERSTATETYPE>(59);
static const D3DRENDERSTATETYPE RML_D3DRS_TEXTUREFACTOR = static_cast<D3DRENDERSTATETYPE>(60);
static const D3DRENDERSTATETYPE RML_D3DRS_COLORWRITEENABLE = static_cast<D3DRENDERSTATETYPE>(168);
static const DWORD RML_COLOR_WRITE_ALL =
	D3DCOLORWRITEENABLE_RED |
	D3DCOLORWRITEENABLE_GREEN |
	D3DCOLORWRITEENABLE_BLUE |
	D3DCOLORWRITEENABLE_ALPHA;

static const char* RmlDx9BlurShaderSource =
R"RMLSHADER(
	sampler2D SourceSampler : register(s0);

	float4 TexelStep : register(c0);
	float4 Weights0123 : register(c1);
	float4 Weight4 : register(c2);

	float4 main(float2 UV : TEXCOORD0) : COLOR0
	{
		float2 Step = TexelStep.xy;

		float4 Result =
			tex2D(
				SourceSampler,
				UV
			) * Weights0123.x;

		Result +=
			(
				tex2D(
					SourceSampler,
					UV + Step
				) +
				tex2D(
					SourceSampler,
					UV - Step
				)
			) * Weights0123.y;

		Result +=
			(
				tex2D(
					SourceSampler,
					UV + Step * 2.0
				) +
				tex2D(
					SourceSampler,
					UV - Step * 2.0
				)
			) * Weights0123.z;

		Result +=
			(
				tex2D(
					SourceSampler,
					UV + Step * 3.0
				) +
				tex2D(
					SourceSampler,
					UV - Step * 3.0
				)
			) * Weights0123.w;

		Result +=
			(
				tex2D(
					SourceSampler,
					UV + Step * 4.0
				) +
				tex2D(
					SourceSampler,
					UV - Step * 4.0
				)
			) * Weight4.x;

		return Result;
	}
)RMLSHADER";


static const char* RmlDx9ShadowShaderSource =
R"RMLSHADER(
	sampler2D SourceSampler : register(s0);

	float4 ShadowColor : register(c0);

	float4 main(float2 UV : TEXCOORD0) : COLOR0
	{
		float SourceAlpha =
			tex2D(
				SourceSampler,
				UV
			).a;

		return ShadowColor * SourceAlpha;
	}
)RMLSHADER";

static bool RmlFileExistsW(const std::wstring& Path)
{
	if (Path.empty())
		return false;

	const DWORD Attributes = GetFileAttributesW(Path.c_str());

	if (Attributes == INVALID_FILE_ATTRIBUTES)
		return false;

	return (Attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

RmlRenderDX9::RmlRenderDX9()
{
	CurrentTransform =
		MakeIdentity();
}

RmlRenderDX9::~RmlRenderDX9()
{
	Shutdown();
}

bool RmlRenderDX9::Init(IDirect3DDevice9* InDevice, const wchar_t* InDataRoot)
{
	if (!InDevice)
	{
		OutputDebugStringA("[RmlUI][DX9] Init failed: null IDirect3DDevice9\n");
		return false;
	}

	Device = InDevice;
	Device->AddRef();
	CreateFilterShaders();

	DataRoot = InDataRoot ? InDataRoot : L"";

	while (!DataRoot.empty() && (DataRoot.back() == L'\\' || DataRoot.back() == L'/'))
		DataRoot.pop_back();

	OutputDebugStringA("[RmlUI][DX9] Render interface initialized\n");
	return true;
}

void RmlRenderDX9::Shutdown()
{
	SetCharacterPortraitTexture(
		nullptr
	);
	
	SetCharacterPreviewTexture(
		nullptr
	);

	if (StateBlock)
	{
		StateBlock->Release();
		StateBlock = nullptr;
	}

	ReleaseLayerResources();
	ReleaseSharedDepthStencil();

	if (BaseRenderTarget)
	{
		BaseRenderTarget->Release();
		BaseRenderTarget = nullptr;
	}

	if (OriginalDepthStencil)
	{
		OriginalDepthStencil->Release();
		OriginalDepthStencil = nullptr;
	}

	ReleasePostProcessTargets();
	ReleaseFilterShaders();

	if (Device)
	{
		Device->Release();
		Device = nullptr;
	}
}

bool RmlRenderDX9::CreatePixelShader(
	const char* SourceCode,
	IDirect3DPixelShader9** OutShader
)
{
	if (
		!Device ||
		!SourceCode ||
		!OutShader
	)
	{
		return false;
	}

	*OutShader =
		nullptr;

	D3DCAPS9 Capabilities{};

	if (
		FAILED(
			Device->GetDeviceCaps(
				&Capabilities
			)
		)
	)
	{
		return false;
	}

	if (
		Capabilities.PixelShaderVersion <
		D3DPS_VERSION(
			2,
			0
		)
	)
	{
		OutputDebugStringA(
			"[RmlUI][DX9] Pixel Shader 2.0 is not supported\n"
		);

		return false;
	}

	const char* ShaderProfile =
		Capabilities.PixelShaderVersion >=
			D3DPS_VERSION(
				3,
				0
			)
		? "ps_3_0"
		: "ps_2_0";

	ID3DXBuffer* CompiledCode =
		nullptr;

	ID3DXBuffer* ErrorMessages =
		nullptr;

	const HRESULT CompileResult =
		D3DXCompileShader(
			SourceCode,
			static_cast<UINT>(
				std::strlen(
					SourceCode
				)
			),
			nullptr,
			nullptr,
			"main",
			ShaderProfile,
			D3DXSHADER_OPTIMIZATION_LEVEL3,
			&CompiledCode,
			&ErrorMessages,
			nullptr
		);

	if (ErrorMessages)
	{
		OutputDebugStringA(
			reinterpret_cast<const char*>(
				ErrorMessages->
					GetBufferPointer()
			)
		);

		ErrorMessages->Release();
		ErrorMessages = nullptr;
	}

	if (
		FAILED(
			CompileResult
		) ||
		!CompiledCode
	)
	{
		OutputDebugStringA(
			"[RmlUI][DX9] Pixel shader compilation failed\n"
		);

		if (CompiledCode)
			CompiledCode->Release();

		return false;
	}

	const HRESULT CreateResult =
		Device->CreatePixelShader(
			reinterpret_cast<const DWORD*>(
				CompiledCode->
					GetBufferPointer()
			),
			OutShader
		);

	CompiledCode->Release();
	CompiledCode = nullptr;

	if (
		FAILED(
			CreateResult
		) ||
		!*OutShader
	)
	{
		OutputDebugStringA(
			"[RmlUI][DX9] CreatePixelShader failed\n"
		);

		return false;
	}

	return true;
}


bool RmlRenderDX9::CreateFilterShaders()
{
	if (
		BlurPixelShader &&
		ShadowPixelShader
	)
	{
		return true;
	}

	ReleaseFilterShaders();

	if (!CreatePixelShader(
		RmlDx9BlurShaderSource,
		&BlurPixelShader
	))
	{
		return false;
	}

	if (!CreatePixelShader(
		RmlDx9ShadowShaderSource,
		&ShadowPixelShader
	))
	{
		ReleaseFilterShaders();
		return false;
	}

	OutputDebugStringA(
		"[RmlUI][DX9] Blur and drop-shadow shaders created\n"
	);

	return true;
}


void RmlRenderDX9::ReleaseFilterShaders()
{
	if (BlurPixelShader)
	{
		BlurPixelShader->Release();
		BlurPixelShader = nullptr;
	}

	if (ShadowPixelShader)
	{
		ShadowPixelShader->Release();
		ShadowPixelShader = nullptr;
	}
}

void RmlRenderDX9::ReleasePostProcessTarget(
	FPostProcessTarget& Target
)
{
	if (Target.Surface)
	{
		Target.Surface->Release();
		Target.Surface = nullptr;
	}

	if (Target.Texture)
	{
		Target.Texture->Release();
		Target.Texture = nullptr;
	}

	Target.Width =
		0;

	Target.Height =
		0;
}


void RmlRenderDX9::ReleasePostProcessTargets()
{
	for (
		FPostProcessTarget& Target :
		PostProcessTargets
	)
	{
		ReleasePostProcessTarget(
			Target
		);
	}
}


bool RmlRenderDX9::EnsurePostProcessTargets()
{
	if (!Device)
		return false;

	for (
		FPostProcessTarget& Target :
		PostProcessTargets
	)
	{
		if (
			Target.Texture &&
			Target.Surface &&
			Target.Width == ViewWidth &&
			Target.Height == ViewHeight
		)
		{
			continue;
		}

		ReleasePostProcessTarget(
			Target
		);

		if (!CreateRenderTargetTexture(
			ViewWidth,
			ViewHeight,
			&Target.Texture,
			&Target.Surface
		))
		{
			ReleasePostProcessTargets();

			OutputDebugStringA(
				"[RmlUI][DX9] Post-process render target creation failed\n"
			);

			return false;
		}

		Target.Width =
			ViewWidth;

		Target.Height =
			ViewHeight;
	}

	return true;
}

void RmlRenderDX9::CalculateGaussianWeights(
	float Sigma,
	float OutWeights[5]
)
{
	if (!OutWeights)
		return;

	Sigma =
		std::max(
			0.1f,
			Sigma
		);

	const float SigmaSquared =
		Sigma *
		Sigma;

	float TotalWeight =
		0.0f;

	for (
		int Index = 0;
		Index < 5;
		++Index
	)
	{
		const float Distance =
			static_cast<float>(
				Index
			);

		OutWeights[Index] =
			std::exp(
				-(
					Distance *
					Distance
				) /
				(
					2.0f *
					SigmaSquared
				)
			);

		TotalWeight +=
			Index == 0
			? OutWeights[Index]
			: OutWeights[Index] *
				2.0f;
	}

	if (TotalWeight <= 0.0f)
	{
		OutWeights[0] = 1.0f;

		for (
			int Index = 1;
			Index < 5;
			++Index
		)
		{
			OutWeights[Index] =
				0.0f;
		}

		return;
	}

	for (
		float& Weight :
		OutWeights
	)
	{
		Weight /=
			TotalWeight;
	}
}

void RmlRenderDX9::DrawPostProcessQuad(
	IDirect3DTexture9* SourceTexture,
	IDirect3DSurface9* DestinationSurface,
	IDirect3DPixelShader9* PixelShader,
	float OffsetX,
	float OffsetY,
	float Opacity,
	bool bEnableBlend,
	bool bClearDestination
)
{
	if (
		!Device ||
		!SourceTexture ||
		!DestinationSurface
	)
	{
		return;
	}

	Device->SetRenderTarget(
		0,
		DestinationSurface
	);

	Device->SetDepthStencilSurface(
		nullptr
	);

	(Device->SetRenderState)(
		RML_D3DRS_STENCILENABLE,
		FALSE
	);

	if (bClearDestination)
	{
		(Device->SetRenderState)(
			RML_D3DRS_SCISSORTESTENABLE,
			FALSE
		);

		Device->Clear(
			0,
			nullptr,
			D3DCLEAR_TARGET,
			0x00000000,
			1.0f,
			0
		);
	}

	(Device->SetRenderState)(
		RML_D3DRS_SCISSORTESTENABLE,
		bScissorEnabled
			? TRUE
			: FALSE
	);

	if (bScissorEnabled)
	{
		Device->SetScissorRect(
			&ScissorRect
		);
	}

	const float Left =
		-0.5f +
		OffsetX;

	const float Top =
		-0.5f +
		OffsetY;

	const float Right =
		static_cast<float>(
			ViewWidth
		) -
		0.5f +
		OffsetX;

	const float Bottom =
		static_cast<float>(
			ViewHeight
		) -
		0.5f +
		OffsetY;

	const DWORD White =
		D3DCOLOR_ARGB(
			255,
			255,
			255,
			255
		);

	const FScreenVertex Vertices[4] =
	{
		{
			Left,
			Top,
			0.0f,
			1.0f,
			White,
			0.0f,
			0.0f
		},
		{
			Right,
			Top,
			0.0f,
			1.0f,
			White,
			1.0f,
			0.0f
		},
		{
			Left,
			Bottom,
			0.0f,
			1.0f,
			White,
			0.0f,
			1.0f
		},
		{
			Right,
			Bottom,
			0.0f,
			1.0f,
			White,
			1.0f,
			1.0f
		}
	};

	Device->SetVertexShader(
		nullptr
	);

	Device->SetPixelShader(
		PixelShader
	);

	Device->SetFVF(
		ScreenVertexFVF
	);

	(Device->SetRenderState)(
		RML_D3DRS_ZENABLE,
		FALSE
	);

	(Device->SetRenderState)(
		RML_D3DRS_ZWRITEENABLE,
		FALSE
	);

	(Device->SetRenderState)(
		RML_D3DRS_CULLMODE,
		D3DCULL_NONE
	);

	(Device->SetRenderState)(
		RML_D3DRS_ALPHABLENDENABLE,
		bEnableBlend
			? TRUE
			: FALSE
	);

	(Device->SetRenderState)(
		RML_D3DRS_SRCBLEND,
		D3DBLEND_ONE
	);

	(Device->SetRenderState)(
		RML_D3DRS_DESTBLEND,
		D3DBLEND_INVSRCALPHA
	);

	Device->SetTexture(
		0,
		SourceTexture
	);

	Device->SetSamplerState(
		0,
		D3DSAMP_MINFILTER,
		D3DTEXF_LINEAR
	);

	Device->SetSamplerState(
		0,
		D3DSAMP_MAGFILTER,
		D3DTEXF_LINEAR
	);

	Device->SetSamplerState(
		0,
		D3DSAMP_MIPFILTER,
		D3DTEXF_NONE
	);

	Device->SetSamplerState(
		0,
		D3DSAMP_ADDRESSU,
		D3DTADDRESS_CLAMP
	);

	Device->SetSamplerState(
		0,
		D3DSAMP_ADDRESSV,
		D3DTADDRESS_CLAMP
	);

	if (!PixelShader)
	{
		Opacity =
			std::max(
				0.0f,
				std::min(
					1.0f,
					Opacity
				)
			);

		const DWORD OpacityByte =
			static_cast<DWORD>(
				Opacity *
				255.0f +
				0.5f
			);

		(Device->SetRenderState)(
			RML_D3DRS_TEXTUREFACTOR,
			D3DCOLOR_ARGB(
				OpacityByte,
				OpacityByte,
				OpacityByte,
				OpacityByte
			)
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_COLOROP,
			D3DTOP_MODULATE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_COLORARG1,
			D3DTA_TEXTURE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_COLORARG2,
			D3DTA_TFACTOR
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_ALPHAOP,
			D3DTOP_MODULATE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_ALPHAARG1,
			D3DTA_TEXTURE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_ALPHAARG2,
			D3DTA_TFACTOR
		);

		Device->SetTextureStageState(
			1,
			D3DTSS_COLOROP,
			D3DTOP_DISABLE
		);

		Device->SetTextureStageState(
			1,
			D3DTSS_ALPHAOP,
			D3DTOP_DISABLE
		);
	}

	Device->DrawPrimitiveUP(
		D3DPT_TRIANGLESTRIP,
		2,
		Vertices,
		sizeof(
			FScreenVertex
		)
	);

	Device->SetTexture(
		0,
		nullptr
	);

	Device->SetPixelShader(
		nullptr
	);
}

void RmlRenderDX9::RenderBlurPass(
	IDirect3DTexture9* SourceTexture,
	IDirect3DSurface9* DestinationSurface,
	float Sigma,
	bool bHorizontal
)
{
	if (
		!Device ||
		!BlurPixelShader ||
		!SourceTexture ||
		!DestinationSurface
	)
	{
		return;
	}

	float Weights[5]{};

	CalculateGaussianWeights(
		Sigma,
		Weights
	);

	const float TexelStep[4] =
	{
		bHorizontal
			? 1.0f /
				static_cast<float>(
					ViewWidth
				)
			: 0.0f,

		bHorizontal
			? 0.0f
			: 1.0f /
				static_cast<float>(
					ViewHeight
				),

		0.0f,
		0.0f
	};

	const float Weights0123[4] =
	{
		Weights[0],
		Weights[1],
		Weights[2],
		Weights[3]
	};

	const float Weight4[4] =
	{
		Weights[4],
		0.0f,
		0.0f,
		0.0f
	};

	Device->SetPixelShaderConstantF(
		0,
		TexelStep,
		1
	);

	Device->SetPixelShaderConstantF(
		1,
		Weights0123,
		1
	);

	Device->SetPixelShaderConstantF(
		2,
		Weight4,
		1
	);

	DrawPostProcessQuad(
		SourceTexture,
		DestinationSurface,
		BlurPixelShader,
		0.0f,
		0.0f,
		1.0f,
		false,
		true
	);
}

bool RmlRenderDX9::ApplyGaussianBlur(
	IDirect3DTexture9* SourceTexture,
	int SourcePostProcessIndex,
	float Sigma,
	IDirect3DTexture9*& OutTexture,
	int& OutPostProcessIndex
)
{
	OutTexture =
		SourceTexture;

	OutPostProcessIndex =
		SourcePostProcessIndex;

	if (
		!SourceTexture ||
		Sigma < 0.1f
	)
	{
		return true;
	}

	if (
		!CreateFilterShaders() ||
		!EnsurePostProcessTargets()
	)
	{
		return false;
	}

	const int HorizontalTargetIndex =
		FindPostProcessTarget(
			SourcePostProcessIndex
		);

	const int VerticalTargetIndex =
		FindPostProcessTarget(
			SourcePostProcessIndex,
			HorizontalTargetIndex
		);

	if (
		HorizontalTargetIndex < 0 ||
		VerticalTargetIndex < 0
	)
	{
		return false;
	}

	Sigma =
		std::min(
			Sigma,
			16.0f
		);

	const int PassCount =
		std::max(
			1,
			std::min(
				4,
				static_cast<int>(
					std::ceil(
						(
							Sigma *
							Sigma
						) /
						9.0f
					)
				)
			)
		);

	const float PassSigma =
		Sigma /
		std::sqrt(
			static_cast<float>(
				PassCount
			)
		);

	IDirect3DTexture9* CurrentTexture =
		SourceTexture;

	for (
		int PassIndex = 0;
		PassIndex < PassCount;
		++PassIndex
	)
	{
		RenderBlurPass(
			CurrentTexture,
			PostProcessTargets[
				HorizontalTargetIndex
			].Surface,
			PassSigma,
			true
		);

		RenderBlurPass(
			PostProcessTargets[
				HorizontalTargetIndex
			].Texture,
			PostProcessTargets[
				VerticalTargetIndex
			].Surface,
			PassSigma,
			false
		);

		CurrentTexture =
			PostProcessTargets[
				VerticalTargetIndex
			].Texture;
	}

	OutTexture =
		CurrentTexture;

	OutPostProcessIndex =
		VerticalTargetIndex;

	return true;
}

int RmlRenderDX9::FindPostProcessTarget(
	int ExcludeA,
	int ExcludeB
) const
{
	for (
		int Index = 0;
		Index < 3;
		++Index
	)
	{
		if (
			Index != ExcludeA &&
			Index != ExcludeB
		)
		{
			return Index;
		}
	}

	return -1;
}

void RmlRenderDX9::
SetCharacterPreviewTexture(
	IDirect3DTexture9* Texture
)
{
	if (
		CharacterPreviewTexture ==
		Texture
	)
	{
		return;
	}

	if (Texture)
	{
		Texture->AddRef();
	}

	if (CharacterPreviewTexture)
	{
		CharacterPreviewTexture->
			Release();
	}

	CharacterPreviewTexture =
		Texture;

	if (CharacterPreviewTexture)
	{
		D3DSURFACE_DESC Description{};

		if (
			SUCCEEDED(
				CharacterPreviewTexture->
					GetLevelDesc(
						0,
						&Description
					)
			)
		)
		{
			r3dOutToLog(
				"[RmlUI][DX9] Character preview "
				"texture bound: %ux%u, format=%d\n",
				Description.Width,
				Description.Height,
				static_cast<int>(
					Description.Format
				)
			);
		}
	}
}

void RmlRenderDX9::
SetCharacterPortraitTexture(
	IDirect3DTexture9* Texture
)
{
	if (
		CharacterPortraitTexture ==
		Texture
	)
	{
		return;
	}

	if (Texture)
	{
		Texture->AddRef();
	}

	if (CharacterPortraitTexture)
	{
		CharacterPortraitTexture->
			Release();
	}

	CharacterPortraitTexture =
		Texture;

	if (CharacterPortraitTexture)
	{
		D3DSURFACE_DESC Description{};

		if (
			SUCCEEDED(
				CharacterPortraitTexture->
					GetLevelDesc(
						0,
						&Description
					)
			)
		)
		{
			r3dOutToLog(
				"[RmlUI][DX9] Character portrait "
				"texture bound: %ux%u, format=%d\n",
				Description.Width,
				Description.Height,
				static_cast<int>(
					Description.Format
				)
			);
		}
	}
}

void RmlRenderDX9::BeginFrame(
	int Width,
	int Height
)
{
	if (
		!Device ||
		bFrameOpen
	)
	{
		return;
	}

	ViewWidth =
		std::max(
			1,
			Width
		);

	ViewHeight =
		std::max(
			1,
			Height
		);

	EnsurePostProcessTargets();

	if (
		!BlurPixelShader ||
		!ShadowPixelShader
	)
	{
		CreateFilterShaders();
	}

	if (StateBlock)
	{
		StateBlock->Release();
		StateBlock = nullptr;
	}

	if (
		SUCCEEDED(
			Device->CreateStateBlock(
				D3DSBT_ALL,
				&StateBlock
			)
		) &&
		StateBlock
	)
	{
		StateBlock->Capture();
	}

	if (BaseRenderTarget)
	{
		BaseRenderTarget->Release();
		BaseRenderTarget = nullptr;
	}

	if (OriginalDepthStencil)
	{
		OriginalDepthStencil->Release();
		OriginalDepthStencil = nullptr;
	}

	Device->GetRenderTarget(
		0,
		&BaseRenderTarget
	);

	Device->GetDepthStencilSurface(
		&OriginalDepthStencil
	);

	EnsureSharedDepthStencil();

	if (SharedDepthStencil)
	{
		Device->SetDepthStencilSurface(
			SharedDepthStencil
		);

		Device->Clear(
			0,
			nullptr,
			D3DCLEAR_STENCIL,
			0,
			1.0f,
			0
		);
	}

	ActiveLayerCount =
		1;

	bFrameOpen =
		true;

	bClipMaskEnabled =
		false;

	ClipMaskReference =
		0;

	ScissorRect.left =
		0;

	ScissorRect.top =
		0;

	ScissorRect.right =
		ViewWidth;

	ScissorRect.bottom =
		ViewHeight;

	SetupRenderState();
}

void RmlRenderDX9::EndFrame()
{
	if (
		!Device ||
		!bFrameOpen
	)
	{
		return;
	}

	ActiveLayerCount =
		1;

	if (BaseRenderTarget)
	{
		Device->SetRenderTarget(
			0,
			BaseRenderTarget
		);
	}

	Device->SetDepthStencilSurface(
		OriginalDepthStencil
	);

	Device->SetPixelShader(
		nullptr
	);

	if (StateBlock)
	{
		StateBlock->Apply();
		StateBlock->Release();
		StateBlock = nullptr;
	}

	if (BaseRenderTarget)
	{
		BaseRenderTarget->Release();
		BaseRenderTarget = nullptr;
	}

	if (OriginalDepthStencil)
	{
		OriginalDepthStencil->Release();
		OriginalDepthStencil = nullptr;
	}

	bFrameOpen =
		false;
}

void RmlRenderDX9::OnDeviceLost()
{
	SetCharacterPreviewTexture(
		nullptr
	);

	SetCharacterPortraitTexture(
		nullptr
	);

	if (StateBlock)
	{
		StateBlock->Release();
		StateBlock = nullptr;
	}

	if (BaseRenderTarget)
	{
		BaseRenderTarget->Release();
		BaseRenderTarget = nullptr;
	}

	if (OriginalDepthStencil)
	{
		OriginalDepthStencil->Release();
		OriginalDepthStencil = nullptr;
	}

	ReleaseLayerResources();
	ReleaseSharedDepthStencil();
	ReleasePostProcessTargets();
	ReleaseFilterShaders();

	ActiveLayerCount =
		0;

	bFrameOpen =
		false;

	OutputDebugStringA(
		"[RmlUI][DX9] Device lost\n"
	);
}

void RmlRenderDX9::OnDeviceReset(int Width, int Height)
{
	ViewWidth = std::max(1, Width);
	ViewHeight = std::max(1, Height);

	ReleasePostProcessTargets();

	EnsurePostProcessTargets();
	CreateFilterShaders();

	OutputDebugStringA("[RmlUI][DX9] Device reset\n");
}

DWORD RmlRenderDX9::ConvertColor(const Rml::ColourbPremultiplied& Color)
{
	return D3DCOLOR_ARGB(Color.alpha, Color.red, Color.green, Color.blue);
}

D3DMATRIX RmlRenderDX9::ConvertTransform(
	const Rml::Matrix4f& Transform
)
{
	D3DMATRIX Result{};

	const float* Source =
		Transform.data();

	float* Destination =
		reinterpret_cast<float*>(
			&Result
		);

	const bool bRowMajor =
		std::is_same<
			Rml::Matrix4f,
			Rml::RowMajorMatrix4f
		>::value;

	if (!bRowMajor)
	{
		/*
		 * RmlUi по умолчанию хранит column-major matrix.
		 * Для D3D9 row-vector pipeline её линейное представление
		 * уже соответствует транспонированной D3D-матрице.
		 */
		std::memcpy(
			Destination,
			Source,
			sizeof(D3DMATRIX)
		);
	}
	else
	{
		for (
			int Row = 0;
			Row < 4;
			++Row
		)
		{
			for (
				int Column = 0;
				Column < 4;
				++Column
			)
			{
				Destination[
					Row * 4 +
					Column
				] =
					Source[
						Column * 4 +
						Row
					];
			}
		}
	}

	return Result;
}

D3DMATRIX RmlRenderDX9::MakeIdentity()
{
	D3DMATRIX M{};
	M._11 = 1.0f;
	M._22 = 1.0f;
	M._33 = 1.0f;
	M._44 = 1.0f;
	return M;
}

D3DMATRIX RmlRenderDX9::MakeOrthoOffCenterLH(float Left, float Right, float Bottom, float Top, float ZNear, float ZFar)
{
	D3DMATRIX M{};

	M._11 = 2.0f / (Right - Left);
	M._22 = 2.0f / (Top - Bottom);
	M._33 = 1.0f / (ZFar - ZNear);
	M._44 = 1.0f;

	M._41 = (Left + Right) / (Left - Right);
	M._42 = (Top + Bottom) / (Bottom - Top);
	M._43 = ZNear / (ZNear - ZFar);

	return M;
}

D3DMATRIX RmlRenderDX9::MakeTranslation(float X, float Y, float Z)
{
	D3DMATRIX M = MakeIdentity();

	M._41 = X;
	M._42 = Y;
	M._43 = Z;

	return M;
}

void RmlRenderDX9::SetupRenderState()
{
	if (!Device)
		return;

	D3DVIEWPORT9 Viewport{};
	Viewport.X = 0;
	Viewport.Y = 0;
	Viewport.Width = static_cast<DWORD>(ViewWidth);
	Viewport.Height = static_cast<DWORD>(ViewHeight);
	Viewport.MinZ = 0.0f;
	Viewport.MaxZ = 1.0f;

	Device->SetViewport(&Viewport);

	const D3DMATRIX World = MakeIdentity();
	const D3DMATRIX View = MakeIdentity();
	const D3DMATRIX Projection = MakeOrthoOffCenterLH(
		0.0f,
		static_cast<float>(ViewWidth),
		static_cast<float>(ViewHeight),
		0.0f,
		-1.0f,
		1.0f
	);

	Device->SetTransform(D3DTS_WORLD, &World);
	Device->SetTransform(D3DTS_VIEW, &View);
	Device->SetTransform(D3DTS_PROJECTION, &Projection);

	Device->SetFVF(VertexFVF);

	(Device->SetRenderState)(RML_D3DRS_LIGHTING, FALSE);
	(Device->SetRenderState)(RML_D3DRS_ZENABLE, FALSE);
	(Device->SetRenderState)(RML_D3DRS_ZWRITEENABLE, FALSE);
	(Device->SetRenderState)(RML_D3DRS_CULLMODE, D3DCULL_NONE);

	(Device->SetRenderState)(RML_D3DRS_ALPHATESTENABLE, FALSE);
	(Device->SetRenderState)(RML_D3DRS_ALPHABLENDENABLE, TRUE);

	(Device->SetRenderState)(RML_D3DRS_SRCBLEND, D3DBLEND_ONE);
	(Device->SetRenderState)(RML_D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	(Device->SetRenderState)(RML_D3DRS_BLENDOP, D3DBLENDOP_ADD);

	(Device->SetRenderState)(RML_D3DRS_SCISSORTESTENABLE, bScissorEnabled ? TRUE : FALSE);

	if (bScissorEnabled)
		Device->SetScissorRect(&ScissorRect);

	Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	Device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	Device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	Device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	Device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	Device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	(Device->SetRenderState)(
		RML_D3DRS_COLORWRITEENABLE,
		RML_COLOR_WRITE_ALL
	);

	ApplyClipMaskState();
}

Rml::CompiledGeometryHandle RmlRenderDX9::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	if (!Device || vertices.empty() || indices.empty())
		return 0;

	FCompiledGeometry* Geometry = new FCompiledGeometry();
	Geometry->NumVertices = static_cast<int>(vertices.size());
	Geometry->NumIndices = static_cast<int>(indices.size());

	const UINT VertexBytes = static_cast<UINT>(sizeof(FDx9Vertex) * Geometry->NumVertices);
	const UINT IndexBytes = static_cast<UINT>(sizeof(unsigned int) * Geometry->NumIndices);

	HRESULT Hr = Device->CreateVertexBuffer(
		VertexBytes,
		0,
		VertexFVF,
		D3DPOOL_MANAGED,
		&Geometry->VertexBuffer,
		nullptr
	);

	if (FAILED(Hr))
	{
		delete Geometry;
		OutputDebugStringA("[RmlUI][DX9] CreateVertexBuffer failed\n");
		return 0;
	}

	Hr = Device->CreateIndexBuffer(
		IndexBytes,
		0,
		D3DFMT_INDEX32,
		D3DPOOL_MANAGED,
		&Geometry->IndexBuffer,
		nullptr
	);

	if (FAILED(Hr))
	{
		Geometry->VertexBuffer->Release();
		delete Geometry;
		OutputDebugStringA("[RmlUI][DX9] CreateIndexBuffer failed\n");
		return 0;
	}

	void* VertexMemory = nullptr;
	Hr = Geometry->VertexBuffer->Lock(0, VertexBytes, &VertexMemory, 0);

	if (FAILED(Hr))
	{
		ReleaseGeometry(reinterpret_cast<Rml::CompiledGeometryHandle>(Geometry));
		OutputDebugStringA("[RmlUI][DX9] VertexBuffer lock failed\n");
		return 0;
	}

	FDx9Vertex* DstVertices = static_cast<FDx9Vertex*>(VertexMemory);

	for (int i = 0; i < Geometry->NumVertices; ++i)
	{
		const Rml::Vertex& Src = vertices[i];

		DstVertices[i].X = Src.position.x;
		DstVertices[i].Y = Src.position.y;
		DstVertices[i].Z = 0.0f;
		DstVertices[i].Color = ConvertColor(Src.colour);
		DstVertices[i].U = Src.tex_coord.x;
		DstVertices[i].V = Src.tex_coord.y;
	}

	Geometry->VertexBuffer->Unlock();

	void* IndexMemory = nullptr;
	Hr = Geometry->IndexBuffer->Lock(0, IndexBytes, &IndexMemory, 0);

	if (FAILED(Hr))
	{
		ReleaseGeometry(reinterpret_cast<Rml::CompiledGeometryHandle>(Geometry));
		OutputDebugStringA("[RmlUI][DX9] IndexBuffer lock failed\n");
		return 0;
	}

	unsigned int* DstIndices = static_cast<unsigned int*>(IndexMemory);

	for (int i = 0; i < Geometry->NumIndices; ++i)
		DstIndices[i] = static_cast<unsigned int>(indices[i]);

	Geometry->IndexBuffer->Unlock();

	return reinterpret_cast<Rml::CompiledGeometryHandle>(Geometry);
}

void RmlRenderDX9::RenderGeometry(
	Rml::CompiledGeometryHandle GeometryHandle,
	Rml::Vector2f Translation,
	Rml::TextureHandle TextureHandle
)
{
	if (
		!Device ||
		!GeometryHandle
	)
	{
		return;
	}

	FCompiledGeometry* Geometry =
		reinterpret_cast<FCompiledGeometry*>(
			GeometryHandle
		);

	FTextureHandle* TextureData =
		reinterpret_cast<FTextureHandle*>(
			TextureHandle
		);

	IDirect3DTexture9* Texture =
		nullptr;

	bool bExternalCharacterTexture =
	false;

	if (TextureData)
	{
		if (
			TextureData->
				bExternalCharacterPortrait
		)
		{
			Texture =
				CharacterPortraitTexture;

			bExternalCharacterTexture =
				true;
		}
		else if (
			TextureData->
				bExternalCharacterPreview
		)
		{
			Texture =
				CharacterPreviewTexture;

			bExternalCharacterTexture =
				true;
		}
		else
		{
			Texture =
				TextureData->Texture;
		}
	}

	constexpr float HalfPixelOffset =
		-0.5f;

	const D3DMATRIX TranslationMatrix =
	MakeTranslation(
		Translation.x +
			HalfPixelOffset,
		Translation.y +
			HalfPixelOffset,
		0.0f
	);

	D3DMATRIX World{};

	D3DXMatrixMultiply(
		reinterpret_cast<D3DXMATRIX*>(
			&World
		),
		reinterpret_cast<const D3DXMATRIX*>(
			&TranslationMatrix
		),
		reinterpret_cast<const D3DXMATRIX*>(
			&CurrentTransform
		)
	);

	Device->SetTransform(
		D3DTS_WORLD,
		&World
	);

	Device->SetStreamSource(
		0,
		Geometry->VertexBuffer,
		0,
		sizeof(FDx9Vertex)
	);

	Device->SetIndices(
		Geometry->IndexBuffer
	);

	Device->SetFVF(
		VertexFVF
	);

	if (Texture)
	{
		Device->SetTexture(
			0,
			Texture
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_COLOROP,
			D3DTOP_MODULATE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_COLORARG1,
			D3DTA_TEXTURE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_COLORARG2,
			D3DTA_DIFFUSE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_ALPHAOP,
			D3DTOP_MODULATE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_ALPHAARG1,
			D3DTA_TEXTURE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_ALPHAARG2,
			D3DTA_DIFFUSE
		);
	}
	else
	{
		Device->SetTexture(
			0,
			nullptr
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_COLOROP,
			D3DTOP_SELECTARG1
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_COLORARG1,
			D3DTA_DIFFUSE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_ALPHAOP,
			D3DTOP_SELECTARG1
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_ALPHAARG1,
			D3DTA_DIFFUSE
		);
	}

	if (bExternalCharacterTexture)
	{
		Device->SetRenderState(
			D3DRS_SRCBLEND,
			D3DBLEND_SRCALPHA
		);
	}
	else
	{
		Device->SetRenderState(
			D3DRS_SRCBLEND,
			D3DBLEND_ONE
		);
	}

	const UINT PrimitiveCount =
		static_cast<UINT>(
			Geometry->NumIndices / 3
		);

	Device->DrawIndexedPrimitive(
		D3DPT_TRIANGLELIST,
		0,
		0,
		static_cast<UINT>(
			Geometry->NumVertices
		),
		0,
		PrimitiveCount
	);

	if (bExternalCharacterTexture)
	{
		Device->SetRenderState(
			D3DRS_SRCBLEND,
			D3DBLEND_ONE
		);
	}
}

void RmlRenderDX9::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
	if (!geometry)
		return;

	FCompiledGeometry* Geometry = reinterpret_cast<FCompiledGeometry*>(geometry);

	if (Geometry->VertexBuffer)
	{
		Geometry->VertexBuffer->Release();
		Geometry->VertexBuffer = nullptr;
	}

	if (Geometry->IndexBuffer)
	{
		Geometry->IndexBuffer->Release();
		Geometry->IndexBuffer = nullptr;
	}

	delete Geometry;
}

std::wstring RmlRenderDX9::ResolvePathW(const Rml::String& path) const
{
	if (path.empty())
		return std::wstring();

	const int Required = MultiByteToWideChar(
		CP_UTF8,
		0,
		path.c_str(),
		static_cast<int>(path.size()),
		nullptr,
		0
	);

	if (Required <= 0)
	{
		OutputDebugStringA(
			"[RmlUI][DX9] ResolvePathW failed: UTF-8 conversion failed\n"
		);

		return std::wstring();
	}

	std::wstring WidePath;
	WidePath.resize(static_cast<size_t>(Required));

	MultiByteToWideChar(
		CP_UTF8,
		0,
		path.c_str(),
		static_cast<int>(path.size()),
		&WidePath[0],
		Required
	);

	for (wchar_t& Character : WidePath)
	{
		if (Character == L'/')
			Character = L'\\';
	}

	const bool IsAbsolute =
		(WidePath.size() >= 2 && WidePath[1] == L':') ||
		(!WidePath.empty() &&
			(WidePath[0] == L'\\' || WidePath[0] == L'/'));

	if (IsAbsolute)
		return WidePath;

	if (DataRoot.empty())
		return WidePath;
	
	const std::wstring DataPath =
		DataRoot + L"\\" + WidePath;

	if (RmlFileExistsW(DataPath))
		return DataPath;
	
	const std::wstring StudioPath =
		DataRoot + L"\\Rml\\Assets\\" + WidePath;

	if (RmlFileExistsW(StudioPath))
		return StudioPath;

	wchar_t DebugText[2048]{};

	_snwprintf_s(
		DebugText,
		_countof(DebugText),
		_TRUNCATE,
		L"[RmlUI][DX9] Texture path not found. Source='%ls' Data='%ls' Studio='%ls'\n",
		WidePath.c_str(),
		DataPath.c_str(),
		StudioPath.c_str()
	);

	OutputDebugStringW(DebugText);
	
	return DataPath;
}

bool RmlRenderDX9::CreateTextureFromRGBA(const unsigned char* PixelsRGBA, int Width, int Height, IDirect3DTexture9** OutTexture)
{
	if (!Device || !PixelsRGBA || Width <= 0 || Height <= 0 || !OutTexture)
		return false;

	*OutTexture = nullptr;

	IDirect3DTexture9* Texture = nullptr;

	HRESULT Hr = Device->CreateTexture(
		static_cast<UINT>(Width),
		static_cast<UINT>(Height),
		1,
		0,
		D3DFMT_A8R8G8B8,
		D3DPOOL_MANAGED,
		&Texture,
		nullptr
	);

	if (FAILED(Hr) || !Texture)
		return false;

	D3DLOCKED_RECT Locked{};
	Hr = Texture->LockRect(0, &Locked, nullptr, 0);

	if (FAILED(Hr))
	{
		Texture->Release();
		return false;
	}

	for (int Y = 0; Y < Height; ++Y)
	{
		DWORD* Dst = reinterpret_cast<DWORD*>(reinterpret_cast<unsigned char*>(Locked.pBits) + Locked.Pitch * Y);

		for (int X = 0; X < Width; ++X)
		{
			const int SrcIndex = (Y * Width + X) * 4;

			const unsigned char R = PixelsRGBA[SrcIndex + 0];
			const unsigned char G = PixelsRGBA[SrcIndex + 1];
			const unsigned char B = PixelsRGBA[SrcIndex + 2];
			const unsigned char A = PixelsRGBA[SrcIndex + 3];

			Dst[X] = D3DCOLOR_ARGB(A, R, G, B);
		}
	}

	Texture->UnlockRect(0);

	*OutTexture = Texture;
	return true;
}

static bool PremultiplyTextureAlpha(
	IDirect3DTexture9* Texture
)
{
	if (!Texture)
		return false;

	D3DSURFACE_DESC Description{};

	HRESULT Result = Texture->GetLevelDesc(
		0,
		&Description
	);

	if (FAILED(Result))
		return false;

	if (Description.Format != D3DFMT_A8R8G8B8)
		return false;

	D3DLOCKED_RECT LockedRectangle{};

	Result = Texture->LockRect(
		0,
		&LockedRectangle,
		nullptr,
		0
	);

	if (FAILED(Result) || !LockedRectangle.pBits)
		return false;

	for (UINT Y = 0; Y < Description.Height; ++Y)
	{
		DWORD* Pixels = reinterpret_cast<DWORD*>(
			reinterpret_cast<unsigned char*>(
				LockedRectangle.pBits
			) +
			LockedRectangle.Pitch * Y
		);

		for (UINT X = 0; X < Description.Width; ++X)
		{
			const DWORD Pixel = Pixels[X];

			const unsigned int Alpha =
				(Pixel >> 24) & 0xFF;

			unsigned int Red =
				(Pixel >> 16) & 0xFF;

			unsigned int Green =
				(Pixel >> 8) & 0xFF;

			unsigned int Blue =
				Pixel & 0xFF;

			Red = (Red * Alpha + 127) / 255;
			Green = (Green * Alpha + 127) / 255;
			Blue = (Blue * Alpha + 127) / 255;

			Pixels[X] = D3DCOLOR_ARGB(
				Alpha,
				Red,
				Green,
				Blue
			);
		}
	}

	Texture->UnlockRect(0);
	return true;
}

bool RmlRenderDX9::LoadTextureD3DX(
	const std::wstring& Filename,
	Rml::Vector2i& OutDimensions,
	IDirect3DTexture9** OutTexture
)
{
	if (!Device || !OutTexture || Filename.empty())
		return false;

	*OutTexture = nullptr;
	OutDimensions = Rml::Vector2i(0, 0);

	D3DXIMAGE_INFO Information{};
	IDirect3DTexture9* Texture = nullptr;

	const HRESULT Result =
		D3DXCreateTextureFromFileExW(
			Device,
			Filename.c_str(),
			D3DX_DEFAULT,
			D3DX_DEFAULT,
			1,
			0,
			D3DFMT_A8R8G8B8,
			D3DPOOL_MANAGED,
			D3DX_FILTER_LINEAR,
			D3DX_FILTER_LINEAR,
			0,
			&Information,
			nullptr,
			&Texture
		);

	if (FAILED(Result) || !Texture)
	{
		wchar_t Text[2048]{};

		_snwprintf_s(
			Text,
			_countof(Text),
			_TRUNCATE,
			L"[RmlUI][DX9] Texture load failed: 0x%08X | %ls\n",
			static_cast<unsigned int>(Result),
			Filename.c_str()
		);

		OutputDebugStringW(Text);

		if (Texture)
			Texture->Release();

		return false;
	}

	if (!PremultiplyTextureAlpha(Texture))
	{
		wchar_t Text[2048]{};

		_snwprintf_s(
			Text,
			_countof(Text),
			_TRUNCATE,
			L"[RmlUI][DX9] Texture premultiply failed: %ls\n",
			Filename.c_str()
		);

		OutputDebugStringW(Text);

		Texture->Release();
		return false;
	}

	OutDimensions.x =
		static_cast<int>(Information.Width);

	OutDimensions.y =
		static_cast<int>(Information.Height);

	*OutTexture = Texture;

	wchar_t Text[2048]{};

	_snwprintf_s(
		Text,
		_countof(Text),
		_TRUNCATE,
		L"[RmlUI][DX9] Texture loaded: %ls (%dx%d)\n",
		Filename.c_str(),
		OutDimensions.x,
		OutDimensions.y
	);

	OutputDebugStringW(Text);

	return true;
}

Rml::TextureHandle RmlRenderDX9::LoadTexture(
	Rml::Vector2i& TextureDimensions,
	const Rml::String& Source
)
{
	TextureDimensions.x = 0;
	TextureDimensions.y = 0;

	if (!Device)
		return 0;

	const bool bCharacterPortraitSource =
	Source == "rml://character-portrait" ||
	Source == "rml:/character-portrait";

	if (bCharacterPortraitSource)
	{
		TextureDimensions.x =
			512;

		TextureDimensions.y =
			512;

		FTextureHandle* Handle =
			new FTextureHandle();

		Handle->
			bExternalCharacterPortrait =
				true;

		r3dOutToLog(
			"[RmlUI][DX9] Registered character "
			"portrait texture source: %s\n",
			Source.c_str()
		);

		return reinterpret_cast<
			Rml::TextureHandle
		>(
			Handle
		);
	}

	const bool bCharacterPreviewSource =
	Source == "rml://character-preview" ||
	Source == "rml:/character-preview";

	if (bCharacterPreviewSource)
	{
		TextureDimensions.x =
			ViewWidth;

		TextureDimensions.y =
			ViewHeight;

		FTextureHandle* Handle =
			new FTextureHandle();

		Handle->
			bExternalCharacterPreview =
				true;

		r3dOutToLog(
			"[RmlUI][DX9] Registered character "
			"preview texture source: %s\n",
			Source.c_str()
		);

		return reinterpret_cast<
			Rml::TextureHandle
		>(
			Handle
		);
	}

	if (Source.empty())
		return 0;

	const std::wstring FullPath =
		ResolvePathW(
			Source
		);

	if (FullPath.empty())
		return 0;

	IDirect3DTexture9* Texture =
		nullptr;

	if (!LoadTextureD3DX(
		FullPath,
		TextureDimensions,
		&Texture
	))
	{
		std::string DebugText =
			"[RmlUI][DX9] Failed to load texture source: ";

		DebugText += Source;
		DebugText += "\n";

		OutputDebugStringA(
			DebugText.c_str()
		);

		return 0;
	}

	FTextureHandle* Handle =
		new FTextureHandle();

	Handle->Texture =
		Texture;

	return reinterpret_cast<
		Rml::TextureHandle
	>(
		Handle
	);
}

Rml::TextureHandle RmlRenderDX9::GenerateTexture(
	Rml::Span<const Rml::byte> Source,
	Rml::Vector2i SourceDimensions
)
{
	if (
		Source.empty() ||
		SourceDimensions.x <= 0 ||
		SourceDimensions.y <= 0
	)
	{
		return 0;
	}

	IDirect3DTexture9* Texture =
		nullptr;

	if (!CreateTextureFromRGBA(
		reinterpret_cast<
			const unsigned char*
		>(
			Source.data()
		),
		SourceDimensions.x,
		SourceDimensions.y,
		&Texture
	))
	{
		OutputDebugStringA(
			"[RmlUI][DX9] GenerateTexture failed\n"
		);

		return 0;
	}

	FTextureHandle* Handle =
		new FTextureHandle();

	Handle->Texture =
		Texture;

	return reinterpret_cast<
		Rml::TextureHandle
	>(
		Handle
	);
}

void RmlRenderDX9::ReleaseTexture(
	Rml::TextureHandle TextureHandle
)
{
	if (!TextureHandle)
		return;

	FTextureHandle* Handle =
		reinterpret_cast<FTextureHandle*>(
			TextureHandle
		);

	if (
		!Handle->
			bExternalCharacterPreview &&
		!Handle->
			bExternalCharacterPortrait &&
		Handle->Texture
	)
	{
		Handle->Texture->Release();
		Handle->Texture = nullptr;
	}

	delete Handle;
}

void RmlRenderDX9::EnableScissorRegion(bool enable)
{
	bScissorEnabled = enable;

	if (Device)
		(Device->SetRenderState)(RML_D3DRS_SCISSORTESTENABLE, enable ? TRUE : FALSE);
}

void RmlRenderDX9::SetScissorRegion(Rml::Rectanglei region)
{
	ScissorRect.left = std::max(0, region.Left());
	ScissorRect.top = std::max(0, region.Top());
	ScissorRect.right = std::min(ViewWidth, region.Right());
	ScissorRect.bottom = std::min(ViewHeight, region.Bottom());

	if (ScissorRect.right < ScissorRect.left)
		ScissorRect.right = ScissorRect.left;

	if (ScissorRect.bottom < ScissorRect.top)
		ScissorRect.bottom = ScissorRect.top;

	if (Device)
		Device->SetScissorRect(&ScissorRect);
}

void RmlRenderDX9::SetTransform(
	const Rml::Matrix4f* Transform
)
{
	if (Transform)
	{
		CurrentTransform =
			ConvertTransform(
				*Transform
			);
	}
	else
	{
		CurrentTransform =
			MakeIdentity();
	}
}

void RmlRenderDX9::ApplyClipMaskState()
{
	if (!Device)
		return;

	if (
		bClipMaskEnabled &&
		ClipMaskReference > 0
	)
	{
		(Device->SetRenderState)(
			RML_D3DRS_STENCILENABLE,
			TRUE
		);

		(Device->SetRenderState)(
			RML_D3DRS_STENCILFUNC,
			D3DCMP_EQUAL
		);

		(Device->SetRenderState)(
			RML_D3DRS_STENCILREF,
			ClipMaskReference
		);

		(Device->SetRenderState)(
			RML_D3DRS_STENCILMASK,
			0xFF
		);

		(Device->SetRenderState)(
			RML_D3DRS_STENCILWRITEMASK,
			0x00
		);

		(Device->SetRenderState)(
			RML_D3DRS_STENCILFAIL,
			D3DSTENCILOP_KEEP
		);

		(Device->SetRenderState)(
			RML_D3DRS_STENCILZFAIL,
			D3DSTENCILOP_KEEP
		);

		(Device->SetRenderState)(
			RML_D3DRS_STENCILPASS,
			D3DSTENCILOP_KEEP
		);
	}
	else
	{
		(Device->SetRenderState)(
			RML_D3DRS_STENCILENABLE,
			FALSE
		);
	}
}

void RmlRenderDX9::EnableClipMask(
	bool Enable
)
{
	bClipMaskEnabled =
		Enable;

	ApplyClipMaskState();
}

void RmlRenderDX9::RenderToClipMask(
	Rml::ClipMaskOperation Operation,
	Rml::CompiledGeometryHandle Geometry,
	Rml::Vector2f Translation
)
{
	if (
		!Device ||
		!Geometry ||
		!SharedDepthStencil
	)
	{
		return;
	}

	(Device->SetRenderState)(
		RML_D3DRS_STENCILENABLE,
		TRUE
	);

	(Device->SetRenderState)(
		RML_D3DRS_STENCILMASK,
		0xFF
	);

	(Device->SetRenderState)(
		RML_D3DRS_STENCILWRITEMASK,
		0xFF
	);

	(Device->SetRenderState)(
		RML_D3DRS_STENCILFAIL,
		D3DSTENCILOP_KEEP
	);

	(Device->SetRenderState)(
		RML_D3DRS_STENCILZFAIL,
		D3DSTENCILOP_KEEP
	);

	(Device->SetRenderState)(
		RML_D3DRS_COLORWRITEENABLE,
		0
	);

	(Device->SetRenderState)(
		RML_D3DRS_ALPHABLENDENABLE,
		FALSE
	);

	switch (Operation)
	{
	case Rml::ClipMaskOperation::Set:
	{
		Device->Clear(
			0,
			nullptr,
			D3DCLEAR_STENCIL,
			0,
			1.0f,
			0
		);

		ClipMaskReference =
			1;

		(Device->SetRenderState)(
			RML_D3DRS_STENCILFUNC,
			D3DCMP_ALWAYS
		);

		(Device->SetRenderState)(
			RML_D3DRS_STENCILREF,
			ClipMaskReference
		);

		(Device->SetRenderState)(
			RML_D3DRS_STENCILPASS,
			D3DSTENCILOP_REPLACE
		);

		break;
	}

	case Rml::ClipMaskOperation::SetInverse:
	{
		Device->Clear(
			0,
			nullptr,
			D3DCLEAR_STENCIL,
			0,
			1.0f,
			1
		);

		ClipMaskReference =
			1;

		(Device->SetRenderState)(
			RML_D3DRS_STENCILFUNC,
			D3DCMP_ALWAYS
		);

		(Device->SetRenderState)(
			RML_D3DRS_STENCILREF,
			0
		);

		(Device->SetRenderState)(
			RML_D3DRS_STENCILPASS,
			D3DSTENCILOP_ZERO
		);

		break;
	}

	case Rml::ClipMaskOperation::Intersect:
	{
		if (ClipMaskReference == 0)
		{
			ClipMaskReference =
				1;

			(Device->SetRenderState)(
				RML_D3DRS_STENCILFUNC,
				D3DCMP_ALWAYS
			);

			(Device->SetRenderState)(
				RML_D3DRS_STENCILREF,
				ClipMaskReference
			);

			(Device->SetRenderState)(
				RML_D3DRS_STENCILPASS,
				D3DSTENCILOP_REPLACE
			);
		}
		else
		{
			(Device->SetRenderState)(
				RML_D3DRS_STENCILFUNC,
				D3DCMP_EQUAL
			);

			(Device->SetRenderState)(
				RML_D3DRS_STENCILREF,
				ClipMaskReference
			);

			(Device->SetRenderState)(
				RML_D3DRS_STENCILPASS,
				D3DSTENCILOP_INCRSAT
			);

			++ClipMaskReference;

			if (ClipMaskReference > 255)
			{
				ClipMaskReference =
					255;
			}
		}

		break;
	}
	}

	RenderGeometry(
		Geometry,
		Translation,
		0
	);

	(Device->SetRenderState)(
		RML_D3DRS_COLORWRITEENABLE,
		RML_COLOR_WRITE_ALL
	);

	(Device->SetRenderState)(
		RML_D3DRS_ALPHABLENDENABLE,
		TRUE
	);

	ApplyClipMaskState();
}

bool RmlRenderDX9::CreateRenderTargetTexture(
	int Width,
	int Height,
	IDirect3DTexture9** OutTexture,
	IDirect3DSurface9** OutSurface
)
{
	if (
		!Device ||
		Width <= 0 ||
		Height <= 0 ||
		!OutTexture ||
		!OutSurface
	)
	{
		return false;
	}

	*OutTexture =
		nullptr;

	*OutSurface =
		nullptr;

	IDirect3DTexture9* Texture =
		nullptr;

	HRESULT Result =
		Device->CreateTexture(
			static_cast<UINT>(
				Width
			),
			static_cast<UINT>(
				Height
			),
			1,
			D3DUSAGE_RENDERTARGET,
			D3DFMT_A8R8G8B8,
			D3DPOOL_DEFAULT,
			&Texture,
			nullptr
		);

	if (
		FAILED(Result) ||
		!Texture
	)
	{
		OutputDebugStringA(
			"[RmlUI][DX9] Create render-target texture failed\n"
		);

		return false;
	}

	IDirect3DSurface9* Surface =
		nullptr;

	Result =
		Texture->GetSurfaceLevel(
			0,
			&Surface
		);

	if (
		FAILED(Result) ||
		!Surface
	)
	{
		Texture->Release();

		OutputDebugStringA(
			"[RmlUI][DX9] Get render-target surface failed\n"
		);

		return false;
	}

	*OutTexture =
		Texture;

	*OutSurface =
		Surface;

	return true;
}

void RmlRenderDX9::ReleaseLayer(
	FRenderLayer& Layer
)
{
	if (Layer.Surface)
	{
		Layer.Surface->Release();
		Layer.Surface = nullptr;
	}

	if (Layer.Texture)
	{
		Layer.Texture->Release();
		Layer.Texture = nullptr;
	}

	Layer.Width =
		0;

	Layer.Height =
		0;
}

void RmlRenderDX9::ReleaseLayerResources()
{
	for (
		FRenderLayer& Layer :
		LayerPool
	)
	{
		ReleaseLayer(
			Layer
		);
	}

	LayerPool.clear();
}

void RmlRenderDX9::ReleaseSharedDepthStencil()
{
	if (SharedDepthStencil)
	{
		SharedDepthStencil->Release();
		SharedDepthStencil = nullptr;
	}
}

bool RmlRenderDX9::EnsureSharedDepthStencil()
{
	if (!Device)
		return false;

	if (SharedDepthStencil)
	{
		D3DSURFACE_DESC Description{};

		if (
			SUCCEEDED(
				SharedDepthStencil->
					GetDesc(
						&Description
					)
			) &&
			static_cast<int>(
				Description.Width
			) == ViewWidth &&
			static_cast<int>(
				Description.Height
			) == ViewHeight
		)
		{
			return true;
		}

		ReleaseSharedDepthStencil();
	}

	HRESULT Result =
		Device->CreateDepthStencilSurface(
			static_cast<UINT>(
				ViewWidth
			),
			static_cast<UINT>(
				ViewHeight
			),
			D3DFMT_D24S8,
			D3DMULTISAMPLE_NONE,
			0,
			TRUE,
			&SharedDepthStencil,
			nullptr
		);

	if (
		FAILED(Result) ||
		!SharedDepthStencil
	)
	{
		OutputDebugStringA(
			"[RmlUI][DX9] D24S8 stencil surface creation failed\n"
		);

		return false;
	}

	return true;
}

bool RmlRenderDX9::EnsureLayer(
	size_t LayerIndex
)
{
	if (!Device)
		return false;

	while (
		LayerPool.size() <=
		LayerIndex
	)
	{
		LayerPool.emplace_back();
	}

	FRenderLayer& Layer =
		LayerPool[
			LayerIndex
		];

	if (
		Layer.Texture &&
		Layer.Surface &&
		Layer.Width == ViewWidth &&
		Layer.Height == ViewHeight
	)
	{
		return true;
	}

	ReleaseLayer(
		Layer
	);

	if (!CreateRenderTargetTexture(
		ViewWidth,
		ViewHeight,
		&Layer.Texture,
		&Layer.Surface
	))
	{
		return false;
	}

	Layer.Width =
		ViewWidth;

	Layer.Height =
		ViewHeight;

	return true;
}

Rml::LayerHandle RmlRenderDX9::GetTopLayerHandle() const
{
	if (ActiveLayerCount == 0)
		return 0;

	return static_cast<Rml::LayerHandle>(
		ActiveLayerCount -
		1
	);
}

IDirect3DSurface9* RmlRenderDX9::GetLayerSurface(
	Rml::LayerHandle Layer
) const
{
	const size_t LayerIndex =
		static_cast<size_t>(
			Layer
		);

	if (LayerIndex == 0)
		return BaseRenderTarget;

	const size_t PoolIndex =
		LayerIndex -
		1;

	if (
		PoolIndex >=
		LayerPool.size()
	)
	{
		return nullptr;
	}

	return LayerPool[
		PoolIndex
	].Surface;
}

IDirect3DTexture9* RmlRenderDX9::GetLayerTexture(
	Rml::LayerHandle Layer
) const
{
	const size_t LayerIndex =
		static_cast<size_t>(
			Layer
		);

	if (LayerIndex == 0)
		return nullptr;

	const size_t PoolIndex =
		LayerIndex -
		1;

	if (
		PoolIndex >=
		LayerPool.size()
	)
	{
		return nullptr;
	}

	return LayerPool[
		PoolIndex
	].Texture;
}

void RmlRenderDX9::BindLayer(
	Rml::LayerHandle Layer
)
{
	if (!Device)
		return;

	IDirect3DSurface9* Surface =
		GetLayerSurface(
			Layer
		);

	if (!Surface)
		return;

	Device->SetRenderTarget(
		0,
		Surface
	);

	Device->SetDepthStencilSurface(
		SharedDepthStencil
	);

	ApplyClipMaskState();
}

Rml::LayerHandle RmlRenderDX9::PushLayer()
{
	if (
		!Device ||
		!bFrameOpen
	)
	{
		return 0;
	}

	const size_t NewLayerHandle =
		ActiveLayerCount;

	const size_t PoolIndex =
		NewLayerHandle -
		1;

	if (!EnsureLayer(
		PoolIndex
	))
	{
		return GetTopLayerHandle();
	}

	++ActiveLayerCount;

	BindLayer(
		static_cast<Rml::LayerHandle>(
			NewLayerHandle
		)
	);

	(Device->SetRenderState)(
		RML_D3DRS_SCISSORTESTENABLE,
		FALSE
	);

	Device->Clear(
		0,
		nullptr,
		D3DCLEAR_TARGET,
		0x00000000,
		1.0f,
		0
	);

	(Device->SetRenderState)(
		RML_D3DRS_SCISSORTESTENABLE,
		bScissorEnabled
			? TRUE
			: FALSE
	);

	if (bScissorEnabled)
	{
		Device->SetScissorRect(
			&ScissorRect
		);
	}

	return static_cast<Rml::LayerHandle>(
		NewLayerHandle
	);
}

void RmlRenderDX9::PopLayer()
{
	if (
		!Device ||
		ActiveLayerCount <= 1
	)
	{
		return;
	}

	--ActiveLayerCount;

	BindLayer(
		GetTopLayerHandle()
	);
}

bool RmlRenderDX9::CopySurface(
	IDirect3DSurface9* Source,
	IDirect3DSurface9* Destination,
	const RECT* SourceRectangle,
	const RECT* DestinationRectangle
)
{
	if (
		!Device ||
		!Source ||
		!Destination
	)
	{
		return false;
	}

	const HRESULT Result =
		Device->StretchRect(
			Source,
			SourceRectangle,
			Destination,
			DestinationRectangle,
			D3DTEXF_NONE
		);

	return SUCCEEDED(
		Result
	);
}

void RmlRenderDX9::DrawLayerTexture(
	IDirect3DTexture9* SourceTexture,
	IDirect3DTexture9* MaskTexture,
	float Opacity,
	bool bEnableBlend
)
{
	if (
		!Device ||
		!SourceTexture
	)
	{
		return;
	}

	const float Left =
		-0.5f;

	const float Top =
		-0.5f;

	const float Right =
		static_cast<float>(
			ViewWidth
		) -
		0.5f;

	const float Bottom =
		static_cast<float>(
			ViewHeight
		) -
		0.5f;

	const DWORD White =
		0xFFFFFFFF;

	const FScreenVertex Vertices[4] =
	{
		{
			Left,
			Top,
			0.0f,
			1.0f,
			White,
			0.0f,
			0.0f
		},
		{
			Right,
			Top,
			0.0f,
			1.0f,
			White,
			1.0f,
			0.0f
		},
		{
			Left,
			Bottom,
			0.0f,
			1.0f,
			White,
			0.0f,
			1.0f
		},
		{
			Right,
			Bottom,
			0.0f,
			1.0f,
			White,
			1.0f,
			1.0f
		}
	};

	Device->SetPixelShader(
		nullptr
	);

	Device->SetFVF(
		ScreenVertexFVF
	);

	(Device->SetRenderState)(
		RML_D3DRS_ZENABLE,
		FALSE
	);

	(Device->SetRenderState)(
		RML_D3DRS_ZWRITEENABLE,
		FALSE
	);

	(Device->SetRenderState)(
		RML_D3DRS_ALPHABLENDENABLE,
		bEnableBlend
			? TRUE
			: FALSE
	);

	(Device->SetRenderState)(
		RML_D3DRS_SRCBLEND,
		D3DBLEND_ONE
	);

	(Device->SetRenderState)(
		RML_D3DRS_DESTBLEND,
		D3DBLEND_INVSRCALPHA
	);

	Device->SetTexture(
		0,
		SourceTexture
	);

	Device->SetTextureStageState(
		0,
		D3DTSS_COLOROP,
		D3DTOP_SELECTARG1
	);

	Device->SetTextureStageState(
		0,
		D3DTSS_COLORARG1,
		D3DTA_TEXTURE
	);

	Device->SetTextureStageState(
		0,
		D3DTSS_ALPHAOP,
		D3DTOP_SELECTARG1
	);

	Device->SetTextureStageState(
		0,
		D3DTSS_ALPHAARG1,
		D3DTA_TEXTURE
	);

	DWORD Stage =
		1;

	Opacity =
		std::max(
			0.0f,
			std::min(
				1.0f,
				Opacity
			)
		);

	if (Opacity < 0.9999f)
	{
		const DWORD OpacityByte =
			static_cast<DWORD>(
				Opacity *
				255.0f +
				0.5f
			);

		const DWORD TextureFactor =
			D3DCOLOR_ARGB(
				OpacityByte,
				OpacityByte,
				OpacityByte,
				OpacityByte
			);

		(Device->SetRenderState)(
			RML_D3DRS_TEXTUREFACTOR,
			TextureFactor
		);

		Device->SetTexture(
			Stage,
			nullptr
		);

		Device->SetTextureStageState(
			Stage,
			D3DTSS_COLOROP,
			D3DTOP_MODULATE
		);

		Device->SetTextureStageState(
			Stage,
			D3DTSS_COLORARG1,
			D3DTA_CURRENT
		);

		Device->SetTextureStageState(
			Stage,
			D3DTSS_COLORARG2,
			D3DTA_TFACTOR
		);

		Device->SetTextureStageState(
			Stage,
			D3DTSS_ALPHAOP,
			D3DTOP_MODULATE
		);

		Device->SetTextureStageState(
			Stage,
			D3DTSS_ALPHAARG1,
			D3DTA_CURRENT
		);

		Device->SetTextureStageState(
			Stage,
			D3DTSS_ALPHAARG2,
			D3DTA_TFACTOR
		);

		++Stage;
	}

	if (MaskTexture)
	{
		Device->SetTexture(
			Stage,
			MaskTexture
		);

		Device->SetTextureStageState(
			Stage,
			D3DTSS_COLOROP,
			D3DTOP_MODULATE
		);

		Device->SetTextureStageState(
			Stage,
			D3DTSS_COLORARG1,
			D3DTA_CURRENT
		);

		Device->SetTextureStageState(
			Stage,
			D3DTSS_COLORARG2,
			D3DTA_TEXTURE |
				D3DTA_ALPHAREPLICATE
		);

		Device->SetTextureStageState(
			Stage,
			D3DTSS_ALPHAOP,
			D3DTOP_MODULATE
		);

		Device->SetTextureStageState(
			Stage,
			D3DTSS_ALPHAARG1,
			D3DTA_CURRENT
		);

		Device->SetTextureStageState(
			Stage,
			D3DTSS_ALPHAARG2,
			D3DTA_TEXTURE
		);

		++Stage;
	}

	Device->SetTextureStageState(
		Stage,
		D3DTSS_COLOROP,
		D3DTOP_DISABLE
	);

	Device->SetTextureStageState(
		Stage,
		D3DTSS_ALPHAOP,
		D3DTOP_DISABLE
	);

	Device->DrawPrimitiveUP(
		D3DPT_TRIANGLESTRIP,
		2,
		Vertices,
		sizeof(FScreenVertex)
	);

	for (
		DWORD TextureStage = 0;
		TextureStage < 4;
		++TextureStage
	)
	{
		Device->SetTexture(
			TextureStage,
			nullptr
		);
	}

	SetupRenderState();
}

void RmlRenderDX9::CompositeLayers(
	Rml::LayerHandle Source,
	Rml::LayerHandle Destination,
	Rml::BlendMode BlendMode,
	Rml::Span<const Rml::CompiledFilterHandle> Filters
)
{
	if (!Device)
		return;

	IDirect3DTexture9* CurrentTexture =
		GetLayerTexture(
			Source
		);

	if (!CurrentTexture)
		return;

	if (
		!CreateFilterShaders() ||
		!EnsurePostProcessTargets()
	)
	{
		return;
	}

	int CurrentPostProcessIndex =
		-1;

	float FinalOpacity =
		1.0f;

	IDirect3DTexture9* MaskTexture =
		nullptr;

	for (
		const Rml::CompiledFilterHandle FilterHandle :
		Filters
	)
	{
		if (!FilterHandle)
			continue;

		const FCompiledFilter* Filter =
			reinterpret_cast<
				const FCompiledFilter*
			>(
				FilterHandle
			);

		switch (Filter->Type)
		{
		case ECompiledFilterType::Opacity:
		{
			FinalOpacity *=
				Filter->Opacity;

			break;
		}

		case ECompiledFilterType::Blur:
		{
			IDirect3DTexture9* BlurredTexture =
				nullptr;

			int BlurredTargetIndex =
				-1;

			if (ApplyGaussianBlur(
				CurrentTexture,
				CurrentPostProcessIndex,
				Filter->Sigma,
				BlurredTexture,
				BlurredTargetIndex
			))
			{
				CurrentTexture =
					BlurredTexture;

				CurrentPostProcessIndex =
					BlurredTargetIndex;
			}

			break;
		}

		case ECompiledFilterType::DropShadow:
		{
			IDirect3DTexture9* BlurredTexture =
				nullptr;

			int BlurredTargetIndex =
				-1;

			if (!ApplyGaussianBlur(
				CurrentTexture,
				CurrentPostProcessIndex,
				Filter->Sigma,
				BlurredTexture,
				BlurredTargetIndex
			))
			{
				break;
			}

			const int CompositionTargetIndex =
				FindPostProcessTarget(
					CurrentPostProcessIndex,
					BlurredTargetIndex
				);

			if (CompositionTargetIndex < 0)
				break;

			const float ShadowColor[4] =
			{
				static_cast<float>(
					Filter->Color.red
				) / 255.0f,

				static_cast<float>(
					Filter->Color.green
				) / 255.0f,

				static_cast<float>(
					Filter->Color.blue
				) / 255.0f,

				static_cast<float>(
					Filter->Color.alpha
				) / 255.0f
			};

			Device->SetPixelShaderConstantF(
				0,
				ShadowColor,
				1
			);

			/*
			 * Сначала рисуем размытую альфу,
			 * окрашенную цветом тени и смещённую
			 * на Offset.
			 */
			DrawPostProcessQuad(
				BlurredTexture,
				PostProcessTargets[
					CompositionTargetIndex
				].Surface,
				ShadowPixelShader,
				Filter->Offset.x,
				Filter->Offset.y,
				1.0f,
				false,
				true
			);

			/*
			 * Затем поверх тени рисуем
			 * исходный элемент.
			 */
			DrawPostProcessQuad(
				CurrentTexture,
				PostProcessTargets[
					CompositionTargetIndex
				].Surface,
				nullptr,
				0.0f,
				0.0f,
				1.0f,
				true,
				false
			);

			CurrentTexture =
				PostProcessTargets[
					CompositionTargetIndex
				].Texture;

			CurrentPostProcessIndex =
				CompositionTargetIndex;

			break;
		}

		case ECompiledFilterType::MaskImage:
		{
			MaskTexture =
				Filter->MaskTexture;

			break;
		}

		default:
			break;
		}
	}

	const Rml::LayerHandle TopLayer =
		GetTopLayerHandle();

	BindLayer(
		Destination
	);

	SetupRenderState();

	DrawLayerTexture(
		CurrentTexture,
		MaskTexture,
		FinalOpacity,
		BlendMode !=
			Rml::BlendMode::Replace
	);

	if (
		Destination !=
		TopLayer
	)
	{
		BindLayer(
			TopLayer
		);
	}

	SetupRenderState();
}

Rml::TextureHandle RmlRenderDX9::SaveLayerAsTexture()
{
	if (
		!Device ||
		ActiveLayerCount == 0
	)
	{
		return 0;
	}

	const int Width =
		ScissorRect.right -
		ScissorRect.left;

	const int Height =
		ScissorRect.bottom -
		ScissorRect.top;

	if (
		Width <= 0 ||
		Height <= 0
	)
	{
		return 0;
	}

	IDirect3DTexture9* Texture =
		nullptr;

	IDirect3DSurface9* Surface =
		nullptr;

	if (!CreateRenderTargetTexture(
		Width,
		Height,
		&Texture,
		&Surface
	))
	{
		return 0;
	}

	const RECT SourceRectangle =
	{
		ScissorRect.left,
		ScissorRect.top,
		ScissorRect.right,
		ScissorRect.bottom
	};

	const RECT DestinationRectangle =
	{
		0,
		0,
		Width,
		Height
	};

	IDirect3DSurface9* SourceSurface =
		GetLayerSurface(
			GetTopLayerHandle()
		);

	const bool bCopied =
		CopySurface(
			SourceSurface,
			Surface,
			&SourceRectangle,
			&DestinationRectangle
		);

	Surface->Release();

	if (!bCopied)
	{
		Texture->Release();
		return 0;
	}

	FTextureHandle* Handle =
		new FTextureHandle();

	Handle->Texture =
		Texture;

	return reinterpret_cast<
		Rml::TextureHandle
	>(
		Handle
	);
}

Rml::CompiledFilterHandle
RmlRenderDX9::SaveLayerAsMaskImage()
{
	if (
		!Device ||
		ActiveLayerCount == 0
	)
	{
		return 0;
	}

	IDirect3DTexture9* Texture =
		nullptr;

	IDirect3DSurface9* Surface =
		nullptr;

	if (!CreateRenderTargetTexture(
		ViewWidth,
		ViewHeight,
		&Texture,
		&Surface
	))
	{
		return 0;
	}

	IDirect3DSurface9* SourceSurface =
		GetLayerSurface(
			GetTopLayerHandle()
		);

	const bool bCopied =
		CopySurface(
			SourceSurface,
			Surface,
			nullptr,
			nullptr
		);

	Surface->Release();

	if (!bCopied)
	{
		Texture->Release();
		return 0;
	}

	FCompiledFilter* Filter =
		new FCompiledFilter();

	Filter->Type =
		ECompiledFilterType::MaskImage;

	Filter->MaskTexture =
		Texture;

	return reinterpret_cast<
		Rml::CompiledFilterHandle
	>(
		Filter
	);
}

Rml::CompiledFilterHandle
RmlRenderDX9::CompileFilter(
	const Rml::String& Name,
	const Rml::Dictionary& Parameters
)
{
	FCompiledFilter* Filter =
		new FCompiledFilter();

	if (Name == "opacity")
	{
		Filter->Type =
			ECompiledFilterType::Opacity;

		Filter->Opacity =
			Rml::Get(
				Parameters,
				"value",
				1.0f
			);
	}
	else if (Name == "blur")
	{
		Filter->Type =
			ECompiledFilterType::Blur;

		Filter->Sigma =
			std::max(
				0.0f,
				Rml::Get(
					Parameters,
					"sigma",
					1.0f
				)
			);
	}
	else if (Name == "drop-shadow")
	{
		Filter->Type =
			ECompiledFilterType::DropShadow;

		Filter->Sigma =
			std::max(
				0.0f,
				Rml::Get(
					Parameters,
					"sigma",
					0.0f
				)
			);

		Filter->Offset =
			Rml::Get(
				Parameters,
				"offset",
				Rml::Vector2f(
					0.0f,
					0.0f
				)
			);

		Filter->Color =
			Rml::Get(
				Parameters,
				"color",
				Rml::Colourb()
			).ToPremultiplied();
	}
	else
	{
		Rml::Log::Message(
			Rml::Log::LT_WARNING,
			"DX9 backend: unsupported filter '%s'.",
			Name.c_str()
		);

		delete Filter;
		return 0;
	}

	return reinterpret_cast<
		Rml::CompiledFilterHandle
	>(
		Filter
	);
}

void RmlRenderDX9::ReleaseFilter(
	Rml::CompiledFilterHandle FilterHandle
)
{
	if (!FilterHandle)
		return;

	FCompiledFilter* Filter =
		reinterpret_cast<
			FCompiledFilter*
		>(
			FilterHandle
		);

	if (
		Filter->Type ==
			ECompiledFilterType::MaskImage &&
		Filter->MaskTexture
	)
	{
		Filter->MaskTexture->Release();
		Filter->MaskTexture = nullptr;
	}

	delete Filter;
}