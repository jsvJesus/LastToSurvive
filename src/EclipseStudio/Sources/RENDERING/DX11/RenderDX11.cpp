#include "r3dPCH.h"
#include "r3d.h"

#include "r3dRendererConfig.h"

#if LTS_STUDIO_DX11

#include <D3D11.h>
#include <DXGI.h>
#include <D3Dcompiler.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "GameCommon.h"
#include "../SF/Console/Config.h"
#include "TrueNature/ITerrain.h"
#include "TrueNature2/Terrain2.h"
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
	static const int DX11_TERRAIN_GRID_DIM = 65;
	static const int DX11_TERRAIN_VERTEX_COUNT =
		DX11_TERRAIN_GRID_DIM * DX11_TERRAIN_GRID_DIM;
	static const int DX11_TERRAIN_INDEX_COUNT =
		(DX11_TERRAIN_GRID_DIM - 1) *
		(DX11_TERRAIN_GRID_DIM - 1) *
		6;
	static const int DX11_TERRAIN_PATCH_DEFAULT_RADIUS = 2;
	static const int DX11_TERRAIN_PATCH_MAX_RADIUS = 3;
	static const int DX11_TERRAIN_PATCH_SIDE =
		DX11_TERRAIN_PATCH_MAX_RADIUS * 2 + 1;
	static const int DX11_TERRAIN_PATCH_COUNT =
		DX11_TERRAIN_PATCH_SIDE * DX11_TERRAIN_PATCH_SIDE;

	struct WorldDX11FrameCB
	{
		float ViewProj[16];
		float View[16];
		float Proj[16];
		float CameraPos[4];
		float ScreenSize[4];
		float NearFar[4];
	};

	struct WorldDX11TerrainCB
	{
		float BaseColor[4];
		float ColorScale[4];
		float DebugParams[4];
		float TerrainSize[4];
		float LayerScale0[4];
		float LayerScale1[4];
		float AtlasTexTransform[4];
		float AtlasWorld[4];
	};

	struct WorldDX11ObjectCB
	{
		float World[16];
		float PrevWorld[16];
		float ObjectColor[4];
		float ObjectParams[4];
	};

	struct WorldDX11MaterialCB
	{
		float DiffuseScale[4];
		float NormalScale[4];
		float SpecularGloss[4];
		float MaterialParams[4];
	};

	struct WorldDX11LightCB
	{
		float SunDir[4];
		float SunColor[4];
		float AmbientColor[4];
		float FogColor[4];
		float FogParams[4];
	};

	struct WorldDX11ShadowCB
	{
		float ShadowParams[4];
		float ShadowAtlasParams[4];
	};

	struct WorldDX11WaterCB
	{
		float WaterColor[4];
		float WaterParams[4];
		float WaterUVParams[4];
	};

	struct WorldDX11GrassCB
	{
		float GrassParams[4];
		float WindParams[4];
	};

	struct WorldDX11TerrainVertex
	{
		float Position[3];
		float Normal[3];
	};

	struct WorldDX11TerrainPatchCacheEntry
	{
		ID3D11Buffer* VertexBuffer;
		int TileX;
		int TileZ;
		int L;
		float PatchSize;
		bool Valid;
		unsigned int LastUsedFrame;
	};

	struct WorldDX11TerrainPatchBounds
	{
		float MinX;
		float MinY;
		float MinZ;
		float MaxX;
		float MaxY;
		float MaxZ;
	};

	struct WorldDX11FrustumPlane
	{
		float A;
		float B;
		float C;
		float D;
	};

	struct WorldDX11Terrain2TextureBridge
	{
		r3dTexture* Source;
		ID3D11Texture2D* Texture;
		ID3D11ShaderResourceView* SRV;
		int Width;
		int Height;
		D3DFORMAT SourceFormat;
		DXGI_FORMAT DXFormat;
	};

	struct WorldDX11Terrain2LayerSlot
	{
		r3dTexture* DiffuseTexture;
		r3dTexture* NormalTexture;
		float ScaleU;
		float ScaleV;
		int SourceLayerIndex;
	};

	static const int DX11_PREVIEW_WIDTH = 960;
	static const int DX11_PREVIEW_HEIGHT = 1080;

	static const int DX11_TERRAIN2_LAYERS_PER_MASK =
		r3dTerrain2::LAYERS_PER_MASK;
	static const int DX11_TERRAIN2_BATCH_LAYER_COUNT =
		DX11_TERRAIN2_LAYERS_PER_MASK + 1;

	typedef char WorldDX11FrameCB_SizeMustBe16ByteAligned[
		(sizeof(WorldDX11FrameCB) % 16) == 0 ? 1 : -1
	];

	typedef char WorldDX11TerrainCB_SizeMustBe16ByteAligned[
		(sizeof(WorldDX11TerrainCB) % 16) == 0 ? 1 : -1
	];

	typedef char WorldDX11ObjectCB_SizeMustBe16ByteAligned[
		(sizeof(WorldDX11ObjectCB) % 16) == 0 ? 1 : -1
	];

	typedef char WorldDX11MaterialCB_SizeMustBe16ByteAligned[
		(sizeof(WorldDX11MaterialCB) % 16) == 0 ? 1 : -1
	];

	typedef char WorldDX11LightCB_SizeMustBe16ByteAligned[
		(sizeof(WorldDX11LightCB) % 16) == 0 ? 1 : -1
	];

	typedef char WorldDX11ShadowCB_SizeMustBe16ByteAligned[
		(sizeof(WorldDX11ShadowCB) % 16) == 0 ? 1 : -1
	];

	typedef char WorldDX11WaterCB_SizeMustBe16ByteAligned[
		(sizeof(WorldDX11WaterCB) % 16) == 0 ? 1 : -1
	];

	typedef char WorldDX11GrassCB_SizeMustBe16ByteAligned[
		(sizeof(WorldDX11GrassCB) % 16) == 0 ? 1 : -1
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

	WorldDX11TerrainPatchCacheEntry
							gDX11TerrainPatchCache[DX11_TERRAIN_PATCH_COUNT] = {};
	ID3D11Buffer*			gDX11TerrainIB = 0;

	unsigned int			gDX11TerrainCacheFrameId = 0;
	int						gDX11TerrainPatchDrawCount = 0;
	int						gDX11TerrainPatchUpdateCount = 0;
	int						gDX11TerrainPatchCullCount = 0;

	r3dTexture*				gDX11Terrain2ColorTexture = 0;
	r3dTexture*				gDX11Terrain2NormalTexture = 0;
	r3dTexture*				gDX11Terrain2HeightTexture = 0;

	WorldDX11Terrain2LayerSlot
							gDX11Terrain2BatchLayers[
								DX11_TERRAIN2_BATCH_LAYER_COUNT
							] = {};
	r3dTexture*				gDX11Terrain2BatchMaskTexture = 0;
	int						gDX11Terrain2LayerCount = 0;
	int						gDX11Terrain2MaskCount = 0;
	int						gDX11Terrain2ActiveMaskIndex = 0;

	int						gDX11Terrain2TextureMask = 0;
	bool					gDX11Terrain2TextureInfoLogged = false;

	WorldDX11Terrain2TextureBridge gDX11Terrain2ColorBridge = {};
	WorldDX11Terrain2TextureBridge gDX11Terrain2NormalBridge = {};
	WorldDX11Terrain2TextureBridge gDX11Terrain2HeightBridge = {};
	WorldDX11Terrain2TextureBridge
		gDX11Terrain2BatchDiffuseBridge[
			DX11_TERRAIN2_BATCH_LAYER_COUNT
		] = {};
	WorldDX11Terrain2TextureBridge
		gDX11Terrain2BatchNormalBridge[
			DX11_TERRAIN2_BATCH_LAYER_COUNT
		] = {};
	WorldDX11Terrain2TextureBridge gDX11Terrain2BatchMaskBridge = {};
	WorldDX11Terrain2TextureBridge* gDX11Terrain2AtlasDiffuseBridges = 0;
	WorldDX11Terrain2TextureBridge* gDX11Terrain2AtlasNormalBridges = 0;
	int						gDX11Terrain2AtlasBridgeCount = 0;

	int						gDX11Terrain2SRVMask = 0;
	int						gDX11Terrain2ActiveAtlasSRVMask = 0;

	ID3D11Buffer*			gDX11FrameCB = 0;
	ID3D11Buffer*			gDX11TerrainCB = 0;
	ID3D11Buffer*			gDX11ObjectCB = 0;
	ID3D11Buffer*			gDX11MaterialCB = 0;
	ID3D11Buffer*			gDX11LightCB = 0;
	ID3D11Buffer*			gDX11ShadowCB = 0;
	ID3D11Buffer*			gDX11WaterCB = 0;
	ID3D11Buffer*			gDX11GrassCB = 0;

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
	bool					gDX11PreviewValid = false;
	bool					gDX11OffscreenOnlyLogged = false;

	ID3D11Texture2D*		gDX11PreviewReadbackTexture = 0;
	DXGI_FORMAT				gDX11PreviewReadbackFormat = DXGI_FORMAT_UNKNOWN;
	int						gDX11PreviewReadbackWidth = 0;
	int						gDX11PreviewReadbackHeight = 0;

	r3dTexture*				gDX11PreviewTexture = 0;

	unsigned int			gDX11PreviewPixels[
		DX11_PREVIEW_WIDTH *
		DX11_PREVIEW_HEIGHT
	] = {};

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
	void RenderDX11_UpdateTerrain2TextureRefs();
	void RenderDX11_BindTerrain2TextureSlots();
	void RenderDX11_ReleaseTerrain2TextureBridges();
	
	bool RenderDX11_UpdateTerrainVertices(
		ID3D11Buffer* VertexBuffer,
		int TileX,
		int TileZ,
		float PatchSize
	);

	bool RenderDX11_IsSwitchBoundary(char Ch)
	{
		return
			Ch == 0 ||
			isspace(static_cast<unsigned char>(Ch)) ||
			Ch == '"' ||
			Ch == '\'';
	}

	bool RenderDX11_CommandLineHasSwitch(const char* SwitchName)
	{
		if (!SwitchName || !SwitchName[0])
			return false;

		const char* CmdLine = GetCommandLineA();

		if (!CmdLine || !CmdLine[0])
			return false;

		const size_t SwitchLen = strlen(SwitchName);

		for (const char* It = CmdLine; *It; ++It)
		{
			const bool bStartBoundary =
				It == CmdLine ||
				RenderDX11_IsSwitchBoundary(*(It - 1));

			if (!bStartBoundary)
				continue;

			if (_strnicmp(It, SwitchName, SwitchLen) != 0)
				continue;

			if (!RenderDX11_IsSwitchBoundary(It[SwitchLen]))
				continue;

			return true;
		}

		return false;
	}

	bool RenderDX11_WantsSmokeDebug()
	{
		return
			RenderDX11_CommandLineHasSwitch("-dx11smoke") ||
			RenderDX11_CommandLineHasSwitch("/dx11smoke");
	}

	bool RenderDX11_WantsDebugPreview()
	{
		return
			RenderDX11_CommandLineHasSwitch("-dx11preview") ||
			RenderDX11_CommandLineHasSwitch("/dx11preview");
	}

	bool RenderDX11_WantsTerrainCullEnabled()
	{
		if (
			RenderDX11_CommandLineHasSwitch("-dx11terrainnocull") ||
			RenderDX11_CommandLineHasSwitch("/dx11terrainnocull")
		)
		{
			return false;
		}

		return
			RenderDX11_CommandLineHasSwitch("-dx11terraincull") ||
			RenderDX11_CommandLineHasSwitch("/dx11terraincull");
	}

	int RenderDX11_GetTerrainPatchRadius()
	{
		if (
			RenderDX11_CommandLineHasSwitch("-dx11terrain3x3") ||
			RenderDX11_CommandLineHasSwitch("/dx11terrain3x3")
		)
		{
			return 1;
		}

		if (
			RenderDX11_CommandLineHasSwitch("-dx11terrainwide") ||
			RenderDX11_CommandLineHasSwitch("/dx11terrainwide") ||
			RenderDX11_CommandLineHasSwitch("-dx11terrain7x7") ||
			RenderDX11_CommandLineHasSwitch("/dx11terrain7x7")
		)
		{
			return DX11_TERRAIN_PATCH_MAX_RADIUS;
		}

		return DX11_TERRAIN_PATCH_DEFAULT_RADIUS;
	}

	int RenderDX11_GetTerrainPatchSide()
	{
		return
			RenderDX11_GetTerrainPatchRadius() *
			2 +
			1;
	}

	void RenderDX11_ResetTerrain2LayerSlot(
		WorldDX11Terrain2LayerSlot& Slot
	)
	{
		Slot.DiffuseTexture = 0;
		Slot.NormalTexture = 0;
		Slot.ScaleU = 1.0f;
		Slot.ScaleV = 1.0f;
		Slot.SourceLayerIndex = -1;
	}

	bool RenderDX11_IsTerrain2TextureReady(
		r3dTexture* Texture
	)
	{
		return
			Texture &&
			Texture->IsLoaded() &&
			!Texture->IsMissing();
	}

	enum RenderDX11PreviewMode
	{
		DX11_PREVIEW_COLOR = 0,
		DX11_PREVIEW_NORMAL,
		DX11_PREVIEW_LINEAR_DEPTH,
		DX11_PREVIEW_AUX,
		DX11_PREVIEW_TERRAIN_MASK,
		DX11_PREVIEW_DEPTH
	};

	RenderDX11PreviewMode RenderDX11_GetPreviewModeFromValue(
		int Mode
	);

	const char* RenderDX11_GetPreviewModeName(
		RenderDX11PreviewMode Mode
	);

	void RenderDX11_UpdatePreviewModeHotkey();

	RenderDX11PreviewMode RenderDX11_GetPreviewMode()
	{
		RenderDX11_UpdatePreviewModeHotkey();

		const int Mode =
			r_dx11_debug_view
			? r_dx11_debug_view->GetInt()
			: 0;

		return RenderDX11_GetPreviewModeFromValue(
			Mode
		);
	}

	RenderDX11PreviewMode RenderDX11_GetPreviewModeFromValue(
		int Mode
	)
	{
		switch (Mode)
		{
		case 2:
			return DX11_PREVIEW_NORMAL;

		case 3:
			return DX11_PREVIEW_LINEAR_DEPTH;

		case 4:
			return DX11_PREVIEW_AUX;

		case 5:
			return DX11_PREVIEW_TERRAIN_MASK;

		case 6:
			return DX11_PREVIEW_DEPTH;

		case 0:
		case 1:
		default:
			return DX11_PREVIEW_COLOR;
		}
	}

	int RenderDX11_PreviewModeToCVarValue(
		RenderDX11PreviewMode Mode
	)
	{
		switch (Mode)
		{
		case DX11_PREVIEW_NORMAL:
			return 2;

		case DX11_PREVIEW_LINEAR_DEPTH:
			return 3;

		case DX11_PREVIEW_AUX:
			return 4;

		case DX11_PREVIEW_TERRAIN_MASK:
			return 5;

		case DX11_PREVIEW_DEPTH:
			return 6;

		case DX11_PREVIEW_COLOR:
		default:
			return 0;
		}
	}

	void RenderDX11_UpdatePreviewModeHotkey()
	{
		if (!RenderDX11_WantsDebugPreview())
			return;

		if (!Keyboard)
			return;

		static bool WasF10Down = false;

		const bool IsF10Down =
			Keyboard->IsPressed(kbsF10);

		if (IsF10Down && !WasF10Down)
		{
			const int CurrentValue =
				r_dx11_debug_view
				? r_dx11_debug_view->GetInt()
				: 0;

			RenderDX11PreviewMode CurrentMode =
				RenderDX11_GetPreviewModeFromValue(
					CurrentValue
				);

			RenderDX11PreviewMode NextMode =
				DX11_PREVIEW_COLOR;

			switch (CurrentMode)
			{
			case DX11_PREVIEW_COLOR:
				NextMode = DX11_PREVIEW_NORMAL;
				break;

			case DX11_PREVIEW_NORMAL:
				NextMode = DX11_PREVIEW_LINEAR_DEPTH;
				break;

			case DX11_PREVIEW_LINEAR_DEPTH:
				NextMode = DX11_PREVIEW_AUX;
				break;

			case DX11_PREVIEW_AUX:
				NextMode = DX11_PREVIEW_TERRAIN_MASK;
				break;

			case DX11_PREVIEW_TERRAIN_MASK:
				NextMode = DX11_PREVIEW_DEPTH;
				break;

			case DX11_PREVIEW_DEPTH:
			default:
				NextMode = DX11_PREVIEW_COLOR;
				break;
			}

			const int NewValue =
				RenderDX11_PreviewModeToCVarValue(
					NextMode
				);

			if (r_dx11_debug_view)
			{
				r_dx11_debug_view->SetInt(NewValue);
			}

			gDX11PreviewValid = false;
			gDX11TerrainGBufferReadbackLogged = false;

			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Preview mode changed by F10: %s\n",
				RenderDX11_GetPreviewModeName(NextMode)
			);

			OutputDebugStringA(Text);
		}

		WasF10Down = IsF10Down;
	}

	const char* RenderDX11_GetPreviewModeName(
		RenderDX11PreviewMode Mode
	)
	{
		switch (Mode)
		{
		case DX11_PREVIEW_COLOR:
			return "Color";

		case DX11_PREVIEW_NORMAL:
			return "Normal";

		case DX11_PREVIEW_LINEAR_DEPTH:
			return "LinearDepth";

		case DX11_PREVIEW_AUX:
			return "Aux";

		case DX11_PREVIEW_TERRAIN_MASK:
			return "TerrainMask";

		case DX11_PREVIEW_DEPTH:
			return "Depth";

		default:
			return "Unknown";
		}
	}

	unsigned int RenderDX11_PackPreviewColor(
		unsigned int R,
		unsigned int G,
		unsigned int B
	)
	{
		if (R > 255) R = 255;
		if (G > 255) G = 255;
		if (B > 255) B = 255;

		return
			(0xffu << 24) |
			(R << 16) |
			(G << 8) |
			B;
	}

	float RenderDX11_SaturateFloat(
		float Value
	)
	{
		if (Value < 0.0f)
			return 0.0f;

		if (Value > 1.0f)
			return 1.0f;

		return Value;
	}

	unsigned int RenderDX11_PackPreviewGray(
		float Value
	)
	{
		const float Saturated =
			RenderDX11_SaturateFloat(Value);

		const unsigned int C =
			static_cast<unsigned int>(
				Saturated * 255.0f + 0.5f
			);

		return RenderDX11_PackPreviewColor(
			C,
			C,
			C
		);
	}

	float RenderDX11_HalfToFloat(
		unsigned short Value
	)
	{
		const unsigned int Sign =
			(static_cast<unsigned int>(Value) & 0x8000u) << 16;

		int Exponent =
			(Value >> 10) & 0x1f;

		unsigned int Mantissa =
			Value & 0x03ffu;

		unsigned int Result = 0;

		if (Exponent == 0)
		{
			if (Mantissa == 0)
			{
				Result = Sign;
			}
			else
			{
				Exponent = 1;

				while ((Mantissa & 0x0400u) == 0)
				{
					Mantissa <<= 1;
					--Exponent;
				}

				Mantissa &= 0x03ffu;

				Result =
					Sign |
					(static_cast<unsigned int>(
						Exponent + 127 - 15
					) << 23) |
					(Mantissa << 13);
			}
		}
		else if (Exponent == 31)
		{
			Result =
				Sign |
				0x7f800000u |
				(Mantissa << 13);
		}
		else
		{
			Result =
				Sign |
				(static_cast<unsigned int>(
					Exponent + 127 - 15
				) << 23) |
				(Mantissa << 13);
		}

		float FloatValue = 0.0f;

		memcpy(
			&FloatValue,
			&Result,
			sizeof(FloatValue)
		);

		return FloatValue;
	}

	ID3D11Texture2D* RenderDX11_GetPreviewSourceTexture(
		RenderDX11PreviewMode Mode,
		DXGI_FORMAT* OutFormat
	)
	{
		if (OutFormat)
			*OutFormat = DXGI_FORMAT_UNKNOWN;

		switch (Mode)
		{
		case DX11_PREVIEW_NORMAL:
			if (OutFormat)
				*OutFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
			return gDX11GBufferNormalTexture;

		case DX11_PREVIEW_LINEAR_DEPTH:
			if (OutFormat)
				*OutFormat = DXGI_FORMAT_R32_FLOAT;
			return gDX11GBufferDepthLinearTexture;

		case DX11_PREVIEW_AUX:
		case DX11_PREVIEW_TERRAIN_MASK:
			if (OutFormat)
				*OutFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			return gDX11GBufferAuxTexture;

		case DX11_PREVIEW_DEPTH:
			if (OutFormat)
				*OutFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			return gDX11DepthTexture;

		case DX11_PREVIEW_COLOR:
		default:
			if (OutFormat)
				*OutFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			return gDX11GBufferColorTexture;
		}
	}

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

			const size_t ReadSize =
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
		for (int i = 0; i < DX11_TERRAIN_PATCH_COUNT; ++i)
		{
			RenderDX11_SafeRelease(
				gDX11TerrainPatchCache[i].VertexBuffer
			);

			gDX11TerrainPatchCache[i].TileX = 0;
			gDX11TerrainPatchCache[i].TileZ = 0;
			gDX11TerrainPatchCache[i].L = 0;
			gDX11TerrainPatchCache[i].PatchSize = 0.0f;
			gDX11TerrainPatchCache[i].Valid = false;
			gDX11TerrainPatchCache[i].LastUsedFrame = 0;
		}

		RenderDX11_SafeRelease(gDX11TerrainIB);

		gDX11TerrainCacheFrameId = 0;
		gDX11TerrainPatchDrawCount = 0;
		gDX11TerrainPatchUpdateCount = 0;
		gDX11TerrainPatchCullCount = 0;

		RenderDX11_ReleaseTerrain2TextureBridges();
	}

	void RenderDX11_ReleasePreviewTexture()
	{
		if (gDX11PreviewTexture && r3dRenderer)
		{
			r3dRenderer->DeleteTexture(
				gDX11PreviewTexture,
				1
			);
		}

		gDX11PreviewTexture = 0;
		gDX11PreviewValid = false;
	}

	bool RenderDX11_EnsurePreviewTexture()
	{
		if (gDX11PreviewTexture)
			return true;

		if (!r3dRenderer || !r3dRenderer->pd3ddev)
			return false;

		gDX11PreviewTexture =
			r3dRenderer->AllocateTexture();

		if (!gDX11PreviewTexture)
			return false;

		if (!gDX11PreviewTexture->Create(
			DX11_PREVIEW_WIDTH,
			DX11_PREVIEW_HEIGHT,
			D3DFMT_A8R8G8B8,
			1,
			D3DPOOL_MANAGED
		))
		{
			RenderDX11_ReleasePreviewTexture();
			return false;
		}

		return true;
	}

	void RenderDX11_ReleaseConstantBuffers()
	{
		RenderDX11_SafeRelease(gDX11GrassCB);
		RenderDX11_SafeRelease(gDX11WaterCB);
		RenderDX11_SafeRelease(gDX11ShadowCB);
		RenderDX11_SafeRelease(gDX11LightCB);
		RenderDX11_SafeRelease(gDX11MaterialCB);
		RenderDX11_SafeRelease(gDX11ObjectCB);
		RenderDX11_SafeRelease(gDX11TerrainCB);
		RenderDX11_SafeRelease(gDX11FrameCB);
	}

	bool RenderDX11_CreateDynamicConstantBuffer(
	UINT ByteWidth,
	const char* DebugName,
	ID3D11Buffer** OutBuffer
)
	{
		if (!gDX11Device || !OutBuffer)
			return false;

		*OutBuffer = 0;

		D3D11_BUFFER_DESC Desc = {};
		Desc.ByteWidth = ByteWidth;
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Desc.MiscFlags = 0;
		Desc.StructureByteStride = 0;

		HRESULT Hr =
			gDX11Device->CreateBuffer(
				&Desc,
				0,
				OutBuffer
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create %s constant buffer failed. HRESULT=0x%08X\n",
				DebugName ? DebugName : "unknown",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		return true;
	}

	bool RenderDX11_CreateConstantBuffers()
	{
		RenderDX11_ReleaseConstantBuffers();

		if (!RenderDX11_CreateDynamicConstantBuffer(
			sizeof(WorldDX11FrameCB),
			"FrameCB",
			&gDX11FrameCB
		))
		{
			RenderDX11_ReleaseConstantBuffers();
			return false;
		}

		if (!RenderDX11_CreateDynamicConstantBuffer(
			sizeof(WorldDX11TerrainCB),
			"TerrainCB",
			&gDX11TerrainCB
		))
		{
			RenderDX11_ReleaseConstantBuffers();
			return false;
		}

		if (!RenderDX11_CreateDynamicConstantBuffer(
			sizeof(WorldDX11ObjectCB),
			"ObjectCB",
			&gDX11ObjectCB
		))
		{
			RenderDX11_ReleaseConstantBuffers();
			return false;
		}

		if (!RenderDX11_CreateDynamicConstantBuffer(
			sizeof(WorldDX11MaterialCB),
			"MaterialCB",
			&gDX11MaterialCB
		))
		{
			RenderDX11_ReleaseConstantBuffers();
			return false;
		}

		if (!RenderDX11_CreateDynamicConstantBuffer(
			sizeof(WorldDX11LightCB),
			"LightCB",
			&gDX11LightCB
		))
		{
			RenderDX11_ReleaseConstantBuffers();
			return false;
		}

		if (!RenderDX11_CreateDynamicConstantBuffer(
			sizeof(WorldDX11ShadowCB),
			"ShadowCB",
			&gDX11ShadowCB
		))
		{
			RenderDX11_ReleaseConstantBuffers();
			return false;
		}

		if (!RenderDX11_CreateDynamicConstantBuffer(
			sizeof(WorldDX11WaterCB),
			"WaterCB",
			&gDX11WaterCB
		))
		{
			RenderDX11_ReleaseConstantBuffers();
			return false;
		}

		if (!RenderDX11_CreateDynamicConstantBuffer(
			sizeof(WorldDX11GrassCB),
			"GrassCB",
			&gDX11GrassCB
		))
		{
			RenderDX11_ReleaseConstantBuffers();
			return false;
		}

		OutputDebugStringA(
			"[RenderDX11] Constant buffers created: FrameCB(b0), TerrainCB(b1), ObjectCB(b2), MaterialCB(b3), LightCB(b4), ShadowCB(b5), WaterCB(b6), GrassCB(b7)\n"
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

	void RenderDX11_BindTerrainCB()
	{
		if (!gDX11Context || !gDX11TerrainCB)
			return;

		gDX11Context->VSSetConstantBuffers(
			1,
			1,
			&gDX11TerrainCB
		);

		gDX11Context->PSSetConstantBuffers(
			1,
			1,
			&gDX11TerrainCB
		);
	}

	bool RenderDX11_UpdateConstantBuffer(
		ID3D11Buffer* Buffer,
		const void* Data,
		size_t DataSize,
		const char* DebugName
	)
	{
		if (!gDX11Context || !Buffer || !Data || DataSize == 0)
			return false;

		D3D11_MAPPED_SUBRESOURCE Mapped = {};

		HRESULT Hr =
			gDX11Context->Map(
				Buffer,
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
				"[RenderDX11] Map %s constant buffer failed. HRESULT=0x%08X\n",
				DebugName ? DebugName : "unknown",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		memcpy(
			Mapped.pData,
			Data,
			DataSize
		);

		gDX11Context->Unmap(
			Buffer,
			0
		);

		return true;
	}

	void RenderDX11_BindWorldConstantBuffers()
	{
		if (!gDX11Context)
			return;

		ID3D11Buffer* Buffers[8] =
		{
			gDX11FrameCB,
			gDX11TerrainCB,
			gDX11ObjectCB,
			gDX11MaterialCB,
			gDX11LightCB,
			gDX11ShadowCB,
			gDX11WaterCB,
			gDX11GrassCB
		};

		gDX11Context->VSSetConstantBuffers(
			0,
			8,
			Buffers
		);

		gDX11Context->PSSetConstantBuffers(
			0,
			8,
			Buffers
		);
	}

	void RenderDX11_UpdateDefaultWorldCBs()
	{
		WorldDX11ObjectCB ObjectCB = {};
		RenderDX11_SetIdentityMatrix(ObjectCB.World);
		RenderDX11_SetIdentityMatrix(ObjectCB.PrevWorld);
		ObjectCB.ObjectColor[0] = 1.0f;
		ObjectCB.ObjectColor[1] = 1.0f;
		ObjectCB.ObjectColor[2] = 1.0f;
		ObjectCB.ObjectColor[3] = 1.0f;

		WorldDX11MaterialCB MaterialCB = {};
		MaterialCB.DiffuseScale[0] = 1.0f;
		MaterialCB.DiffuseScale[1] = 1.0f;
		MaterialCB.DiffuseScale[2] = 1.0f;
		MaterialCB.DiffuseScale[3] = 1.0f;
		MaterialCB.NormalScale[0] = 1.0f;
		MaterialCB.SpecularGloss[0] = 0.0f;
		MaterialCB.SpecularGloss[1] = 0.5f;

		WorldDX11LightCB LightCB = {};
		LightCB.SunDir[0] = 0.0f;
		LightCB.SunDir[1] = -1.0f;
		LightCB.SunDir[2] = 0.0f;
		LightCB.SunDir[3] = 0.0f;
		LightCB.SunColor[0] = 1.0f;
		LightCB.SunColor[1] = 1.0f;
		LightCB.SunColor[2] = 1.0f;
		LightCB.SunColor[3] = 1.0f;
		LightCB.AmbientColor[0] = 0.12f;
		LightCB.AmbientColor[1] = 0.12f;
		LightCB.AmbientColor[2] = 0.12f;
		LightCB.AmbientColor[3] = 1.0f;
		LightCB.FogColor[0] = 0.45f;
		LightCB.FogColor[1] = 0.50f;
		LightCB.FogColor[2] = 0.55f;
		LightCB.FogColor[3] = 1.0f;

		WorldDX11ShadowCB ShadowCB = {};
		ShadowCB.ShadowParams[0] = 0.0f;
		ShadowCB.ShadowAtlasParams[0] = 1.0f;

		WorldDX11WaterCB WaterCB = {};
		WaterCB.WaterColor[0] = 0.05f;
		WaterCB.WaterColor[1] = 0.10f;
		WaterCB.WaterColor[2] = 0.12f;
		WaterCB.WaterColor[3] = 0.65f;

		WorldDX11GrassCB GrassCB = {};
		GrassCB.GrassParams[0] = 1.0f;
		GrassCB.WindParams[0] = 0.0f;
		GrassCB.WindParams[1] = 0.0f;
		GrassCB.WindParams[2] = 0.0f;
		GrassCB.WindParams[3] = 0.0f;

		RenderDX11_UpdateConstantBuffer(
			gDX11ObjectCB,
			&ObjectCB,
			sizeof(ObjectCB),
			"ObjectCB"
		);

		RenderDX11_UpdateConstantBuffer(
			gDX11MaterialCB,
			&MaterialCB,
			sizeof(MaterialCB),
			"MaterialCB"
		);

		RenderDX11_UpdateConstantBuffer(
			gDX11LightCB,
			&LightCB,
			sizeof(LightCB),
			"LightCB"
		);

		RenderDX11_UpdateConstantBuffer(
			gDX11ShadowCB,
			&ShadowCB,
			sizeof(ShadowCB),
			"ShadowCB"
		);

		RenderDX11_UpdateConstantBuffer(
			gDX11WaterCB,
			&WaterCB,
			sizeof(WaterCB),
			"WaterCB"
		);

		RenderDX11_UpdateConstantBuffer(
			gDX11GrassCB,
			&GrassCB,
			sizeof(GrassCB),
			"GrassCB"
		);

		RenderDX11_BindWorldConstantBuffers();
	}

	void RenderDX11_FillTerrainCB(
		WorldDX11TerrainCB* TerrainCB,
		const r3dTerrain2::DX11AtlasTileInfo* AtlasTile
	)
	{
		if (!TerrainCB)
			return;

		TerrainCB->BaseColor[0] = 0.10f;
		TerrainCB->BaseColor[1] = 0.24f;
		TerrainCB->BaseColor[2] = 0.10f;
		TerrainCB->BaseColor[3] = 1.00f;

		TerrainCB->ColorScale[0] = 0.25f;
		TerrainCB->ColorScale[1] = 0.35f;
		TerrainCB->ColorScale[2] = 0.10f;
		TerrainCB->ColorScale[3] = 1.0f;

		float InvTerrainSizeX = 1.0f;
		float InvTerrainSizeZ = 1.0f;
		float TerrainSizeX = 1.0f;
		float TerrainSizeZ = 1.0f;

		if (Terrain && Terrain->IsLoaded())
		{
			const r3dTerrainDesc& TerrainDesc =
				Terrain->GetDesc();

			TerrainSizeX =
				R3D_MAX(
					TerrainDesc.XSize,
					1.0f
				);

			TerrainSizeZ =
				R3D_MAX(
					TerrainDesc.ZSize,
					1.0f
				);

			if (TerrainDesc.XSize > 0.01f)
				InvTerrainSizeX = 1.0f / TerrainDesc.XSize;

			if (TerrainDesc.ZSize > 0.01f)
				InvTerrainSizeZ = 1.0f / TerrainDesc.ZSize;
		}

		TerrainCB->DebugParams[0] = 64.0f;        // height offset
		TerrainCB->DebugParams[1] = 1.0f / 128.0f; // height range inverse
		TerrainCB->DebugParams[2] =
			static_cast<float>(
				gDX11Terrain2SRVMask |
				gDX11Terrain2ActiveAtlasSRVMask
			);
		TerrainCB->DebugParams[3] =
			gDX11Terrain2BatchLayers[0].ScaleV;

		TerrainCB->ColorScale[3] =
			gDX11Terrain2BatchLayers[0].ScaleU;

		TerrainCB->TerrainSize[0] = TerrainSizeX;
		TerrainCB->TerrainSize[1] = TerrainSizeZ;
		TerrainCB->TerrainSize[2] = InvTerrainSizeX;
		TerrainCB->TerrainSize[3] = InvTerrainSizeZ;

		TerrainCB->LayerScale0[0] =
			gDX11Terrain2BatchLayers[0].ScaleU;
		TerrainCB->LayerScale0[1] =
			gDX11Terrain2BatchLayers[0].ScaleV;
		TerrainCB->LayerScale0[2] =
			gDX11Terrain2BatchLayers[1].ScaleU;
		TerrainCB->LayerScale0[3] =
			gDX11Terrain2BatchLayers[1].ScaleV;

		TerrainCB->LayerScale1[0] =
			gDX11Terrain2BatchLayers[2].ScaleU;
		TerrainCB->LayerScale1[1] =
			gDX11Terrain2BatchLayers[2].ScaleV;
		TerrainCB->LayerScale1[2] =
			gDX11Terrain2BatchLayers[3].ScaleU;
		TerrainCB->LayerScale1[3] =
			gDX11Terrain2BatchLayers[3].ScaleV;

		TerrainCB->AtlasTexTransform[0] = 0.0f;
		TerrainCB->AtlasTexTransform[1] = 0.0f;
		TerrainCB->AtlasTexTransform[2] = 0.0f;
		TerrainCB->AtlasTexTransform[3] = 0.0f;

		TerrainCB->AtlasWorld[0] = 0.0f;
		TerrainCB->AtlasWorld[1] = 0.0f;
		TerrainCB->AtlasWorld[2] = 0.0f;
		TerrainCB->AtlasWorld[3] = 0.0f;

		if (AtlasTile)
		{
			TerrainCB->AtlasTexTransform[0] =
				AtlasTile->TexScaleU;
			TerrainCB->AtlasTexTransform[1] =
				AtlasTile->TexScaleV;
			TerrainCB->AtlasTexTransform[2] =
				AtlasTile->TexOffsetU;
			TerrainCB->AtlasTexTransform[3] =
				AtlasTile->TexOffsetV;

			TerrainCB->AtlasWorld[0] =
				AtlasTile->WorldX;
			TerrainCB->AtlasWorld[1] =
				AtlasTile->WorldZ;
			TerrainCB->AtlasWorld[2] =
				AtlasTile->WorldDim;
			TerrainCB->AtlasWorld[3] =
				AtlasTile->WorldDim > 0.01f ?
					1.0f / AtlasTile->WorldDim :
					0.0f;
		}
	}

	bool RenderDX11_WriteTerrainCB(
		const r3dTerrain2::DX11AtlasTileInfo* AtlasTile
	)
	{
		if (!gDX11Context || !gDX11TerrainCB)
			return false;

		D3D11_MAPPED_SUBRESOURCE Mapped = {};

		HRESULT Hr =
			gDX11Context->Map(
				gDX11TerrainCB,
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
				"[RenderDX11] Map terrain constant buffer failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		WorldDX11TerrainCB* TerrainCB =
			reinterpret_cast<WorldDX11TerrainCB*>(
				Mapped.pData
			);

		RenderDX11_FillTerrainCB(
			TerrainCB,
			AtlasTile
		);

		gDX11Context->Unmap(
			gDX11TerrainCB,
			0
		);

		RenderDX11_BindTerrainCB();

		return true;
	}

	void RenderDX11_UpdateTerrainCB()
	{
		RenderDX11_UpdateTerrain2TextureRefs();

		gDX11Terrain2ActiveAtlasSRVMask = 0;

		RenderDX11_WriteTerrainCB(0);
	}

	enum
	{
		DX11_TERRAIN2_TEXTURE_COLOR = 1 << 0,
		DX11_TERRAIN2_TEXTURE_NORMAL = 1 << 1,
		DX11_TERRAIN2_TEXTURE_HEIGHT = 1 << 2,

		DX11_TERRAIN2_TEXTURE_LAYER0_DIFFUSE = 1 << 3,
		DX11_TERRAIN2_TEXTURE_LAYER0_NORMAL = 1 << 4,

		DX11_TERRAIN2_TEXTURE_MASK0 = 1 << 5,
		DX11_TERRAIN2_TEXTURE_LAYER1_DIFFUSE = 1 << 6,
		DX11_TERRAIN2_TEXTURE_LAYER2_DIFFUSE = 1 << 7,
		DX11_TERRAIN2_TEXTURE_LAYER3_DIFFUSE = 1 << 8,

		DX11_TERRAIN2_TEXTURE_ATLAS_DIFFUSE = 1 << 9,
		DX11_TERRAIN2_TEXTURE_ATLAS_NORMAL = 1 << 10
	};

	void RenderDX11_ResetTerrain2TextureBridge(
		WorldDX11Terrain2TextureBridge& Bridge
	)
	{
		RenderDX11_SafeRelease(Bridge.SRV);
		RenderDX11_SafeRelease(Bridge.Texture);

		Bridge.Source = 0;
		Bridge.Width = 0;
		Bridge.Height = 0;
		Bridge.SourceFormat = D3DFMT_UNKNOWN;
		Bridge.DXFormat = DXGI_FORMAT_UNKNOWN;
	}

	void RenderDX11_ReleaseTerrain2TextureBridges()
	{
		RenderDX11_ResetTerrain2TextureBridge(
			gDX11Terrain2ColorBridge
		);

		RenderDX11_ResetTerrain2TextureBridge(
			gDX11Terrain2NormalBridge
		);

		RenderDX11_ResetTerrain2TextureBridge(
			gDX11Terrain2HeightBridge
		);

		for (
			int i = 0;
			i < DX11_TERRAIN2_BATCH_LAYER_COUNT;
			++i
		)
		{
			RenderDX11_ResetTerrain2TextureBridge(
				gDX11Terrain2BatchDiffuseBridge[i]
			);

			RenderDX11_ResetTerrain2TextureBridge(
				gDX11Terrain2BatchNormalBridge[i]
			);
		}

		RenderDX11_ResetTerrain2TextureBridge(
			gDX11Terrain2BatchMaskBridge
		);

		for (
			int i = 0;
			i < gDX11Terrain2AtlasBridgeCount;
			++i
		)
		{
			RenderDX11_ResetTerrain2TextureBridge(
				gDX11Terrain2AtlasDiffuseBridges[i]
			);

			RenderDX11_ResetTerrain2TextureBridge(
				gDX11Terrain2AtlasNormalBridges[i]
			);
		}

		delete [] gDX11Terrain2AtlasDiffuseBridges;
		delete [] gDX11Terrain2AtlasNormalBridges;

		gDX11Terrain2AtlasDiffuseBridges = 0;
		gDX11Terrain2AtlasNormalBridges = 0;
		gDX11Terrain2AtlasBridgeCount = 0;

		gDX11Terrain2SRVMask = 0;
		gDX11Terrain2ActiveAtlasSRVMask = 0;
	}

	bool RenderDX11_TranslateTerrain2TextureFormat(
		D3DFORMAT SourceFormat,
		DXGI_FORMAT* OutDXFormat,
		bool* OutBlockCompressed,
		bool* OutBGRA8ToRGBA8,
		bool* OutXRGB8ToRGBA8,
		bool* OutR5G6B5ToRGBA8
	)
	{
		if (OutDXFormat)
			*OutDXFormat = DXGI_FORMAT_UNKNOWN;

		if (OutBlockCompressed)
			*OutBlockCompressed = false;

		if (OutBGRA8ToRGBA8)
			*OutBGRA8ToRGBA8 = false;

		if (OutXRGB8ToRGBA8)
			*OutXRGB8ToRGBA8 = false;

		if (OutR5G6B5ToRGBA8)
			*OutR5G6B5ToRGBA8 = false;

		switch (SourceFormat)
		{
		case D3DFMT_DXT1:
			if (OutDXFormat)
				*OutDXFormat = DXGI_FORMAT_BC1_UNORM;
			if (OutBlockCompressed)
				*OutBlockCompressed = true;
			return true;

		case D3DFMT_DXT3:
			if (OutDXFormat)
				*OutDXFormat = DXGI_FORMAT_BC2_UNORM;
			if (OutBlockCompressed)
				*OutBlockCompressed = true;
			return true;

		case D3DFMT_DXT5:
			if (OutDXFormat)
				*OutDXFormat = DXGI_FORMAT_BC3_UNORM;
			if (OutBlockCompressed)
				*OutBlockCompressed = true;
			return true;

		case D3DFMT_A8R8G8B8:
			if (OutDXFormat)
				*OutDXFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			if (OutBGRA8ToRGBA8)
				*OutBGRA8ToRGBA8 = true;
			return true;

		case D3DFMT_X8R8G8B8:
			if (OutDXFormat)
				*OutDXFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			if (OutXRGB8ToRGBA8)
				*OutXRGB8ToRGBA8 = true;
			return true;

		case D3DFMT_R5G6B5:
			if (OutDXFormat)
				*OutDXFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			if (OutR5G6B5ToRGBA8)
				*OutR5G6B5ToRGBA8 = true;
			return true;

		case D3DFMT_A8B8G8R8:
			if (OutDXFormat)
				*OutDXFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			return true;

		case D3DFMT_R32F:
			if (OutDXFormat)
				*OutDXFormat = DXGI_FORMAT_R32_FLOAT;
			return true;

		case D3DFMT_L16:
			if (OutDXFormat)
				*OutDXFormat = DXGI_FORMAT_R16_UNORM;
			return true;

		case D3DFMT_L8:
			if (OutDXFormat)
				*OutDXFormat = DXGI_FORMAT_R8_UNORM;
			return true;

		case D3DFMT_A16B16G16R16F:
			if (OutDXFormat)
				*OutDXFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
			return true;

		default:
			return false;
		}
	}

	UINT RenderDX11_GetTerrain2DXFormatBytesPerPixel(
		DXGI_FORMAT Format
	)
	{
		switch (Format)
		{
		case DXGI_FORMAT_R8_UNORM:
			return 1;

		case DXGI_FORMAT_R16_UNORM:
			return 2;

		case DXGI_FORMAT_R32_FLOAT:
			return 4;

		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return 4;

		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return 8;

		default:
			return 0;
		}
	}

	UINT RenderDX11_GetTerrain2BCBlockBytes(
		DXGI_FORMAT Format
	)
	{
		switch (Format)
		{
		case DXGI_FORMAT_BC1_UNORM:
			return 8;

		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC3_UNORM:
			return 16;

		default:
			return 0;
		}
	}

	bool RenderDX11_CopyTerrain2MipToUpload(
		const D3DLOCKED_RECT& Locked,
		int Width,
		int Height,
		DXGI_FORMAT DXFormat,
		bool bBlockCompressed,
		bool bBGRA8ToRGBA8,
		bool bXRGB8ToRGBA8,
		bool bR5G6B5ToRGBA8,
		unsigned char** OutData,
		UINT* OutRowPitch,
		UINT* OutRows,
		UINT* OutSlicePitch
	)
	{
		if (
			!Locked.pBits ||
			Width <= 0 ||
			Height <= 0 ||
			!OutData ||
			!OutRowPitch ||
			!OutRows ||
			!OutSlicePitch
		)
		{
			return false;
		}

		*OutData = 0;
		*OutRowPitch = 0;
		*OutRows = 0;
		*OutSlicePitch = 0;

		if (bBlockCompressed)
		{
			const UINT BlockBytes =
				RenderDX11_GetTerrain2BCBlockBytes(
					DXFormat
				);

			if (!BlockBytes)
				return false;

			const UINT BlocksX =
				static_cast<UINT>(
					(Width + 3) / 4
				);

			const UINT BlocksY =
				static_cast<UINT>(
					(Height + 3) / 4
				);

			*OutRowPitch =
				BlocksX * BlockBytes;

			*OutRows =
				BlocksY;
		}
		else
		{
			const UINT BytesPerPixel =
				RenderDX11_GetTerrain2DXFormatBytesPerPixel(
					DXFormat
				);

			if (!BytesPerPixel)
				return false;

			*OutRowPitch =
				static_cast<UINT>(Width) *
				BytesPerPixel;

			*OutRows =
				static_cast<UINT>(Height);
		}

		*OutSlicePitch =
			*OutRowPitch *
			*OutRows;

		unsigned char* UploadData =
			new unsigned char[*OutSlicePitch];

		if (!UploadData)
			return false;

		memset(
			UploadData,
			0,
			*OutSlicePitch
		);

		const unsigned char* SourceBytes =
			reinterpret_cast<const unsigned char*>(
				Locked.pBits
			);

		if (
			bBGRA8ToRGBA8 ||
			bXRGB8ToRGBA8 ||
			bR5G6B5ToRGBA8
		)
		{
			for (int y = 0; y < Height; ++y)
			{
				const unsigned char* SrcRow =
					SourceBytes +
					y * Locked.Pitch;

				unsigned char* DstRow =
					UploadData +
					y * (*OutRowPitch);

				for (int x = 0; x < Width; ++x)
				{
					if (bR5G6B5ToRGBA8)
					{
						const unsigned short Pixel =
							static_cast<unsigned short>(
								SrcRow[x * 2 + 0] |
								(SrcRow[x * 2 + 1] << 8)
							);

						const unsigned char R5 =
							static_cast<unsigned char>(
								(Pixel >> 11) & 0x1F
							);

						const unsigned char G6 =
							static_cast<unsigned char>(
								(Pixel >> 5) & 0x3F
							);

						const unsigned char B5 =
							static_cast<unsigned char>(
								Pixel & 0x1F
							);

						DstRow[x * 4 + 0] =
							static_cast<unsigned char>(
								(R5 << 3) | (R5 >> 2)
							);

						DstRow[x * 4 + 1] =
							static_cast<unsigned char>(
								(G6 << 2) | (G6 >> 4)
							);

						DstRow[x * 4 + 2] =
							static_cast<unsigned char>(
								(B5 << 3) | (B5 >> 2)
							);

						DstRow[x * 4 + 3] = 255;
					}
					else
					{
						const unsigned char B =
							SrcRow[x * 4 + 0];

						const unsigned char G =
							SrcRow[x * 4 + 1];

						const unsigned char R =
							SrcRow[x * 4 + 2];

						const unsigned char A =
							bXRGB8ToRGBA8
							? 255
							: SrcRow[x * 4 + 3];

						DstRow[x * 4 + 0] = R;
						DstRow[x * 4 + 1] = G;
						DstRow[x * 4 + 2] = B;
						DstRow[x * 4 + 3] = A;
					}
				}
			}
		}
		else
		{
			for (UINT y = 0; y < *OutRows; ++y)
			{
				const unsigned char* SrcRow =
					SourceBytes +
					y * Locked.Pitch;

				unsigned char* DstRow =
					UploadData +
					y * (*OutRowPitch);

				const UINT CopyBytes =
					*OutRowPitch <
					static_cast<UINT>(Locked.Pitch)
					? *OutRowPitch
					: static_cast<UINT>(Locked.Pitch);

				memcpy(
					DstRow,
					SrcRow,
					CopyBytes
				);
			}
		}

		*OutData = UploadData;
		return true;
	}

	bool RenderDX11_UploadTerrain2TextureToDX11(
		WorldDX11Terrain2TextureBridge& Bridge,
		r3dTexture* Source,
		const char* DebugName
	)
	{
		if (!DebugName)
			DebugName = "unknown";

		if (
			!Source ||
			!Source->IsLoaded() ||
			Source->IsMissing() ||
			!Source->IsValid()
		)
		{
			RenderDX11_ResetTerrain2TextureBridge(
				Bridge
			);

			return false;
		}

		const int Width =
			Source->GetWidth();

		const int Height =
			Source->GetHeight();

		const D3DFORMAT SourceFormat =
			Source->GetD3DFormat();

		DXGI_FORMAT DXFormat =
			DXGI_FORMAT_UNKNOWN;

		bool bBlockCompressed = false;
		bool bBGRA8ToRGBA8 = false;
		bool bXRGB8ToRGBA8 = false;
		bool bR5G6B5ToRGBA8 = false;

		if (!RenderDX11_TranslateTerrain2TextureFormat(
			SourceFormat,
			&DXFormat,
			&bBlockCompressed,
			&bBGRA8ToRGBA8,
			&bXRGB8ToRGBA8,
			&bR5G6B5ToRGBA8
		))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Terrain2 %s texture unsupported D3D9 format=0x%08X\n",
				DebugName,
				static_cast<unsigned int>(SourceFormat)
			);

			OutputDebugStringA(Text);

			RenderDX11_ResetTerrain2TextureBridge(
				Bridge
			);

			return false;
		}

		if (
			Bridge.Source == Source &&
			Bridge.Texture &&
			Bridge.SRV &&
			Bridge.Width == Width &&
			Bridge.Height == Height &&
			Bridge.SourceFormat == SourceFormat &&
			Bridge.DXFormat == DXFormat
		)
		{
			return true;
		}

		RenderDX11_ResetTerrain2TextureBridge(
			Bridge
		);

		IDirect3DTexture9* D3D9Texture =
			Source->AsTex2D();

		if (!D3D9Texture)
			return false;

		UINT MipLevels =
			D3D9Texture->GetLevelCount();

		if (!MipLevels)
			MipLevels = 1;

		D3D11_SUBRESOURCE_DATA* InitData =
			new D3D11_SUBRESOURCE_DATA[MipLevels];

		unsigned char** UploadDatas =
			new unsigned char*[MipLevels];

		if (!InitData || !UploadDatas)
		{
			delete[] InitData;
			delete[] UploadDatas;
			return false;
		}

		memset(
			InitData,
			0,
			sizeof(D3D11_SUBRESOURCE_DATA) * MipLevels
		);

		memset(
			UploadDatas,
			0,
			sizeof(unsigned char*) * MipLevels
		);

		HRESULT Hr = S_OK;

		for (UINT Mip = 0; Mip < MipLevels; ++Mip)
		{
			D3DSURFACE_DESC LevelDesc = {};
			Hr =
				D3D9Texture->GetLevelDesc(
					Mip,
					&LevelDesc
				);

			if (FAILED(Hr))
				break;

			D3DLOCKED_RECT Locked = {};
			Hr =
				D3D9Texture->LockRect(
					Mip,
					&Locked,
					0,
					D3DLOCK_READONLY
				);

			if (FAILED(Hr))
				break;

			UINT UploadRowPitch = 0;
			UINT UploadRows = 0;
			UINT UploadSlicePitch = 0;

			const bool bCopied =
				RenderDX11_CopyTerrain2MipToUpload(
					Locked,
					static_cast<int>(LevelDesc.Width),
					static_cast<int>(LevelDesc.Height),
					DXFormat,
					bBlockCompressed,
					bBGRA8ToRGBA8,
					bXRGB8ToRGBA8,
					bR5G6B5ToRGBA8,
					&UploadDatas[Mip],
					&UploadRowPitch,
					&UploadRows,
					&UploadSlicePitch
				);

			D3D9Texture->UnlockRect(
				Mip
			);

			if (!bCopied)
			{
				Hr = E_FAIL;
				break;
			}

			InitData[Mip].pSysMem =
				UploadDatas[Mip];
			InitData[Mip].SysMemPitch =
				UploadRowPitch;
			InitData[Mip].SysMemSlicePitch =
				UploadSlicePitch;
		}

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Terrain2 %s mip upload failed. HRESULT=0x%08X\n",
				DebugName,
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);

			for (UINT Mip = 0; Mip < MipLevels; ++Mip)
				delete[] UploadDatas[Mip];

			delete[] UploadDatas;
			delete[] InitData;

			RenderDX11_ResetTerrain2TextureBridge(
				Bridge
			);

			return false;
		}

		D3D11_TEXTURE2D_DESC TextureDesc = {};
		TextureDesc.Width =
			static_cast<UINT>(Width);
		TextureDesc.Height =
			static_cast<UINT>(Height);
		TextureDesc.MipLevels = MipLevels;
		TextureDesc.ArraySize = 1;
		TextureDesc.Format = DXFormat;
		TextureDesc.SampleDesc.Count = 1;
		TextureDesc.SampleDesc.Quality = 0;
		TextureDesc.Usage = D3D11_USAGE_DEFAULT;
		TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		TextureDesc.CPUAccessFlags = 0;
		TextureDesc.MiscFlags = 0;

		Hr =
			gDX11Device->CreateTexture2D(
				&TextureDesc,
				InitData,
				&Bridge.Texture
			);

		for (UINT Mip = 0; Mip < MipLevels; ++Mip)
			delete[] UploadDatas[Mip];

		delete[] UploadDatas;
		delete[] InitData;

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Terrain2 %s CreateTexture2D failed. HRESULT=0x%08X\n",
				DebugName,
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);

			RenderDX11_ResetTerrain2TextureBridge(
				Bridge
			);

			return false;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = DXFormat;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.MipLevels = MipLevels;

		Hr =
			gDX11Device->CreateShaderResourceView(
				Bridge.Texture,
				&SRVDesc,
				&Bridge.SRV
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Terrain2 %s CreateShaderResourceView failed. HRESULT=0x%08X\n",
				DebugName,
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);

			RenderDX11_ResetTerrain2TextureBridge(
				Bridge
			);

			return false;
		}

		Bridge.Source = Source;
		Bridge.Width = Width;
		Bridge.Height = Height;
		Bridge.SourceFormat = SourceFormat;
		Bridge.DXFormat = DXFormat;

		char Text[512] = {};
		sprintf_s(
			Text,
			"[RenderDX11] Terrain2 %s uploaded to DX11 SRV: %dx%d mips=%u d3d9fmt=0x%08X dxgifmt=%d\n",
			DebugName,
			Width,
			Height,
			static_cast<unsigned int>(MipLevels),
			static_cast<unsigned int>(SourceFormat),
			static_cast<int>(DXFormat)
		);

		OutputDebugStringA(Text);

		return true;
	}

	void RenderDX11_LogTerrain2TextureInfo(
		const char* Name,
		r3dTexture* Texture
	)
	{
		if (!Name)
			Name = "unknown";

		if (!Texture)
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Terrain2 %s texture: null\n",
				Name
			);

			OutputDebugStringA(Text);
			return;
		}

		char Text[512] = {};
		sprintf_s(
			Text,
			"[RenderDX11] Terrain2 %s texture: %dx%d fmt=0x%08X loaded=%d missing=%d\n",
			Name,
			Texture->GetWidth(),
			Texture->GetHeight(),
			static_cast<unsigned int>(Texture->GetD3DFormat()),
			Texture->IsLoaded(),
			Texture->IsMissing() ? 1 : 0
		);

		OutputDebugStringA(Text);
	}

	void RenderDX11_SetTerrain2BatchLayerSlot(
		int SlotIndex,
		int SourceLayerIndex
	)
	{
		if (
			!Terrain2 ||
			SlotIndex < 0 ||
			SlotIndex >= DX11_TERRAIN2_BATCH_LAYER_COUNT ||
			SourceLayerIndex < 0 ||
			SourceLayerIndex >= Terrain2->GetNumLayers()
		)
		{
			return;
		}

		const r3dTerrainLayer& Layer =
			Terrain2->GetLayer(
				SourceLayerIndex
			);

		WorldDX11Terrain2LayerSlot& Slot =
			gDX11Terrain2BatchLayers[SlotIndex];

		Slot.DiffuseTexture =
			Layer.DiffuseTex;

		Slot.NormalTexture =
			Layer.NormalTex;

		if (fabsf(Layer.ShaderScaleU) > 0.00001f)
			Slot.ScaleU =
				Layer.ShaderScaleU;

		if (fabsf(Layer.ShaderScaleV) > 0.00001f)
			Slot.ScaleV =
				Layer.ShaderScaleV;

		Slot.SourceLayerIndex =
			SourceLayerIndex;
	}

	void RenderDX11_UpdateTerrain2TextureRefs()
	{
		gDX11Terrain2ColorTexture = 0;
		gDX11Terrain2NormalTexture = 0;
		gDX11Terrain2HeightTexture = 0;

		for (
			int i = 0;
			i < DX11_TERRAIN2_BATCH_LAYER_COUNT;
			++i
		)
		{
			RenderDX11_ResetTerrain2LayerSlot(
				gDX11Terrain2BatchLayers[i]
			);
		}

		gDX11Terrain2BatchMaskTexture = 0;
		gDX11Terrain2LayerCount = 0;
		gDX11Terrain2MaskCount = 0;
		gDX11Terrain2ActiveMaskIndex = 0;

		gDX11Terrain2TextureMask = 0;
		gDX11Terrain2SRVMask = 0;

		if (!Terrain2)
			return;

		gDX11Terrain2LayerCount =
			Terrain2->GetNumLayers();

		gDX11Terrain2MaskCount =
			Terrain2->GetNumMasks();

		gDX11Terrain2ActiveMaskIndex = 0;

		gDX11Terrain2ColorTexture =
			Terrain2->GetColorTexture();

		gDX11Terrain2NormalTexture =
			Terrain2->GetNormalTexture();

		gDX11Terrain2HeightTexture =
			Terrain2->GetHeightTexture();

		RenderDX11_SetTerrain2BatchLayerSlot(
			0,
			0
		);

		if (gDX11Terrain2MaskCount > 0)
		{
			gDX11Terrain2BatchMaskTexture =
				Terrain2->GetLayerMask(
					gDX11Terrain2ActiveMaskIndex
				);
		}

		for (
			int SlotIndex = 1;
			SlotIndex < DX11_TERRAIN2_BATCH_LAYER_COUNT;
			++SlotIndex
		)
		{
			const int SourceLayerIndex =
				gDX11Terrain2ActiveMaskIndex *
				DX11_TERRAIN2_LAYERS_PER_MASK +
				SlotIndex;

			RenderDX11_SetTerrain2BatchLayerSlot(
				SlotIndex,
				SourceLayerIndex
			);
		}

		if (RenderDX11_IsTerrain2TextureReady(gDX11Terrain2ColorTexture))
		{
			gDX11Terrain2TextureMask |=
				DX11_TERRAIN2_TEXTURE_COLOR;
		}

		if (RenderDX11_IsTerrain2TextureReady(gDX11Terrain2NormalTexture))
		{
			gDX11Terrain2TextureMask |=
				DX11_TERRAIN2_TEXTURE_NORMAL;
		}

		if (RenderDX11_IsTerrain2TextureReady(gDX11Terrain2HeightTexture))
		{
			gDX11Terrain2TextureMask |=
				DX11_TERRAIN2_TEXTURE_HEIGHT;
		}

		if (RenderDX11_IsTerrain2TextureReady(
			gDX11Terrain2BatchLayers[0].DiffuseTexture
		))
		{
			gDX11Terrain2TextureMask |=
				DX11_TERRAIN2_TEXTURE_LAYER0_DIFFUSE;
		}

		if (RenderDX11_IsTerrain2TextureReady(
			gDX11Terrain2BatchLayers[0].NormalTexture
		))
		{
			gDX11Terrain2TextureMask |=
				DX11_TERRAIN2_TEXTURE_LAYER0_NORMAL;
		}

		if (RenderDX11_IsTerrain2TextureReady(
			gDX11Terrain2BatchMaskTexture
		))
		{
			gDX11Terrain2TextureMask |=
				DX11_TERRAIN2_TEXTURE_MASK0;
		}

		if (RenderDX11_IsTerrain2TextureReady(
			gDX11Terrain2BatchLayers[1].DiffuseTexture
		))
		{
			gDX11Terrain2TextureMask |=
				DX11_TERRAIN2_TEXTURE_LAYER1_DIFFUSE;
		}

		if (RenderDX11_IsTerrain2TextureReady(
			gDX11Terrain2BatchLayers[2].DiffuseTexture
		))
		{
			gDX11Terrain2TextureMask |=
				DX11_TERRAIN2_TEXTURE_LAYER2_DIFFUSE;
		}

		if (RenderDX11_IsTerrain2TextureReady(
			gDX11Terrain2BatchLayers[3].DiffuseTexture
		))
		{
			gDX11Terrain2TextureMask |=
				DX11_TERRAIN2_TEXTURE_LAYER3_DIFFUSE;
		}

		if (!gDX11Terrain2TextureInfoLogged)
		{
			gDX11Terrain2TextureInfoLogged = true;

			OutputDebugStringA(
				"[RenderDX11] Terrain2 public texture refs acquired\n"
			);

			char BatchText[256] = {};
			sprintf_s(
				BatchText,
				"[RenderDX11] Terrain2 layers=%d masks=%d activeMask=%d batchLayers=[%d,%d,%d,%d]\n",
				gDX11Terrain2LayerCount,
				gDX11Terrain2MaskCount,
				gDX11Terrain2ActiveMaskIndex,
				gDX11Terrain2BatchLayers[0].SourceLayerIndex,
				gDX11Terrain2BatchLayers[1].SourceLayerIndex,
				gDX11Terrain2BatchLayers[2].SourceLayerIndex,
				gDX11Terrain2BatchLayers[3].SourceLayerIndex
			);

			OutputDebugStringA(BatchText);

			RenderDX11_LogTerrain2TextureInfo(
				"color",
				gDX11Terrain2ColorTexture
			);

			RenderDX11_LogTerrain2TextureInfo(
				"normal",
				gDX11Terrain2NormalTexture
			);

			RenderDX11_LogTerrain2TextureInfo(
				"height",
				gDX11Terrain2HeightTexture
			);

			RenderDX11_LogTerrain2TextureInfo(
				"batch layer0 diffuse",
				gDX11Terrain2BatchLayers[0].DiffuseTexture
			);

			RenderDX11_LogTerrain2TextureInfo(
				"batch layer0 normal",
				gDX11Terrain2BatchLayers[0].NormalTexture
			);

			RenderDX11_LogTerrain2TextureInfo(
				"batch mask",
				gDX11Terrain2BatchMaskTexture
			);

			RenderDX11_LogTerrain2TextureInfo(
				"batch layer1 diffuse",
				gDX11Terrain2BatchLayers[1].DiffuseTexture
			);

			RenderDX11_LogTerrain2TextureInfo(
				"batch layer2 diffuse",
				gDX11Terrain2BatchLayers[2].DiffuseTexture
			);

			RenderDX11_LogTerrain2TextureInfo(
				"batch layer3 diffuse",
				gDX11Terrain2BatchLayers[3].DiffuseTexture
			);
		}

		if (
			RenderDX11_UploadTerrain2TextureToDX11(
				gDX11Terrain2ColorBridge,
				gDX11Terrain2ColorTexture,
				"color"
			)
		)
		{
			gDX11Terrain2SRVMask |=
				DX11_TERRAIN2_TEXTURE_COLOR;
		}

		if (
			RenderDX11_UploadTerrain2TextureToDX11(
				gDX11Terrain2NormalBridge,
				gDX11Terrain2NormalTexture,
				"normal"
			)
		)
		{
			gDX11Terrain2SRVMask |=
				DX11_TERRAIN2_TEXTURE_NORMAL;
		}

		if (
			RenderDX11_UploadTerrain2TextureToDX11(
				gDX11Terrain2HeightBridge,
				gDX11Terrain2HeightTexture,
				"height"
			)
		)
		{
			gDX11Terrain2SRVMask |=
				DX11_TERRAIN2_TEXTURE_HEIGHT;
		}

		if (
			RenderDX11_UploadTerrain2TextureToDX11(
				gDX11Terrain2BatchDiffuseBridge[0],
				gDX11Terrain2BatchLayers[0].DiffuseTexture,
				"batch layer0 diffuse"
			)
		)
		{
			gDX11Terrain2SRVMask |=
				DX11_TERRAIN2_TEXTURE_LAYER0_DIFFUSE;
		}

		if (
			RenderDX11_UploadTerrain2TextureToDX11(
				gDX11Terrain2BatchNormalBridge[0],
				gDX11Terrain2BatchLayers[0].NormalTexture,
				"batch layer0 normal"
			)
		)
		{
			gDX11Terrain2SRVMask |=
				DX11_TERRAIN2_TEXTURE_LAYER0_NORMAL;
		}

		if (
			RenderDX11_UploadTerrain2TextureToDX11(
				gDX11Terrain2BatchMaskBridge,
				gDX11Terrain2BatchMaskTexture,
				"batch mask"
			)
		)
		{
			gDX11Terrain2SRVMask |=
				DX11_TERRAIN2_TEXTURE_MASK0;
		}

		if (
			RenderDX11_UploadTerrain2TextureToDX11(
				gDX11Terrain2BatchDiffuseBridge[1],
				gDX11Terrain2BatchLayers[1].DiffuseTexture,
				"batch layer1 diffuse"
			)
		)
		{
			gDX11Terrain2SRVMask |=
				DX11_TERRAIN2_TEXTURE_LAYER1_DIFFUSE;
		}

		if (
			RenderDX11_UploadTerrain2TextureToDX11(
				gDX11Terrain2BatchDiffuseBridge[2],
				gDX11Terrain2BatchLayers[2].DiffuseTexture,
				"batch layer2 diffuse"
			)
		)
		{
			gDX11Terrain2SRVMask |=
				DX11_TERRAIN2_TEXTURE_LAYER2_DIFFUSE;
		}

		if (
			RenderDX11_UploadTerrain2TextureToDX11(
				gDX11Terrain2BatchDiffuseBridge[3],
				gDX11Terrain2BatchLayers[3].DiffuseTexture,
				"batch layer3 diffuse"
			)
		)
		{
			gDX11Terrain2SRVMask |=
				DX11_TERRAIN2_TEXTURE_LAYER3_DIFFUSE;
		}
	}

	void RenderDX11_BindTerrain2TextureSlots()
	{
		if (!gDX11Context)
			return;

		ID3D11ShaderResourceView* SRVs[9] =
		{
			gDX11Terrain2ColorBridge.SRV,          // t0
			gDX11Terrain2NormalBridge.SRV,         // t1
			gDX11Terrain2HeightBridge.SRV,         // t2
			gDX11Terrain2BatchDiffuseBridge[0].SRV, // t3
			gDX11Terrain2BatchNormalBridge[0].SRV,  // t4
			gDX11Terrain2BatchMaskBridge.SRV,       // t5
			gDX11Terrain2BatchDiffuseBridge[1].SRV, // t6
			gDX11Terrain2BatchDiffuseBridge[2].SRV, // t7
			gDX11Terrain2BatchDiffuseBridge[3].SRV  // t8
		};

		gDX11Context->PSSetShaderResources(
			0,
			9,
			SRVs
		);

		ID3D11SamplerState* Sampler =
			gDX11SamplerLinearWrap
			? gDX11SamplerLinearWrap
			: gDX11SamplerLinearClamp;

		if (Sampler)
		{
			gDX11Context->PSSetSamplers(
				0,
				1,
				&Sampler
			);
		}
	}

	bool RenderDX11_EnsureTerrain2AtlasBridgeCount(
		int VolumeCount
	)
	{
		if (VolumeCount < 0)
			VolumeCount = 0;

		if (VolumeCount == gDX11Terrain2AtlasBridgeCount)
			return true;

		for (
			int i = 0;
			i < gDX11Terrain2AtlasBridgeCount;
			++i
		)
		{
			RenderDX11_ResetTerrain2TextureBridge(
				gDX11Terrain2AtlasDiffuseBridges[i]
			);

			RenderDX11_ResetTerrain2TextureBridge(
				gDX11Terrain2AtlasNormalBridges[i]
			);
		}

		delete [] gDX11Terrain2AtlasDiffuseBridges;
		delete [] gDX11Terrain2AtlasNormalBridges;

		gDX11Terrain2AtlasDiffuseBridges = 0;
		gDX11Terrain2AtlasNormalBridges = 0;
		gDX11Terrain2AtlasBridgeCount = 0;

		if (!VolumeCount)
			return true;

		gDX11Terrain2AtlasDiffuseBridges =
			new WorldDX11Terrain2TextureBridge[VolumeCount]();

		gDX11Terrain2AtlasNormalBridges =
			new WorldDX11Terrain2TextureBridge[VolumeCount]();

		if (
			!gDX11Terrain2AtlasDiffuseBridges ||
			!gDX11Terrain2AtlasNormalBridges
		)
		{
			delete [] gDX11Terrain2AtlasDiffuseBridges;
			delete [] gDX11Terrain2AtlasNormalBridges;

			gDX11Terrain2AtlasDiffuseBridges = 0;
			gDX11Terrain2AtlasNormalBridges = 0;
			gDX11Terrain2AtlasBridgeCount = 0;

			return false;
		}

		gDX11Terrain2AtlasBridgeCount =
			VolumeCount;

		return true;
	}

	int RenderDX11_EnsureTerrain2AtlasVolumeSRVs(
		int VolumeID
	)
	{
		if (!Terrain2 || VolumeID < 0)
			return 0;

		const int VolumeCount =
			Terrain2->GetDX11AtlasVolumeCount();

		if (
			VolumeID >= VolumeCount ||
			!RenderDX11_EnsureTerrain2AtlasBridgeCount(
				VolumeCount
			)
		)
		{
			return 0;
		}

		int AtlasMask = 0;

		if (
			RenderDX11_UploadTerrain2TextureToDX11(
				gDX11Terrain2AtlasDiffuseBridges[VolumeID],
				Terrain2->GetDX11AtlasDiffuseTexture(VolumeID),
				"atlas diffuse"
			)
		)
		{
			AtlasMask |=
				DX11_TERRAIN2_TEXTURE_ATLAS_DIFFUSE;
		}

		if (
			RenderDX11_UploadTerrain2TextureToDX11(
				gDX11Terrain2AtlasNormalBridges[VolumeID],
				Terrain2->GetDX11AtlasNormalTexture(VolumeID),
				"atlas normal"
			)
		)
		{
			AtlasMask |=
				DX11_TERRAIN2_TEXTURE_ATLAS_NORMAL;
		}

		return AtlasMask;
	}

	void RenderDX11_BindTerrain2AtlasTextureSlots(
		int VolumeID
	)
	{
		if (!gDX11Context)
			return;

		ID3D11ShaderResourceView* SRVs[2] = {};

		if (
			VolumeID >= 0 &&
			VolumeID < gDX11Terrain2AtlasBridgeCount
		)
		{
			SRVs[0] =
				gDX11Terrain2AtlasDiffuseBridges[VolumeID].SRV;
			SRVs[1] =
				gDX11Terrain2AtlasNormalBridges[VolumeID].SRV;
		}

		gDX11Context->PSSetShaderResources(
			9,
			2,
			SRVs
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

	int RenderDX11_ClampInt(
		int Value,
		int MinValue,
		int MaxValue
	)
	{
		if (Value < MinValue)
			return MinValue;

		if (Value > MaxValue)
			return MaxValue;

		return Value;
	}

	////////////////////////////////////////////////////

#include "RenderDX11_Terrain.hpp"

	//////////////////////////////////////

	bool RenderDX11_DrawTerrainGBuffer()
	{
		if (
			!gDX11Context ||
			!gDX11TerrainVS ||
			!gDX11TerrainPS ||
			!RenderDX11_CreateTerrainResources()
		)
		{
			return false;
		}

		RenderDX11_UpdateTerrainCB();

		gDX11Context->VSSetShader(
			gDX11TerrainVS,
			0,
			0
		);

		gDX11Context->RSSetState(
			gDX11RasterSolidNoCull
		);

		gDX11Context->PSSetShader(
			gDX11TerrainPS,
			0,
			0
		);

		RenderDX11_BindTerrain2TextureSlots();

		if (RenderDX11_DrawTerrain2AtlasTiles())
			return true;

		gDX11Terrain2ActiveAtlasSRVMask = 0;

		RenderDX11_WriteTerrainCB(0);

		return RenderDX11_DrawTerrainPatchSet();
	}

	bool RenderDX11_UpdatePreviewTexture()
	{
		if (!RenderDX11_EnsurePreviewTexture())
			return false;

		IDirect3DTexture9* PreviewD3DTexture =
			gDX11PreviewTexture->AsTex2D();

		if (!PreviewD3DTexture)
			return false;

		D3DLOCKED_RECT LockedRect = {};

		HRESULT Hr =
			PreviewD3DTexture->LockRect(
				0,
				&LockedRect,
				0,
				0
			);

		if (FAILED(Hr))
		{
			static bool bLockFailedLogged = false;

			if (!bLockFailedLogged)
			{
				char Text[256] = {};
				sprintf_s(
					Text,
					"[RenderDX11] Preview texture LockRect failed. HRESULT=0x%08X\n",
					static_cast<unsigned int>(Hr)
				);

				OutputDebugStringA(Text);
				bLockFailedLogged = true;
			}

			return false;
		}

		for (int y = 0; y < DX11_PREVIEW_HEIGHT; ++y)
		{
			unsigned char* DestRow =
				reinterpret_cast<unsigned char*>(
					LockedRect.pBits
				) + LockedRect.Pitch * y;

			const unsigned int* SrcRow =
				gDX11PreviewPixels +
				y * DX11_PREVIEW_WIDTH;

			memcpy(
				DestRow,
				SrcRow,
				sizeof(unsigned int) * DX11_PREVIEW_WIDTH
			);
		}

		PreviewD3DTexture->UnlockRect(0);

		return true;
	}

	bool RenderDX11_EnsurePreviewReadbackTexture(
		DXGI_FORMAT Format
	)
	{
		if (
			gDX11PreviewReadbackTexture &&
			gDX11PreviewReadbackFormat == Format &&
			gDX11PreviewReadbackWidth == gDX11FrameWidth &&
			gDX11PreviewReadbackHeight == gDX11FrameHeight
		)
		{
			return true;
		}

		RenderDX11_SafeRelease(gDX11PreviewReadbackTexture);

		gDX11PreviewReadbackFormat = DXGI_FORMAT_UNKNOWN;
		gDX11PreviewReadbackWidth = 0;
		gDX11PreviewReadbackHeight = 0;

		if (
			!gDX11Device ||
			gDX11FrameWidth <= 0 ||
			gDX11FrameHeight <= 0 ||
			Format == DXGI_FORMAT_UNKNOWN
		)
		{
			return false;
		}

		D3D11_TEXTURE2D_DESC Desc = {};
		Desc.Width = static_cast<UINT>(gDX11FrameWidth);
		Desc.Height = static_cast<UINT>(gDX11FrameHeight);
		Desc.MipLevels = 1;
		Desc.ArraySize = 1;
		Desc.Format = Format;
		Desc.SampleDesc.Count = 1;
		Desc.SampleDesc.Quality = 0;
		Desc.Usage = D3D11_USAGE_STAGING;
		Desc.BindFlags = 0;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		Desc.MiscFlags = 0;

		HRESULT Hr =
			gDX11Device->CreateTexture2D(
				&Desc,
				0,
				&gDX11PreviewReadbackTexture
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create preview readback texture failed. Format=%d HRESULT=0x%08X\n",
				static_cast<int>(Format),
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		gDX11PreviewReadbackFormat = Format;
		gDX11PreviewReadbackWidth = gDX11FrameWidth;
		gDX11PreviewReadbackHeight = gDX11FrameHeight;

		return true;
	}

	void RenderDX11_UpdatePreviewFromMapped(
		const D3D11_MAPPED_SUBRESOURCE& Mapped,
		RenderDX11PreviewMode Mode
	)
	{
		if (
			!Mapped.pData ||
			gDX11FrameWidth <= 0 ||
			gDX11FrameHeight <= 0
		)
		{
			gDX11PreviewValid = false;
			return;
		}

		for (int y = 0; y < DX11_PREVIEW_HEIGHT; ++y)
		{
			const int SourceY =
				(y * gDX11FrameHeight) /
				DX11_PREVIEW_HEIGHT;

			const unsigned char* Row =
				reinterpret_cast<const unsigned char*>(
					Mapped.pData
				) + Mapped.RowPitch * SourceY;

			for (int x = 0; x < DX11_PREVIEW_WIDTH; ++x)
			{
				const int SourceX =
					(x * gDX11FrameWidth) /
					DX11_PREVIEW_WIDTH;

				unsigned int PackedColor = 0xff000000u;

				switch (Mode)
				{
				case DX11_PREVIEW_NORMAL:
				{
					const unsigned short* Pixel =
						reinterpret_cast<const unsigned short*>(
							Row + SourceX * 8
						);

					const float R =
						RenderDX11_SaturateFloat(
							RenderDX11_HalfToFloat(Pixel[0])
						);

					const float G =
						RenderDX11_SaturateFloat(
							RenderDX11_HalfToFloat(Pixel[1])
						);

					const float B =
						RenderDX11_SaturateFloat(
							RenderDX11_HalfToFloat(Pixel[2])
						);

					PackedColor =
						RenderDX11_PackPreviewColor(
							static_cast<unsigned int>(R * 255.0f + 0.5f),
							static_cast<unsigned int>(G * 255.0f + 0.5f),
							static_cast<unsigned int>(B * 255.0f + 0.5f)
						);

					break;
				}

				case DX11_PREVIEW_LINEAR_DEPTH:
				{
					const float* Pixel =
						reinterpret_cast<const float*>(
							Row + SourceX * 4
						);

					PackedColor =
						RenderDX11_PackPreviewGray(
							Pixel[0]
						);

					break;
				}

				case DX11_PREVIEW_DEPTH:
				{
					const unsigned int RawDepthStencil =
						*reinterpret_cast<const unsigned int*>(
							Row + SourceX * 4
						);

					const unsigned int Depth24 =
						RawDepthStencil & 0x00ffffffu;

					const float Depth =
						static_cast<float>(Depth24) /
						16777215.0f;

					PackedColor =
						RenderDX11_PackPreviewGray(
							Depth
						);

					break;
				}

				case DX11_PREVIEW_AUX:
				{
					const unsigned char* Pixel =
						Row + SourceX * 4;

					PackedColor =
						RenderDX11_PackPreviewColor(
							static_cast<unsigned int>(Pixel[0]),
							static_cast<unsigned int>(Pixel[1]),
							static_cast<unsigned int>(Pixel[2])
						);

					break;
				}

				case DX11_PREVIEW_TERRAIN_MASK:
				{
					const unsigned char* Pixel =
						Row + SourceX * 4;

					PackedColor =
						RenderDX11_PackPreviewGray(
							static_cast<float>(Pixel[3]) /
							255.0f
						);

					break;
				}

				case DX11_PREVIEW_COLOR:
				default:
				{
					const unsigned char* Pixel =
						Row + SourceX * 4;

					PackedColor =
						RenderDX11_PackPreviewColor(
							static_cast<unsigned int>(Pixel[0]),
							static_cast<unsigned int>(Pixel[1]),
							static_cast<unsigned int>(Pixel[2])
						);

					break;
				}
				}

				gDX11PreviewPixels[
					y * DX11_PREVIEW_WIDTH + x
				] = PackedColor;
			}
		}

		gDX11PreviewValid =
			RenderDX11_UpdatePreviewTexture();
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
		const bool bWantPreview =
			RenderDX11_WantsDebugPreview();

		if (
			gDX11TerrainGBufferReadbackLogged &&
			!bWantPreview
		)
		{
			return;
		}

		if (
			!gDX11Context ||
			gDX11FrameWidth <= 0 ||
			gDX11FrameHeight <= 0
		)
		{
			return;
		}

		const RenderDX11PreviewMode PreviewMode =
			RenderDX11_GetPreviewMode();

		DXGI_FORMAT PreviewFormat =
			DXGI_FORMAT_UNKNOWN;

		ID3D11Texture2D* PreviewSource =
			RenderDX11_GetPreviewSourceTexture(
				PreviewMode,
				&PreviewFormat
			);

		if (!PreviewSource)
		{
			gDX11PreviewValid = false;
			return;
		}

		if (!RenderDX11_EnsurePreviewReadbackTexture(PreviewFormat))
		{
			gDX11PreviewValid = false;
			return;
		}

		gDX11Context->CopyResource(
			gDX11PreviewReadbackTexture,
			PreviewSource
		);

		D3D11_MAPPED_SUBRESOURCE Mapped = {};

		HRESULT Hr =
			gDX11Context->Map(
				gDX11PreviewReadbackTexture,
				0,
				D3D11_MAP_READ,
				0,
				&Mapped
			);

		if (FAILED(Hr))
		{
			if (!gDX11TerrainGBufferReadbackLogged)
			{
				char Text[256] = {};
				sprintf_s(
					Text,
					"[RenderDX11] Preview readback Map failed. Mode=%s HRESULT=0x%08X\n",
					RenderDX11_GetPreviewModeName(PreviewMode),
					static_cast<unsigned int>(Hr)
				);

				OutputDebugStringA(Text);
			}

			gDX11PreviewValid = false;
			gDX11TerrainGBufferReadbackLogged = true;
			return;
		}

		if (bWantPreview)
		{
			RenderDX11_UpdatePreviewFromMapped(
				Mapped,
				PreviewMode
			);
		}

		if (gDX11TerrainGBufferReadbackLogged)
		{
			gDX11Context->Unmap(
				gDX11PreviewReadbackTexture,
				0
			);

			return;
		}

		char Text[512] = {};
		sprintf_s(
			Text,
			"[RenderDX11] Preview readback ready: %dx%d Mode=%s Format=%d\n",
			gDX11FrameWidth,
			gDX11FrameHeight,
			RenderDX11_GetPreviewModeName(PreviewMode),
			static_cast<int>(PreviewFormat)
		);

		OutputDebugStringA(Text);

		gDX11Context->Unmap(
			gDX11PreviewReadbackTexture,
			0
		);

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
		Desc.Filter = D3D11_FILTER_ANISOTROPIC;
		Desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		Desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		Desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		Desc.MipLODBias = 0.0f;
		Desc.MaxAnisotropy = 8;
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
		RenderDX11_SafeRelease(gDX11PreviewReadbackTexture);

		gDX11PreviewReadbackFormat = DXGI_FORMAT_UNKNOWN;
		gDX11PreviewReadbackWidth = 0;
		gDX11PreviewReadbackHeight = 0;

		RenderDX11_SafeRelease(gDX11GBufferAuxTexture);
		RenderDX11_SafeRelease(gDX11GBufferDepthLinearTexture);
		RenderDX11_SafeRelease(gDX11GBufferNormalTexture);
		RenderDX11_SafeRelease(gDX11GBufferColorTexture);
		RenderDX11_SafeRelease(gDX11DepthTexture);

		gDX11FrameWidth = 0;
		gDX11FrameHeight = 0;
		gDX11SmokeReadbackLogged = false;
		gDX11TerrainGBufferReadbackLogged = false;
		gDX11PreviewValid = false;

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

	void RenderDX11_LogOffscreenOnlyModeOnce()
	{
		if (gDX11OffscreenOnlyLogged)
			return;

		gDX11OffscreenOnlyLogged = true;

		OutputDebugStringA(
			"[RenderDX11] Offscreen-only mode confirmed. "
			"DX11 has no swapchain and does not Present. "
			"DX9 owns window/backbuffer/UI/Present.\n"
		);
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

void RenderDX11_DrawDebugPreviewDX9()
{
	if (
		!RenderDX11_WantsDebugPreview() ||
		!gDX11PreviewValid ||
		!gDX11PreviewTexture ||
		!r3dRenderer
	)
	{
		return;
	}

	const float PreviewW =
		r3dRenderer->ScreenW * 0.5f;

	const float PreviewH =
		r3dRenderer->ScreenH;

	const float X =
		r3dRenderer->ScreenW * 0.5f;

	const float Y = 0.0f;

	r3dDrawBox2D(
		X,
		Y,
		PreviewW,
		PreviewH,
		r3dColor(0, 0, 0, 180)
	);

	r3dDrawBox2D(
		X,
		Y,
		PreviewW,
		PreviewH,
		r3dColor(255, 255, 255, 255),
		gDX11PreviewTexture
	);

	r3dDrawLine2D(
		X,
		Y,
		X,
		Y + PreviewH,
		2.0f,
		r3dColor(255, 255, 255, 220)
	);

	if (Font_Label)
	{
		const RenderDX11PreviewMode PreviewMode =
			RenderDX11_GetPreviewMode();

		const int AtlasVolumes =
			Terrain2 ?
				Terrain2->GetDX11AtlasVolumeCount() :
				0;

		const int AtlasTiles =
			Terrain2 ?
				Terrain2->GetDX11VisibleAtlasTileCount() :
				0;

		Font_Label->PrintF(
			X + 10.0f,
			Y + 10.0f,
			r3dColor(255, 230, 120),
			"DX11 Preview: %s\nDX11 Terrain Coverage: %dx%d\nDX11 Terrain Culling: %s\nDX11 Terrain Drawn: %d\nDX11 Terrain Culled: %d\nDX11 Terrain Cache Updates: %d\nDX11 Terrain2: Layers %d Masks %d AtlasVol %d AtlasTiles %d\nDX11 Terrain2 Batch: C%d N%d H%d B0D%d B0N%d M%d B1D%d B2D%d B3D%d\nF10: next preview mode",
			RenderDX11_GetPreviewModeName(PreviewMode),
			RenderDX11_GetTerrainPatchSide(),
			RenderDX11_GetTerrainPatchSide(),
			RenderDX11_WantsTerrainCullEnabled() ? "ON" : "OFF",
			gDX11TerrainPatchDrawCount,
			gDX11TerrainPatchCullCount,
			gDX11TerrainPatchUpdateCount,
			gDX11Terrain2LayerCount,
			gDX11Terrain2MaskCount,
			AtlasVolumes,
			AtlasTiles,
			(gDX11Terrain2SRVMask & DX11_TERRAIN2_TEXTURE_COLOR) ? 1 : 0,
			(gDX11Terrain2SRVMask & DX11_TERRAIN2_TEXTURE_NORMAL) ? 1 : 0,
			(gDX11Terrain2SRVMask & DX11_TERRAIN2_TEXTURE_HEIGHT) ? 1 : 0,
			(gDX11Terrain2SRVMask & DX11_TERRAIN2_TEXTURE_LAYER0_DIFFUSE) ? 1 : 0,
			(gDX11Terrain2SRVMask & DX11_TERRAIN2_TEXTURE_LAYER0_NORMAL) ? 1 : 0,
			(gDX11Terrain2SRVMask & DX11_TERRAIN2_TEXTURE_MASK0) ? 1 : 0,
			(gDX11Terrain2SRVMask & DX11_TERRAIN2_TEXTURE_LAYER1_DIFFUSE) ? 1 : 0,
			(gDX11Terrain2SRVMask & DX11_TERRAIN2_TEXTURE_LAYER2_DIFFUSE) ? 1 : 0,
			(gDX11Terrain2SRVMask & DX11_TERRAIN2_TEXTURE_LAYER3_DIFFUSE) ? 1 : 0
		);
	}
}

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
	RenderDX11_ReleasePreviewTexture();
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
	gDX11OffscreenOnlyLogged = false;

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
	RenderDX11_UpdateDefaultWorldCBs();
	RenderDX11_BeginTerrainCacheFrame();

	RenderDX11_ClearFrameTargets();

	if (RenderDX11_WantsSmokeDebug())
	{
		// Real DX11 smoke draw.
		// This renders fullscreen triangle into DX11 offscreen target.
		// Result is not presented; old DX9 world still renders after fallback.
		RenderDX11_DrawClearTriangle();
		RenderDX11_SmokeReadbackOnce();
	}

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

	if (!DrawWorldDX11_Transparent(Desc))
	{
		RenderDX11_UnbindFrameTargets();
		return false;
	}

	if (!DrawWorldDX11_Post(Desc))
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
