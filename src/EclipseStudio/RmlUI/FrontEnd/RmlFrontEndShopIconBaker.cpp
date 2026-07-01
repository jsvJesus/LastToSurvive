#include "r3dPCH.h"
#include "r3d.h"

#include "r3dRender.h"
#include "r3dObj.h"
#include <d3d9.h>
#include <d3dx9.h>

#include "RmlFrontEndShopIconBaker.h"
#include "RmlFrontEndShopSizing.h"

#include "GameCode/UserProfile.h"
#include "ObjectsCode/weapons/WeaponArmory.h"

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

namespace
{
	const char* GetSafeString(
		const char* Text
	)
	{
		return Text
			? Text
			: "";
	}

	bool RenderMeshToIconDds(
		r3dMesh* Mesh,
		const char* OutputFileName,
		int Width,
		int Height
	)
	{
		if (
			!Mesh ||
			!OutputFileName ||
			!OutputFileName[0] ||
			Width <= 0 ||
			Height <= 0
		)
		{
			return false;
		}

		if (
			!r3dRenderer ||
			!r3dRenderer->pd3ddev
		)
		{
			r3dOutToLog(
				"[ShopIconBaker] D3D device is not ready.\n"
			);

			return false;
		}

		if (!Mesh->IsDrawable())
		{
			r3dOutToLog(
				"[ShopIconBaker] Mesh is not drawable: %s\n",
				Mesh->FileName.c_str()
			);

			return false;
		}

		IDirect3DDevice9* Device =
			r3dRenderer->pd3ddev;

		IDirect3DTexture9* RenderTexture =
			NULL;

		IDirect3DSurface9* RenderSurface =
			NULL;

		IDirect3DSurface9* DepthSurface =
			NULL;

		IDirect3DSurface9* OldRenderSurface =
			NULL;

		IDirect3DSurface9* OldDepthSurface =
			NULL;

		IDirect3DSurface9* SystemSurface =
			NULL;

		D3DVIEWPORT9 OldViewport{};

		D3DXMATRIXA16 OldView =
			r3dRenderer->ViewMatrix;

		D3DXMATRIXA16 OldProj =
			r3dRenderer->ProjMatrix;

		D3DXMATRIXA16 OldViewProj =
			r3dRenderer->ViewProjMatrix;

		r3dRenderer->GetRT(
			0,
			&OldRenderSurface
		);

		r3dRenderer->GetDSS(
			&OldDepthSurface
		);

		r3dRenderer->DoGetViewport(
			&OldViewport
		);

		HRESULT Hr =
			Device->CreateTexture(
				static_cast<UINT>(Width),
				static_cast<UINT>(Height),
				1,
				D3DUSAGE_RENDERTARGET,
				D3DFMT_A8R8G8B8,
				D3DPOOL_DEFAULT,
				&RenderTexture,
				NULL
			);

		if (
			FAILED(Hr) ||
			!RenderTexture
		)
		{
			r3dOutToLog(
				"[ShopIconBaker] Failed to create render texture %dx%d hr=0x%08X\n",
				Width,
				Height,
				static_cast<unsigned int>(Hr)
			);

			goto CleanupFail;
		}

		Hr =
			RenderTexture->GetSurfaceLevel(
				0,
				&RenderSurface
			);

		if (
			FAILED(Hr) ||
			!RenderSurface
		)
		{
			goto CleanupFail;
		}

		Hr =
			Device->CreateDepthStencilSurface(
				static_cast<UINT>(Width),
				static_cast<UINT>(Height),
				D3DFMT_D24S8,
				D3DMULTISAMPLE_NONE,
				0,
				TRUE,
				&DepthSurface,
				NULL
			);

		if (
			FAILED(Hr) ||
			!DepthSurface
		)
		{
			goto CleanupFail;
		}

		r3dRenderer->SetRT(
			0,
			RenderSurface
		);

		r3dRenderer->SetDSS(
			DepthSurface
		);

		D3DVIEWPORT9 Viewport{};
		Viewport.X = 0;
		Viewport.Y = 0;
		Viewport.Width =
			static_cast<DWORD>(Width);
		Viewport.Height =
			static_cast<DWORD>(Height);
		Viewport.MinZ = 0.0f;
		Viewport.MaxZ = 1.0f;

		r3dRenderer->DoSetViewport(
			0.0f,
			0.0f,
			static_cast<float>(Width),
			static_cast<float>(Height)
		);

		Device->Clear(
			0,
			NULL,
			D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
			0x00000000,
			1.0f,
			0
		);

		const r3dPoint3D BoxSize =
			Mesh->localBBox.Size;

		const r3dPoint3D BoxOrg =
			Mesh->localBBox.Org;

		const r3dPoint3D Center =
			BoxOrg +
			BoxSize * 0.5f;

		float Radius =
			R3D_MAX(
				BoxSize.X,
				R3D_MAX(
					BoxSize.Y,
					BoxSize.Z
				)
			) * 0.5f;

		Radius =
			R3D_MAX(
				Radius,
				0.25f
			);

		const float Aspect =
			static_cast<float>(Width) /
			static_cast<float>(Height);

		const float Fov =
			R3D_DEG2RAD(28.0f);

		const float Distance =
			Radius /
			tanf(Fov * 0.5f) *
			1.35f;

		D3DXVECTOR3 Eye(
			0.0f,
			Radius * 0.18f,
			-Distance
		);

		D3DXVECTOR3 At(
			0.0f,
			0.0f,
			0.0f
		);

		D3DXVECTOR3 Up(
			0.0f,
			1.0f,
			0.0f
		);

		D3DXMatrixLookAtLH(
			&r3dRenderer->ViewMatrix,
			&Eye,
			&At,
			&Up
		);

		D3DXMatrixPerspectiveFovLH(
			&r3dRenderer->ProjMatrix,
			Fov,
			Aspect,
			0.01f,
			Distance + Radius * 8.0f
		);

		r3dRenderer->ViewProjMatrix =
			r3dRenderer->ViewMatrix *
			r3dRenderer->ProjMatrix;

		D3DXMATRIXA16 MoveToOrigin;
		D3DXMATRIXA16 Rotation;
		D3DXMATRIXA16 World;

		D3DXMatrixTranslation(
			&MoveToOrigin,
			-Center.X,
			-Center.Y,
			-Center.Z
		);

		D3DXMatrixRotationYawPitchRoll(
			&Rotation,
			R3D_DEG2RAD(35.0f),
			R3D_DEG2RAD(-8.0f),
			R3D_DEG2RAD(0.0f)
		);

		World =
			MoveToOrigin *
			Rotation;

		Device->SetRenderState(
			D3DRS_ZENABLE,
			TRUE
		);

		Device->SetRenderState(
			D3DRS_ZWRITEENABLE,
			TRUE
		);

		Device->SetRenderState(
			D3DRS_ALPHABLENDENABLE,
			FALSE
		);

		r3dRenderer->SetCullMode(D3DCULL_CCW);

		Mesh->SetVSConsts(
			World
		);

		Mesh->DrawMeshDeferred(
			r3dColor::white,
			0
		);

		Hr =
			Device->CreateOffscreenPlainSurface(
				static_cast<UINT>(Width),
				static_cast<UINT>(Height),
				D3DFMT_A8R8G8B8,
				D3DPOOL_SYSTEMMEM,
				&SystemSurface,
				NULL
			);

		if (
			FAILED(Hr) ||
			!SystemSurface
		)
		{
			goto CleanupFail;
		}

		Hr =
			Device->GetRenderTargetData(
				RenderSurface,
				SystemSurface
			);

		if (FAILED(Hr))
		{
			r3dOutToLog(
				"[ShopIconBaker] GetRenderTargetData failed hr=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			goto CleanupFail;
		}

		Hr =
			D3DXSaveSurfaceToFileA(
				OutputFileName,
				D3DXIFF_DDS,
				SystemSurface,
				NULL,
				NULL
			);

		if (FAILED(Hr))
		{
			r3dOutToLog(
				"[ShopIconBaker] Save DDS failed: %s hr=0x%08X\n",
				OutputFileName,
				static_cast<unsigned int>(Hr)
			);

			goto CleanupFail;
		}

		r3dRenderer->SetRT(
			0,
			OldRenderSurface
		);

		r3dRenderer->SetDSS(
			OldDepthSurface
		);

		r3dRenderer->DoSetViewport(
			static_cast<float>(OldViewport.X),
			static_cast<float>(OldViewport.Y),
			static_cast<float>(OldViewport.Width),
			static_cast<float>(OldViewport.Height)
		);

		r3dRenderer->ViewMatrix =
			OldView;

		r3dRenderer->ProjMatrix =
			OldProj;

		r3dRenderer->ViewProjMatrix =
			OldViewProj;

		if (SystemSurface)
			SystemSurface->Release();

		if (DepthSurface)
			DepthSurface->Release();

		if (RenderSurface)
			RenderSurface->Release();

		if (RenderTexture)
			RenderTexture->Release();

		if (OldDepthSurface)
			OldDepthSurface->Release();

		if (OldRenderSurface)
			OldRenderSurface->Release();

		return true;

		CleanupFail:

			r3dRenderer->SetRT(
				0,
				OldRenderSurface
			);

		r3dRenderer->SetDSS(
			OldDepthSurface
		);

		r3dRenderer->DoSetViewport(
			static_cast<float>(OldViewport.X),
			static_cast<float>(OldViewport.Y),
			static_cast<float>(OldViewport.Width),
			static_cast<float>(OldViewport.Height)
		);

		r3dRenderer->ViewMatrix =
			OldView;

		r3dRenderer->ProjMatrix =
			OldProj;

		r3dRenderer->ViewProjMatrix =
			OldViewProj;

		if (SystemSurface)
			SystemSurface->Release();

		if (DepthSurface)
			DepthSurface->Release();

		if (RenderSurface)
			RenderSurface->Release();

		if (RenderTexture)
			RenderTexture->Release();

		if (OldDepthSurface)
			OldDepthSurface->Release();

		if (OldRenderSurface)
			OldRenderSurface->Release();

		return false;
	}

	bool ResizeDdsIcon(
		const char* SourcePath,
		const char* TargetPath,
		int TargetWidth,
		int TargetHeight
	)
	{
		if (
			!SourcePath ||
			!SourcePath[0] ||
			!TargetPath ||
			!TargetPath[0] ||
			TargetWidth <= 0 ||
			TargetHeight <= 0
		)
		{
			return false;
		}

		if (
			!r3dRenderer ||
			!r3dRenderer->pd3ddev
		)
		{
			r3dOutToLog(
				"[ShopIconBaker] D3D device is not ready.\n"
			);

			return false;
		}

		IDirect3DTexture9* Texture =
			NULL;

		D3DXIMAGE_INFO ImageInfo{};

		const HRESULT LoadResult =
			D3DXCreateTextureFromFileExA(
				r3dRenderer->pd3ddev,
				SourcePath,
				static_cast<UINT>(TargetWidth),
				static_cast<UINT>(TargetHeight),
				1,
				0,
				D3DFMT_A8R8G8B8,
				D3DPOOL_SCRATCH,
				D3DX_FILTER_LINEAR,
				D3DX_FILTER_LINEAR,
				0,
				&ImageInfo,
				NULL,
				&Texture
			);

		if (
			FAILED(LoadResult) ||
			!Texture
		)
		{
			r3dOutToLog(
				"[ShopIconBaker] Failed to load/resize DDS: %s hr=0x%08X\n",
				SourcePath,
				static_cast<unsigned int>(LoadResult)
			);

			return false;
		}

		const HRESULT SaveResult =
			D3DXSaveTextureToFileA(
				TargetPath,
				D3DXIFF_DDS,
				Texture,
				NULL
			);

		Texture->Release();

		if (FAILED(SaveResult))
		{
			r3dOutToLog(
				"[ShopIconBaker] Failed to save DDS: %s hr=0x%08X\n",
				TargetPath,
				static_cast<unsigned int>(SaveResult)
			);

			return false;
		}

		return true;
	}

	bool DoesPhysicalFileExist(const char* FileName)
	{
		if (
			!FileName ||
			!FileName[0]
		)
		{
			return false;
		}

		const DWORD Attributes =
			GetFileAttributesA(
				FileName
			);

		return
			Attributes != INVALID_FILE_ATTRIBUTES &&
			!(Attributes & FILE_ATTRIBUTE_DIRECTORY);
	}

	std::string NormalizeStoreIconToPhysicalPath(
		const BaseItemConfig* Config
	)
	{
		if (
			!Config ||
			!Config->m_StoreIcon ||
			!Config->m_StoreIcon[0]
		)
		{
			return "";
		}

		std::string Path =
			Config->m_StoreIcon;

		std::replace(
			Path.begin(),
			Path.end(),
			'\\',
			'/'
		);

		const std::string DataMacroPrefix =
			"$Data/";

		if (
			Path.compare(
				0,
				DataMacroPrefix.length(),
				DataMacroPrefix
			) == 0
		)
		{
			Path.erase(
				0,
				DataMacroPrefix.length()
			);
		}
		else if (
			Path.compare(
				0,
				5,
				"Data/"
			) == 0
		)
		{
			Path.erase(
				0,
				5
			);
		}

		if (
			Path.find('/') ==
			std::string::npos
		)
		{
			Path =
				std::string(
					"Weapons/StoreIcons/"
				) +
				Path;
		}

		if (
			Path.length() < 4 ||
			_stricmp(
				Path.c_str() +
					Path.length() -
					4,
				".dds"
			) != 0
		)
		{
			Path += ".dds";
		}

		Path =
			std::string("Data/") +
			Path;

		std::replace(
			Path.begin(),
			Path.end(),
			'/',
			'\\'
		);

		return Path;
	}

	std::string BuildGeneratedIconPhysicalPath(
		const char* OutputDirectory,
		uint32_t ItemId
	)
	{
		char Path[512]{};

		sprintf_s(
			Path,
			sizeof(Path),
			"%s\\%u.dds",
			OutputDirectory,
			ItemId
		);

		return Path;
	}

	void WriteJsonString(
		FILE* File,
		const char* Text
	)
	{
		fputc(
			'"',
			File
		);

		if (Text)
		{
			for (
				const unsigned char* Cursor =
					reinterpret_cast<const unsigned char*>(
						Text
					);
				*Cursor;
				++Cursor
			)
			{
				const unsigned char Character =
					*Cursor;

				switch (Character)
				{
					case '\\':
						fputs(
							"\\\\",
							File
						);
						break;

					case '"':
						fputs(
							"\\\"",
							File
						);
						break;

					case '\n':
						fputs(
							"\\n",
							File
						);
						break;

					case '\r':
						fputs(
							"\\r",
							File
						);
						break;

					case '\t':
						fputs(
							"\\t",
							File
						);
						break;

					default:
						fputc(
							Character,
							File
						);
						break;
				}
			}
		}

		fputc(
			'"',
			File
		);
	}

	const ModelItemConfig* GetModelConfigForItem(
		uint32_t ItemId,
		const BaseItemConfig* Config
	)
	{
		if (
			!g_pWeaponArmory ||
			!Config
		)
		{
			return NULL;
		}

		const WeaponConfig* Weapon =
			g_pWeaponArmory->getWeaponConfig(
				ItemId
			);

		if (Weapon)
			return Weapon;

		const WeaponAttachmentConfig* Attachment =
			g_pWeaponArmory->getAttachmentConfig(
				ItemId
			);

		if (Attachment)
			return Attachment;

		const GearConfig* Gear =
			g_pWeaponArmory->getGearConfig(
				ItemId
			);

		if (Gear)
			return Gear;

		const BackpackConfig* Backpack =
			g_pWeaponArmory->getBackpackConfig(
				ItemId
			);

		if (Backpack)
			return Backpack;

		const FoodConfig* Food =
			g_pWeaponArmory->getFoodConfig(
				ItemId
			);

		if (Food)
			return Food;

		if (
			Config->category != storecat_LootBox
		)
		{
			const ModelItemConfig* Item =
				g_pWeaponArmory->getItemConfig(
					ItemId
				);

			if (Item)
				return Item;
		}

		return NULL;
	}

	int GetBakeRuntimeCategory(
		const BaseItemConfig* Config
	)
	{
		if (!Config)
			return storecat_INVALID;

		if (
			Config->m_StoreCategoryOverride > 0
		)
		{
			return Config->m_StoreCategoryOverride;
		}

		return static_cast<int>(
			Config->category
		);
	}
}

bool RmlFrontEndShopIconBaker::WriteBakeList(
	const char* OutputFileName
)
{
	if (
		!OutputFileName ||
		!OutputFileName[0]
	)
	{
		r3dOutToLog(
			"[ShopIconBaker] Empty output filename.\n"
		);

		return false;
	}

	if (!g_pWeaponArmory)
	{
		r3dOutToLog(
			"[ShopIconBaker] Weapon armory is not initialized.\n"
		);

		return false;
	}

	CreateDirectoryA(
		"Data\\Weapons\\GeneratedShopIcons",
		NULL
	);

	FILE* File = NULL;

	fopen_s(
		&File,
		OutputFileName,
		"wb"
	);

	if (!File)
	{
		r3dOutToLog(
			"[ShopIconBaker] Failed to open output file: %s\n",
			OutputFileName
		);

		return false;
	}

	fprintf(
		File,
		"{\n"
	);

	fprintf(
		File,
		"  \"cellSize\": %d,\n",
		RmlFrontEndShopSizing::ShopGridCellSize
	);

	fprintf(
		File,
		"  \"columns\": %d,\n",
		RmlFrontEndShopSizing::ShopGridColumns
	);

	fprintf(
		File,
		"  \"items\": [\n"
	);

	int WrittenCount =
		0;

	g_pWeaponArmory->startItemSearch();

	while (
		g_pWeaponArmory->searchNextItem()
	)
	{
		const uint32_t ItemId =
			g_pWeaponArmory->getCurrentSearchItemID();

		const BaseItemConfig* Config =
			g_pWeaponArmory->getConfig(
				ItemId
			);

		if (!Config)
			continue;

		const int RuntimeCategory =
			GetBakeRuntimeCategory(
				Config
			);

		const RmlFrontEndShopSizing::FShopGridSize GridSize =
			RmlFrontEndShopSizing::GetShopGridSize(
				Config,
				RuntimeCategory
			);

		const int IconWidth =
			RmlFrontEndShopSizing::GetShopIconWidthPx(
				GridSize
			);

		const int IconHeight =
			RmlFrontEndShopSizing::GetShopIconHeightPx(
				GridSize
			);

		const ModelItemConfig* ModelConfig =
			GetModelConfigForItem(
				ItemId,
				Config
			);

		if (WrittenCount > 0)
		{
			fprintf(
				File,
				",\n"
			);
		}

		fprintf(
			File,
			"    {\n"
		);

		fprintf(
			File,
			"      \"itemID\": %u,\n",
			ItemId
		);

		fprintf(
			File,
			"      \"category\": %d,\n",
			RuntimeCategory
		);

		fprintf(
			File,
			"      \"gridW\": %d,\n",
			GridSize.Width
		);

		fprintf(
			File,
			"      \"gridH\": %d,\n",
			GridSize.Height
		);

		fprintf(
			File,
			"      \"width\": %d,\n",
			IconWidth
		);

		fprintf(
			File,
			"      \"height\": %d,\n",
			IconHeight
		);

		fprintf(
			File,
			"      \"name\": "
		);

		WriteJsonString(
			File,
			GetSafeString(
				Config->m_StoreName
			)
		);

		fprintf(
			File,
			",\n"
		);

		fprintf(
			File,
			"      \"storeIcon\": "
		);

		WriteJsonString(
			File,
			GetSafeString(
				Config->m_StoreIcon
			)
		);

		fprintf(
			File,
			",\n"
		);

		fprintf(
			File,
			"      \"model\": "
		);

		WriteJsonString(
			File,
			ModelConfig
				? GetSafeString(
					ModelConfig->m_ModelPath
				)
				: ""
		);

		fprintf(
			File,
			",\n"
		);

		fprintf(
			File,
			"      \"output\": "
		);

		char OutputIcon[256]{};

		sprintf_s(
			OutputIcon,
			sizeof(OutputIcon),
			"Data/Weapons/GeneratedShopIcons/%u.dds",
			ItemId
		);

		WriteJsonString(
			File,
			OutputIcon
		);

		fprintf(
			File,
			"\n"
		);

		fprintf(
			File,
			"    }"
		);

		++WrittenCount;
	}

	fprintf(
		File,
		"\n  ]\n"
	);

	fprintf(
		File,
		"}\n"
	);

	fclose(
		File
	);

	r3dOutToLog(
		"[ShopIconBaker] Bake list written: %s Items=%d\n",
		OutputFileName,
		WrittenCount
	);

	return true;
}

bool RmlFrontEndShopIconBaker::CopyExistingStoreIcons(
	const char* OutputDirectory,
	bool bOverwriteExisting
)
{
	if (
		!OutputDirectory ||
		!OutputDirectory[0]
	)
	{
		r3dOutToLog(
			"[ShopIconBaker] Empty output directory.\n"
		);

		return false;
	}

	if (!g_pWeaponArmory)
	{
		r3dOutToLog(
			"[ShopIconBaker] Weapon armory is not initialized.\n"
		);

		return false;
	}

	CreateDirectoryA(
		"Data\\Weapons\\GeneratedShopIcons",
		NULL
	);

	int CopiedCount = 0;
	int SkippedCount = 0;
	int MissingCount = 0;
	int FailedCount = 0;

	g_pWeaponArmory->startItemSearch();

	while (
		g_pWeaponArmory->searchNextItem()
	)
	{
		const uint32_t ItemId =
			g_pWeaponArmory->getCurrentSearchItemID();

		const BaseItemConfig* Config =
			g_pWeaponArmory->getConfig(
				ItemId
			);

		if (!Config)
		{
			++SkippedCount;
			continue;
		}

		const std::string SourcePath =
			NormalizeStoreIconToPhysicalPath(
				Config
			);

		if (
			SourcePath.empty() ||
			!DoesPhysicalFileExist(
				SourcePath.c_str()
			)
		)
		{
			++MissingCount;
			continue;
		}

		const std::string TargetPath =
			BuildGeneratedIconPhysicalPath(
				OutputDirectory,
				ItemId
			);

		if (
			!bOverwriteExisting &&
			DoesPhysicalFileExist(
				TargetPath.c_str()
			)
		)
		{
			++SkippedCount;
			continue;
		}

		const BOOL bCopied =
			CopyFileA(
				SourcePath.c_str(),
				TargetPath.c_str(),
				bOverwriteExisting
					? FALSE
					: TRUE
			);

		if (bCopied)
		{
			++CopiedCount;
		}
		else
		{
			++FailedCount;

			r3dOutToLog(
				"[ShopIconBaker] Failed to copy icon item=%u from=%s to=%s error=%lu\n",
				ItemId,
				SourcePath.c_str(),
				TargetPath.c_str(),
				GetLastError()
			);
		}
	}

	r3dOutToLog(
		"[ShopIconBaker] DDS copy finished. Copied=%d Skipped=%d Missing=%d Failed=%d\n",
		CopiedCount,
		SkippedCount,
		MissingCount,
		FailedCount
	);

	return FailedCount == 0;
}

bool RmlFrontEndShopIconBaker::BakeResizedStoreIcons(
	const char* OutputDirectory,
	bool bOverwriteExisting
)
{
	if (
		!OutputDirectory ||
		!OutputDirectory[0]
	)
	{
		r3dOutToLog(
			"[ShopIconBaker] Empty output directory.\n"
		);

		return false;
	}

	if (!g_pWeaponArmory)
	{
		r3dOutToLog(
			"[ShopIconBaker] Weapon armory is not initialized.\n"
		);

		return false;
	}

	CreateDirectoryA(
		"Data\\Weapons\\GeneratedShopIcons",
		NULL
	);

	int BakedCount = 0;
	int SkippedCount = 0;
	int MissingCount = 0;
	int FailedCount = 0;

	g_pWeaponArmory->startItemSearch();

	while (
		g_pWeaponArmory->searchNextItem()
	)
	{
		const uint32_t ItemId =
			g_pWeaponArmory->getCurrentSearchItemID();

		const BaseItemConfig* Config =
			g_pWeaponArmory->getConfig(
				ItemId
			);

		if (!Config)
		{
			++SkippedCount;
			continue;
		}

		const std::string SourcePath =
			NormalizeStoreIconToPhysicalPath(
				Config
			);

		if (
			SourcePath.empty() ||
			!DoesPhysicalFileExist(
				SourcePath.c_str()
			)
		)
		{
			++MissingCount;
			continue;
		}

		const int RuntimeCategory =
			GetBakeRuntimeCategory(
				Config
			);

		const RmlFrontEndShopSizing::FShopGridSize GridSize =
			RmlFrontEndShopSizing::GetShopGridSize(
				Config,
				RuntimeCategory
			);

		const int TargetWidth =
			RmlFrontEndShopSizing::GetShopIconWidthPx(
				GridSize
			);

		const int TargetHeight =
			RmlFrontEndShopSizing::GetShopIconHeightPx(
				GridSize
			);

		const std::string TargetPath =
			BuildGeneratedIconPhysicalPath(
				OutputDirectory,
				ItemId
			);

		if (
			!bOverwriteExisting &&
			DoesPhysicalFileExist(
				TargetPath.c_str()
			)
		)
		{
			++SkippedCount;
			continue;
		}

		if (
			ResizeDdsIcon(
				SourcePath.c_str(),
				TargetPath.c_str(),
				TargetWidth,
				TargetHeight
			)
		)
		{
			++BakedCount;
		}
		else
		{
			++FailedCount;
		}
	}

	r3dOutToLog(
		"[ShopIconBaker] DDS resize finished. Baked=%d Skipped=%d Missing=%d Failed=%d\n",
		BakedCount,
		SkippedCount,
		MissingCount,
		FailedCount
	);

	return FailedCount == 0;
}

bool RmlFrontEndShopIconBaker::BakeSingleItem3D(
	uint32_t ItemId,
	const char* OutputDirectory,
	int Width,
	int Height,
	bool bOverwriteExisting
)
{
	if (
		!OutputDirectory ||
		!OutputDirectory[0]
	)
	{
		r3dOutToLog(
			"[ShopIconBaker] Empty output directory.\n"
		);

		return false;
	}

	if (!g_pWeaponArmory)
	{
		r3dOutToLog(
			"[ShopIconBaker] Weapon armory is not initialized.\n"
		);

		return false;
	}

	const BaseItemConfig* Config =
		g_pWeaponArmory->getConfig(
			ItemId
		);

	if (!Config)
	{
		r3dOutToLog(
			"[ShopIconBaker] Item not found: %u\n",
			ItemId
		);

		return false;
	}

	const int RuntimeCategory =
		GetBakeRuntimeCategory(
			Config
		);

	const RmlFrontEndShopSizing::FShopGridSize GridSize =
		RmlFrontEndShopSizing::GetShopGridSize(
			Config,
			RuntimeCategory
		);

	if (Width <= 0)
	{
		Width =
			RmlFrontEndShopSizing::GetShopIconWidthPx(
				GridSize
			);
	}

	if (Height <= 0)
	{
		Height =
			RmlFrontEndShopSizing::GetShopIconHeightPx(
				GridSize
			);
	}

	const ModelItemConfig* ModelConfig =
		GetModelConfigForItem(
			ItemId,
			Config
		);

	if (!ModelConfig)
	{
		r3dOutToLog(
			"[ShopIconBaker] Item has no model config: %u\n",
			ItemId
		);

		return false;
	}

	r3dMesh* Mesh =
		ModelConfig->getMesh();

	if (!Mesh)
	{
		r3dOutToLog(
			"[ShopIconBaker] Failed to load mesh for item: %u\n",
			ItemId
		);

		return false;
	}

	CreateDirectoryA(
		"Data\\Weapons\\GeneratedShopIcons",
		NULL
	);

	const std::string OutputPath =
		BuildGeneratedIconPhysicalPath(
			OutputDirectory,
			ItemId
		);

	if (
		!bOverwriteExisting &&
		DoesPhysicalFileExist(
			OutputPath.c_str()
		)
	)
	{
		r3dOutToLog(
			"[ShopIconBaker] Icon already exists, skipped: %s\n",
			OutputPath.c_str()
		);

		return true;
	}

	const bool bResult =
		RenderMeshToIconDds(
			Mesh,
			OutputPath.c_str(),
			Width,
			Height
		);

	r3dOutToLog(
		"[ShopIconBaker] 3D bake item=%u size=%dx%d result=%s output=%s\n",
		ItemId,
		Width,
		Height,
		bResult
			? "OK"
			: "FAILED",
		OutputPath.c_str()
	);

	return bResult;
}