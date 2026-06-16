#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <d3d9.h>
#include <d3dx9tex.h>

#include <string>

class RmlRenderDX9 final :
	public Rml::RenderInterface
{
public:
	RmlRenderDX9();
	~RmlRenderDX9() override;

	bool Init(
		IDirect3DDevice9* InDevice,
		const wchar_t* InDataRoot
	);

	void Shutdown();

	void BeginFrame(
		int Width,
		int Height
	);

	void EndFrame();

	void OnDeviceLost();

	void OnDeviceReset(
		int Width,
		int Height
	);

	void SetCharacterPreviewTexture(
		IDirect3DTexture9* Texture
	);

	Rml::CompiledGeometryHandle CompileGeometry(
		Rml::Span<const Rml::Vertex> Vertices,
		Rml::Span<const int> Indices
	) override;

	void RenderGeometry(
		Rml::CompiledGeometryHandle Geometry,
		Rml::Vector2f Translation,
		Rml::TextureHandle Texture
	) override;

	void ReleaseGeometry(
		Rml::CompiledGeometryHandle Geometry
	) override;

	Rml::TextureHandle LoadTexture(
		Rml::Vector2i& TextureDimensions,
		const Rml::String& Source
	) override;

	Rml::TextureHandle GenerateTexture(
		Rml::Span<const Rml::byte> Source,
		Rml::Vector2i SourceDimensions
	) override;

	void ReleaseTexture(
		Rml::TextureHandle Texture
	) override;

	void EnableScissorRegion(
		bool Enable
	) override;

	void SetScissorRegion(
		Rml::Rectanglei Region
	) override;

	void SetTransform(
		const Rml::Matrix4f* Transform
	) override;

private:
	struct FDx9Vertex
	{
		float X;
		float Y;
		float Z;
		DWORD Color;
		float U;
		float V;
	};

	struct FCompiledGeometry
	{
		IDirect3DVertexBuffer9* VertexBuffer =
			nullptr;

		IDirect3DIndexBuffer9* IndexBuffer =
			nullptr;

		int NumVertices = 0;
		int NumIndices = 0;
	};

	struct FTextureHandle
	{
		IDirect3DTexture9* Texture =
			nullptr;

		bool bExternalCharacterPreview =
			false;
	};

private:
	IDirect3DDevice9* Device = nullptr;

	IDirect3DStateBlock9* StateBlock =
		nullptr;

	IDirect3DTexture9* CharacterPreviewTexture =
		nullptr;

	int ViewWidth = 1;
	int ViewHeight = 1;

	bool bScissorEnabled = false;

	RECT ScissorRect{
		0,
		0,
		1,
		1
	};

	std::wstring DataRoot;

	static constexpr DWORD VertexFVF =
		D3DFVF_XYZ |
		D3DFVF_DIFFUSE |
		D3DFVF_TEX1;

	static DWORD ConvertColor(
		const Rml::ColourbPremultiplied& Color
	);

	static D3DMATRIX MakeIdentity();

	static D3DMATRIX MakeOrthoOffCenterLH(
		float Left,
		float Right,
		float Bottom,
		float Top,
		float ZNear,
		float ZFar
	);

	static D3DMATRIX MakeTranslation(
		float X,
		float Y,
		float Z
	);

	std::wstring ResolvePathW(
		const Rml::String& Path
	) const;

	bool CreateTextureFromRGBA(
		const unsigned char* PixelsRGBA,
		int Width,
		int Height,
		IDirect3DTexture9** OutTexture
	);

	bool LoadTextureD3DX(
		const std::wstring& Filename,
		Rml::Vector2i& OutDimensions,
		IDirect3DTexture9** OutTexture
	);

	void SetupRenderState();
};