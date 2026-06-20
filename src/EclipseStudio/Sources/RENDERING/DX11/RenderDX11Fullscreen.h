#pragma once

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

class r3dDX11RenderTarget;

class r3dDX11FullscreenPass final
{
public:
	r3dDX11FullscreenPass();
	~r3dDX11FullscreenPass();

	bool Init(ID3D11Device* device, ID3D11DeviceContext* context);
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
	bool CreateShaders(ID3D11Device* device);
	bool CreateGeometry(ID3D11Device* device);
	bool CreateStates(ID3D11Device* device);

private:
	ID3D11DeviceContext* Context = nullptr;
	ID3D11VertexShader* CopyVS = nullptr;
	ID3D11PixelShader* CopyPS = nullptr;
	ID3D11InputLayout* InputLayout = nullptr;
	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11SamplerState* SamplerState = nullptr;
	ID3D11RasterizerState* RasterizerState = nullptr;
	ID3D11DepthStencilState* DepthStencilState = nullptr;
	ID3D11BlendState* BlendState = nullptr;
	bool bInitialized = false;
};

