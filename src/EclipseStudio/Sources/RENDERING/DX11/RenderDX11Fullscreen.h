#pragma once

struct ID3D11DepthStencilView;
struct ID3D11Device;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

class r3dDX11CommonStates;
class r3dDX11DrawContext;
class r3dDX11InputLayout;
class r3dDX11PixelShader;
class r3dDX11RenderTarget;
class r3dDX11ShaderLibrary;
class r3dDX11VertexBuffer;
class r3dDX11VertexShader;

class r3dDX11FullscreenPass final
{
public:
	r3dDX11FullscreenPass();
	~r3dDX11FullscreenPass();

	bool Init(ID3D11Device* device, r3dDX11DrawContext* drawContext, r3dDX11ShaderLibrary* shaderLibrary, r3dDX11CommonStates* commonStates);
	void Shutdown();

	bool Copy(
		ID3D11ShaderResourceView* source,
		ID3D11RenderTargetView* destination,
		ID3D11DepthStencilView* depthStencil,
		int width,
		int height
	);

	bool Copy(ID3D11ShaderResourceView* source, r3dDX11RenderTarget& destination);

	bool IsInitialized() const;

private:
	bool CreateShadersAndLayout(ID3D11Device* device);
	bool CreateGeometry(ID3D11Device* device);

private:
	r3dDX11DrawContext* DrawContext = nullptr;
	r3dDX11ShaderLibrary* ShaderLibrary = nullptr;
	r3dDX11CommonStates* CommonStates = nullptr;
	r3dDX11VertexShader* CopyVS = nullptr;
	r3dDX11PixelShader* CopyPS = nullptr;
	r3dDX11InputLayout* InputLayout = nullptr;
	r3dDX11VertexBuffer* VertexBuffer = nullptr;
	bool bInitialized = false;
};
