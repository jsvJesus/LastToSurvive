#pragma once

#include <d3d11_1.h>

#include <string>

enum r3dDX11TextureBindFlags
{
	R3D_DX11_BIND_SHADER_RESOURCE = 1 << 0,
	R3D_DX11_BIND_RENDER_TARGET = 1 << 1,
	R3D_DX11_BIND_DEPTH_STENCIL = 1 << 2
};

class r3dDX11Texture2D final
{
public:
	r3dDX11Texture2D();
	~r3dDX11Texture2D();

	bool Create(
		ID3D11Device* device,
		int width,
		int height,
		DXGI_FORMAT format,
		unsigned int bindFlags,
		const char* debugName = nullptr,
		int mipLevels = 1
	);

	void Shutdown();

	ID3D11Texture2D* GetTexture() const;
	ID3D11ShaderResourceView* GetSRV() const;
	ID3D11RenderTargetView* GetRTV() const;
	ID3D11DepthStencilView* GetDSV() const;

	int GetWidth() const;
	int GetHeight() const;
	DXGI_FORMAT GetFormat() const;
	bool IsValid() const;

private:
	bool CreateViews(ID3D11Device* device, unsigned int bindFlags);

private:
	ID3D11Texture2D* Texture = nullptr;
	ID3D11ShaderResourceView* SRV = nullptr;
	ID3D11RenderTargetView* RTV = nullptr;
	ID3D11DepthStencilView* DSV = nullptr;
	int Width = 0;
	int Height = 0;
	DXGI_FORMAT Format;
};

class r3dDX11RenderTarget final
{
public:
	r3dDX11RenderTarget();
	~r3dDX11RenderTarget();

	bool Create(
		ID3D11Device* device,
		const char* name,
		int width,
		int height,
		DXGI_FORMAT colorFormat,
		bool withDepth,
		DXGI_FORMAT depthFormat
	);

	void Shutdown();

	r3dDX11Texture2D& GetColorTexture();
	const r3dDX11Texture2D& GetColorTexture() const;
	r3dDX11Texture2D& GetDepthTexture();
	const r3dDX11Texture2D& GetDepthTexture() const;

	ID3D11ShaderResourceView* GetSRV() const;
	ID3D11RenderTargetView* GetRTV() const;
	ID3D11DepthStencilView* GetDSV() const;

	int GetWidth() const;
	int GetHeight() const;
	const std::string& GetName() const;
	bool IsValid() const;

private:
	std::string Name;
	r3dDX11Texture2D ColorTexture;
	r3dDX11Texture2D DepthTexture;
};
