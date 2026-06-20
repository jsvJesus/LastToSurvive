#include "r3dPCH.h"

#include "RENDERING/DX11/RenderDX11Device.h"
#include "RENDERING/DX11/RenderDX11Platform.h"

#include <algorithm>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace
{
	template <typename T>
	void SafeReleaseDX11(T*& value)
	{
		if (value)
		{
			value->Release();
			value = nullptr;
		}
	}
}

r3dDX11Device::r3dDX11Device()
{
}

r3dDX11Device::~r3dDX11Device()
{
	Shutdown();
}

bool r3dDX11Device::Init(HWND windowHandle, int width, int height, bool fullscreen, bool enableDebug)
{
	if (bInitialized)
		return true;

	if (!windowHandle)
		return false;

	WindowHandle = windowHandle;
	Width = std::max(1, width);
	Height = std::max(1, height);
	bFullscreen = fullscreen;

	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferDesc.Width = static_cast<UINT>(Width);
	swapChainDesc.BufferDesc.Height = static_cast<UINT>(Height);
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = WindowHandle;
	swapChainDesc.Windowed = fullscreen ? FALSE : TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	const D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	D3D_FEATURE_LEVEL createdFeatureLevel = D3D_FEATURE_LEVEL_10_0;
	UINT createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	if (enableDebug)
		createFlags |= D3D11_CREATE_DEVICE_DEBUG;

	HRESULT result = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createFlags,
		featureLevels,
		static_cast<UINT>(_countof(featureLevels)),
		D3D11_SDK_VERSION,
		&swapChainDesc,
		&SwapChain,
		&Device,
		&createdFeatureLevel,
		&Context
	);

	if (FAILED(result) && enableDebug)
	{
		createFlags &= ~D3D11_CREATE_DEVICE_DEBUG;

		result = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			createFlags,
			featureLevels,
			static_cast<UINT>(_countof(featureLevels)),
			D3D11_SDK_VERSION,
			&swapChainDesc,
			&SwapChain,
			&Device,
			&createdFeatureLevel,
			&Context
		);
	}

	if (FAILED(result) || !Device || !Context || !SwapChain)
	{
		Shutdown();
		return false;
	}

	if (!CreateSwapChainResources())
	{
		Shutdown();
		return false;
	}

	bInitialized = true;
	return true;
}

void r3dDX11Device::Shutdown()
{
	if (SwapChain)
		SwapChain->SetFullscreenState(FALSE, nullptr);

	ReleaseSwapChainResources();
	SafeReleaseDX11(SwapChain);
	SafeReleaseDX11(Context);
	SafeReleaseDX11(Device);

	WindowHandle = nullptr;
	Width = 1;
	Height = 1;
	bFullscreen = false;
	bInitialized = false;
}

bool r3dDX11Device::Resize(int width, int height)
{
	if (!SwapChain || !Context)
		return false;

	Width = std::max(1, width);
	Height = std::max(1, height);

	Context->OMSetRenderTargets(0, nullptr, nullptr);
	ReleaseSwapChainResources();

	HRESULT result = SwapChain->ResizeBuffers(
		0,
		static_cast<UINT>(Width),
		static_cast<UINT>(Height),
		DXGI_FORMAT_UNKNOWN,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	);

	if (FAILED(result))
		return false;

	return CreateSwapChainResources();
}

void r3dDX11Device::Present(bool vsync)
{
	if (SwapChain)
		SwapChain->Present(vsync ? 1 : 0, 0);
}

void r3dDX11Device::BeginBackBuffer(float clearR, float clearG, float clearB, float clearA, bool clearDepth)
{
	if (!Context || !BackBufferRTV)
		return;

	const float color[4] = { clearR, clearG, clearB, clearA };
	Context->OMSetRenderTargets(1, &BackBufferRTV, DepthStencilView);
	Context->ClearRenderTargetView(BackBufferRTV, color);

	if (clearDepth && DepthStencilView)
		Context->ClearDepthStencilView(DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(Width);
	viewport.Height = static_cast<float>(Height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	Context->RSSetViewports(1, &viewport);
}

ID3D11Device* r3dDX11Device::GetDevice() const
{
	return Device;
}

ID3D11DeviceContext* r3dDX11Device::GetContext() const
{
	return Context;
}

IDXGISwapChain* r3dDX11Device::GetSwapChain() const
{
	return SwapChain;
}

ID3D11RenderTargetView* r3dDX11Device::GetBackBufferRTV() const
{
	return BackBufferRTV;
}

ID3D11DepthStencilView* r3dDX11Device::GetDepthStencilView() const
{
	return DepthStencilView;
}

int r3dDX11Device::GetWidth() const
{
	return Width;
}

int r3dDX11Device::GetHeight() const
{
	return Height;
}

bool r3dDX11Device::IsInitialized() const
{
	return bInitialized;
}

bool r3dDX11Device::CreateSwapChainResources()
{
	if (!Device || !Context || !SwapChain)
		return false;

	ID3D11Texture2D* backBuffer = nullptr;
	HRESULT result = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));

	if (FAILED(result) || !backBuffer)
		return false;

	result = Device->CreateRenderTargetView(backBuffer, nullptr, &BackBufferRTV);
	backBuffer->Release();

	if (FAILED(result) || !BackBufferRTV)
		return false;

	D3D11_TEXTURE2D_DESC depthDesc{};
	depthDesc.Width = static_cast<UINT>(Width);
	depthDesc.Height = static_cast<UINT>(Height);
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	result = Device->CreateTexture2D(&depthDesc, nullptr, &DepthStencilTexture);

	if (FAILED(result) || !DepthStencilTexture)
		return false;

	result = Device->CreateDepthStencilView(DepthStencilTexture, nullptr, &DepthStencilView);

	if (FAILED(result) || !DepthStencilView)
		return false;

	return true;
}

void r3dDX11Device::ReleaseSwapChainResources()
{
	SafeReleaseDX11(DepthStencilView);
	SafeReleaseDX11(DepthStencilTexture);
	SafeReleaseDX11(BackBufferRTV);
}
