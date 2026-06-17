#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <d3d9.h>
#include <d3dx9.h>

#include <string>
#include <vector>

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

	void SetCharacterPortraitTexture(
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

	void EnableClipMask(
		bool Enable
	) override;

	void RenderToClipMask(
		Rml::ClipMaskOperation Operation,
		Rml::CompiledGeometryHandle Geometry,
		Rml::Vector2f Translation
	) override;

	void SetTransform(
		const Rml::Matrix4f* Transform
	) override;

	Rml::LayerHandle PushLayer() override;

	void CompositeLayers(
		Rml::LayerHandle Source,
		Rml::LayerHandle Destination,
		Rml::BlendMode BlendMode,
		Rml::Span<const Rml::CompiledFilterHandle> Filters
	) override;

	void PopLayer() override;

	Rml::TextureHandle SaveLayerAsTexture() override;
	Rml::CompiledFilterHandle SaveLayerAsMaskImage() override;

	Rml::CompiledFilterHandle CompileFilter(
		const Rml::String& Name,
		const Rml::Dictionary& Parameters
	) override;

	void ReleaseFilter(
		Rml::CompiledFilterHandle Filter
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

	struct FScreenVertex
	{
		float X;
		float Y;
		float Z;
		float RHW;

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

		bool bExternalCharacterPortrait =
			false;
	};

	struct FRenderLayer
	{
		IDirect3DTexture9* Texture =
			nullptr;

		IDirect3DSurface9* Surface =
			nullptr;

		int Width = 0;
		int Height = 0;
	};

	enum class ECompiledFilterType
	{
		Invalid = 0,
		Opacity,
		Blur,
		DropShadow,
		MaskImage
	};

	struct FCompiledFilter
	{
		ECompiledFilterType Type =
			ECompiledFilterType::Invalid;

		float Opacity =
			1.0f;

		float Sigma =
			0.0f;

		Rml::Vector2f Offset =
			Rml::Vector2f(
				0.0f,
				0.0f
			);

		Rml::ColourbPremultiplied Color{};

		IDirect3DTexture9* MaskTexture =
			nullptr;
	};

	struct FPostProcessTarget
	{
		IDirect3DTexture9* Texture =
			nullptr;

		IDirect3DSurface9* Surface =
			nullptr;

		int Width = 0;
		int Height = 0;
	};

private:
	static constexpr DWORD VertexFVF =
		D3DFVF_XYZ |
		D3DFVF_DIFFUSE |
		D3DFVF_TEX1;

	static constexpr DWORD ScreenVertexFVF =
		D3DFVF_XYZRHW |
		D3DFVF_DIFFUSE |
		D3DFVF_TEX1;

private:
	IDirect3DDevice9* Device =
		nullptr;

	IDirect3DStateBlock9* StateBlock =
		nullptr;

	IDirect3DSurface9* BaseRenderTarget =
		nullptr;

	IDirect3DSurface9* OriginalDepthStencil =
		nullptr;

	IDirect3DSurface9* SharedDepthStencil =
		nullptr;

	IDirect3DTexture9* CharacterPreviewTexture =
		nullptr;

	IDirect3DTexture9* CharacterPortraitTexture =
		nullptr;

	std::vector<FRenderLayer> LayerPool;

	size_t ActiveLayerCount =
		0;

	FPostProcessTarget PostProcessTargets[3];
	FPostProcessTarget LayerCompositeScratch;

	IDirect3DPixelShader9* BlurPixelShader =
		nullptr;

	IDirect3DPixelShader9* ShadowPixelShader =
		nullptr;

	int ViewWidth = 1;
	int ViewHeight = 1;

	bool bFrameOpen =
		false;

	bool bScissorEnabled =
		false;

	bool bClipMaskEnabled =
		false;

	unsigned int ClipMaskReference =
		0;

	RECT ScissorRect{
		0,
		0,
		1,
		1
	};

	D3DMATRIX CurrentTransform{};

	std::wstring DataRoot;

private:
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

	static D3DMATRIX ConvertTransform(
		const Rml::Matrix4f& Transform
	);

	void SetupRenderState();
	void ApplyClipMaskState();

	void BindLayer(
		Rml::LayerHandle Layer
	);

	Rml::LayerHandle GetTopLayerHandle() const;

	IDirect3DSurface9* GetLayerSurface(
		Rml::LayerHandle Layer
	) const;

	IDirect3DTexture9* GetLayerTexture(
		Rml::LayerHandle Layer
	) const;

	bool EnsureLayer(
		size_t LayerIndex
	);

	bool EnsureSharedDepthStencil();

	void ReleaseLayer(
		FRenderLayer& Layer
	);

	void ReleaseLayerResources();
	void ReleaseSharedDepthStencil();

	bool CreateRenderTargetTexture(
		int Width,
		int Height,
		IDirect3DTexture9** OutTexture,
		IDirect3DSurface9** OutSurface
	);

	bool CopySurface(
		IDirect3DSurface9* Source,
		IDirect3DSurface9* Destination,
		const RECT* SourceRectangle,
		const RECT* DestinationRectangle
	);

	void DrawLayerTexture(
		IDirect3DTexture9* SourceTexture,
		IDirect3DTexture9* MaskTexture,
		float Opacity,
		bool bEnableBlend
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

	bool CreateFilterShaders();

	bool CreatePixelShader(
		const char* SourceCode,
		IDirect3DPixelShader9** OutShader
	);

	void ReleaseFilterShaders();

	bool EnsurePostProcessTargets();
	void ReleasePostProcessTargets();
	bool EnsureLayerCompositeScratch();
	void ReleaseLayerCompositeScratch();

	int FindPostProcessTarget(
		int ExcludeA,
		int ExcludeB = -1
	) const;

	void ReleasePostProcessTarget(
		FPostProcessTarget& Target
	);

	void DrawPostProcessQuad(
		IDirect3DTexture9* SourceTexture,
		IDirect3DSurface9* DestinationSurface,
		IDirect3DPixelShader9* PixelShader,
		float OffsetX,
		float OffsetY,
		float Opacity,
		bool bEnableBlend,
		bool bClearDestination
	);

	void RenderBlurPass(
		IDirect3DTexture9* SourceTexture,
		IDirect3DSurface9* DestinationSurface,
		float Sigma,
		bool bHorizontal
	);

	bool ApplyGaussianBlur(
		IDirect3DTexture9* SourceTexture,
		int SourcePostProcessIndex,
		float Sigma,
		IDirect3DTexture9*& OutTexture,
		int& OutPostProcessIndex
	);

	static void CalculateGaussianWeights(
		float Sigma,
		float OutWeights[5]
	);
};