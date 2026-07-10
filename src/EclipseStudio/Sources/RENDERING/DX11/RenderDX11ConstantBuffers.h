#pragma once

#include "r3dRendererConfig.h"

#if LTS_STUDIO_DX11

#include <D3D11.h>
#include <stddef.h>

struct RenderDX11ConstantBuffersCreateDesc
{
	UINT FrameByteWidth;
	UINT TerrainByteWidth;
	UINT ObjectByteWidth;
	UINT MaterialByteWidth;
	UINT LightByteWidth;
	UINT ShadowByteWidth;
	UINT WaterByteWidth;
	UINT GrassByteWidth;
	UINT SunGlareByteWidth;

	RenderDX11ConstantBuffersCreateDesc()
		: FrameByteWidth(0)
		, TerrainByteWidth(0)
		, ObjectByteWidth(0)
		, MaterialByteWidth(0)
		, LightByteWidth(0)
		, ShadowByteWidth(0)
		, WaterByteWidth(0)
		, GrassByteWidth(0)
		, SunGlareByteWidth(0)
	{
	}
};

class RenderDX11ConstantBuffers
{
public:
	RenderDX11ConstantBuffers();

	bool Initialize(
		const RenderDX11ConstantBuffersCreateDesc& Desc
	);

	void Shutdown();

	bool IsReady() const;

	bool Update(
		ID3D11Buffer* Buffer,
		const void* Data,
		size_t DataSize,
		const char* DebugName
	);

	void BindFrame();
	void BindTerrain();
	void BindWorld();

	ID3D11Buffer*& Frame()
	{
		return Frame_;
	}

	ID3D11Buffer*& Terrain()
	{
		return Terrain_;
	}

	ID3D11Buffer*& Object()
	{
		return Object_;
	}

	ID3D11Buffer*& Material()
	{
		return Material_;
	}

	ID3D11Buffer*& Light()
	{
		return Light_;
	}

	ID3D11Buffer*& Shadow()
	{
		return Shadow_;
	}

	ID3D11Buffer*& Water()
	{
		return Water_;
	}

	ID3D11Buffer*& Grass()
	{
		return Grass_;
	}

	ID3D11Buffer*& SunGlare()
	{
		return SunGlare_;
	}

private:
	RenderDX11ConstantBuffers(
		const RenderDX11ConstantBuffers&
	);

	RenderDX11ConstantBuffers& operator=(
		const RenderDX11ConstantBuffers&
	);

	bool CreateBuffer(
		UINT ByteWidth,
		const char* DebugName,
		ID3D11Buffer** OutBuffer
	);

	bool ValidateByteWidth(
		UINT ByteWidth,
		const char* DebugName
	);

	void LogFailure(
		const char* Stage,
		HRESULT Result
	);

private:
	ID3D11Buffer* Frame_;
	ID3D11Buffer* Terrain_;
	ID3D11Buffer* Object_;
	ID3D11Buffer* Material_;
	ID3D11Buffer* Light_;
	ID3D11Buffer* Shadow_;
	ID3D11Buffer* Water_;
	ID3D11Buffer* Grass_;
	ID3D11Buffer* SunGlare_;
};

RenderDX11ConstantBuffers&
RenderDX11_GetConstantBuffers();

#endif // LTS_STUDIO_DX11