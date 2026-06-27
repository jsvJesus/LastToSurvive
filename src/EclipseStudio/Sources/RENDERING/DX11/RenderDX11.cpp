#include "r3dPCH.h"
#include "r3d.h"

#include "r3dRendererConfig.h"

#if LTS_STUDIO_DX11

#include <D3D11.h>
#include <DXGI.h>
#include <D3Dcompiler.h>

#include <stdio.h>

#include "GameCommon.h"
#include "TrueNature/ITerrain.h"
#include "rendering/DX11/RenderDX11.h"

bool RenderDX11_Init();
void RenderDX11_Shutdown();
bool RenderDX11_IsReady();

bool RenderDX11_RenderWorld(
	const WorldDX11FrameDesc& Desc
);

extern r3dITerrain* Terrain;
class r3dTerrain2;
extern r3dTerrain2* Terrain2;

static void RenderDX11_LogText(
	const char* Text
)
{
	if (!Text)
		return;

	OutputDebugStringA(Text);
	r3dOutToLog("%s", Text);
}

#define OutputDebugStringA RenderDX11_LogText

namespace
{
	static const int DX11_TERRAIN_GRID_DIM = 17;
	static const int DX11_TERRAIN_VERTEX_COUNT =
		DX11_TERRAIN_GRID_DIM * DX11_TERRAIN_GRID_DIM;
	static const int DX11_TERRAIN_INDEX_COUNT =
		(DX11_TERRAIN_GRID_DIM - 1) *
		(DX11_TERRAIN_GRID_DIM - 1) *
		6;

	struct WorldDX11FrameCB
	{
		float ViewProj[16];
		float View[16];
		float Proj[16];
		float CameraPos[4];
		float ScreenSize[4];
		float NearFar[4];
	};

	struct WorldDX11TerrainVertex
	{
		float Position[3];
	};

	typedef char WorldDX11FrameCB_SizeMustBe16ByteAligned[
		(sizeof(WorldDX11FrameCB) % 16) == 0 ? 1 : -1
	];

	ID3D11Device*			gDX11Device = 0;
	ID3D11DeviceContext*	gDX11Context = 0;

	ID3D11Texture2D*		gDX11GBufferColorTexture = 0;
	ID3D11Texture2D*		gDX11GBufferNormalTexture = 0;
	ID3D11Texture2D*		gDX11GBufferDepthLinearTexture = 0;
	ID3D11Texture2D*		gDX11GBufferAuxTexture = 0;
	ID3D11Texture2D*		gDX11DepthTexture = 0;
	ID3D11Texture2D*		gDX11SmokeReadbackTexture = 0;

	ID3D11RenderTargetView*	gDX11GBufferColorRTV = 0;
	ID3D11RenderTargetView*	gDX11GBufferNormalRTV = 0;
	ID3D11RenderTargetView*	gDX11GBufferDepthLinearRTV = 0;
	ID3D11RenderTargetView*	gDX11GBufferAuxRTV = 0;
	ID3D11DepthStencilView*	gDX11DepthDSV = 0;

	ID3D11VertexShader*		gDX11ClearVS = 0;
	ID3D11PixelShader*		gDX11ClearPS = 0;

	ID3D11VertexShader*		gDX11TerrainVS = 0;
	ID3D11PixelShader*		gDX11TerrainPS = 0;
	ID3D11InputLayout*		gDX11TerrainInputLayout = 0;

	ID3D11Buffer*			gDX11TerrainVB = 0;
	ID3D11Buffer*			gDX11TerrainIB = 0;

	ID3D11Buffer*			gDX11FrameCB = 0;

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
	bool					gDX11SmokeReadbackLogged = false;
	bool					gDX11TerrainGBufferReadbackLogged = false;

	template <typename T>
	void RenderDX11_SafeRelease(T*& Ptr)
	{
		if (Ptr)
		{
			Ptr->Release();
			Ptr = 0;
		}
	}

	int RenderDX11_ClampSize(int Value);

	class RenderDX11IncludeHandler : public ID3DInclude
	{
	public:
		explicit RenderDX11IncludeHandler(
			const char* ShaderFileName
		)
		{
			BasePath[0] = 0;

			if (!ShaderFileName || !ShaderFileName[0])
				return;

			r3dscpy(BasePath, ShaderFileName);

			for (char* It = BasePath; *It; ++It)
			{
				if (*It == '/')
					*It = '\\';
			}

			char* LastSlash = strrchr(BasePath, '\\');

			if (LastSlash)
				*LastSlash = 0;
			else
				BasePath[0] = 0;
		}

		STDMETHOD(Open)(
			D3D_INCLUDE_TYPE IncludeType,
			LPCSTR pFileName,
			LPCVOID pParentData,
			LPCVOID* ppData,
			UINT* pBytes
		)
		{
			(void)IncludeType;
			(void)pParentData;

			if (!ppData || !pBytes || !pFileName)
				return E_FAIL;

			*ppData = 0;
			*pBytes = 0;

			char FileName[MAX_PATH] = {};

			if (BasePath[0])
			{
				sprintf_s(
					FileName,
					"%s\\%s",
					BasePath,
					pFileName
				);
			}
			else
			{
				sprintf_s(
					FileName,
					"%s",
					pFileName
				);
			}

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
					"[RenderDX11] Missing shader include: %s\n",
					FileName
				);

				OutputDebugStringA(Text);
				return E_FAIL;
			}

			char* Data =
				new char[File->size + 1];

			const int ReadSize =
				fread(
					Data,
					1,
					File->size,
					File
				);

			fclose(File);

			Data[ReadSize] = 0;

			*ppData = Data;
			*pBytes = static_cast<UINT>(ReadSize);

			return S_OK;
		}

		STDMETHOD(Close)(
			LPCVOID pData
		)
		{
			delete[] reinterpret_cast<const char*>(pData);
			return S_OK;
		}

	private:
		char BasePath[MAX_PATH];
	};

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

	void RenderDX11_ReleaseShaders()
	{
		RenderDX11_SafeRelease(gDX11TerrainInputLayout);
		RenderDX11_SafeRelease(gDX11TerrainPS);
		RenderDX11_SafeRelease(gDX11TerrainVS);

		RenderDX11_SafeRelease(gDX11ClearPS);
		RenderDX11_SafeRelease(gDX11ClearVS);
	}

	void RenderDX11_ReleaseTerrainResources()
	{
		RenderDX11_SafeRelease(gDX11TerrainIB);
		RenderDX11_SafeRelease(gDX11TerrainVB);
	}

	void RenderDX11_ReleaseConstantBuffers()
	{
		RenderDX11_SafeRelease(gDX11FrameCB);
	}

	bool RenderDX11_CreateConstantBuffers()
	{
		RenderDX11_ReleaseConstantBuffers();

		D3D11_BUFFER_DESC Desc = {};
		Desc.ByteWidth = sizeof(WorldDX11FrameCB);
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT Hr =
			gDX11Device->CreateBuffer(
				&Desc,
				0,
				&gDX11FrameCB
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create frame constant buffer failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		OutputDebugStringA(
			"[RenderDX11] Constant buffers created\n"
		);

		return true;
	}

	void RenderDX11_SetIdentityMatrix(
		float* Matrix
	)
	{
		if (!Matrix)
			return;

		for (int i = 0; i < 16; ++i)
			Matrix[i] = 0.0f;

		Matrix[0] = 1.0f;
		Matrix[5] = 1.0f;
		Matrix[10] = 1.0f;
		Matrix[15] = 1.0f;
	}

	void RenderDX11_CopyMatrix(
		float* Out,
		const D3DXMATRIX& Matrix
	)
	{
		if (!Out)
			return;

		const float* Source =
			reinterpret_cast<const float*>(&Matrix);

		for (int i = 0; i < 16; ++i)
			Out[i] = Source[i];
	}

	void RenderDX11_BindFrameCB()
	{
		if (!gDX11Context || !gDX11FrameCB)
			return;

		gDX11Context->VSSetConstantBuffers(
			0,
			1,
			&gDX11FrameCB
		);

		gDX11Context->PSSetConstantBuffers(
			0,
			1,
			&gDX11FrameCB
		);
	}

	void RenderDX11_UpdateFrameCB(
		const WorldDX11FrameDesc& Desc
	)
	{
		if (!gDX11Context || !gDX11FrameCB)
			return;

		D3D11_MAPPED_SUBRESOURCE Mapped = {};

		HRESULT Hr =
			gDX11Context->Map(
				gDX11FrameCB,
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&Mapped
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Map frame constant buffer failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return;
		}

		WorldDX11FrameCB* FrameCB =
			reinterpret_cast<WorldDX11FrameCB*>(
				Mapped.pData
			);

		if (r3dRenderer)
		{
			RenderDX11_CopyMatrix(
				FrameCB->ViewProj,
				r3dRenderer->ViewProjMatrix
			);

			RenderDX11_CopyMatrix(
				FrameCB->View,
				r3dRenderer->ViewMatrix
			);

			RenderDX11_CopyMatrix(
				FrameCB->Proj,
				r3dRenderer->ProjMatrix
			);
		}
		else
		{
			RenderDX11_SetIdentityMatrix(FrameCB->ViewProj);
			RenderDX11_SetIdentityMatrix(FrameCB->View);
			RenderDX11_SetIdentityMatrix(FrameCB->Proj);
		}

		FrameCB->CameraPos[0] = gCam.x;
		FrameCB->CameraPos[1] = gCam.y;
		FrameCB->CameraPos[2] = gCam.z;
		FrameCB->CameraPos[3] = 1.0f;

		const float Width =
			static_cast<float>(
				RenderDX11_ClampSize(Desc.Width)
			);

		const float Height =
			static_cast<float>(
				RenderDX11_ClampSize(Desc.Height)
			);

		FrameCB->ScreenSize[0] = Width;
		FrameCB->ScreenSize[1] = Height;
		FrameCB->ScreenSize[2] = 1.0f / Width;
		FrameCB->ScreenSize[3] = 1.0f / Height;

		FrameCB->NearFar[0] = Desc.NearClip;
		FrameCB->NearFar[1] = Desc.FarClip;
		FrameCB->NearFar[2] = 0.0f;
		FrameCB->NearFar[3] = 0.0f;

		gDX11Context->Unmap(
			gDX11FrameCB,
			0
		);

		RenderDX11_BindFrameCB();
	}

	void RenderDX11_MakeShaderFileName(
		char* OutFileName,
		size_t OutFileNameSize,
		const char* RelativeFileName
	)
	{
		if (!OutFileName || !OutFileNameSize)
			return;

		OutFileName[0] = 0;

		if (!RelativeFileName || !RelativeFileName[0])
			return;

		sprintf_s(
			OutFileName,
			OutFileNameSize,
			"Data\\Shaders\\DX11_P1\\%s",
			RelativeFileName
		);
	}

	bool RenderDX11_LoadShaderSource(
	const char* FileName,
	char** OutData,
	UINT* OutSize
)
	{
		if (!FileName || !FileName[0] || !OutData || !OutSize)
			return false;

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
				"[RenderDX11] Missing shader file: %s\n",
				FileName
			);

			OutputDebugStringA(Text);
			return false;
		}

		char* Data =
			new char[File->size + 1];

		const int ReadSize =
			fread(
				Data,
				1,
				File->size,
				File
			);

		fclose(File);

		Data[ReadSize] = 0;

		*OutData = Data;
		*OutSize = static_cast<UINT>(ReadSize);

		return true;
	}

	bool RenderDX11_CompileShaderFromFile(
	const char* RelativeFileName,
	const char* EntryPoint,
	const char* Profile,
	ID3DBlob** OutBlob
)
	{
		if (!RelativeFileName || !EntryPoint || !Profile || !OutBlob)
			return false;

		*OutBlob = 0;

		char FileName[MAX_PATH] = {};

		RenderDX11_MakeShaderFileName(
			FileName,
			_countof(FileName),
			RelativeFileName
		);

		char* SourceData = 0;
		UINT SourceSize = 0;

		if (!RenderDX11_LoadShaderSource(
			FileName,
			&SourceData,
			&SourceSize
		))
		{
			return false;
		}

		UINT Flags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(_DEBUG)
		Flags |= D3DCOMPILE_DEBUG;
		Flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		ID3DBlob* ErrorBlob = 0;

		RenderDX11IncludeHandler IncludeHandler(
			FileName
		);

		HRESULT Hr =
			D3DCompile(
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

		if (FAILED(Hr))
		{
			const char* ErrorText =
				ErrorBlob
				? reinterpret_cast<const char*>(
					ErrorBlob->GetBufferPointer()
				)
				: "unknown error";

			char Text[4096] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Shader compile failed: %s entry=%s profile=%s HRESULT=0x%08X\n%s\n",
				FileName,
				EntryPoint,
				Profile,
				static_cast<unsigned int>(Hr),
				ErrorText
			);

			OutputDebugStringA(Text);

			RenderDX11_SafeRelease(ErrorBlob);

			if (*OutBlob)
			{
				(*OutBlob)->Release();
				*OutBlob = 0;
			}

			return false;
		}

		RenderDX11_SafeRelease(ErrorBlob);

		char Text[512] = {};
		sprintf_s(
			Text,
			"[RenderDX11] Shader compiled: %s entry=%s profile=%s\n",
			FileName,
			EntryPoint,
			Profile
		);

		OutputDebugStringA(Text);

		return true;
	}

	bool RenderDX11_CreateShaders()
	{
		RenderDX11_ReleaseShaders();

		ID3DBlob* VSBlob = 0;
		ID3DBlob* PSBlob = 0;

		if (!RenderDX11_CompileShaderFromFile(
			"system\\dx11_clear.hls",
			"VSMain",
			"vs_5_0",
			&VSBlob
		))
		{
			RenderDX11_SafeRelease(VSBlob);
			RenderDX11_SafeRelease(PSBlob);
			return false;
		}

		HRESULT Hr =
			gDX11Device->CreateVertexShader(
				VSBlob->GetBufferPointer(),
				VSBlob->GetBufferSize(),
				0,
				&gDX11ClearVS
			);

		RenderDX11_SafeRelease(VSBlob);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create clear VS failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		if (!RenderDX11_CompileShaderFromFile(
			"system\\dx11_clear.hls",
			"PSMain",
			"ps_5_0",
			&PSBlob
		))
		{
			RenderDX11_SafeRelease(PSBlob);
			return false;
		}

		Hr =
			gDX11Device->CreatePixelShader(
				PSBlob->GetBufferPointer(),
				PSBlob->GetBufferSize(),
				0,
				&gDX11ClearPS
			);

		RenderDX11_SafeRelease(PSBlob);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create clear PS failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		if (!RenderDX11_CompileShaderFromFile(
			"Nature\\dx11_terrain.hls",
			"VSMain",
			"vs_5_0",
			&VSBlob
		))
		{
			RenderDX11_SafeRelease(VSBlob);
			RenderDX11_SafeRelease(PSBlob);
			return false;
		}

		Hr =
			gDX11Device->CreateVertexShader(
				VSBlob->GetBufferPointer(),
				VSBlob->GetBufferSize(),
				0,
				&gDX11TerrainVS
			);

		if (FAILED(Hr))
		{
			RenderDX11_SafeRelease(VSBlob);

			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create terrain VS failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		const D3D11_INPUT_ELEMENT_DESC TerrainLayoutDesc[] =
		{
			{
				"POSITION",
				0,
				DXGI_FORMAT_R32G32B32_FLOAT,
				0,
				0,
				D3D11_INPUT_PER_VERTEX_DATA,
				0
			}
		};

		Hr =
			gDX11Device->CreateInputLayout(
				TerrainLayoutDesc,
				_countof(TerrainLayoutDesc),
				VSBlob->GetBufferPointer(),
				VSBlob->GetBufferSize(),
				&gDX11TerrainInputLayout
			);

		RenderDX11_SafeRelease(VSBlob);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create terrain input layout failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		if (!RenderDX11_CompileShaderFromFile(
			"Nature\\dx11_terrain.hls",
			"PSMain",
			"ps_5_0",
			&PSBlob
		))
		{
			RenderDX11_SafeRelease(PSBlob);
			return false;
		}

		Hr =
			gDX11Device->CreatePixelShader(
				PSBlob->GetBufferPointer(),
				PSBlob->GetBufferSize(),
				0,
				&gDX11TerrainPS
			);

		RenderDX11_SafeRelease(PSBlob);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create terrain PS failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		OutputDebugStringA(
			"[RenderDX11] Shaders created\n"
		);

		return true;
	}

	void RenderDX11_BindClearShaders()
	{
		if (!gDX11Context)
			return;

		gDX11Context->IASetInputLayout(0);
		gDX11Context->IASetPrimitiveTopology(
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
		);

		gDX11Context->VSSetShader(
			gDX11ClearVS,
			0,
			0
		);

		gDX11Context->PSSetShader(
			gDX11ClearPS,
			0,
			0
		);
	}

	void RenderDX11_DrawClearTriangle()
	{
		if (!gDX11Context || !gDX11ClearVS || !gDX11ClearPS)
			return;

		RenderDX11_BindClearShaders();

		gDX11Context->Draw(
			3,
			0
		);
	}

	bool RenderDX11_CreateTerrainResources()
	{
		if (gDX11TerrainVB && gDX11TerrainIB)
			return true;

		RenderDX11_ReleaseTerrainResources();

		D3D11_BUFFER_DESC VBDesc = {};
		VBDesc.ByteWidth =
			sizeof(WorldDX11TerrainVertex) *
			DX11_TERRAIN_VERTEX_COUNT;
		VBDesc.Usage = D3D11_USAGE_DYNAMIC;
		VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		VBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT Hr =
			gDX11Device->CreateBuffer(
				&VBDesc,
				0,
				&gDX11TerrainVB
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create terrain VB failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		unsigned short Indices[DX11_TERRAIN_INDEX_COUNT] = {};
		int Index = 0;

		for (int z = 0; z < DX11_TERRAIN_GRID_DIM - 1; ++z)
		{
			for (int x = 0; x < DX11_TERRAIN_GRID_DIM - 1; ++x)
			{
				const unsigned short I0 =
					static_cast<unsigned short>(
						z * DX11_TERRAIN_GRID_DIM + x
					);
				const unsigned short I1 =
					static_cast<unsigned short>(
						z * DX11_TERRAIN_GRID_DIM + x + 1
					);
				const unsigned short I2 =
					static_cast<unsigned short>(
						(z + 1) * DX11_TERRAIN_GRID_DIM + x
					);
				const unsigned short I3 =
					static_cast<unsigned short>(
						(z + 1) * DX11_TERRAIN_GRID_DIM + x + 1
					);

				Indices[Index++] = I0;
				Indices[Index++] = I1;
				Indices[Index++] = I2;
				Indices[Index++] = I1;
				Indices[Index++] = I3;
				Indices[Index++] = I2;
			}
		}

		D3D11_BUFFER_DESC IBDesc = {};
		IBDesc.ByteWidth = sizeof(Indices);
		IBDesc.Usage = D3D11_USAGE_DEFAULT;
		IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA InitData = {};
		InitData.pSysMem = Indices;

		Hr =
			gDX11Device->CreateBuffer(
				&IBDesc,
				&InitData,
				&gDX11TerrainIB
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create terrain IB failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		OutputDebugStringA(
			"[RenderDX11] Terrain DX11 buffers created\n"
		);

		return true;
	}

	bool RenderDX11_UpdateTerrainVertices()
	{
		if (!gDX11Context || !gDX11TerrainVB || !Terrain)
			return false;

		if (!Terrain->IsLoaded())
			return false;

		D3D11_MAPPED_SUBRESOURCE Mapped = {};

		HRESULT Hr =
			gDX11Context->Map(
				gDX11TerrainVB,
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&Mapped
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Map terrain VB failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		WorldDX11TerrainVertex* Vertices =
			reinterpret_cast<WorldDX11TerrainVertex*>(
				Mapped.pData
			);

		const r3dTerrainDesc& TerrainDesc =
			Terrain->GetDesc();

		float Step = TerrainDesc.CellSize * 4.0f;

		if (Step <= 0.01f)
			Step = 4.0f;

		const float HalfSize =
			Step *
			static_cast<float>(DX11_TERRAIN_GRID_DIM - 1) *
			0.5f;

		int VertexIndex = 0;

		for (int z = 0; z < DX11_TERRAIN_GRID_DIM; ++z)
		{
			for (int x = 0; x < DX11_TERRAIN_GRID_DIM; ++x)
			{
				const float WorldX =
					gCam.x - HalfSize +
					static_cast<float>(x) * Step;

				const float WorldZ =
					gCam.z - HalfSize +
					static_cast<float>(z) * Step;

				r3dPoint3D Pos(
					WorldX,
					0.0f,
					WorldZ
				);

				Vertices[VertexIndex].Position[0] = WorldX;
				Vertices[VertexIndex].Position[1] =
					Terrain->GetHeight(Pos);
				Vertices[VertexIndex].Position[2] = WorldZ;

				++VertexIndex;
			}
		}

		gDX11Context->Unmap(
			gDX11TerrainVB,
			0
		);

		return true;
	}

	bool RenderDX11_BindTerrainGeometry()
	{
		if (
			!gDX11Context ||
			!gDX11TerrainInputLayout ||
			!gDX11TerrainVB ||
			!gDX11TerrainIB
		)
		{
			return false;
		}

		UINT Stride = sizeof(WorldDX11TerrainVertex);
		UINT Offset = 0;

		gDX11Context->IASetInputLayout(
			gDX11TerrainInputLayout
		);

		gDX11Context->IASetPrimitiveTopology(
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
		);

		gDX11Context->IASetVertexBuffers(
			0,
			1,
			&gDX11TerrainVB,
			&Stride,
			&Offset
		);

		gDX11Context->IASetIndexBuffer(
			gDX11TerrainIB,
			DXGI_FORMAT_R16_UINT,
			0
		);

		return true;
	}

	bool RenderDX11_DrawTerrainDepth()
	{
		if (
			!gDX11Context ||
			!gDX11TerrainVS ||
			!RenderDX11_CreateTerrainResources() ||
			!RenderDX11_UpdateTerrainVertices() ||
			!RenderDX11_BindTerrainGeometry()
		)
		{
			return false;
		}

		gDX11Context->VSSetShader(
			gDX11TerrainVS,
			0,
			0
		);

		gDX11Context->PSSetShader(
			0,
			0,
			0
		);

		gDX11Context->DrawIndexed(
			DX11_TERRAIN_INDEX_COUNT,
			0,
			0
		);

		return true;
	}

	bool RenderDX11_DrawTerrainGBuffer()
	{
		if (
			!gDX11Context ||
			!gDX11TerrainVS ||
			!gDX11TerrainPS ||
			!RenderDX11_CreateTerrainResources() ||
			!RenderDX11_UpdateTerrainVertices() ||
			!RenderDX11_BindTerrainGeometry()
		)
		{
			return false;
		}

		gDX11Context->VSSetShader(
			gDX11TerrainVS,
			0,
			0
		);

		gDX11Context->PSSetShader(
			gDX11TerrainPS,
			0,
			0
		);

		gDX11Context->DrawIndexed(
			DX11_TERRAIN_INDEX_COUNT,
			0,
			0
		);

		return true;
	}

	void RenderDX11_SmokeReadbackOnce()
	{
		if (gDX11SmokeReadbackLogged)
			return;

		if (
			!gDX11Context ||
			!gDX11GBufferColorTexture ||
			!gDX11SmokeReadbackTexture ||
			gDX11FrameWidth <= 0 ||
			gDX11FrameHeight <= 0
		)
		{
			return;
		}

		gDX11Context->CopyResource(
			gDX11SmokeReadbackTexture,
			gDX11GBufferColorTexture
		);

		D3D11_MAPPED_SUBRESOURCE Mapped = {};

		HRESULT Hr =
			gDX11Context->Map(
				gDX11SmokeReadbackTexture,
				0,
				D3D11_MAP_READ,
				0,
				&Mapped
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Smoke readback Map failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			gDX11SmokeReadbackLogged = true;
			return;
		}

		const int SampleX =
			gDX11FrameWidth / 2;

		const int SampleY =
			gDX11FrameHeight / 2;

		const unsigned char* Row =
			reinterpret_cast<const unsigned char*>(
				Mapped.pData
			) + Mapped.RowPitch * SampleY;

		const unsigned char* Pixel =
			Row + SampleX * 4;

		const unsigned int R = Pixel[0];
		const unsigned int G = Pixel[1];
		const unsigned int B = Pixel[2];
		const unsigned int A = Pixel[3];

		gDX11Context->Unmap(
			gDX11SmokeReadbackTexture,
			0
		);

		char Text[512] = {};
		sprintf_s(
			Text,
			"[RenderDX11] Smoke shader draw readback: %dx%d center RGBA=(%u,%u,%u,%u)\n",
			gDX11FrameWidth,
			gDX11FrameHeight,
			R,
			G,
			B,
			A
		);

		OutputDebugStringA(Text);

		gDX11SmokeReadbackLogged = true;
	}

	void RenderDX11_TerrainGBufferReadbackOnce()
	{
		if (gDX11TerrainGBufferReadbackLogged)
			return;

		if (
			!gDX11Context ||
			!gDX11GBufferColorTexture ||
			!gDX11SmokeReadbackTexture ||
			gDX11FrameWidth <= 0 ||
			gDX11FrameHeight <= 0
		)
		{
			return;
		}

		gDX11Context->CopyResource(
			gDX11SmokeReadbackTexture,
			gDX11GBufferColorTexture
		);

		D3D11_MAPPED_SUBRESOURCE Mapped = {};

		HRESULT Hr =
			gDX11Context->Map(
				gDX11SmokeReadbackTexture,
				0,
				D3D11_MAP_READ,
				0,
				&Mapped
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Terrain GBuffer readback Map failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			gDX11TerrainGBufferReadbackLogged = true;
			return;
		}

		const int SampleXs[5] =
		{
			gDX11FrameWidth / 2,
			gDX11FrameWidth / 4,
			(gDX11FrameWidth * 3) / 4,
			gDX11FrameWidth / 2,
			gDX11FrameWidth / 2
		};

		const int SampleYs[5] =
		{
			gDX11FrameHeight / 2,
			gDX11FrameHeight / 2,
			gDX11FrameHeight / 2,
			gDX11FrameHeight / 4,
			(gDX11FrameHeight * 3) / 4
		};

		unsigned int R[5] = {};
		unsigned int G[5] = {};
		unsigned int B[5] = {};
		unsigned int A[5] = {};

		for (int i = 0; i < 5; ++i)
		{
			const unsigned char* Row =
				reinterpret_cast<const unsigned char*>(
					Mapped.pData
				) + Mapped.RowPitch * SampleYs[i];

			const unsigned char* Pixel =
				Row + SampleXs[i] * 4;

			R[i] = Pixel[0];
			G[i] = Pixel[1];
			B[i] = Pixel[2];
			A[i] = Pixel[3];
		}

		gDX11Context->Unmap(
			gDX11SmokeReadbackTexture,
			0
		);

		char Text[1024] = {};
		sprintf_s(
			Text,
			"[RenderDX11] Terrain GBuffer readback: %dx%d center=(%u,%u,%u,%u) left=(%u,%u,%u,%u) right=(%u,%u,%u,%u) top=(%u,%u,%u,%u) bottom=(%u,%u,%u,%u)\n",
			gDX11FrameWidth,
			gDX11FrameHeight,
			R[0], G[0], B[0], A[0],
			R[1], G[1], B[1], A[1],
			R[2], G[2], B[2], A[2],
			R[3], G[3], B[3], A[3],
			R[4], G[4], B[4], A[4]
		);

		OutputDebugStringA(Text);

		gDX11TerrainGBufferReadbackLogged = true;
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
			ID3D11RenderTargetView* NullRTV[4] =
			{
				0,
				0,
				0,
				0
			};

			gDX11Context->OMSetRenderTargets(
				4,
				NullRTV,
				0
			);
		}

		RenderDX11_SafeRelease(gDX11GBufferAuxRTV);
		RenderDX11_SafeRelease(gDX11GBufferDepthLinearRTV);
		RenderDX11_SafeRelease(gDX11GBufferNormalRTV);
		RenderDX11_SafeRelease(gDX11GBufferColorRTV);
		RenderDX11_SafeRelease(gDX11DepthDSV);

		RenderDX11_SafeRelease(gDX11SmokeReadbackTexture);
		RenderDX11_SafeRelease(gDX11GBufferAuxTexture);
		RenderDX11_SafeRelease(gDX11GBufferDepthLinearTexture);
		RenderDX11_SafeRelease(gDX11GBufferNormalTexture);
		RenderDX11_SafeRelease(gDX11GBufferColorTexture);
		RenderDX11_SafeRelease(gDX11DepthTexture);

		gDX11FrameWidth = 0;
		gDX11FrameHeight = 0;
		gDX11SmokeReadbackLogged = false;
		gDX11TerrainGBufferReadbackLogged = false;

		gDX11Viewport = D3D11_VIEWPORT();
	}

	bool RenderDX11_CreateGBufferTarget(
		int Width,
		int Height,
		DXGI_FORMAT Format,
		const char* DebugName,
		ID3D11Texture2D** OutTexture,
		ID3D11RenderTargetView** OutRTV
	)
	{
		if (!OutTexture || !OutRTV)
			return false;

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
			gDX11Device->CreateTexture2D(
				&TextureDesc,
				0,
				OutTexture
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create %s texture failed. HRESULT=0x%08X\n",
				DebugName ? DebugName : "gbuffer",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		Hr =
			gDX11Device->CreateRenderTargetView(
				*OutTexture,
				0,
				OutRTV
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create %s RTV failed. HRESULT=0x%08X\n",
				DebugName ? DebugName : "gbuffer",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		return true;
	}

	bool RenderDX11_CreateGBufferTargets(
		int Width,
		int Height
	)
	{
		if (!RenderDX11_CreateGBufferTarget(
			Width,
			Height,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			"GBufferColor",
			&gDX11GBufferColorTexture,
			&gDX11GBufferColorRTV
		))
		{
			return false;
		}

		if (!RenderDX11_CreateGBufferTarget(
			Width,
			Height,
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			"GBufferNormal",
			&gDX11GBufferNormalTexture,
			&gDX11GBufferNormalRTV
		))
		{
			return false;
		}

		if (!RenderDX11_CreateGBufferTarget(
			Width,
			Height,
			DXGI_FORMAT_R32_FLOAT,
			"GBufferDepthLinear",
			&gDX11GBufferDepthLinearTexture,
			&gDX11GBufferDepthLinearRTV
		))
		{
			return false;
		}

		if (!RenderDX11_CreateGBufferTarget(
			Width,
			Height,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			"GBufferAux",
			&gDX11GBufferAuxTexture,
			&gDX11GBufferAuxRTV
		))
		{
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

	bool RenderDX11_CreateSmokeReadbackTarget(
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
		TextureDesc.Usage = D3D11_USAGE_STAGING;
		TextureDesc.BindFlags = 0;
		TextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		TextureDesc.MiscFlags = 0;

		HRESULT Hr =
			gDX11Device->CreateTexture2D(
				&TextureDesc,
				0,
				&gDX11SmokeReadbackTexture
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create smoke readback texture failed. HRESULT=0x%08X\n",
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
			gDX11GBufferColorTexture &&
			gDX11GBufferNormalTexture &&
			gDX11GBufferDepthLinearTexture &&
			gDX11GBufferAuxTexture &&
			gDX11DepthTexture &&
			gDX11SmokeReadbackTexture &&
			gDX11GBufferColorRTV &&
			gDX11GBufferNormalRTV &&
			gDX11GBufferDepthLinearRTV &&
			gDX11GBufferAuxRTV &&
			gDX11DepthDSV &&
			gDX11FrameWidth == Width &&
			gDX11FrameHeight == Height
		)
		{
			return true;
		}

		RenderDX11_ReleaseFrameTargets();

		if (!RenderDX11_CreateGBufferTargets(Width, Height))
		{
			RenderDX11_ReleaseFrameTargets();
			return false;
		}

		if (!RenderDX11_CreateDepthTarget(Width, Height))
		{
			RenderDX11_ReleaseFrameTargets();
			return false;
		}

		if (!RenderDX11_CreateSmokeReadbackTarget(Width, Height))
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
			"[RenderDX11] GBuffer targets ready %dx%d\n",
			Width,
			Height
		);

		OutputDebugStringA(Text);

		return true;
	}

	void RenderDX11_BindFrameTargets()
	{
		ID3D11RenderTargetView* RTViews[4] =
		{
			gDX11GBufferColorRTV,
			gDX11GBufferNormalRTV,
			gDX11GBufferDepthLinearRTV,
			gDX11GBufferAuxRTV
		};

		gDX11Context->OMSetRenderTargets(
			4,
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

		gDX11Context->ClearRenderTargetView(
			gDX11GBufferColorRTV,
			ClearColorAlbedo
		);

		gDX11Context->ClearRenderTargetView(
			gDX11GBufferNormalRTV,
			ClearNormal
		);

		gDX11Context->ClearRenderTargetView(
			gDX11GBufferDepthLinearRTV,
			ClearDepthLinear
		);

		gDX11Context->ClearRenderTargetView(
			gDX11GBufferAuxRTV,
			ClearAux
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
		ID3D11RenderTargetView* NullRTV[4] =
		{
			0,
			0,
			0,
			0
		};

		gDX11Context->OMSetRenderTargets(
			4,
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

	if (!RenderDX11_CreateConstantBuffers())
	{
		OutputDebugStringA(
			"[RenderDX11] Failed to create constant buffers\n"
		);

		RenderDX11_Shutdown();
		return false;
	}

	if (!RenderDX11_CreateShaders())
	{
		OutputDebugStringA(
			"[RenderDX11] Failed to create shaders\n"
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
	RenderDX11_ReleaseTerrainResources();

	if (gDX11Context)
	{
		gDX11Context->ClearState();
		gDX11Context->Flush();
	}

	RenderDX11_ReleaseShaders();
	RenderDX11_ReleaseConstantBuffers();
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
	RenderDX11_UpdateFrameCB(Desc);

	RenderDX11_ClearFrameTargets();

	// Real DX11 smoke draw.
	// This renders fullscreen triangle into DX11 offscreen target.
	// Result is not presented yet; old DX9 world still renders after fallback.
	RenderDX11_DrawClearTriangle();

	RenderDX11_SmokeReadbackOnce();

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

	RenderDX11_TerrainGBufferReadbackOnce();

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

	if (gDX11Context)
	{
		gDX11Context->Flush();
	}

	RenderDX11_UnbindFrameTargets();

	OutputDebugStringA(
		"[RenderDX11] World path executed. Falling back to DX9 world.\n"
	);

	// Пока возвращаем false.
	// Это важно: старый DX9 RenderDeferredScene1() продолжит рисовать мир.
	return false;
}

#undef OutputDebugStringA

#endif // LTS_STUDIO_DX11
