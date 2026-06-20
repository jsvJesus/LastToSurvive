#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <string>

struct ID3D11BlendState;
struct ID3D11Buffer;
struct ID3D11DepthStencilState;
struct ID3D11DepthStencilView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11InputLayout;
struct ID3D11PixelShader;
struct ID3D11RasterizerState;
struct ID3D11RenderTargetView;
struct ID3D11SamplerState;
struct ID3D11ShaderResourceView;
struct ID3D11VertexShader;

class RmlRenderDX11 final : public Rml::RenderInterface
{
public:
	RmlRenderDX11();
	~RmlRenderDX11() override;

	bool Init(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* dataRoot);
	void Shutdown();

	void BeginFrame(ID3D11RenderTargetView* renderTarget, ID3D11DepthStencilView* depthStencil, int width, int height);
	void EndFrame();

	void SetCharacterPreviewTexture(ID3D11ShaderResourceView* texture, int width, int height);
	void SetCharacterPortraitTexture(ID3D11ShaderResourceView* texture, int width, int height);

	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
	void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& textureDimensions, const Rml::String& source) override;
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i sourceDimensions) override;
	void ReleaseTexture(Rml::TextureHandle texture) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;
	void SetTransform(const Rml::Matrix4f* transform) override;

private:
	struct FVertex
	{
		float Position[3];
		unsigned int Color;
		float TexCoord[2];
	};

	struct FConstants
	{
		float Transform[16];
		float Translate[4];
		float TextureEnabled[4];
	};

	struct FCompiledGeometry
	{
		ID3D11Buffer* VertexBuffer = nullptr;
		ID3D11Buffer* IndexBuffer = nullptr;
		unsigned int NumIndices = 0;
	};

	struct FTextureHandle
	{
		ID3D11ShaderResourceView* SRV = nullptr;
		bool bExternalCharacterPreview = false;
		bool bExternalCharacterPortrait = false;
		int Width = 0;
		int Height = 0;
	};

private:
	bool CreateDeviceObjects();
	void ReleaseDeviceObjects();
	void SetupPipeline(bool textureEnabled);

	bool LoadTextureFromFile(const std::wstring& filename, Rml::Vector2i& dimensions, ID3D11ShaderResourceView** outSRV);
	std::wstring ResolvePathW(const Rml::String& path) const;

	static unsigned int ConvertColor(const Rml::ColourbPremultiplied& color);
	static void SetIdentity(float outMatrix[16]);
	static void CopyTransform(float outMatrix[16], const Rml::Matrix4f& transform);

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* Context = nullptr;

	ID3D11VertexShader* VertexShader = nullptr;
	ID3D11PixelShader* PixelShader = nullptr;
	ID3D11InputLayout* InputLayout = nullptr;
	ID3D11Buffer* ConstantBuffer = nullptr;
	ID3D11SamplerState* SamplerState = nullptr;
	ID3D11BlendState* BlendState = nullptr;
	ID3D11DepthStencilState* DepthStencilState = nullptr;
	ID3D11RasterizerState* RasterizerState = nullptr;
	ID3D11RasterizerState* ScissorRasterizerState = nullptr;

	ID3D11ShaderResourceView* CharacterPreviewTexture = nullptr;
	ID3D11ShaderResourceView* CharacterPortraitTexture = nullptr;
	int CharacterPreviewWidth = 0;
	int CharacterPreviewHeight = 0;
	int CharacterPortraitWidth = 0;
	int CharacterPortraitHeight = 0;

	ID3D11RenderTargetView* ActiveRenderTarget = nullptr;
	ID3D11DepthStencilView* ActiveDepthStencil = nullptr;

	float ViewportTopLeftX = 0.0f;
	float ViewportTopLeftY = 0.0f;
	float ViewportWidth = 1.0f;
	float ViewportHeight = 1.0f;
	long ScissorLeft = 0;
	long ScissorTop = 0;
	long ScissorRight = 1;
	long ScissorBottom = 1;

	float CurrentTransform[16]{};
	std::wstring DataRoot;

	int ViewWidth = 1;
	int ViewHeight = 1;
	bool bInitialized = false;
	bool bFrameOpen = false;
	bool bScissorEnabled = false;
};
