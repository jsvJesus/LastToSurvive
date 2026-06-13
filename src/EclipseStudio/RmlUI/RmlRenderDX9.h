#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include <d3d9.h>
#include <d3dx9tex.h>
#include <string>

class RmlRenderDX9 final : public Rml::RenderInterface
{
public:
	RmlRenderDX9();
	~RmlRenderDX9() override;

	bool Init(IDirect3DDevice9* InDevice, const wchar_t* InDataRoot);
	void Shutdown();

	void BeginFrame(int Width, int Height);
	void EndFrame();

	void OnDeviceLost();
	void OnDeviceReset(int Width, int Height);

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;

	void SetTransform(const Rml::Matrix4f* transform) override;

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
		IDirect3DVertexBuffer9* VertexBuffer = nullptr;
		IDirect3DIndexBuffer9* IndexBuffer = nullptr;
		int NumVertices = 0;
		int NumIndices = 0;
	};

	IDirect3DDevice9* Device = nullptr;
	IDirect3DStateBlock9* StateBlock = nullptr;

	int ViewWidth = 1;
	int ViewHeight = 1;

	bool bScissorEnabled = false;
	RECT ScissorRect{ 0, 0, 1, 1 };

	std::wstring DataRoot;

	static constexpr DWORD VertexFVF = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;

	static DWORD ConvertColor(const Rml::ColourbPremultiplied& Color);
	static D3DMATRIX MakeIdentity();
	static D3DMATRIX MakeOrthoOffCenterLH(float Left, float Right, float Bottom, float Top, float ZNear, float ZFar);
	static D3DMATRIX MakeTranslation(float X, float Y, float Z);

	std::wstring ResolvePathW(const Rml::String& path) const;
	bool CreateTextureFromRGBA(const unsigned char* PixelsRGBA, int Width, int Height, IDirect3DTexture9** OutTexture);
	bool LoadTextureD3DX(const std::wstring& Filename, Rml::Vector2i& OutDimensions, IDirect3DTexture9** OutTexture);

	void SetupRenderState();
};