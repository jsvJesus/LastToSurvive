#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <d3d9.h>
#include <d3dx9.h>

#include <string>

class RmlEditorRenderDX9 final : public Rml::RenderInterface
{
public:
	RmlEditorRenderDX9();
	~RmlEditorRenderDX9() override;

	bool Initialize(
		IDirect3DDevice9* Device,
		const wchar_t* DataRoot
	);

	void Shutdown();

	void BeginFrame(int Width, int Height);
	void EndFrame();

	void OnDeviceLost();
	void OnDeviceReset(int Width, int Height);

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

	void EnableScissorRegion(bool Enable) override;
	void SetScissorRegion(Rml::Rectanglei Region) override;

	void SetTransform(const Rml::Matrix4f* Transform) override;

private:
	struct DX9Vertex
	{
		float X = 0.0f;
		float Y = 0.0f;
		float Z = 0.0f;

		DWORD Color = 0;

		float U = 0.0f;
		float V = 0.0f;
	};

	struct CompiledGeometry
	{
		IDirect3DVertexBuffer9* VertexBuffer = nullptr;
		IDirect3DIndexBuffer9* IndexBuffer = nullptr;

		int VertexCount = 0;
		int IndexCount = 0;
	};

	static constexpr DWORD VertexFormat =
		D3DFVF_XYZ |
		D3DFVF_DIFFUSE |
		D3DFVF_TEX1;

	IDirect3DDevice9* Device = nullptr;
	IDirect3DStateBlock9* StateBlock = nullptr;

	std::wstring RootDirectory;

	int ViewWidth = 1;
	int ViewHeight = 1;

	bool ScissorEnabled = false;
	RECT ScissorRectangle{0, 0, 1, 1};

	void SetupRenderState();

	std::wstring ResolvePath(
		const Rml::String& Path
	) const;

	bool CreateTextureFromRGBA(
		const unsigned char* Source,
		int Width,
		int Height,
		IDirect3DTexture9** OutputTexture
	);

	static DWORD ConvertColor(
		const Rml::ColourbPremultiplied& Color
	);

	static D3DMATRIX IdentityMatrix();

	static D3DMATRIX OrthographicMatrix(
		float Left,
		float Right,
		float Bottom,
		float Top,
		float NearPlane,
		float FarPlane
	);

	static D3DMATRIX TranslationMatrix(
		float X,
		float Y,
		float Z
	);
};