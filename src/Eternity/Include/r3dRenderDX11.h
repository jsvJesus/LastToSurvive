#pragma once

#include <d3d11.h>
#include <dxgi.h>

struct r3dDX11DeviceCreateParams
{
	HWND Window;
	unsigned int Width;
	unsigned int Height;
	bool Windowed;
	bool VSync;
	bool EnableDebugLayer;

	r3dDX11DeviceCreateParams();
};

// Owns the single DX11 device and presentation chain used by the engine.
// Rendering subsystems borrow these interfaces; they must not create devices.
class r3dRenderDX11
{
public:
	r3dRenderDX11();
	~r3dRenderDX11();

	bool Initialize(const r3dDX11DeviceCreateParams& Params);
	void Shutdown();
	bool Resize(unsigned int Width, unsigned int Height);
	bool BeginFrame(const float ClearColor[4]);
	bool CopyToBackBuffer(ID3D11Texture2D* Source);
	bool Present();

	bool IsInitialized() const;
	bool IsOccluded() const;
	unsigned int GetWidth() const;
	unsigned int GetHeight() const;
	D3D_FEATURE_LEVEL GetFeatureLevel() const;
	ID3D11Device* GetDevice() const;
	ID3D11DeviceContext* GetContext() const;
	IDXGISwapChain* GetSwapChain() const;
	ID3D11Texture2D* GetBackBufferTexture() const;
	ID3D11RenderTargetView* GetBackBufferRTV() const;

private:
	r3dRenderDX11(const r3dRenderDX11&);
	r3dRenderDX11& operator=(const r3dRenderDX11&);

	bool CreateBackBuffer();
	void ReleaseBackBuffer();

	HWND Window_;
	unsigned int Width_;
	unsigned int Height_;
	bool VSync_;
	bool Occluded_;
	D3D_FEATURE_LEVEL FeatureLevel_;
	ID3D11Device* Device_;
	ID3D11DeviceContext* Context_;
	IDXGISwapChain* SwapChain_;
	ID3D11Texture2D* BackBufferTexture_;
	ID3D11RenderTargetView* BackBufferRTV_;
	HMODULE D3D11Module_;
};

r3dRenderDX11& r3dGetRenderDX11();
