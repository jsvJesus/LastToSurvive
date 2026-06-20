#include "r3dPCH.h"
#include "r3d.h"

#include "RmlRenderDX9.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/DecorationTypes.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>

#include <windows.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cstring>
#include <type_traits>
#include <wchar.h>

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

// Device->SetRenderTarget					=>	r3dRenderer->SetRT
// Device->SetDepthStencilSurface			=>	r3dRenderer->SetDSS
// Device->GetRenderTarget					=>	r3dRenderer->GetRT
// Device->GetDepthStencilSurface			=>	r3dRenderer->GetDSS
// Device->DrawPrimitiveUP					=>	r3dRenderer->DrawUP

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

static const D3DRENDERSTATETYPE RML_D3DRS_TWOSIDEDSTENCILMODE =
	static_cast<D3DRENDERSTATETYPE>(
		185
	);

static const D3DRENDERSTATETYPE  RML_D3DRS_SEPARATEALPHABLENDENABLE =
	static_cast<D3DRENDERSTATETYPE>(
		206
	);

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

static const char* RmlDx9ColorMatrixShaderSource =
R"RMLSHADER(
	sampler2D SourceSampler : register(s0);

	float4 ColorMatrixRow0 : register(c0);
	float4 ColorMatrixRow1 : register(c1);
	float4 ColorMatrixRow2 : register(c2);

	float4 main(float2 UV : TEXCOORD0) : COLOR0
	{
		/*
		 * Source texture уже premultiplied RGBA.
		 *
		 * Так как эти CSS-фильтры не меняют alpha,
		 * преобразование можно выполнять непосредственно
		 * в premultiplied-пространстве.
		 *
		 * Четвёртая компонента SourceColor — alpha,
		 * поэтому constant term матрицы автоматически
		 * умножается на alpha. На полностью прозрачных
		 * пикселях не появляется цветной фон.
		 */
		float4 SourceColor =
			tex2D(
				SourceSampler,
				UV
			);

		float4 Result;

		Result.r =
			dot(
				ColorMatrixRow0,
				SourceColor
			);

		Result.g =
			dot(
				ColorMatrixRow1,
				SourceColor
			);

		Result.b =
			dot(
				ColorMatrixRow2,
				SourceColor
			);

		Result.a =
			SourceColor.a;

		return Result;
	}
)RMLSHADER";

static const char* RmlDx9GradientShaderSource =
R"RMLSHADER(
	#define MAX_NUM_STOPS 16
	#define LINEAR 0
	#define RADIAL 1
	#define CONIC 2
	#define REPEATING_LINEAR 3
	#define REPEATING_RADIAL 4
	#define REPEATING_CONIC 5
	#define PI 3.14159265

	float4 GradientParams : register(c0);
	float4 GradientP : register(c1);
	float4 GradientV : register(c2);
	float4 StopColors[MAX_NUM_STOPS] : register(c3);
	float4 StopPositions0123 : register(c19);
	float4 StopPositions4567 : register(c20);
	float4 StopPositions891011 : register(c21);
	float4 StopPositions12131415 : register(c22);

	float GetStopPosition(int Index)
	{
		if (Index == 0) return StopPositions0123.x;
		if (Index == 1) return StopPositions0123.y;
		if (Index == 2) return StopPositions0123.z;
		if (Index == 3) return StopPositions0123.w;
		if (Index == 4) return StopPositions4567.x;
		if (Index == 5) return StopPositions4567.y;
		if (Index == 6) return StopPositions4567.z;
		if (Index == 7) return StopPositions4567.w;
		if (Index == 8) return StopPositions891011.x;
		if (Index == 9) return StopPositions891011.y;
		if (Index == 10) return StopPositions891011.z;
		if (Index == 11) return StopPositions891011.w;
		if (Index == 12) return StopPositions12131415.x;
		if (Index == 13) return StopPositions12131415.y;
		if (Index == 14) return StopPositions12131415.z;
		return StopPositions12131415.w;
	}

	float4 MixStopColors(float T, int NumStops)
	{
		float4 Color = StopColors[0];

		for (int Index = 1; Index < MAX_NUM_STOPS; ++Index)
		{
			if (Index < NumStops)
			{
				const float Previous =
					GetStopPosition(
						Index - 1
					);

				const float Current =
					GetStopPosition(
						Index
					);

				Color =
					lerp(
						Color,
						StopColors[Index],
						smoothstep(
							Previous,
							Current,
							T
						)
					);
			}
		}

		return Color;
	}

	float4 main(float2 UV : TEXCOORD0, float4 Diffuse : COLOR0) : COLOR0
	{
		const int Function =
			(int)GradientParams.x;

		const int NumStops =
			(int)GradientParams.y;

		float T =
			0.0;

		if (
			Function == LINEAR ||
			Function == REPEATING_LINEAR
		)
		{
			const float2 V =
				GradientV.xy;

			const float DistSquare =
				max(
					dot(
						V,
						V
					),
					0.000001
				);

			T =
				dot(
					V,
					UV - GradientP.xy
				) /
				DistSquare;
		}
		else if (
			Function == RADIAL ||
			Function == REPEATING_RADIAL
		)
		{
			T =
				length(
					GradientV.xy *
					(UV - GradientP.xy)
				);
		}
		else if (
			Function == CONIC ||
			Function == REPEATING_CONIC
		)
		{
			const float2 Direction =
				GradientV.xy;

			const float2 Delta =
				UV - GradientP.xy;

			const float2 Rotated =
				float2(
					Direction.x * Delta.x -
						Direction.y * Delta.y,
					Direction.y * Delta.x +
						Direction.x * Delta.y
				);

			T =
				0.5 +
				atan2(
					-Rotated.x,
					Rotated.y
				) /
				(2.0 * PI);
		}

		if (
			Function == REPEATING_LINEAR ||
			Function == REPEATING_RADIAL ||
			Function == REPEATING_CONIC
		)
		{
			const float T0 =
				GetStopPosition(
					0
				);

			const float T1 =
				GetStopPosition(
					max(
						NumStops - 1,
						0
					)
				);

			const float Span =
				max(
					T1 - T0,
					0.000001
				);

			T =
				T0 +
				fmod(
					fmod(
						T - T0,
						Span
					) +
					Span,
					Span
				);
		}

		return
			Diffuse *
			MixStopColors(
				T,
				NumStops
			);
	}
)RMLSHADER";

static const char* RmlDx9CreationShaderSource =
R"RMLSHADER(
	float4 CreationParams : register(c0);

	float4 main(float2 UV : TEXCOORD0, float4 Diffuse : COLOR0) : COLOR0
	{
		const float Time =
			CreationParams.x;

		const float2 Dimensions =
			max(
				CreationParams.yz,
				float2(
					1.0,
					1.0
				)
			);

		float3 Color =
			float3(
				0.0,
				0.0,
				0.0
			);

		float LengthValue =
			1.0;

		for (int Index = 0; Index < 3; ++Index)
		{
			float2 P =
				UV;

			float2 LocalUV =
				P;

			P -=
				0.5;

			P.x *=
				Dimensions.x /
				Dimensions.y;

			const float Z =
				Time +
				(float)Index *
				0.07;

			LengthValue =
				max(
					length(
						P
					),
					0.0001
				);

			LocalUV +=
				P /
				LengthValue *
				(sin(Z) + 1.0) *
				abs(
					sin(
						LengthValue * 9.0 -
						Z -
						Z
					)
				);

			Color[Index] =
				0.01 /
				max(
					length(
						fmod(
							LocalUV,
							1.0
						) -
						0.5
					),
					0.0001
				);
		}

		return
			float4(
				Color /
					LengthValue,
				Diffuse.a
			);
	}
)RMLSHADER";

static void RmlCopyColorMatrix(
	float Destination[16],
	const float Source[16]
)
{
	if (
		!Destination ||
		!Source
	)
	{
		return;
	}

	std::memcpy(
		Destination,
		Source,
		sizeof(float) * 16
	);
}

static bool RmlFileExistsW(const std::wstring& Path)
{
	if (Path.empty())
		return false;

	const DWORD Attributes = GetFileAttributesW(Path.c_str());

	if (Attributes == INVALID_FILE_ATTRIBUTES)
		return false;

	return (Attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static bool RmlStartsWithNoCaseW(
	const std::wstring& Text,
	const wchar_t* Prefix
)
{
	if (!Prefix)
		return false;

	const size_t PrefixLength =
		wcslen(Prefix);

	if (Text.length() < PrefixLength)
		return false;

	return _wcsnicmp(
		Text.c_str(),
		Prefix,
		PrefixLength
	) == 0;
}

static bool RmlHasExtensionNoCaseW(
	const std::wstring& Filename,
	const wchar_t* Extension
)
{
	if (!Extension)
		return false;

	const size_t ExtensionLength =
		wcslen(Extension);

	if (Filename.length() < ExtensionLength)
		return false;

	return _wcsicmp(
		Filename.c_str() +
			Filename.length() -
			ExtensionLength,
		Extension
	) == 0;
}

static void RmlStripDataPrefixW(
	std::wstring& Path
)
{
	for (wchar_t& Character : Path)
	{
		if (Character == L'/')
			Character = L'\\';
	}

	if (RmlStartsWithNoCaseW(Path, L"$Data\\"))
	{
		Path.erase(
			0,
			wcslen(L"$Data\\")
		);

		return;
	}

	if (RmlStartsWithNoCaseW(Path, L"Data\\"))
	{
		Path.erase(
			0,
			wcslen(L"Data\\")
		);

		return;
	}
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
	ReleaseLayerCompositeScratch();

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


bool RmlRenderDX9::EnsureBlurShader()
{
	if (BlurPixelShader)
		return true;

	if (!CreatePixelShader(
		RmlDx9BlurShaderSource,
		&BlurPixelShader
	))
	{
		OutputDebugStringA(
			"[RmlUI][DX9] Blur shader creation failed\n"
		);

		return false;
	}

	OutputDebugStringA(
		"[RmlUI][DX9] Blur shader created\n"
	);

	return true;
}

bool RmlRenderDX9::EnsureShadowShader()
{
	if (ShadowPixelShader)
		return true;

	if (!CreatePixelShader(
		RmlDx9ShadowShaderSource,
		&ShadowPixelShader
	))
	{
		OutputDebugStringA(
			"[RmlUI][DX9] Drop-shadow shader creation failed\n"
		);

		return false;
	}

	OutputDebugStringA(
		"[RmlUI][DX9] Drop-shadow shader created\n"
	);

	return true;
}

bool RmlRenderDX9::EnsureColorMatrixShader()
{
	if (ColorMatrixPixelShader)
		return true;

	if (!CreatePixelShader(
		RmlDx9ColorMatrixShaderSource,
		&ColorMatrixPixelShader
	))
	{
		OutputDebugStringA(
			"[RmlUI][DX9] Color-matrix shader creation failed\n"
		);

		return false;
	}

	OutputDebugStringA(
		"[RmlUI][DX9] Color-matrix shader created\n"
	);

	return true;
}

bool RmlRenderDX9::EnsureGradientShader()
{
	if (GradientPixelShader)
		return true;

	if (!CreatePixelShader(
		RmlDx9GradientShaderSource,
		&GradientPixelShader
	))
	{
		OutputDebugStringA(
			"[RmlUI][DX9] Gradient shader creation failed\n"
		);

		return false;
	}

	OutputDebugStringA(
		"[RmlUI][DX9] Gradient shader created\n"
	);

	return true;
}

bool RmlRenderDX9::EnsureCreationShader()
{
	if (CreationPixelShader)
		return true;

	if (!CreatePixelShader(
		RmlDx9CreationShaderSource,
		&CreationPixelShader
	))
	{
		OutputDebugStringA(
			"[RmlUI][DX9] Creation shader creation failed\n"
		);

		return false;
	}

	OutputDebugStringA(
		"[RmlUI][DX9] Creation shader created\n"
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

	if (ColorMatrixPixelShader)
	{
		ColorMatrixPixelShader->Release();
		ColorMatrixPixelShader = nullptr;
	}

	if (GradientPixelShader)
	{
		GradientPixelShader->Release();
		GradientPixelShader = nullptr;
	}

	if (CreationPixelShader)
	{
		CreationPixelShader->Release();
		CreationPixelShader = nullptr;
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

void RmlRenderDX9::ReleaseLayerCompositeScratch()
{
	ReleasePostProcessTarget(
		LayerCompositeScratch
	);
}

bool RmlRenderDX9::EnsureLayerCompositeScratch()
{
	if (!Device)
		return false;

	if (
		LayerCompositeScratch.Texture &&
		LayerCompositeScratch.Surface &&
		LayerCompositeScratch.Width ==
			ViewWidth &&
		LayerCompositeScratch.Height ==
			ViewHeight
	)
	{
		return true;
	}

	ReleaseLayerCompositeScratch();

	if (!CreateRenderTargetTexture(
		ViewWidth,
		ViewHeight,
		&LayerCompositeScratch.Texture,
		&LayerCompositeScratch.Surface
	))
	{
		OutputDebugStringA(
			"[RmlUI][DX9] Layer composite "
			"scratch target creation failed\n"
		);

		return false;
	}

	LayerCompositeScratch.Width =
		ViewWidth;

	LayerCompositeScratch.Height =
		ViewHeight;

	return true;
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
			: OutWeights[Index] * 2.0f;
	}

	if (TotalWeight <= 0.0f)
	{
		OutWeights[0] =
			1.0f;

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
		int Index = 0;
		Index < 5;
		++Index
	)
	{
		OutWeights[Index] /=
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
		!r3dRenderer ||
		!SourceTexture ||
		!DestinationSurface
	)
	{
		return;
	}

	r3dRenderer->SetRT(
		0,
		DestinationSurface
	);

	r3dRenderer->SetDSS(
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

	r3dRenderer->DrawUP(
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

void RmlRenderDX9::DrawMaskedPostProcessQuad(
	IDirect3DTexture9* SourceTexture,
	IDirect3DTexture9* MaskTexture,
	IDirect3DSurface9* DestinationSurface,
	bool bClearDestination
)
{
	if (
		!Device ||
		!r3dRenderer ||
		!SourceTexture ||
		!MaskTexture ||
		!DestinationSurface
	)
	{
		return;
	}

	/*
	 * Destination texture не должна оставаться
	 * привязанной ни к одному sampler stage.
	 */
	for (
		DWORD Stage = 0;
		Stage < 4;
		++Stage
	)
	{
		Device->SetTexture(
			Stage,
			nullptr
		);
	}

	r3dRenderer->SetRT(
		0,
		DestinationSurface
	);

	r3dRenderer->SetDSS(
		nullptr
	);

	(Device->SetRenderState)(
		RML_D3DRS_STENCILENABLE,
		FALSE
	);

	(Device->SetRenderState)(
		RML_D3DRS_TWOSIDEDSTENCILMODE,
		FALSE
	);

	(Device->SetRenderState)(
		RML_D3DRS_COLORWRITEENABLE,
		RML_COLOR_WRITE_ALL
	);

	if (bClearDestination)
	{
		/*
		 * Очищаем весь post-process target,
		 * независимо от текущего scissor.
		 */
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
		RML_D3DRS_CULLMODE,
		D3DCULL_NONE
	);

	(Device->SetRenderState)(
		RML_D3DRS_ALPHATESTENABLE,
		FALSE
	);

	/*
	 * Это replace-pass в отдельный target.
	 * Смешивание здесь не требуется.
	 */
	(Device->SetRenderState)(
		RML_D3DRS_ALPHABLENDENABLE,
		FALSE
	);

	(Device->SetRenderState)(
		RML_D3DRS_SEPARATEALPHABLENDENABLE,
		FALSE
	);

	Device->SetTexture(
		0,
		SourceTexture
	);

	Device->SetTexture(
		1,
		MaskTexture
	);

	for (
		DWORD Sampler = 0;
		Sampler < 2;
		++Sampler
	)
	{
		Device->SetSamplerState(
			Sampler,
			D3DSAMP_MINFILTER,
			D3DTEXF_LINEAR
		);

		Device->SetSamplerState(
			Sampler,
			D3DSAMP_MAGFILTER,
			D3DTEXF_LINEAR
		);

		Device->SetSamplerState(
			Sampler,
			D3DSAMP_MIPFILTER,
			D3DTEXF_NONE
		);

		Device->SetSamplerState(
			Sampler,
			D3DSAMP_ADDRESSU,
			D3DTADDRESS_CLAMP
		);

		Device->SetSamplerState(
			Sampler,
			D3DSAMP_ADDRESSV,
			D3DTADDRESS_CLAMP
		);
	}

	/*
	 * Stage 0:
	 *
	 * Current = Source RGBA.
	 */
	Device->SetTextureStageState(
		0,
		D3DTSS_TEXCOORDINDEX,
		0
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

	/*
	 * Stage 1:
	 *
	 * Result.rgb = Source.rgb * Mask.a
	 * Result.a   = Source.a   * Mask.a
	 *
	 * Source является premultiplied RGBA, поэтому
	 * обязательно умножаем и RGB, и alpha.
	 */
	Device->SetTextureStageState(
		1,
		D3DTSS_TEXCOORDINDEX,
		0
	);

	Device->SetTextureStageState(
		1,
		D3DTSS_COLOROP,
		D3DTOP_MODULATE
	);

	Device->SetTextureStageState(
		1,
		D3DTSS_COLORARG1,
		D3DTA_CURRENT
	);

	Device->SetTextureStageState(
		1,
		D3DTSS_COLORARG2,
		D3DTA_TEXTURE |
			D3DTA_ALPHAREPLICATE
	);

	Device->SetTextureStageState(
		1,
		D3DTSS_ALPHAOP,
		D3DTOP_MODULATE
	);

	Device->SetTextureStageState(
		1,
		D3DTSS_ALPHAARG1,
		D3DTA_CURRENT
	);

	Device->SetTextureStageState(
		1,
		D3DTSS_ALPHAARG2,
		D3DTA_TEXTURE
	);

	for (
		DWORD Stage = 2;
		Stage < 4;
		++Stage
	)
	{
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
	}

	r3dRenderer->DrawUP(
		D3DPT_TRIANGLESTRIP,
		2,
		Vertices,
		sizeof(
			FScreenVertex
		)
	);

	for (
		DWORD Stage = 0;
		Stage < 4;
		++Stage
	)
	{
		Device->SetTexture(
			Stage,
			nullptr
		);
	}

	for (
		DWORD Stage = 1;
		Stage < 4;
		++Stage
	)
	{
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
	}

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
		!r3dRenderer ||
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
		!Device ||
		!r3dRenderer ||
		!SourceTexture
	)
	{
		return false;
	}

	if (Sigma < 0.1f)
		return true;

	if (
		!EnsureBlurShader() ||
		!EnsurePostProcessTargets()
	)
	{
		return false;
	}

	/*
	 * Ограничиваем максимальный blur текущей
	 * реализации. Более крупные значения позже
	 * можно оптимизировать downsampling-проходами.
	 */
	Sigma =
		std::max(
			0.1f,
			std::min(
				16.0f,
				Sigma
			)
		);

	/*
	 * Первый target используется как чистая staging-копия.
	 * Это не позволяет содержимому соседних элементов
	 * проникнуть в blur через края filter-region.
	 */
	const int StagingTargetIndex =
		FindPostProcessTarget(
			SourcePostProcessIndex
		);

	const int WorkTargetIndex =
		FindPostProcessTarget(
			SourcePostProcessIndex,
			StagingTargetIndex
		);

	if (
		StagingTargetIndex < 0 ||
		WorkTargetIndex < 0
	)
	{
		return false;
	}

	const bool SavedScissorEnabled =
		bScissorEnabled;

	/*
	 * Destination полностью очищается, но source
	 * копируется только внутрь текущего scissor.
	 *
	 * За пределами filter-region staging texture
	 * остаётся прозрачной.
	 */
	DrawPostProcessQuad(
		SourceTexture,
		PostProcessTargets[
			StagingTargetIndex
		].Surface,
		nullptr,
		0.0f,
		0.0f,
		1.0f,
		false,
		true
	);

	bScissorEnabled =
		SavedScissorEnabled;

	/*
	 * Радиус текущего kernel равен четырём пикселям
	 * в каждую сторону. Для больших sigma выполняем
	 * несколько нормализованных проходов.
	 *
	 * Дисперсии Gaussian blur складываются:
	 *
	 * sigma_total² =
	 *     sigma_pass² * pass_count.
	 */
	constexpr float MaxSinglePassSigma =
		3.0f;

	const float RequiredPasses =
		(
			Sigma *
			Sigma
		) /
		(
			MaxSinglePassSigma *
			MaxSinglePassSigma
		);

	const int PassCount =
		std::max(
			1,
			std::min(
				32,
				static_cast<int>(
					std::ceil(
						RequiredPasses
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
		PostProcessTargets[
			StagingTargetIndex
		].Texture;

	for (
		int PassIndex = 0;
		PassIndex < PassCount;
		++PassIndex
	)
	{
		/*
		 * Horizontal:
		 * staging/current -> work.
		 */
		RenderBlurPass(
			CurrentTexture,
			PostProcessTargets[
				WorkTargetIndex
			].Surface,
			PassSigma,
			true
		);

		/*
		 * Vertical:
		 * work -> staging.
		 *
		 * После вертикального прохода staging снова
		 * становится текущим результатом.
		 */
		RenderBlurPass(
			PostProcessTargets[
				WorkTargetIndex
			].Texture,
			PostProcessTargets[
				StagingTargetIndex
			].Surface,
			PassSigma,
			false
		);

		CurrentTexture =
			PostProcessTargets[
				StagingTargetIndex
			].Texture;
	}

	OutTexture =
		CurrentTexture;

	OutPostProcessIndex =
		StagingTargetIndex;

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
		!r3dRenderer ||
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

	r3dRenderer->GetRT(
		0,
		&BaseRenderTarget
	);

	r3dRenderer->GetDSS(
		&OriginalDepthStencil
	);

	EnsureSharedDepthStencil();

	if (SharedDepthStencil)
	{
		r3dRenderer->SetDSS(
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

	bScissorEnabled =
	false;

	bClipMaskEnabled =
		false;

	ClipMaskReference =
		0;

	CurrentTransform =
		MakeIdentity();

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
		r3dRenderer->SetRT(
			0,
			BaseRenderTarget
		);
	}

	r3dRenderer->SetDSS(
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
	ReleaseLayerCompositeScratch();
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

void RmlRenderDX9::OnDeviceReset(
	int Width,
	int Height
)
{
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

	ReleasePostProcessTargets();
	ReleaseLayerCompositeScratch();

	/*
	 * Default-pool render targets будут
	 * лениво восстановлены при первом blur
	 * или self-composite.
	 */
	OutputDebugStringA(
		"[RmlUI][DX9] Device reset\n"
	);
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

	Viewport.X =
		0;

	Viewport.Y =
		0;

	Viewport.Width =
		static_cast<DWORD>(
			ViewWidth
		);

	Viewport.Height =
		static_cast<DWORD>(
			ViewHeight
		);

	Viewport.MinZ =
		0.0f;

	Viewport.MaxZ =
		1.0f;

	Device->SetViewport(
		&Viewport
	);

	const D3DMATRIX World =
		MakeIdentity();

	const D3DMATRIX View =
		MakeIdentity();

	const D3DMATRIX Projection =
		MakeOrthoOffCenterLH(
			0.0f,
			static_cast<float>(
				ViewWidth
			),
			static_cast<float>(
				ViewHeight
			),
			0.0f,
			-1.0f,
			1.0f
		);

	Device->SetTransform(
		D3DTS_WORLD,
		&World
	);

	Device->SetTransform(
		D3DTS_VIEW,
		&View
	);

	Device->SetTransform(
		D3DTS_PROJECTION,
		&Projection
	);

	Device->SetVertexShader(
		nullptr
	);

	Device->SetPixelShader(
		nullptr
	);

	Device->SetFVF(
		VertexFVF
	);

	for (
		DWORD Stage = 0;
		Stage < 4;
		++Stage
	)
	{
		Device->SetTexture(
			Stage,
			nullptr
		);
	}

	(Device->SetRenderState)(
		RML_D3DRS_LIGHTING,
		FALSE
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
		RML_D3DRS_ALPHATESTENABLE,
		FALSE
	);

	(Device->SetRenderState)(
		RML_D3DRS_ALPHABLENDENABLE,
		TRUE
	);

	(Device->SetRenderState)(
		RML_D3DRS_SEPARATEALPHABLENDENABLE,
		FALSE
	);

	(Device->SetRenderState)(
		RML_D3DRS_SRCBLEND,
		D3DBLEND_ONE
	);

	(Device->SetRenderState)(
		RML_D3DRS_DESTBLEND,
		D3DBLEND_INVSRCALPHA
	);

	(Device->SetRenderState)(
		RML_D3DRS_BLENDOP,
		D3DBLENDOP_ADD
	);

	(Device->SetRenderState)(
		RML_D3DRS_TWOSIDEDSTENCILMODE,
		FALSE
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

	for (
		DWORD Stage = 1;
		Stage < 4;
		++Stage
	)
	{
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
	}

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

	(Device->SetRenderState)(
		RML_D3DRS_TEXTUREFACTOR,
		0xFFFFFFFF
	);

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
		!r3dRenderer ||
		!GeometryHandle
	)
	{
		return;
	}

	FCompiledGeometry* Geometry =
		reinterpret_cast<FCompiledGeometry*>(
			GeometryHandle
		);

	if (
		!Geometry->VertexBuffer ||
		!Geometry->IndexBuffer ||
		Geometry->NumVertices <= 0 ||
		Geometry->NumIndices < 3
	)
	{
		return;
	}

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

	r3dRenderer->DrawIndexed(
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

void RmlRenderDX9::RenderGeometryWithPixelShader(
	const FCompiledGeometry* Geometry,
	Rml::Vector2f Translation,
	IDirect3DPixelShader9* PixelShader
)
{
	if (
		!Device ||
		!r3dRenderer ||
		!Geometry ||
		!Geometry->VertexBuffer ||
		!Geometry->IndexBuffer ||
		Geometry->NumVertices <= 0 ||
		Geometry->NumIndices < 3 ||
		!PixelShader
	)
	{
		return;
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

	Device->SetVertexShader(
		nullptr
	);

	Device->SetPixelShader(
		PixelShader
	);

	Device->SetFVF(
		VertexFVF
	);

	for (
		DWORD Stage = 0;
		Stage < 4;
		++Stage
	)
	{
		Device->SetTexture(
			Stage,
			nullptr
		);

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
	}

	(Device->SetRenderState)(
		RML_D3DRS_SRCBLEND,
		D3DBLEND_ONE
	);

	(Device->SetRenderState)(
		RML_D3DRS_DESTBLEND,
		D3DBLEND_INVSRCALPHA
	);

	const UINT PrimitiveCount =
		static_cast<UINT>(
			Geometry->NumIndices / 3
		);

	r3dRenderer->DrawIndexed(
		D3DPT_TRIANGLELIST,
		0,
		0,
		static_cast<UINT>(
			Geometry->NumVertices
		),
		0,
		PrimitiveCount
	);

	Device->SetPixelShader(
		nullptr
	);
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

	RmlStripDataPrefixW(WidePath);

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

	OutDimensions =
		Rml::Vector2i(
			0,
			0
		);

	D3DXIMAGE_INFO Information{};

	const HRESULT InfoResult =
		D3DXGetImageInfoFromFileW(
			Filename.c_str(),
			&Information
		);

	if (FAILED(InfoResult))
	{
		wchar_t Text[2048]{};

		_snwprintf_s(
			Text,
			_countof(Text),
			_TRUNCATE,
			L"[RmlUI][DX9] Texture info failed: 0x%08X | %ls\n",
			static_cast<unsigned int>(
				InfoResult
			),
			Filename.c_str()
		);

		OutputDebugStringW(
			Text
		);

		return false;
	}

	const bool bIsDds =
		RmlHasExtensionNoCaseW(
			Filename,
			L".dds"
		);

	IDirect3DTexture9* Texture =
		nullptr;

	HRESULT Result =
		E_FAIL;

	if (bIsDds)
	{
		/*
		 * Старые WarZ DDS могут быть DXT1/DXT3/DXT5.
		 *
		 * Для RmlUI нам нужен обычный A8R8G8B8, потому что
		 * backend рисует premultiplied alpha.
		 *
		 * Поэтому DDS грузим не как compressed texture,
		 * а распаковываем через D3DXLoadSurfaceFromFileW
		 * в A8R8G8B8 texture.
		 */
		const UINT Width =
			std::max<UINT>(
				1,
				Information.Width
			);

		const UINT Height =
			std::max<UINT>(
				1,
				Information.Height
			);

		Result =
			Device->CreateTexture(
				Width,
				Height,
				1,
				0,
				D3DFMT_A8R8G8B8,
				D3DPOOL_MANAGED,
				&Texture,
				nullptr
			);

		if (SUCCEEDED(Result) && Texture)
		{
			IDirect3DSurface9* Surface =
				nullptr;

			Result =
				Texture->GetSurfaceLevel(
					0,
					&Surface
				);

			if (SUCCEEDED(Result) && Surface)
			{
				Result =
					D3DXLoadSurfaceFromFileW(
						Surface,
						nullptr,
						nullptr,
						Filename.c_str(),
						nullptr,
						D3DX_FILTER_NONE,
						0,
						nullptr
					);
			}

			if (Surface)
			{
				Surface->Release();
				Surface = nullptr;
			}

			if (FAILED(Result))
			{
				Texture->Release();
				Texture = nullptr;
			}
		}
	}
	else
	{
		Result =
			D3DXCreateTextureFromFileExW(
				Device,
				Filename.c_str(),
				D3DX_DEFAULT_NONPOW2,
				D3DX_DEFAULT_NONPOW2,
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
	}

	if (FAILED(Result) || !Texture)
	{
		wchar_t Text[2048]{};

		_snwprintf_s(
			Text,
			_countof(Text),
			_TRUNCATE,
			L"[RmlUI][DX9] Texture load failed: 0x%08X | %ls\n",
			static_cast<unsigned int>(
				Result
			),
			Filename.c_str()
		);

		OutputDebugStringW(
			Text
		);

		if (Texture)
		{
			Texture->Release();
			Texture = nullptr;
		}

		return false;
	}

	if (!PremultiplyTextureAlpha(Texture))
	{
		wchar_t Text[2048]{};

		_snwprintf_s(
			Text,
			_countof(Text),
			_TRUNCATE,
			L"[RmlUI][DX9] Texture premultiply skipped: %ls\n",
			Filename.c_str()
		);

		OutputDebugStringW(
			Text
		);
	}

	OutDimensions.x =
		static_cast<int>(
			Information.Width
		);

	OutDimensions.y =
		static_cast<int>(
			Information.Height
		);

	*OutTexture =
		Texture;

	wchar_t Text[2048]{};

	_snwprintf_s(
		Text,
		_countof(Text),
		_TRUNCATE,
		L"[RmlUI][DX9] Texture loaded: %ls (%dx%d)%ls\n",
		Filename.c_str(),
		OutDimensions.x,
		OutDimensions.y,
		bIsDds
			? L" [DDS]"
			: L""
	);

	OutputDebugStringW(
		Text
	);

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
	if (
		!Device ||
		!r3dRenderer
	)
	{
		return;
	}

	IDirect3DSurface9* Surface =
		GetLayerSurface(
			Layer
		);

	if (!Surface)
		return;

	/*
	 * Нельзя устанавливать поверхность как render target,
	 * пока соответствующая texture всё ещё может быть
	 * привязана к sampler stage.
	 */
	for (
		DWORD Stage = 0;
		Stage < 4;
		++Stage
	)
	{
		Device->SetTexture(
			Stage,
			nullptr
		);
	}

	r3dRenderer->SetRT(
		0,
		Surface
	);

	r3dRenderer->SetDSS(
		SharedDepthStencil
	);

	ApplyClipMaskState();
}

Rml::LayerHandle RmlRenderDX9::PushLayer()
{
	if (
		!Device ||
		!r3dRenderer ||
		!bFrameOpen
	)
	{
		return 0;
	}

	if (
		!SharedDepthStencil &&
		!EnsureSharedDepthStencil()
	)
	{
		OutputDebugStringA(
			"[RmlUI][DX9] PushLayer failed: "
			"stencil surface unavailable\n"
		);

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
		OutputDebugStringA(
			"[RmlUI][DX9] PushLayer failed: "
			"render target unavailable\n"
		);

		return 0;
	}

	++ActiveLayerCount;

	BindLayer(
		static_cast<Rml::LayerHandle>(
			NewLayerHandle
		)
	);

	/*
	 * Слой переиспользуется между кадрами,
	 * поэтому перед использованием полностью
	 * очищаем его до transparent black.
	 */
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
	D3DTSS_TEXCOORDINDEX,
	0
);

		Device->SetSamplerState(
			Stage,
			D3DSAMP_MINFILTER,
			D3DTEXF_LINEAR
		);

		Device->SetSamplerState(
			Stage,
			D3DSAMP_MAGFILTER,
			D3DTEXF_LINEAR
		);

		Device->SetSamplerState(
			Stage,
			D3DSAMP_MIPFILTER,
			D3DTEXF_NONE
		);

		Device->SetSamplerState(
			Stage,
			D3DSAMP_ADDRESSU,
			D3DTADDRESS_CLAMP
		);

		Device->SetSamplerState(
			Stage,
			D3DSAMP_ADDRESSV,
			D3DTADDRESS_CLAMP
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

	r3dRenderer->DrawUP(
		D3DPT_TRIANGLESTRIP,
		2,
		Vertices,
		sizeof(
			FScreenVertex
		)
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
	Rml::Span<
		const Rml::CompiledFilterHandle
	> Filters
)
{
	if (
		!Device ||
		!r3dRenderer ||
		!bFrameOpen
	)
	{
		return;
	}

	IDirect3DTexture9* CurrentTexture =
		GetLayerTexture(
			Source
		);

	if (!CurrentTexture)
		return;

	if (!GetLayerSurface(
		Destination
	))
	{
		return;
	}

	const Rml::LayerHandle TopLayer =
		GetTopLayerHandle();

	/*
	 * Быстрый fixed-function путь:
	 *
	 * - композиция без фильтров;
	 * - один opacity;
	 * - несколько последовательных opacity.
	 *
	 * Для opacity не нужны pixel shaders
	 * и post-process render targets.
	 */
	bool bOpacityOnly =
		true;

	float LayerOpacity =
		1.0f;

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

		if (
			Filter->Type !=
			ECompiledFilterType::Opacity
		)
		{
			bOpacityOnly =
				false;

			break;
		}

		LayerOpacity *=
			Filter->Opacity;
	}

	LayerOpacity =
		std::max(
			0.0f,
			std::min(
				1.0f,
				LayerOpacity
			)
		);

	if (bOpacityOnly)
	{
		/*
		 * RmlUi разрешает Source == Destination.
		 *
		 * DX9 не разрешает читать texture,
		 * соответствующая surface которой в этот
		 * момент установлена как render target.
		 *
		 * Используем уже проверенный scratch target
		 * из этапа layers + clip-mask.
		 */
		if (Source == Destination)
		{
			if (!EnsureLayerCompositeScratch())
				return;

			for (
				DWORD Stage = 0;
				Stage < 4;
				++Stage
			)
			{
				Device->SetTexture(
					Stage,
					nullptr
				);
			}

			const bool SavedScissorEnabled =
				bScissorEnabled;

			/*
			 * В scratch копируется весь source layer.
			 * Текущий scissor применяется только при
			 * последующей композиции в Destination.
			 */
			bScissorEnabled =
				false;

			DrawPostProcessQuad(
				CurrentTexture,
				LayerCompositeScratch.Surface,
				nullptr,
				0.0f,
				0.0f,
				1.0f,
				false,
				true
			);

			bScissorEnabled =
				SavedScissorEnabled;

			CurrentTexture =
				LayerCompositeScratch.Texture;
		}

		BindLayer(
			Destination
		);

		SetupRenderState();

		/*
		 * DrawLayerTexture масштабирует все четыре
		 * premultiplied-компонента: R, G, B и A.
		 */
		DrawLayerTexture(
			CurrentTexture,
			nullptr,
			LayerOpacity,
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
		return;
	}

	if (!EnsurePostProcessTargets())
		return;

	int CurrentPostProcessIndex =
		FindPostProcessTarget(
			-1
		);

	if (CurrentPostProcessIndex < 0)
		return;

	DrawPostProcessQuad(
		CurrentTexture,
		PostProcessTargets[
			CurrentPostProcessIndex
		].Surface,
		nullptr,
		0.0f,
		0.0f,
		1.0f,
		false,
		true
	);

	CurrentTexture =
		PostProcessTargets[
			CurrentPostProcessIndex
		].Texture;

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
			/*
			 * В opacity-only режиме используется быстрый
			 * fixed-function путь выше.
			 *
			 * Здесь opacity является частью смешанной
			 * цепочки и должен применяться на своём месте,
			 * до или после drop-shadow согласно RCSS.
			 */
			if (Filter->Opacity >= 0.9999f)
				break;

			const int OpacityTargetIndex =
				FindPostProcessTarget(
					CurrentPostProcessIndex
				);

			if (OpacityTargetIndex < 0)
				break;

			DrawPostProcessQuad(
				CurrentTexture,
				PostProcessTargets[
					OpacityTargetIndex
				].Surface,
				nullptr,
				0.0f,
				0.0f,
				Filter->Opacity,
				false,
				true
			);

			CurrentTexture =
				PostProcessTargets[
					OpacityTargetIndex
				].Texture;

			CurrentPostProcessIndex =
				OpacityTargetIndex;

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
			/*
			 * Полностью прозрачная тень является
			 * identity-фильтром.
			 */
			if (Filter->Color.alpha == 0)
				break;

			if (!EnsureShadowShader())
				break;

			/*
			 * Размываем копию текущего изображения.
			 * CurrentTexture при этом сохраняется,
			 * поскольку оригинал должен быть наложен
			 * поверх сформированной тени.
			 */
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

			/*
			 * Composition target не должен совпадать
			 * ни с оригиналом, ни с blurred texture.
			 *
			 * При sigma == 0 оба индекса одинаковые,
			 * но третий target всё равно корректно
			 * находится.
			 */
			const int CompositionTargetIndex =
				FindPostProcessTarget(
					CurrentPostProcessIndex,
					BlurredTargetIndex
				);

			if (CompositionTargetIndex < 0)
				break;

			/*
			 * Color хранится в premultiplied формате.
			 *
			 * Shadow shader умножает все четыре
			 * компонента цвета на alpha размытого
			 * исходного изображения.
			 */
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
			 * Первый проход:
			 *
			 * blurred alpha -> coloured shadow
			 * с указанным offset.
			 *
			 * Destination очищается полностью.
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
			 * Второй проход:
			 *
			 * исходное текущее изображение накладывается
			 * поверх тени через premultiplied-alpha blend.
			 *
			 * Сам исходный элемент не размывается.
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

		case ECompiledFilterType::ColorMatrix:
			{
				if (!EnsureColorMatrixShader())
					break;

				const int ColorMatrixTargetIndex =
					FindPostProcessTarget(
						CurrentPostProcessIndex
					);

				if (ColorMatrixTargetIndex < 0)
					break;

				/*
				 * Три первых float4 — строки RGB-преобразования.
				 *
				 * Четвёртая строка не передаётся, поскольку
				 * shader сохраняет исходную alpha напрямую.
				 */
				Device->SetPixelShaderConstantF(
					0,
					Filter->ColorMatrix,
					3
				);

				DrawPostProcessQuad(
					CurrentTexture,
					PostProcessTargets[
						ColorMatrixTargetIndex
					].Surface,
					ColorMatrixPixelShader,
					0.0f,
					0.0f,
					1.0f,
					false,
					true
				);

				CurrentTexture =
					PostProcessTargets[
						ColorMatrixTargetIndex
					].Texture;

				CurrentPostProcessIndex =
					ColorMatrixTargetIndex;

				break;
			}

		case ECompiledFilterType::MaskImage:
			{
				if (!Filter->MaskTexture)
					break;

				/*
				 * Маска является полноценным последовательным
				 * фильтром, а не отложенным параметром финального
				 * DrawLayerTexture.
				 *
				 * Благодаря этому корректно работают:
				 *
				 * - несколько mask-image подряд;
				 * - mask -> blur;
				 * - blur -> mask;
				 * - дочерняя маска внутри родительской;
				 * - mask + opacity + drop-shadow.
				 */
				const int MaskTargetIndex =
					FindPostProcessTarget(
						CurrentPostProcessIndex
					);

				if (MaskTargetIndex < 0)
					break;

				DrawMaskedPostProcessQuad(
					CurrentTexture,
					Filter->MaskTexture,
					PostProcessTargets[
						MaskTargetIndex
					].Surface,
					true
				);

				CurrentTexture =
					PostProcessTargets[
						MaskTargetIndex
					].Texture;

				CurrentPostProcessIndex =
					MaskTargetIndex;

				break;
			}

		default:
			break;
		}
	}

	BindLayer(
		Destination
	);

	SetupRenderState();

	DrawLayerTexture(
		CurrentTexture,
		nullptr,
		1.0f,
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
		!r3dRenderer ||
		!bFrameOpen ||
		ActiveLayerCount <= 1
	)
	{
		return 0;
	}

	const Rml::LayerHandle TopLayer =
		GetTopLayerHandle();

	IDirect3DTexture9* SourceTexture =
		GetLayerTexture(
			TopLayer
		);

	if (!SourceTexture)
		return 0;

	IDirect3DTexture9* MaskTexture =
		nullptr;

	IDirect3DSurface9* MaskSurface =
		nullptr;

	if (!CreateRenderTargetTexture(
		ViewWidth,
		ViewHeight,
		&MaskTexture,
		&MaskSurface
	))
	{
		return 0;
	}

	/*
	 * Mask layer уже содержит результат всех
	 * mask decorators, clip-mask и текущего scissor.
	 *
	 * Destination полностью очищается, после чего
	 * копируется только активная filter-region.
	 */
	DrawPostProcessQuad(
		SourceTexture,
		MaskSurface,
		nullptr,
		0.0f,
		0.0f,
		1.0f,
		false,
		true
	);
	
	BindLayer(
		TopLayer
	);

	SetupRenderState();

	MaskSurface->Release();
	MaskSurface = nullptr;

	FCompiledFilter* Filter =
		new FCompiledFilter();

	Filter->Type =
		ECompiledFilterType::MaskImage;

	Filter->MaskTexture =
		MaskTexture;

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

		const float OpacityValue =
			Rml::Get(
				Parameters,
				"value",
				1.0f
			);

		Filter->Opacity =
			std::max(
				0.0f,
				std::min(
					1.0f,
					OpacityValue
				)
			);
	}
	else if (Name == "blur")
	{
		Filter->Type =
			ECompiledFilterType::Blur;

		const float BlurSigma =
			Rml::Get(
				Parameters,
				"sigma",
				1.0f
			);

		Filter->Sigma =
			std::max(
				0.0f,
				std::min(
					16.0f,
					BlurSigma
				)
			);
	}
	else if (Name == "drop-shadow")
	{
		Filter->Type =
			ECompiledFilterType::DropShadow;

		const float ShadowSigma =
			Rml::Get(
				Parameters,
				"sigma",
				0.0f
			);

		Filter->Sigma =
			std::max(
				0.0f,
				std::min(
					16.0f,
					ShadowSigma
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
	else if (Name == "brightness")
	{
		Filter->Type =
			ECompiledFilterType::ColorMatrix;

		const float Value =
			Rml::Get(
				Parameters,
				"value",
				1.0f
			);

		const float Matrix[16] =
		{
			Value, 0.0f,  0.0f,  0.0f,
			0.0f,  Value, 0.0f,  0.0f,
			0.0f,  0.0f,  Value, 0.0f,
			0.0f,  0.0f,  0.0f,  1.0f
		};

		RmlCopyColorMatrix(
			Filter->ColorMatrix,
			Matrix
		);
	}
	else if (Name == "contrast")
	{
		Filter->Type =
			ECompiledFilterType::ColorMatrix;

		const float Value =
			Rml::Get(
				Parameters,
				"value",
				1.0f
			);

		const float Grayness =
			0.5f -
			0.5f *
			Value;

		const float Matrix[16] =
		{
			Value, 0.0f,  0.0f,  Grayness,
			0.0f,  Value, 0.0f,  Grayness,
			0.0f,  0.0f,  Value, Grayness,
			0.0f,  0.0f,  0.0f,  1.0f
		};

		RmlCopyColorMatrix(
			Filter->ColorMatrix,
			Matrix
		);
	}
	else if (Name == "invert")
	{
		Filter->Type =
			ECompiledFilterType::ColorMatrix;

		const float RequestedValue =
			Rml::Get(
				Parameters,
				"value",
				1.0f
			);

		const float Value =
			std::max(
				0.0f,
				std::min(
					1.0f,
					RequestedValue
				)
			);

		const float Inverted =
			1.0f -
			2.0f *
			Value;

		const float Matrix[16] =
		{
			Inverted, 0.0f,     0.0f,     Value,
			0.0f,     Inverted, 0.0f,     Value,
			0.0f,     0.0f,     Inverted, Value,
			0.0f,     0.0f,     0.0f,     1.0f
		};

		RmlCopyColorMatrix(
			Filter->ColorMatrix,
			Matrix
		);
	}
	else if (Name == "grayscale")
	{
		Filter->Type =
			ECompiledFilterType::ColorMatrix;

		const float Value =
			Rml::Get(
				Parameters,
				"value",
				1.0f
			);

		const float ReverseValue =
			1.0f -
			Value;

		const float GrayRed =
			Value *
			0.2126f;

		const float GrayGreen =
			Value *
			0.7152f;

		const float GrayBlue =
			Value *
			0.0722f;

		const float Matrix[16] =
		{
			GrayRed + ReverseValue,
			GrayGreen,
			GrayBlue,
			0.0f,

			GrayRed,
			GrayGreen + ReverseValue,
			GrayBlue,
			0.0f,

			GrayRed,
			GrayGreen,
			GrayBlue + ReverseValue,
			0.0f,

			0.0f,
			0.0f,
			0.0f,
			1.0f
		};

		RmlCopyColorMatrix(
			Filter->ColorMatrix,
			Matrix
		);
	}
	else if (Name == "sepia")
	{
		Filter->Type =
			ECompiledFilterType::ColorMatrix;

		const float Value =
			Rml::Get(
				Parameters,
				"value",
				1.0f
			);

		const float ReverseValue =
			1.0f -
			Value;

		const float Matrix[16] =
		{
			Value * 0.393f + ReverseValue,
			Value * 0.769f,
			Value * 0.189f,
			0.0f,

			Value * 0.349f,
			Value * 0.686f + ReverseValue,
			Value * 0.168f,
			0.0f,

			Value * 0.272f,
			Value * 0.534f,
			Value * 0.131f + ReverseValue,
			0.0f,

			0.0f,
			0.0f,
			0.0f,
			1.0f
		};

		RmlCopyColorMatrix(
			Filter->ColorMatrix,
			Matrix
		);
	}
	else if (Name == "hue-rotate")
	{
		Filter->Type =
			ECompiledFilterType::ColorMatrix;

		/*
		 * RmlUi передаёт hue-rotate value
		 * уже преобразованным в радианы.
		 */
		const float Value =
			Rml::Get(
				Parameters,
				"value",
				1.0f
			);

		const float Sine =
			std::sin(
				Value
			);

		const float Cosine =
			std::cos(
				Value
			);

		const float Matrix[16] =
		{
			0.213f +
				0.787f * Cosine -
				0.213f * Sine,

			0.715f -
				0.715f * Cosine -
				0.715f * Sine,

			0.072f -
				0.072f * Cosine +
				0.928f * Sine,

			0.0f,

			0.213f -
				0.213f * Cosine +
				0.143f * Sine,

			0.715f +
				0.285f * Cosine +
				0.140f * Sine,

			0.072f -
				0.072f * Cosine -
				0.283f * Sine,

			0.0f,

			0.213f -
				0.213f * Cosine -
				0.787f * Sine,

			0.715f -
				0.715f * Cosine +
				0.715f * Sine,

			0.072f +
				0.928f * Cosine +
				0.072f * Sine,

			0.0f,

			0.0f,
			0.0f,
			0.0f,
			1.0f
		};

		RmlCopyColorMatrix(
			Filter->ColorMatrix,
			Matrix
		);
	}
	else if (Name == "saturate")
	{
		Filter->Type =
			ECompiledFilterType::ColorMatrix;

		const float Value =
			Rml::Get(
				Parameters,
				"value",
				1.0f
			);

		const float Matrix[16] =
		{
			0.213f +
				0.787f * Value,

			0.715f -
				0.715f * Value,

			0.072f -
				0.072f * Value,

			0.0f,

			0.213f -
				0.213f * Value,

			0.715f +
				0.285f * Value,

			0.072f -
				0.072f * Value,

			0.0f,

			0.213f -
				0.213f * Value,

			0.715f -
				0.715f * Value,

			0.072f +
				0.928f * Value,

			0.0f,

			0.0f,
			0.0f,
			0.0f,
			1.0f
		};

		RmlCopyColorMatrix(
			Filter->ColorMatrix,
			Matrix
		);
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

Rml::CompiledShaderHandle RmlRenderDX9::CompileShader(
	const Rml::String& Name,
	const Rml::Dictionary& Parameters
)
{
	FCompiledShader* Shader =
		new FCompiledShader();

	if (Name == "linear-gradient")
	{
		Shader->Type =
			ECompiledShaderType::Gradient;

		const bool bRepeating =
			Rml::Get(
				Parameters,
				"repeating",
				false
			);

		Shader->GradientFunction =
			bRepeating
			? EGradientFunction::RepeatingLinear
			: EGradientFunction::Linear;

		Shader->P =
			Rml::Get(
				Parameters,
				"p0",
				Rml::Vector2f(
					0.0f
				)
			);

		Shader->V =
			Rml::Get(
				Parameters,
				"p1",
				Rml::Vector2f(
					0.0f
				)
			) -
			Shader->P;
	}
	else if (Name == "radial-gradient")
	{
		Shader->Type =
			ECompiledShaderType::Gradient;

		const bool bRepeating =
			Rml::Get(
				Parameters,
				"repeating",
				false
			);

		Shader->GradientFunction =
			bRepeating
			? EGradientFunction::RepeatingRadial
			: EGradientFunction::Radial;

		Shader->P =
			Rml::Get(
				Parameters,
				"center",
				Rml::Vector2f(
					0.0f
				)
			);

		const Rml::Vector2f Radius =
			Rml::Get(
				Parameters,
				"radius",
				Rml::Vector2f(
					1.0f
				)
			);

		Shader->V =
			Rml::Vector2f(
				Radius.x != 0.0f
					? 1.0f / Radius.x
					: 1.0f,
				Radius.y != 0.0f
					? 1.0f / Radius.y
					: 1.0f
			);
	}
	else if (Name == "conic-gradient")
	{
		Shader->Type =
			ECompiledShaderType::Gradient;

		const bool bRepeating =
			Rml::Get(
				Parameters,
				"repeating",
				false
			);

		Shader->GradientFunction =
			bRepeating
			? EGradientFunction::RepeatingConic
			: EGradientFunction::Conic;

		Shader->P =
			Rml::Get(
				Parameters,
				"center",
				Rml::Vector2f(
					0.0f
				)
			);

		const float Angle =
			Rml::Get(
				Parameters,
				"angle",
				0.0f
			);

		Shader->V =
			Rml::Vector2f(
				std::cos(
					Angle
				),
				std::sin(
					Angle
				)
			);
	}
	else if (Name == "shader")
	{
		const Rml::String Value =
			Rml::Get(
				Parameters,
				"value",
				Rml::String()
			);

		if (Value == "creation")
		{
			Shader->Type =
				ECompiledShaderType::Creation;

			Shader->Dimensions =
				Rml::Get(
					Parameters,
					"dimensions",
					Rml::Vector2f(
						0.0f
					)
				);
		}
	}

	if (
		Shader->Type ==
		ECompiledShaderType::Gradient
	)
	{
		const Rml::ColorStopList ColorStops =
			Rml::Get(
				Parameters,
				"color_stop_list",
				Rml::ColorStopList()
			);

		const size_t StopCount =
			std::min<size_t>(
				ColorStops.size(),
				16
			);

		Shader->StopPositions.reserve(
			StopCount
		);

		Shader->StopColors.reserve(
			StopCount
		);

		for (
			size_t Index = 0;
			Index < StopCount;
			++Index
		)
		{
			Shader->StopPositions.push_back(
				ColorStops[Index].
					position.
					number
			);

			Shader->StopColors.push_back(
				ColorStops[Index].
					color
			);
		}

		if (Shader->StopPositions.empty())
		{
			delete Shader;
			return 0;
		}
	}

	if (
		Shader->Type !=
		ECompiledShaderType::Invalid
	)
	{
		return reinterpret_cast<
			Rml::CompiledShaderHandle
		>(
			Shader
		);
	}

	Rml::Log::Message(
		Rml::Log::LT_WARNING,
		"DX9 backend: unsupported shader '%s'.",
		Name.c_str()
	);

	delete Shader;
	return 0;
}

void RmlRenderDX9::RenderShader(
	Rml::CompiledShaderHandle ShaderHandle,
	Rml::CompiledGeometryHandle GeometryHandle,
	Rml::Vector2f Translation,
	Rml::TextureHandle
)
{
	if (
		!ShaderHandle ||
		!GeometryHandle
	)
	{
		return;
	}

	const FCompiledShader* Shader =
		reinterpret_cast<
			const FCompiledShader*
		>(
			ShaderHandle
		);

	const FCompiledGeometry* Geometry =
		reinterpret_cast<
			const FCompiledGeometry*
		>(
			GeometryHandle
		);

	switch (Shader->Type)
	{
	case ECompiledShaderType::Gradient:
	{
		if (!EnsureGradientShader())
			return;

		const int StopCount =
			static_cast<int>(
				std::min<size_t>(
					Shader->StopPositions.size(),
					16
				)
			);

		if (StopCount <= 0)
			return;

		float GradientParams[4] =
		{
			static_cast<float>(
				Shader->GradientFunction
			),
			static_cast<float>(
				StopCount
			),
			0.0f,
			0.0f
		};

		float GradientP[4] =
		{
			Shader->P.x,
			Shader->P.y,
			0.0f,
			0.0f
		};

		float GradientV[4] =
		{
			Shader->V.x,
			Shader->V.y,
			0.0f,
			0.0f
		};

		float StopColorConstants[16][4] = {};
		float StopPositionConstants[4][4] = {};

		for (
			int Index = 0;
			Index < StopCount;
			++Index
		)
		{
			const Rml::ColourbPremultiplied& Color =
				Shader->StopColors[
					static_cast<size_t>(
						Index
					)
				];

			StopColorConstants[Index][0] =
				static_cast<float>(
					Color.red
				) /
				255.0f;

			StopColorConstants[Index][1] =
				static_cast<float>(
					Color.green
				) /
				255.0f;

			StopColorConstants[Index][2] =
				static_cast<float>(
					Color.blue
				) /
				255.0f;

			StopColorConstants[Index][3] =
				static_cast<float>(
					Color.alpha
				) /
				255.0f;

			StopPositionConstants[Index / 4][Index % 4] =
				Shader->StopPositions[
					static_cast<size_t>(
						Index
					)
				];
		}

		Device->SetPixelShaderConstantF(
			0,
			GradientParams,
			1
		);

		Device->SetPixelShaderConstantF(
			1,
			GradientP,
			1
		);

		Device->SetPixelShaderConstantF(
			2,
			GradientV,
			1
		);

		Device->SetPixelShaderConstantF(
			3,
			&StopColorConstants[0][0],
			16
		);

		Device->SetPixelShaderConstantF(
			19,
			&StopPositionConstants[0][0],
			4
		);

		RenderGeometryWithPixelShader(
			Geometry,
			Translation,
			GradientPixelShader
		);

		break;
	}

	case ECompiledShaderType::Creation:
	{
		if (!EnsureCreationShader())
			return;

		Rml::SystemInterface* SystemInterface =
			Rml::GetSystemInterface();

		const float Time =
			SystemInterface
			? static_cast<float>(
				SystemInterface->GetElapsedTime()
			)
			: 0.0f;

		const float CreationParams[4] =
		{
			Time,
			Shader->Dimensions.x,
			Shader->Dimensions.y,
			0.0f
		};

		Device->SetPixelShaderConstantF(
			0,
			CreationParams,
			1
		);

		RenderGeometryWithPixelShader(
			Geometry,
			Translation,
			CreationPixelShader
		);

		break;
	}

	default:
		break;
	}

	SetupRenderState();
}

void RmlRenderDX9::ReleaseShader(
	Rml::CompiledShaderHandle ShaderHandle
)
{
	if (!ShaderHandle)
		return;

	FCompiledShader* Shader =
		reinterpret_cast<
			FCompiledShader*
		>(
			ShaderHandle
		);

	delete Shader;
}
