#include "r3dPCH.h"
#include "r3d.h"

#include "rendering/DX11/RenderDX11Core.h"
#include "rendering/DX11/RenderDX11Shaders.h"

#if LTS_STUDIO_DX11

#include <D3Dcompiler.h>
#include <stdio.h>
#include <string.h>

namespace
{
	HRESULT OfflineShaderCompilationUnavailable(
		LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*,
		LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**)
	{
		return E_NOTIMPL;
	}
	template <typename T>
	void RenderDX11Shaders_SafeRelease(
		T*& Object
	)
	{
		if (!Object)
			return;

		Object->Release();
		Object = 0;
	}

	void RenderDX11Shaders_Log(
		const char* Text
	)
	{
		if (!Text)
			return;

		OutputDebugStringA(Text);
		r3dOutToLog("%s", Text);
	}

	class RenderDX11ShaderIncludeHandler :
		public ID3DInclude
	{
	public:
		explicit RenderDX11ShaderIncludeHandler(
			const char* ShaderFileName
		)
		{
			BasePath_[0] = 0;

			if (
				!ShaderFileName ||
				!ShaderFileName[0]
			)
			{
				return;
			}

			r3dscpy(
				BasePath_,
				ShaderFileName
			);

			for (
				char* It = BasePath_;
				*It;
				++It
			)
			{
				if (*It == '/')
					*It = '\\';
			}

			char* LastSlash =
				strrchr(
					BasePath_,
					'\\'
				);

			if (LastSlash)
				*LastSlash = 0;
			else
				BasePath_[0] = 0;
		}

		STDMETHOD(Open)(
			D3D_INCLUDE_TYPE IncludeType,
			LPCSTR FileName,
			LPCVOID ParentData,
			LPCVOID* OutData,
			UINT* OutBytes
		)
		{
			(void)IncludeType;
			(void)ParentData;

			if (
				!FileName ||
				!OutData ||
				!OutBytes
			)
			{
				return E_FAIL;
			}

			*OutData = 0;
			*OutBytes = 0;

			char FullFileName[MAX_PATH] = {};

			if (BasePath_[0])
			{
				sprintf_s(
					FullFileName,
					"%s\\%s",
					BasePath_,
					FileName
				);
			}
			else
			{
				sprintf_s(
					FullFileName,
					"%s",
					FileName
				);
			}

			r3dFile* File =
				r3d_open(
					FullFileName,
					"rb"
				);

			if (!File)
			{
				char Text[512] = {};

				sprintf_s(
					Text,
					"[DX11][Shaders] Missing include: %s\n",
					FullFileName
				);

				RenderDX11Shaders_Log(Text);
				return E_FAIL;
			}

			char* Data =
				new char[File->size + 1];

			if (!Data)
			{
				fclose(File);
				return E_OUTOFMEMORY;
			}

			const size_t ReadSize =
				fread(
					Data,
					1,
					File->size,
					File
				);

			fclose(File);

			Data[ReadSize] = 0;

			*OutData = Data;
			*OutBytes =
				static_cast<UINT>(
					ReadSize
				);

			return S_OK;
		}

		STDMETHOD(Close)(
			LPCVOID Data
		)
		{
			delete[] reinterpret_cast<
				const char*
			>(Data);

			return S_OK;
		}

	private:
		char BasePath_[MAX_PATH];
	};

	const char* RenderDX11_GetSunGlareSource()
	{
		return
			"Texture2D gMaskTex : register(t0);\n"
			"SamplerState gMaskSampler : register(s0);\n"
			"\n"
			"cbuffer SunGlareCB : register(b8)\n"
			"{\n"
			"	float4 gThreshold;\n"
			"	float4 gTint[10];\n"
			"	float4 gTexTransform[10];\n"
			"	float4 gParams;\n"
			"};\n"
			"\n"
			"struct VSOut\n"
			"{\n"
			"	float4 Pos : SV_POSITION;\n"
			"	float2 UV : TEXCOORD0;\n"
			"};\n"
			"\n"
			"VSOut VSMain(uint VertexID : SV_VertexID)\n"
			"{\n"
			"	VSOut Output;\n"
			"	float2 Position;\n"
			"	Position.x = (VertexID == 2) ? 3.0f : -1.0f;\n"
			"	Position.y = (VertexID == 1) ? 3.0f : -1.0f;\n"
			"	Output.Pos = float4(Position, 0.0f, 1.0f);\n"
			"	Output.UV = float2(\n"
			"		Position.x * 0.5f + 0.5f,\n"
			"		-Position.y * 0.5f + 0.5f\n"
			"	);\n"
			"	return Output;\n"
			"}\n"
			"\n"
			"float4 PSMain(VSOut Input) : SV_TARGET\n"
			"{\n"
			"	float3 Color = float3(0.0f, 0.0f, 0.0f);\n"
			"	float Alpha = 0.0f;\n"
			"	int Count = clamp((int)gParams.x, 1, 10);\n"
			"\n"
			"	[loop]\n"
			"	for (int Index = 0; Index < Count; ++Index)\n"
			"	{\n"
			"		float2 UV =\n"
			"			Input.UV * gTexTransform[Index].xy +\n"
			"			gTexTransform[Index].zw;\n"
			"\n"
			"		float Mask =\n"
			"			gMaskTex.Sample(gMaskSampler, UV).r;\n"
			"\n"
			"		float Threshold =\n"
			"			gThreshold[min(Index, 3)];\n"
			"\n"
			"		float Glare = saturate(\n"
			"			(Mask - Threshold) /\n"
			"			max(1.0f - Threshold, 0.001f)\n"
			"		);\n"
			"\n"
			"		Color += Glare * gTint[Index].rgb;\n"
			"		Alpha += Glare * gTint[Index].a;\n"
			"	}\n"
			"\n"
			"	return float4(Color, saturate(Alpha));\n"
			"}\n";
	}
}

RenderDX11Shaders::RenderDX11Shaders()
	: ClearVS_(0)
	, ClearPS_(0)
	, LightingVS_(0)
	, LightingPS_(0)
	, TonemapVS_(0)
	, TonemapPS_(0)
	, TerrainVS_(0)
	, TerrainPS_(0)
	, TerrainInputLayout_(0)
	, StaticMeshVS_(0)
	, StaticMeshPS_(0)
	, StaticMeshInputLayout_(0)
	, SunGlareVS_(0)
	, SunGlarePS_(0)
{
}

void RenderDX11Shaders::LogFailure(
	const char* Stage,
	HRESULT Result
)
{
	char Text[512] = {};

	sprintf_s(
		Text,
		"[DX11][Shaders] %s failed. HRESULT=0x%08X\n",
		Stage ? Stage : "Unknown operation",
		static_cast<unsigned int>(Result)
	);

	RenderDX11Shaders_Log(Text);
}

void RenderDX11Shaders::MakeShaderFileName(
	char* OutFileName,
	size_t OutFileNameSize,
	const char* RelativeFileName
)
{
	if (
		!OutFileName ||
		OutFileNameSize == 0
	)
	{
		return;
	}

	OutFileName[0] = 0;

	if (
		!RelativeFileName ||
		!RelativeFileName[0]
	)
	{
		return;
	}

	sprintf_s(
		OutFileName,
		OutFileNameSize,
		"Data\\Shaders\\DX11_P1\\%s",
		RelativeFileName
	);
}

bool RenderDX11Shaders::LoadShaderSource(
	const char* FileName,
	char** OutData,
	UINT* OutSize
)
{
	if (
		!FileName ||
		!FileName[0] ||
		!OutData ||
		!OutSize
	)
	{
		return false;
	}

	*OutData = 0;
	*OutSize = 0;

	r3dFile* File =
		r3d_open(
			FileName,
			"rb"
		);

	if (!File)
	{
		char Text[512] = {};

		sprintf_s(
			Text,
			"[DX11][Shaders] Missing shader file: %s\n",
			FileName
		);

		RenderDX11Shaders_Log(Text);
		return false;
	}

	char* Data =
		new char[File->size + 1];

	if (!Data)
	{
		fclose(File);
		return false;
	}

	const size_t ReadSize =
		fread(
			Data,
			1,
			File->size,
			File
		);

	fclose(File);

	Data[ReadSize] = 0;

	*OutData = Data;
	*OutSize =
		static_cast<UINT>(
			ReadSize
		);

	return true;
}

bool RenderDX11Shaders::CompileFromFile(
	const char* RelativeFileName,
	const char* EntryPoint,
	const char* Profile,
	ID3DBlob** OutBlob
)
{
	if (
		!RelativeFileName ||
		!EntryPoint ||
		!Profile ||
		!OutBlob
	)
	{
		return false;
	}

	*OutBlob = 0;

	char FileName[MAX_PATH] = {};

	MakeShaderFileName(
		FileName,
		_countof(FileName),
		RelativeFileName
	);

	char* SourceData = 0;
	UINT SourceSize = 0;

	if (!LoadShaderSource(
		FileName,
		&SourceData,
		&SourceSize
	))
	{
		return false;
	}

	UINT Flags =
		D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(_DEBUG)
	Flags |= D3DCOMPILE_DEBUG;
	Flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	Flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	ID3DBlob* ErrorBlob = 0;

	RenderDX11ShaderIncludeHandler IncludeHandler(
		FileName
	);

	const HRESULT Result =
		OfflineShaderCompilationUnavailable(
			SourceData,
			SourceSize,
			FileName,
			0,
			&IncludeHandler,
			EntryPoint,
			Profile,
			Flags,
			0,
			OutBlob,
			&ErrorBlob
		);

	delete[] SourceData;

	if (FAILED(Result))
	{
		const char* ErrorText =
			ErrorBlob
			? reinterpret_cast<const char*>(
				ErrorBlob->GetBufferPointer()
			)
			: "Unknown compiler error";

		char Text[4096] = {};

		sprintf_s(
			Text,
			"[DX11][Shaders] Compile failed: "
			"%s entry=%s profile=%s HRESULT=0x%08X\n%s\n",
			FileName,
			EntryPoint,
			Profile,
			static_cast<unsigned int>(Result),
			ErrorText
		);

		RenderDX11Shaders_Log(Text);

		RenderDX11Shaders_SafeRelease(
			ErrorBlob
		);

		RenderDX11Shaders_SafeRelease(
			*OutBlob
		);

		return false;
	}

	RenderDX11Shaders_SafeRelease(
		ErrorBlob
	);

	char Text[512] = {};

	sprintf_s(
		Text,
		"[DX11][Shaders] Compiled: %s entry=%s profile=%s\n",
		FileName,
		EntryPoint,
		Profile
	);

	RenderDX11Shaders_Log(Text);

	return true;
}

bool RenderDX11Shaders::CompileFromMemory(
	const char* DebugName,
	const char* Source,
	const char* EntryPoint,
	const char* Profile,
	ID3DBlob** OutBlob
)
{
	if (
		!Source ||
		!EntryPoint ||
		!Profile ||
		!OutBlob
	)
	{
		return false;
	}

	*OutBlob = 0;

	UINT Flags =
		D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(_DEBUG)
	Flags |= D3DCOMPILE_DEBUG;
	Flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	Flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	ID3DBlob* ErrorBlob = 0;

	const HRESULT Result =
		OfflineShaderCompilationUnavailable(
			Source,
			strlen(Source),
			DebugName ? DebugName : "memory",
			0,
			0,
			EntryPoint,
			Profile,
			Flags,
			0,
			OutBlob,
			&ErrorBlob
		);

	if (FAILED(Result))
	{
		const char* ErrorText =
			ErrorBlob
			? reinterpret_cast<const char*>(
				ErrorBlob->GetBufferPointer()
			)
			: "Unknown compiler error";

		char Text[4096] = {};

		sprintf_s(
			Text,
			"[DX11][Shaders] Memory compile failed: "
			"%s entry=%s profile=%s HRESULT=0x%08X\n%s\n",
			DebugName ? DebugName : "memory",
			EntryPoint,
			Profile,
			static_cast<unsigned int>(Result),
			ErrorText
		);

		RenderDX11Shaders_Log(Text);

		RenderDX11Shaders_SafeRelease(
			ErrorBlob
		);

		RenderDX11Shaders_SafeRelease(
			*OutBlob
		);

		return false;
	}

	RenderDX11Shaders_SafeRelease(
		ErrorBlob
	);

	return true;
}

bool RenderDX11Shaders::CreateFileProgram(
	const char* RelativeFileName,
	const char* DebugName,
	const D3D11_INPUT_ELEMENT_DESC* InputElements,
	UINT InputElementCount,
	ID3D11VertexShader** OutVS,
	ID3D11PixelShader** OutPS,
	ID3D11InputLayout** OutInputLayout
)
{
	if (
		!RelativeFileName ||
		!OutVS ||
		!OutPS
	)
	{
		return false;
	}

	*OutVS = 0;
	*OutPS = 0;

	if (OutInputLayout)
		*OutInputLayout = 0;

	ID3D11Device* Device =
		RenderDX11_GetCore().GetDevice();

	if (!Device)
		return false;

	ID3DBlob* VSBlob = 0;
	ID3DBlob* PSBlob = 0;

	if (!CompileFromFile(
		RelativeFileName,
		"VSMain",
		"vs_5_0",
		&VSBlob
	))
	{
		return false;
	}

	HRESULT Result =
		Device->CreateVertexShader(
			VSBlob->GetBufferPointer(),
			VSBlob->GetBufferSize(),
			0,
			OutVS
		);

	if (FAILED(Result))
	{
		char Stage[256] = {};

		sprintf_s(
			Stage,
			"Create %s vertex shader",
			DebugName ? DebugName : RelativeFileName
		);

		LogFailure(Stage, Result);

		RenderDX11Shaders_SafeRelease(
			VSBlob
		);

		return false;
	}

	if (
		OutInputLayout &&
		InputElements &&
		InputElementCount > 0
	)
	{
		Result =
			Device->CreateInputLayout(
				InputElements,
				InputElementCount,
				VSBlob->GetBufferPointer(),
				VSBlob->GetBufferSize(),
				OutInputLayout
			);

		if (FAILED(Result))
		{
			char Stage[256] = {};

			sprintf_s(
				Stage,
				"Create %s input layout",
				DebugName ? DebugName : RelativeFileName
			);

			LogFailure(Stage, Result);

			RenderDX11Shaders_SafeRelease(
				VSBlob
			);

			RenderDX11Shaders_SafeRelease(
				*OutVS
			);

			return false;
		}
	}

	RenderDX11Shaders_SafeRelease(
		VSBlob
	);

	if (!CompileFromFile(
		RelativeFileName,
		"PSMain",
		"ps_5_0",
		&PSBlob
	))
	{
		if (OutInputLayout)
		{
			RenderDX11Shaders_SafeRelease(
				*OutInputLayout
			);
		}

		RenderDX11Shaders_SafeRelease(
			*OutVS
		);

		return false;
	}

	Result =
		Device->CreatePixelShader(
			PSBlob->GetBufferPointer(),
			PSBlob->GetBufferSize(),
			0,
			OutPS
		);

	RenderDX11Shaders_SafeRelease(
		PSBlob
	);

	if (FAILED(Result))
	{
		char Stage[256] = {};

		sprintf_s(
			Stage,
			"Create %s pixel shader",
			DebugName ? DebugName : RelativeFileName
		);

		LogFailure(Stage, Result);

		if (OutInputLayout)
		{
			RenderDX11Shaders_SafeRelease(
				*OutInputLayout
			);
		}

		RenderDX11Shaders_SafeRelease(
			*OutVS
		);

		return false;
	}

	return true;
}

bool RenderDX11Shaders::CreateMemoryProgram(
	const char* DebugName,
	const char* Source,
	const char* VSProfile,
	const char* PSProfile,
	ID3D11VertexShader** OutVS,
	ID3D11PixelShader** OutPS
)
{
	if (
		!Source ||
		!VSProfile ||
		!PSProfile ||
		!OutVS ||
		!OutPS
	)
	{
		return false;
	}

	*OutVS = 0;
	*OutPS = 0;

	ID3D11Device* Device =
		RenderDX11_GetCore().GetDevice();

	if (!Device)
		return false;

	ID3DBlob* VSBlob = 0;
	ID3DBlob* PSBlob = 0;

	if (!CompileFromMemory(
		DebugName,
		Source,
		"VSMain",
		VSProfile,
		&VSBlob
	))
	{
		return false;
	}

	if (!CompileFromMemory(
		DebugName,
		Source,
		"PSMain",
		PSProfile,
		&PSBlob
	))
	{
		RenderDX11Shaders_SafeRelease(
			VSBlob
		);

		return false;
	}

	HRESULT Result =
		Device->CreateVertexShader(
			VSBlob->GetBufferPointer(),
			VSBlob->GetBufferSize(),
			0,
			OutVS
		);

	if (FAILED(Result))
	{
		LogFailure(
			"Create memory vertex shader",
			Result
		);

		RenderDX11Shaders_SafeRelease(
			VSBlob
		);

		RenderDX11Shaders_SafeRelease(
			PSBlob
		);

		return false;
	}

	Result =
		Device->CreatePixelShader(
			PSBlob->GetBufferPointer(),
			PSBlob->GetBufferSize(),
			0,
			OutPS
		);

	RenderDX11Shaders_SafeRelease(
		VSBlob
	);

	RenderDX11Shaders_SafeRelease(
		PSBlob
	);

	if (FAILED(Result))
	{
		LogFailure(
			"Create memory pixel shader",
			Result
		);

		RenderDX11Shaders_SafeRelease(
			*OutVS
		);

		return false;
	}

	return true;
}

bool RenderDX11Shaders::Initialize()
{
	if (IsReady())
		return true;

	Shutdown();

	if (!RenderDX11_GetCore().IsReady())
	{
		RenderDX11Shaders_Log(
			"[DX11][Shaders] Cannot initialize without DX11 core\n"
		);

		return false;
	}

	if (!CreateFileProgram(
		"system\\dx11_clear.hls",
		"Clear",
		0,
		0,
		&ClearVS_,
		&ClearPS_,
		0
	))
	{
		Shutdown();
		return false;
	}

	if (!CreateFileProgram(
		"system\\dx11_directional_light.hls",
		"DirectionalLighting",
		0,
		0,
		&LightingVS_,
		&LightingPS_,
		0
	))
	{
		Shutdown();
		return false;
	}

	if (!CreateFileProgram(
		"system\\dx11_tonemap.hls",
		"Tonemap",
		0,
		0,
		&TonemapVS_,
		&TonemapPS_,
		0
	))
	{
		Shutdown();
		return false;
	}

	const D3D11_INPUT_ELEMENT_DESC TerrainLayout[] =
	{
		{
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			0,
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		},
		{
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			sizeof(float) * 3,
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		}
	};

	if (!CreateFileProgram(
		"Nature\\dx11_terrain.hls",
		"Terrain",
		TerrainLayout,
		_countof(TerrainLayout),
		&TerrainVS_,
		&TerrainPS_,
		&TerrainInputLayout_
	))
	{
		Shutdown();
		return false;
	}

	const D3D11_INPUT_ELEMENT_DESC StaticMeshLayout[] =
	{
		{
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			0,
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		},
		{
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			sizeof(float) * 3,
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		},
		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			sizeof(float) * 6,
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		},
		{
			"TANGENT",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			sizeof(float) * 8,
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		}
	};

	if (!CreateFileProgram(
		"system\\dx11_static_mesh.hls",
		"StaticMesh",
		StaticMeshLayout,
		_countof(StaticMeshLayout),
		&StaticMeshVS_,
		&StaticMeshPS_,
		&StaticMeshInputLayout_
	))
	{
		Shutdown();
		return false;
	}

	RenderDX11Shaders_Log(
		"[DX11][Shaders] Main shader programs created\n"
	);

	return true;
}

bool RenderDX11Shaders::EnsureSunGlare()
{
	if (
		SunGlareVS_ &&
		SunGlarePS_
	)
	{
		return true;
	}

	RenderDX11Shaders_SafeRelease(
		SunGlarePS_
	);

	RenderDX11Shaders_SafeRelease(
		SunGlareVS_
	);

	if (!CreateMemoryProgram(
		"DX11_SunGlare",
		RenderDX11_GetSunGlareSource(),
		"vs_4_0",
		"ps_4_0",
		&SunGlareVS_,
		&SunGlarePS_
	))
	{
		return false;
	}

	RenderDX11Shaders_Log(
		"[DX11][Shaders] SunGlare program created\n"
	);

	return true;
}

bool RenderDX11Shaders::IsReady() const
{
	return
		ClearVS_ != 0 &&
		ClearPS_ != 0 &&
		LightingVS_ != 0 &&
		LightingPS_ != 0 &&
		TonemapVS_ != 0 &&
		TonemapPS_ != 0 &&
		TerrainVS_ != 0 &&
		TerrainPS_ != 0 &&
		TerrainInputLayout_ != 0 &&
		StaticMeshVS_ != 0 &&
		StaticMeshPS_ != 0 &&
		StaticMeshInputLayout_ != 0;
}

void RenderDX11Shaders::Shutdown()
{
	RenderDX11Shaders_SafeRelease(
		SunGlarePS_
	);

	RenderDX11Shaders_SafeRelease(
		SunGlareVS_
	);

	RenderDX11Shaders_SafeRelease(
		StaticMeshInputLayout_
	);

	RenderDX11Shaders_SafeRelease(
		StaticMeshPS_
	);

	RenderDX11Shaders_SafeRelease(
		StaticMeshVS_
	);

	RenderDX11Shaders_SafeRelease(
		TerrainInputLayout_
	);

	RenderDX11Shaders_SafeRelease(
		TerrainPS_
	);

	RenderDX11Shaders_SafeRelease(
		TerrainVS_
	);

	RenderDX11Shaders_SafeRelease(
		TonemapPS_
	);

	RenderDX11Shaders_SafeRelease(
		TonemapVS_
	);

	RenderDX11Shaders_SafeRelease(
		LightingPS_
	);

	RenderDX11Shaders_SafeRelease(
		LightingVS_
	);

	RenderDX11Shaders_SafeRelease(
		ClearPS_
	);

	RenderDX11Shaders_SafeRelease(
		ClearVS_
	);
}

RenderDX11Shaders& RenderDX11_GetShaders()
{
	static RenderDX11Shaders Shaders;
	return Shaders;
}

#endif // LTS_STUDIO_DX11
