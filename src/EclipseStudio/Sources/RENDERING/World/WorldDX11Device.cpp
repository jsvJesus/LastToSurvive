#include "r3dPCH.h"
#include "r3d.h"

#include "rendering/World/WorldDX11Device.h"

#if LTS_STUDIO_DX11

#include <stdio.h>

namespace
{
	ID3D11Device* GWorldDX11Device = 0;
	ID3D11DeviceContext* GWorldDX11Context = 0;

	WorldDX11DeviceCaps GWorldDX11Caps = {};

	const char* WorldDX11Device_FeatureLevelToString(
		D3D_FEATURE_LEVEL FeatureLevel
	)
	{
		switch (FeatureLevel)
		{
		case D3D_FEATURE_LEVEL_11_1:
			return "11_1";

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

	template <typename T>
	void WorldDX11Device_SafeRelease(T*& Ptr)
	{
		if (Ptr)
		{
			Ptr->Release();
			Ptr = 0;
		}
	}
}

bool WorldDX11Device_Init()
{
	if (GWorldDX11Device && GWorldDX11Context)
		return true;

	const D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	UINT Flags = 0;

#if defined(_DEBUG)
	Flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL CreatedFeatureLevel =
		D3D_FEATURE_LEVEL_10_0;

	HRESULT Hr =
		D3D11CreateDevice(
			0,
			D3D_DRIVER_TYPE_HARDWARE,
			0,
			Flags,
			FeatureLevels,
			_countof(FeatureLevels),
			D3D11_SDK_VERSION,
			&GWorldDX11Device,
			&CreatedFeatureLevel,
			&GWorldDX11Context
		);

#if defined(_DEBUG)
	// На некоторых машинах debug layer не установлен.
	// Тогда пробуем создать обычный device.
	if (FAILED(Hr) && (Flags & D3D11_CREATE_DEVICE_DEBUG))
	{
		OutputDebugStringA(
			"[WorldDX11Device] Debug device failed, retrying without debug layer.\n"
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
				&GWorldDX11Device,
				&CreatedFeatureLevel,
				&GWorldDX11Context
			);
	}
#endif

	if (FAILED(Hr))
	{
		char Text[256] = {};
		sprintf_s(
			Text,
			"[WorldDX11Device] D3D11CreateDevice failed. HRESULT=0x%08X\n",
			static_cast<unsigned int>(Hr)
		);

		OutputDebugStringA(Text);

		WorldDX11Device_Shutdown();
		return false;
	}

	GWorldDX11Caps.FeatureLevel = CreatedFeatureLevel;
	GWorldDX11Caps.bDebugDevice =
		(Flags & D3D11_CREATE_DEVICE_DEBUG) != 0;

	GWorldDX11Caps.bInitialized = true;

	char Text[256] = {};
	sprintf_s(
		Text,
		"[WorldDX11Device] Initialized. FeatureLevel=%s Debug=%d\n",
		WorldDX11Device_FeatureLevelToString(CreatedFeatureLevel),
		GWorldDX11Caps.bDebugDevice ? 1 : 0
	);

	OutputDebugStringA(Text);

	return true;
}

void WorldDX11Device_Shutdown()
{
	if (GWorldDX11Context)
	{
		GWorldDX11Context->ClearState();
		GWorldDX11Context->Flush();
	}

	WorldDX11Device_SafeRelease(GWorldDX11Context);
	WorldDX11Device_SafeRelease(GWorldDX11Device);

	GWorldDX11Caps = WorldDX11DeviceCaps();

	OutputDebugStringA(
		"[WorldDX11Device] Shutdown\n"
	);
}

bool WorldDX11Device_IsInitialized()
{
	return
		GWorldDX11Device != 0 &&
		GWorldDX11Context != 0;
}

ID3D11Device* WorldDX11Device_GetDevice()
{
	return GWorldDX11Device;
}

ID3D11DeviceContext* WorldDX11Device_GetContext()
{
	return GWorldDX11Context;
}

const WorldDX11DeviceCaps& WorldDX11Device_GetCaps()
{
	return GWorldDX11Caps;
}

#endif // LTS_STUDIO_DX11