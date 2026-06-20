#pragma once

#ifndef _WINDEF_
struct HWND__;
typedef HWND__* HWND;
#endif

struct ID3D11DepthStencilView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct ID3D11Texture2D;
struct IDXGISwapChain;

class r3dDX11Device final
{
public:
	r3dDX11Device();
	~r3dDX11Device();

	bool Init(HWND windowHandle, int width, int height, bool fullscreen, bool enableDebug);
	void Shutdown();

	bool Resize(int width, int height);
	void Present(bool vsync);

	void BeginBackBuffer(float clearR, float clearG, float clearB, float clearA, bool clearDepth);

	ID3D11Device* GetDevice() const;
	ID3D11DeviceContext* GetContext() const;
	IDXGISwapChain* GetSwapChain() const;
	ID3D11RenderTargetView* GetBackBufferRTV() const;
	ID3D11DepthStencilView* GetDepthStencilView() const;

	int GetWidth() const;
	int GetHeight() const;
	bool IsInitialized() const;

private:
	bool CreateSwapChainResources();
	void ReleaseSwapChainResources();

private:
	HWND WindowHandle = nullptr;
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* Context = nullptr;
	IDXGISwapChain* SwapChain = nullptr;
	ID3D11RenderTargetView* BackBufferRTV = nullptr;
	ID3D11Texture2D* DepthStencilTexture = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;

	int Width = 1;
	int Height = 1;
	bool bFullscreen = false;
	bool bInitialized = false;
};
