#pragma once

#include "RENDERING/DX11/RenderDX11Fullscreen.h"
#include "RENDERING/DX11/RenderDX11Resources.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11DepthStencilView;
struct ID3D11RenderTargetView;

class r3dDX11FrameResources final
{
public:
	r3dDX11FrameResources();
	~r3dDX11FrameResources();

	bool Init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height);
	void Shutdown();
	bool Resize(int width, int height);

	void BeginScene(float clearR, float clearG, float clearB, float clearA);
	bool CopySceneToTemp();
	bool CopyTempToScene();
	bool CopySceneToBlurred();
	bool CopySceneToBackBuffer(ID3D11RenderTargetView* backBuffer, ID3D11DepthStencilView* depthStencil);

	r3dDX11RenderTarget& GetSceneColor();
	r3dDX11RenderTarget& GetTempColor();
	r3dDX11RenderTarget& GetBlurredColor();

	const r3dDX11RenderTarget& GetSceneColor() const;
	const r3dDX11RenderTarget& GetTempColor() const;
	const r3dDX11RenderTarget& GetBlurredColor() const;

	int GetWidth() const;
	int GetHeight() const;
	bool IsInitialized() const;

private:
	bool CreateTargets();

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* Context = nullptr;

	r3dDX11RenderTarget SceneColor;
	r3dDX11RenderTarget TempColor;
	r3dDX11RenderTarget BlurredColor;
	r3dDX11FullscreenPass FullscreenPass;

	int Width = 0;
	int Height = 0;
	bool bInitialized = false;
};

