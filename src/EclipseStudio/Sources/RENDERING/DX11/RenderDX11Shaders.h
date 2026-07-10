#pragma once

#include "r3dRendererConfig.h"

#if LTS_STUDIO_DX11

#include <D3D11.h>

class RenderDX11Shaders
{
public:
	RenderDX11Shaders();

	bool Initialize();
	void Shutdown();

	bool IsReady() const;

	/*
	 * Sun Glare пока создаётся лениво,
	 * только при первом вызове эффекта.
	 */
	bool EnsureSunGlare();

	ID3D11VertexShader*& ClearVS()
	{
		return ClearVS_;
	}

	ID3D11PixelShader*& ClearPS()
	{
		return ClearPS_;
	}

	ID3D11VertexShader*& LightingVS()
	{
		return LightingVS_;
	}

	ID3D11PixelShader*& LightingPS()
	{
		return LightingPS_;
	}

	ID3D11VertexShader*& TonemapVS()
	{
		return TonemapVS_;
	}

	ID3D11PixelShader*& TonemapPS()
	{
		return TonemapPS_;
	}

	ID3D11VertexShader*& TerrainVS()
	{
		return TerrainVS_;
	}

	ID3D11PixelShader*& TerrainPS()
	{
		return TerrainPS_;
	}

	ID3D11InputLayout*& TerrainInputLayout()
	{
		return TerrainInputLayout_;
	}

	ID3D11VertexShader*& StaticMeshVS()
	{
		return StaticMeshVS_;
	}

	ID3D11PixelShader*& StaticMeshPS()
	{
		return StaticMeshPS_;
	}

	ID3D11InputLayout*& StaticMeshInputLayout()
	{
		return StaticMeshInputLayout_;
	}

	ID3D11VertexShader*& SunGlareVS()
	{
		return SunGlareVS_;
	}

	ID3D11PixelShader*& SunGlarePS()
	{
		return SunGlarePS_;
	}

private:
	RenderDX11Shaders(
		const RenderDX11Shaders&
	);

	RenderDX11Shaders& operator=(
		const RenderDX11Shaders&
	);

	bool CreateFileProgram(
		const char* RelativeFileName,
		const char* DebugName,
		const D3D11_INPUT_ELEMENT_DESC* InputElements,
		UINT InputElementCount,
		ID3D11VertexShader** OutVS,
		ID3D11PixelShader** OutPS,
		ID3D11InputLayout** OutInputLayout
	);

	bool CreateMemoryProgram(
		const char* DebugName,
		const char* Source,
		const char* VSProfile,
		const char* PSProfile,
		ID3D11VertexShader** OutVS,
		ID3D11PixelShader** OutPS
	);

	bool CompileFromFile(
		const char* RelativeFileName,
		const char* EntryPoint,
		const char* Profile,
		ID3DBlob** OutBlob
	);

	bool CompileFromMemory(
		const char* DebugName,
		const char* Source,
		const char* EntryPoint,
		const char* Profile,
		ID3DBlob** OutBlob
	);

	bool LoadShaderSource(
		const char* FileName,
		char** OutData,
		UINT* OutSize
	);

	void MakeShaderFileName(
		char* OutFileName,
		size_t OutFileNameSize,
		const char* RelativeFileName
	);

	void LogFailure(
		const char* Stage,
		HRESULT Result
	);

private:
	ID3D11VertexShader* ClearVS_;
	ID3D11PixelShader* ClearPS_;

	ID3D11VertexShader* LightingVS_;
	ID3D11PixelShader* LightingPS_;

	ID3D11VertexShader* TonemapVS_;
	ID3D11PixelShader* TonemapPS_;

	ID3D11VertexShader* TerrainVS_;
	ID3D11PixelShader* TerrainPS_;
	ID3D11InputLayout* TerrainInputLayout_;

	ID3D11VertexShader* StaticMeshVS_;
	ID3D11PixelShader* StaticMeshPS_;
	ID3D11InputLayout* StaticMeshInputLayout_;

	ID3D11VertexShader* SunGlareVS_;
	ID3D11PixelShader* SunGlarePS_;
};

RenderDX11Shaders& RenderDX11_GetShaders();

#endif // LTS_STUDIO_DX11