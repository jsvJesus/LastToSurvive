#include "r3dPCH.h"

#include "RENDERING/DX11/RenderDX11Resources.h"

#include <d3d11_1.h>

#include <algorithm>

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

	UINT ToD3DBindFlags(unsigned int bindFlags)
	{
		UINT result = 0;

		if (bindFlags & R3D_DX11_BIND_SHADER_RESOURCE)
			result |= D3D11_BIND_SHADER_RESOURCE;
		if (bindFlags & R3D_DX11_BIND_RENDER_TARGET)
			result |= D3D11_BIND_RENDER_TARGET;
		if (bindFlags & R3D_DX11_BIND_DEPTH_STENCIL)
			result |= D3D11_BIND_DEPTH_STENCIL;

		return result;
	}

	void SetDebugName(ID3D11DeviceChild* object, const char* name)
	{
		if (!object || !name || !name[0])
			return;

		object->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name);
	}
}

r3dDX11Texture2D::r3dDX11Texture2D()
	: Format(DXGI_FORMAT_UNKNOWN)
{
}

r3dDX11Texture2D::~r3dDX11Texture2D()
{
	Shutdown();
}

bool r3dDX11Texture2D::Create(
	ID3D11Device* device,
	int width,
	int height,
	DXGI_FORMAT format,
	unsigned int bindFlags,
	const char* debugName,
	int mipLevels
)
{
	Shutdown();

	if (!device || width <= 0 || height <= 0 || format == DXGI_FORMAT_UNKNOWN)
		return false;

	Width = std::max(1, width);
	Height = std::max(1, height);
	Format = format;

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = static_cast<UINT>(Width);
	desc.Height = static_cast<UINT>(Height);
	desc.MipLevels = static_cast<UINT>(std::max(1, mipLevels));
	desc.ArraySize = 1;
	desc.Format = Format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = ToD3DBindFlags(bindFlags);

	HRESULT result = device->CreateTexture2D(&desc, nullptr, &Texture);
	if (FAILED(result) || !Texture)
	{
		Shutdown();
		return false;
	}

	SetDebugName(Texture, debugName);

	if (!CreateViews(device, bindFlags))
	{
		Shutdown();
		return false;
	}

	return true;
}

void r3dDX11Texture2D::Shutdown()
{
	SafeReleaseDX11(DSV);
	SafeReleaseDX11(RTV);
	SafeReleaseDX11(SRV);
	SafeReleaseDX11(Texture);

	Width = 0;
	Height = 0;
	Format = DXGI_FORMAT_UNKNOWN;
}

ID3D11Texture2D* r3dDX11Texture2D::GetTexture() const
{
	return Texture;
}

ID3D11ShaderResourceView* r3dDX11Texture2D::GetSRV() const
{
	return SRV;
}

ID3D11RenderTargetView* r3dDX11Texture2D::GetRTV() const
{
	return RTV;
}

ID3D11DepthStencilView* r3dDX11Texture2D::GetDSV() const
{
	return DSV;
}

int r3dDX11Texture2D::GetWidth() const
{
	return Width;
}

int r3dDX11Texture2D::GetHeight() const
{
	return Height;
}

DXGI_FORMAT r3dDX11Texture2D::GetFormat() const
{
	return Format;
}

bool r3dDX11Texture2D::IsValid() const
{
	return Texture != nullptr;
}

bool r3dDX11Texture2D::CreateViews(ID3D11Device* device, unsigned int bindFlags)
{
	if ((bindFlags & R3D_DX11_BIND_SHADER_RESOURCE) != 0)
	{
		HRESULT result = device->CreateShaderResourceView(Texture, nullptr, &SRV);
		if (FAILED(result) || !SRV)
			return false;
	}

	if ((bindFlags & R3D_DX11_BIND_RENDER_TARGET) != 0)
	{
		HRESULT result = device->CreateRenderTargetView(Texture, nullptr, &RTV);
		if (FAILED(result) || !RTV)
			return false;
	}

	if ((bindFlags & R3D_DX11_BIND_DEPTH_STENCIL) != 0)
	{
		HRESULT result = device->CreateDepthStencilView(Texture, nullptr, &DSV);
		if (FAILED(result) || !DSV)
			return false;
	}

	return true;
}

r3dDX11RenderTarget::r3dDX11RenderTarget()
{
}

r3dDX11RenderTarget::~r3dDX11RenderTarget()
{
	Shutdown();
}

bool r3dDX11RenderTarget::Create(
	ID3D11Device* device,
	const char* name,
	int width,
	int height,
	DXGI_FORMAT colorFormat,
	bool withDepth,
	DXGI_FORMAT depthFormat
)
{
	Shutdown();

	if (!device || width <= 0 || height <= 0)
		return false;

	Name = name ? name : "";

	std::string colorName = Name.empty() ? "DX11RenderTarget.Color" : Name + ".Color";
	if (!ColorTexture.Create(
		device,
		width,
		height,
		colorFormat,
		R3D_DX11_BIND_SHADER_RESOURCE | R3D_DX11_BIND_RENDER_TARGET,
		colorName.c_str()
	))
	{
		Shutdown();
		return false;
	}

	if (withDepth)
	{
		std::string depthName = Name.empty() ? "DX11RenderTarget.Depth" : Name + ".Depth";
		if (!DepthTexture.Create(
			device,
			width,
			height,
			depthFormat,
			R3D_DX11_BIND_DEPTH_STENCIL,
			depthName.c_str()
		))
		{
			Shutdown();
			return false;
		}
	}

	return true;
}

void r3dDX11RenderTarget::Shutdown()
{
	DepthTexture.Shutdown();
	ColorTexture.Shutdown();
	Name.clear();
}

r3dDX11Texture2D& r3dDX11RenderTarget::GetColorTexture()
{
	return ColorTexture;
}

const r3dDX11Texture2D& r3dDX11RenderTarget::GetColorTexture() const
{
	return ColorTexture;
}

r3dDX11Texture2D& r3dDX11RenderTarget::GetDepthTexture()
{
	return DepthTexture;
}

const r3dDX11Texture2D& r3dDX11RenderTarget::GetDepthTexture() const
{
	return DepthTexture;
}

ID3D11ShaderResourceView* r3dDX11RenderTarget::GetSRV() const
{
	return ColorTexture.GetSRV();
}

ID3D11RenderTargetView* r3dDX11RenderTarget::GetRTV() const
{
	return ColorTexture.GetRTV();
}

ID3D11DepthStencilView* r3dDX11RenderTarget::GetDSV() const
{
	return DepthTexture.GetDSV();
}

int r3dDX11RenderTarget::GetWidth() const
{
	return ColorTexture.GetWidth();
}

int r3dDX11RenderTarget::GetHeight() const
{
	return ColorTexture.GetHeight();
}

const std::string& r3dDX11RenderTarget::GetName() const
{
	return Name;
}

bool r3dDX11RenderTarget::IsValid() const
{
	return ColorTexture.IsValid() && ColorTexture.GetRTV() && ColorTexture.GetSRV();
}

