#include "r3dRenderDX11.h"

namespace
{
	typedef HRESULT (WINAPI* r3dD3D11CreateDeviceAndSwapChainFn)(
		IDXGIAdapter*,
		D3D_DRIVER_TYPE,
		HMODULE,
		UINT,
		const D3D_FEATURE_LEVEL*,
		UINT,
		UINT,
		const DXGI_SWAP_CHAIN_DESC*,
		IDXGISwapChain**,
		ID3D11Device**,
		D3D_FEATURE_LEVEL*,
		ID3D11DeviceContext**
	);

	template <typename T>
	void r3dDX11SafeRelease(T*& Value)
	{
		if (Value)
		{
			Value->Release();
			Value = 0;
		}
	}
}

r3dDX11DeviceCreateParams::r3dDX11DeviceCreateParams()
	: Window(0), Width(1), Height(1), Windowed(true), VSync(true),
	  EnableDebugLayer(false)
{
}

r3dRenderDX11::r3dRenderDX11()
	: Window_(0), Width_(0), Height_(0), VSync_(true), Occluded_(false),
	  FeatureLevel_(D3D_FEATURE_LEVEL_10_0), Device_(0), Context_(0),
	  SwapChain_(0), BackBufferTexture_(0), BackBufferRTV_(0),
	  D3D11Module_(0)
{
}

r3dRenderDX11::~r3dRenderDX11()
{
	Shutdown();
}

bool r3dRenderDX11::Initialize(const r3dDX11DeviceCreateParams& Params)
{
	if (IsInitialized())
		return true;
	if (!Params.Window)
		return false;

	D3D11Module_ = LoadLibraryW(L"d3d11.dll");
	if (!D3D11Module_)
		return false;

	const r3dD3D11CreateDeviceAndSwapChainFn CreateDeviceAndSwapChain =
		reinterpret_cast<r3dD3D11CreateDeviceAndSwapChainFn>(
			GetProcAddress(D3D11Module_, "D3D11CreateDeviceAndSwapChain")
		);

	if (!CreateDeviceAndSwapChain)
	{
		Shutdown();
		return false;
	}

	Window_ = Params.Window;
	Width_ = Params.Width ? Params.Width : 1;
	Height_ = Params.Height ? Params.Height : 1;
	VSync_ = Params.VSync;
	Occluded_ = false;

	DXGI_SWAP_CHAIN_DESC Desc = {};
	Desc.BufferDesc.Width = Width_;
	Desc.BufferDesc.Height = Height_;
	Desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	Desc.SampleDesc.Count = 1;
	Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
	Desc.BufferCount = 2;
	Desc.OutputWindow = Window_;
	Desc.Windowed = Params.Windowed ? TRUE : FALSE;
	Desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	Desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	const D3D_FEATURE_LEVEL Levels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};

	// Resource loading in Eternity is not restricted to the render thread.
	// Keep the default thread-safe device behavior.
	UINT Flags = 0;
	if (Params.EnableDebugLayer)
		Flags |= D3D11_CREATE_DEVICE_DEBUG;

	HRESULT Hr = CreateDeviceAndSwapChain(
		0, D3D_DRIVER_TYPE_HARDWARE, 0, Flags, Levels, _countof(Levels),
		D3D11_SDK_VERSION, &Desc, &SwapChain_, &Device_, &FeatureLevel_,
		&Context_);

	if (FAILED(Hr) && (Flags & D3D11_CREATE_DEVICE_DEBUG))
	{
		Flags &= ~D3D11_CREATE_DEVICE_DEBUG;
		Hr = CreateDeviceAndSwapChain(
			0, D3D_DRIVER_TYPE_HARDWARE, 0, Flags, Levels, _countof(Levels),
			D3D11_SDK_VERSION, &Desc, &SwapChain_, &Device_, &FeatureLevel_,
			&Context_);
	}

	if (FAILED(Hr) || !CreateBackBuffer())
	{
		Shutdown();
		return false;
	}

	return true;
}

void r3dRenderDX11::Shutdown()
{
	ReleaseBackBuffer();

	if (Context_)
	{
		Context_->ClearState();
		Context_->Flush();
	}
	if (SwapChain_)
		SwapChain_->SetFullscreenState(FALSE, 0);

	r3dDX11SafeRelease(SwapChain_);
	r3dDX11SafeRelease(Context_);
	r3dDX11SafeRelease(Device_);

	if (D3D11Module_)
	{
		FreeLibrary(D3D11Module_);
		D3D11Module_ = 0;
	}

	Window_ = 0;
	Width_ = Height_ = 0;
	VSync_ = true;
	Occluded_ = false;
	FeatureLevel_ = D3D_FEATURE_LEVEL_10_0;
}

bool r3dRenderDX11::Resize(unsigned int Width, unsigned int Height)
{
	if (!IsInitialized())
		return false;

	Width = Width ? Width : 1;
	Height = Height ? Height : 1;
	if (Width_ == Width && Height_ == Height && BackBufferRTV_)
		return true;

	Context_->OMSetRenderTargets(0, 0, 0);
	ReleaseBackBuffer();

	const HRESULT Hr = SwapChain_->ResizeBuffers(
		0, Width, Height, DXGI_FORMAT_UNKNOWN,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
	if (FAILED(Hr))
		return false;

	Width_ = Width;
	Height_ = Height;
	return CreateBackBuffer();
}

bool r3dRenderDX11::BeginFrame(const float ClearColor[4])
{
	if (!IsInitialized() || !BackBufferRTV_)
		return false;

	Context_->OMSetRenderTargets(1, &BackBufferRTV_, 0);
	if (ClearColor)
		Context_->ClearRenderTargetView(BackBufferRTV_, ClearColor);

	D3D11_VIEWPORT Viewport = {};
	Viewport.Width = static_cast<float>(Width_);
	Viewport.Height = static_cast<float>(Height_);
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;
	Context_->RSSetViewports(1, &Viewport);
	return true;
}

bool r3dRenderDX11::CopyToBackBuffer(ID3D11Texture2D* Source)
{
	if (!IsInitialized() || !Source || !BackBufferTexture_)
		return false;

	D3D11_TEXTURE2D_DESC SourceDesc = {};
	D3D11_TEXTURE2D_DESC BackBufferDesc = {};
	Source->GetDesc(&SourceDesc);
	BackBufferTexture_->GetDesc(&BackBufferDesc);

	if (
		SourceDesc.Width != BackBufferDesc.Width ||
		SourceDesc.Height != BackBufferDesc.Height ||
		SourceDesc.Format != BackBufferDesc.Format ||
		SourceDesc.SampleDesc.Count != BackBufferDesc.SampleDesc.Count
	)
	{
		return false;
	}

	Context_->OMSetRenderTargets(0, 0, 0);
	Context_->CopyResource(BackBufferTexture_, Source);
	return true;
}

bool r3dRenderDX11::Present()
{
	if (!IsInitialized())
		return false;

	const HRESULT Hr = SwapChain_->Present(VSync_ ? 1 : 0, 0);
	Occluded_ = Hr == DXGI_STATUS_OCCLUDED;
	return SUCCEEDED(Hr) || Occluded_;
}

bool r3dRenderDX11::IsInitialized() const
{
	return Device_ && Context_ && SwapChain_;
}

bool r3dRenderDX11::IsOccluded() const { return Occluded_; }
unsigned int r3dRenderDX11::GetWidth() const { return Width_; }
unsigned int r3dRenderDX11::GetHeight() const { return Height_; }
D3D_FEATURE_LEVEL r3dRenderDX11::GetFeatureLevel() const { return FeatureLevel_; }
ID3D11Device* r3dRenderDX11::GetDevice() const { return Device_; }
ID3D11DeviceContext* r3dRenderDX11::GetContext() const { return Context_; }
IDXGISwapChain* r3dRenderDX11::GetSwapChain() const { return SwapChain_; }
ID3D11Texture2D* r3dRenderDX11::GetBackBufferTexture() const { return BackBufferTexture_; }
ID3D11RenderTargetView* r3dRenderDX11::GetBackBufferRTV() const { return BackBufferRTV_; }

bool r3dRenderDX11::CreateBackBuffer()
{
	const HRESULT GetHr = SwapChain_->GetBuffer(
		0, __uuidof(ID3D11Texture2D),
		reinterpret_cast<void**>(&BackBufferTexture_));
	if (FAILED(GetHr))
		return false;

	const HRESULT ViewHr = Device_->CreateRenderTargetView(
		BackBufferTexture_, 0, &BackBufferRTV_);

	if (FAILED(ViewHr))
		ReleaseBackBuffer();

	return SUCCEEDED(ViewHr);
}

void r3dRenderDX11::ReleaseBackBuffer()
{
	r3dDX11SafeRelease(BackBufferRTV_);
	r3dDX11SafeRelease(BackBufferTexture_);
}

r3dRenderDX11& r3dGetRenderDX11()
{
	static r3dRenderDX11 Device;
	return Device;
}
