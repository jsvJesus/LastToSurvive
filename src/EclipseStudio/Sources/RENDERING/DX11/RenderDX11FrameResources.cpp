#include "r3dPCH.h"

#include "RENDERING/DX11/RenderDX11FrameResources.h"

#include <d3d11_1.h>

#include <algorithm>

r3dDX11FrameResources::r3dDX11FrameResources()
{
}

r3dDX11FrameResources::~r3dDX11FrameResources()
{
	Shutdown();
}

bool r3dDX11FrameResources::Init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height)
{
	if (bInitialized)
		return true;

	if (!device || !context || width <= 0 || height <= 0)
		return false;

	Device = device;
	Context = context;
	Width = std::max(1, width);
	Height = std::max(1, height);

	if (!CreateTargets() || !FullscreenPass.Init(Device, Context))
	{
		Shutdown();
		return false;
	}

	bInitialized = true;
	return true;
}

void r3dDX11FrameResources::Shutdown()
{
	FullscreenPass.Shutdown();
	BlurredColor.Shutdown();
	TempColor.Shutdown();
	SceneColor.Shutdown();

	Device = nullptr;
	Context = nullptr;
	Width = 0;
	Height = 0;
	bInitialized = false;
}

bool r3dDX11FrameResources::Resize(int width, int height)
{
	if (!Device || !Context)
		return false;

	width = std::max(1, width);
	height = std::max(1, height);

	if (bInitialized && width == Width && height == Height)
		return true;

	SceneColor.Shutdown();
	TempColor.Shutdown();
	BlurredColor.Shutdown();

	Width = width;
	Height = height;

	if (!CreateTargets())
	{
		Shutdown();
		return false;
	}

	bInitialized = true;
	return true;
}

void r3dDX11FrameResources::BeginScene(float clearR, float clearG, float clearB, float clearA)
{
	if (!bInitialized || !Context || !SceneColor.GetRTV())
		return;

	const float clearColor[4] = { clearR, clearG, clearB, clearA };
	ID3D11RenderTargetView* sceneRTV = SceneColor.GetRTV();
	ID3D11DepthStencilView* sceneDSV = SceneColor.GetDSV();

	Context->OMSetRenderTargets(1, &sceneRTV, sceneDSV);
	Context->ClearRenderTargetView(sceneRTV, clearColor);

	if (sceneDSV)
		Context->ClearDepthStencilView(sceneDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(Width);
	viewport.Height = static_cast<float>(Height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	Context->RSSetViewports(1, &viewport);
}

bool r3dDX11FrameResources::CopySceneToTemp()
{
	return FullscreenPass.Copy(SceneColor.GetSRV(), TempColor);
}

bool r3dDX11FrameResources::CopyTempToScene()
{
	return FullscreenPass.Copy(TempColor.GetSRV(), SceneColor);
}

bool r3dDX11FrameResources::CopySceneToBlurred()
{
	return FullscreenPass.Copy(SceneColor.GetSRV(), BlurredColor);
}

bool r3dDX11FrameResources::CopySceneToBackBuffer(ID3D11RenderTargetView* backBuffer, ID3D11DepthStencilView* depthStencil)
{
	return FullscreenPass.Copy(SceneColor.GetSRV(), backBuffer, depthStencil, Width, Height);
}

r3dDX11RenderTarget& r3dDX11FrameResources::GetSceneColor()
{
	return SceneColor;
}

r3dDX11RenderTarget& r3dDX11FrameResources::GetTempColor()
{
	return TempColor;
}

r3dDX11RenderTarget& r3dDX11FrameResources::GetBlurredColor()
{
	return BlurredColor;
}

const r3dDX11RenderTarget& r3dDX11FrameResources::GetSceneColor() const
{
	return SceneColor;
}

const r3dDX11RenderTarget& r3dDX11FrameResources::GetTempColor() const
{
	return TempColor;
}

const r3dDX11RenderTarget& r3dDX11FrameResources::GetBlurredColor() const
{
	return BlurredColor;
}

int r3dDX11FrameResources::GetWidth() const
{
	return Width;
}

int r3dDX11FrameResources::GetHeight() const
{
	return Height;
}

bool r3dDX11FrameResources::IsInitialized() const
{
	return bInitialized;
}

bool r3dDX11FrameResources::CreateTargets()
{
	const DXGI_FORMAT hdrFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	const DXGI_FORMAT depthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	if (!SceneColor.Create(Device, "DX11.SceneColor", Width, Height, hdrFormat, true, depthFormat))
		return false;

	if (!TempColor.Create(Device, "DX11.TempColor", Width, Height, hdrFormat, false, DXGI_FORMAT_UNKNOWN))
		return false;

	if (!BlurredColor.Create(Device, "DX11.BlurredColor", Width, Height, hdrFormat, false, DXGI_FORMAT_UNKNOWN))
		return false;

	return true;
}

