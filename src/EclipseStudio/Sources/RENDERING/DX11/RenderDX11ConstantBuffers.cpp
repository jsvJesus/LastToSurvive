#include "r3dPCH.h"
#include "r3d.h"

#include "rendering/DX11/RenderDX11Core.h"
#include "rendering/DX11/RenderDX11ConstantBuffers.h"

#if LTS_STUDIO_DX11

#include <stdio.h>
#include <string.h>

namespace
{
	template <typename T>
	void RenderDX11ConstantBuffers_SafeRelease(
		T*& Object
	)
	{
		if (!Object)
			return;

		Object->Release();
		Object = 0;
	}

	void RenderDX11ConstantBuffers_Log(
		const char* Text
	)
	{
		if (!Text)
			return;

		OutputDebugStringA(Text);
		r3dOutToLog("%s", Text);
	}
}

RenderDX11ConstantBuffers::RenderDX11ConstantBuffers()
	: Frame_(0)
	, Terrain_(0)
	, Object_(0)
	, Material_(0)
	, Light_(0)
	, Shadow_(0)
	, Water_(0)
	, Grass_(0)
	, SunGlare_(0)
{
}

void RenderDX11ConstantBuffers::LogFailure(
	const char* Stage,
	HRESULT Result
)
{
	char Text[512] = {};

	sprintf_s(
		Text,
		"[DX11][ConstantBuffers] %s failed. HRESULT=0x%08X\n",
		Stage ? Stage : "Unknown operation",
		static_cast<unsigned int>(Result)
	);

	RenderDX11ConstantBuffers_Log(Text);
}

bool RenderDX11ConstantBuffers::ValidateByteWidth(
	UINT ByteWidth,
	const char* DebugName
)
{
	if (
		ByteWidth == 0 ||
		(ByteWidth & 15) != 0 ||
		ByteWidth > 65536
	)
	{
		char Text[512] = {};

		sprintf_s(
			Text,
			"[DX11][ConstantBuffers] Invalid size for %s: "
			"%u bytes. Size must be 16-byte aligned "
			"and not exceed 65536 bytes.\n",
			DebugName ? DebugName : "unknown",
			static_cast<unsigned int>(ByteWidth)
		);

		RenderDX11ConstantBuffers_Log(Text);
		return false;
	}

	return true;
}

bool RenderDX11ConstantBuffers::CreateBuffer(
	UINT ByteWidth,
	const char* DebugName,
	ID3D11Buffer** OutBuffer
)
{
	if (!OutBuffer)
		return false;

	*OutBuffer = 0;

	if (!ValidateByteWidth(
		ByteWidth,
		DebugName
	))
	{
		return false;
	}

	ID3D11Device* Device =
		RenderDX11_GetCore().GetDevice();

	if (!Device)
		return false;

	D3D11_BUFFER_DESC Desc = {};

	Desc.ByteWidth = ByteWidth;

	Desc.Usage =
		D3D11_USAGE_DYNAMIC;

	Desc.BindFlags =
		D3D11_BIND_CONSTANT_BUFFER;

	Desc.CPUAccessFlags =
		D3D11_CPU_ACCESS_WRITE;

	Desc.MiscFlags = 0;
	Desc.StructureByteStride = 0;

	const HRESULT Result =
		Device->CreateBuffer(
			&Desc,
			0,
			OutBuffer
		);

	if (FAILED(Result))
	{
		char Stage[256] = {};

		sprintf_s(
			Stage,
			"Create %s",
			DebugName ? DebugName : "constant buffer"
		);

		LogFailure(
			Stage,
			Result
		);

		return false;
	}

	return true;
}

bool RenderDX11ConstantBuffers::Initialize(
	const RenderDX11ConstantBuffersCreateDesc& Desc
)
{
	if (IsReady())
		return true;

	Shutdown();

	if (!RenderDX11_GetCore().IsReady())
	{
		RenderDX11ConstantBuffers_Log(
			"[DX11][ConstantBuffers] Cannot initialize "
			"without DX11 core\n"
		);

		return false;
	}

	if (!CreateBuffer(
		Desc.FrameByteWidth,
		"FrameCB",
		&Frame_
	))
	{
		Shutdown();
		return false;
	}

	if (!CreateBuffer(
		Desc.TerrainByteWidth,
		"TerrainCB",
		&Terrain_
	))
	{
		Shutdown();
		return false;
	}

	if (!CreateBuffer(
		Desc.ObjectByteWidth,
		"ObjectCB",
		&Object_
	))
	{
		Shutdown();
		return false;
	}

	if (!CreateBuffer(
		Desc.MaterialByteWidth,
		"MaterialCB",
		&Material_
	))
	{
		Shutdown();
		return false;
	}

	if (!CreateBuffer(
		Desc.LightByteWidth,
		"LightCB",
		&Light_
	))
	{
		Shutdown();
		return false;
	}

	if (!CreateBuffer(
		Desc.ShadowByteWidth,
		"ShadowCB",
		&Shadow_
	))
	{
		Shutdown();
		return false;
	}

	if (!CreateBuffer(
		Desc.WaterByteWidth,
		"WaterCB",
		&Water_
	))
	{
		Shutdown();
		return false;
	}

	if (!CreateBuffer(
		Desc.GrassByteWidth,
		"GrassCB",
		&Grass_
	))
	{
		Shutdown();
		return false;
	}

	if (!CreateBuffer(
		Desc.SunGlareByteWidth,
		"SunGlareCB",
		&SunGlare_
	))
	{
		Shutdown();
		return false;
	}

	RenderDX11ConstantBuffers_Log(
		"[DX11][ConstantBuffers] Created: "
		"Frame(b0), Terrain(b1), Object(b2), "
		"Material(b3), Light(b4), Shadow(b5), "
		"Water(b6), Grass(b7), SunGlare(b8)\n"
	);

	return true;
}

void RenderDX11ConstantBuffers::Shutdown()
{
	RenderDX11ConstantBuffers_SafeRelease(
		SunGlare_
	);

	RenderDX11ConstantBuffers_SafeRelease(
		Grass_
	);

	RenderDX11ConstantBuffers_SafeRelease(
		Water_
	);

	RenderDX11ConstantBuffers_SafeRelease(
		Shadow_
	);

	RenderDX11ConstantBuffers_SafeRelease(
		Light_
	);

	RenderDX11ConstantBuffers_SafeRelease(
		Material_
	);

	RenderDX11ConstantBuffers_SafeRelease(
		Object_
	);

	RenderDX11ConstantBuffers_SafeRelease(
		Terrain_
	);

	RenderDX11ConstantBuffers_SafeRelease(
		Frame_
	);
}

bool RenderDX11ConstantBuffers::IsReady() const
{
	return
		Frame_ != 0 &&
		Terrain_ != 0 &&
		Object_ != 0 &&
		Material_ != 0 &&
		Light_ != 0 &&
		Shadow_ != 0 &&
		Water_ != 0 &&
		Grass_ != 0 &&
		SunGlare_ != 0;
}

bool RenderDX11ConstantBuffers::Update(
	ID3D11Buffer* Buffer,
	const void* Data,
	size_t DataSize,
	const char* DebugName
)
{
	ID3D11DeviceContext* Context =
		RenderDX11_GetCore().GetContext();

	if (
		!Context ||
		!Buffer ||
		!Data ||
		DataSize == 0
	)
	{
		return false;
	}

	D3D11_BUFFER_DESC BufferDesc = {};
	Buffer->GetDesc(&BufferDesc);

	if (DataSize > BufferDesc.ByteWidth)
	{
		char Text[512] = {};

		sprintf_s(
			Text,
			"[DX11][ConstantBuffers] Update %s rejected: "
			"data=%u buffer=%u\n",
			DebugName ? DebugName : "unknown",
			static_cast<unsigned int>(DataSize),
			static_cast<unsigned int>(
				BufferDesc.ByteWidth
			)
		);

		RenderDX11ConstantBuffers_Log(Text);
		return false;
	}

	D3D11_MAPPED_SUBRESOURCE Mapped = {};

	const HRESULT Result =
		Context->Map(
			Buffer,
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&Mapped
		);

	if (FAILED(Result))
	{
		char Stage[256] = {};

		sprintf_s(
			Stage,
			"Map %s",
			DebugName ? DebugName : "constant buffer"
		);

		LogFailure(
			Stage,
			Result
		);

		return false;
	}

	memcpy(
		Mapped.pData,
		Data,
		DataSize
	);

	Context->Unmap(
		Buffer,
		0
	);

	return true;
}

void RenderDX11ConstantBuffers::BindFrame()
{
	ID3D11DeviceContext* Context =
		RenderDX11_GetCore().GetContext();

	if (!Context || !Frame_)
		return;

	Context->VSSetConstantBuffers(
		0,
		1,
		&Frame_
	);

	Context->PSSetConstantBuffers(
		0,
		1,
		&Frame_
	);
}

void RenderDX11ConstantBuffers::BindTerrain()
{
	ID3D11DeviceContext* Context =
		RenderDX11_GetCore().GetContext();

	if (!Context || !Terrain_)
		return;

	Context->VSSetConstantBuffers(
		1,
		1,
		&Terrain_
	);

	Context->PSSetConstantBuffers(
		1,
		1,
		&Terrain_
	);
}

void RenderDX11ConstantBuffers::BindWorld()
{
	ID3D11DeviceContext* Context =
		RenderDX11_GetCore().GetContext();

	if (!Context || !IsReady())
		return;

	ID3D11Buffer* Buffers[8] =
	{
		Frame_,
		Terrain_,
		Object_,
		Material_,
		Light_,
		Shadow_,
		Water_,
		Grass_
	};

	Context->VSSetConstantBuffers(
		0,
		8,
		Buffers
	);

	Context->PSSetConstantBuffers(
		0,
		8,
		Buffers
	);
}

RenderDX11ConstantBuffers&
RenderDX11_GetConstantBuffers()
{
	static RenderDX11ConstantBuffers Buffers;
	return Buffers;
}

#endif // LTS_STUDIO_DX11