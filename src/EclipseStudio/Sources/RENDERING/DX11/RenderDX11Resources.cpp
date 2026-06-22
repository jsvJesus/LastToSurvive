#include "r3dPCH.h"

#include "RENDERING/DX11/RenderDX11Resources.h"
#include "RENDERING/DX11/RenderDX11Platform.h"

#include <algorithm>
#include <cstring>

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
	if (!device || !Texture)
		return false;

	const bool isDepthTypeless =
		Format == DXGI_FORMAT_R24G8_TYPELESS;

	if ((bindFlags & R3D_DX11_BIND_SHADER_RESOURCE) != 0)
	{
		HRESULT result = E_FAIL;

		if (isDepthTypeless)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = 1;

			result =
				device->CreateShaderResourceView(
					Texture,
					&srvDesc,
					&SRV
				);
		}
		else
		{
			result =
				device->CreateShaderResourceView(
					Texture,
					nullptr,
					&SRV
				);
		}

		if (FAILED(result) || !SRV)
			return false;
	}

	if ((bindFlags & R3D_DX11_BIND_RENDER_TARGET) != 0)
	{
		HRESULT result =
			device->CreateRenderTargetView(
				Texture,
				nullptr,
				&RTV
			);

		if (FAILED(result) || !RTV)
			return false;
	}

	if ((bindFlags & R3D_DX11_BIND_DEPTH_STENCIL) != 0)
	{
		HRESULT result = E_FAIL;

		if (isDepthTypeless)
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;

			result =
				device->CreateDepthStencilView(
					Texture,
					&dsvDesc,
					&DSV
				);
		}
		else
		{
			result =
				device->CreateDepthStencilView(
					Texture,
					nullptr,
					&DSV
				);
		}

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

r3dDX11ConstantBuffer::r3dDX11ConstantBuffer()
{
}

r3dDX11ConstantBuffer::~r3dDX11ConstantBuffer()
{
	Shutdown();
}

bool r3dDX11ConstantBuffer::Create(ID3D11Device* device, size_t byteSize, const char* debugName)
{
	Shutdown();

	if (!device || byteSize == 0)
		return false;

	ByteSize = (byteSize + 15) & ~static_cast<size_t>(15);

	D3D11_BUFFER_DESC desc{};
	desc.ByteWidth = static_cast<UINT>(ByteSize);
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT result = device->CreateBuffer(&desc, nullptr, &Buffer);
	if (FAILED(result) || !Buffer)
	{
		Shutdown();
		return false;
	}

	SetDebugName(Buffer, debugName);
	return true;
}

void r3dDX11ConstantBuffer::Shutdown()
{
	SafeReleaseDX11(Buffer);
	ByteSize = 0;
}

bool r3dDX11ConstantBuffer::Update(ID3D11DeviceContext* context, const void* data, size_t byteSize)
{
	if (!context || !Buffer || !data || byteSize > ByteSize)
		return false;

	D3D11_MAPPED_SUBRESOURCE mapped{};
	HRESULT result = context->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	if (FAILED(result) || !mapped.pData)
		return false;

	memcpy(mapped.pData, data, byteSize);
	context->Unmap(Buffer, 0);
	return true;
}

void r3dDX11ConstantBuffer::BindVS(ID3D11DeviceContext* context, unsigned int slot)
{
	if (context && Buffer)
		context->VSSetConstantBuffers(slot, 1, &Buffer);
}

void r3dDX11ConstantBuffer::BindPS(ID3D11DeviceContext* context, unsigned int slot)
{
	if (context && Buffer)
		context->PSSetConstantBuffers(slot, 1, &Buffer);
}

void r3dDX11ConstantBuffer::BindVSPS(ID3D11DeviceContext* context, unsigned int slot)
{
	BindVS(context, slot);
	BindPS(context, slot);
}

ID3D11Buffer* r3dDX11ConstantBuffer::GetBuffer() const
{
	return Buffer;
}

size_t r3dDX11ConstantBuffer::GetByteSize() const
{
	return ByteSize;
}

bool r3dDX11ConstantBuffer::IsValid() const
{
	return Buffer != nullptr;
}

namespace
{
	D3D11_USAGE ToD3DUsage(r3dDX11BufferUsage usage)
	{
		switch (usage)
		{
		case R3D_DX11_BUFFER_IMMUTABLE:
			return D3D11_USAGE_IMMUTABLE;
		case R3D_DX11_BUFFER_DYNAMIC:
			return D3D11_USAGE_DYNAMIC;
		default:
			return D3D11_USAGE_DEFAULT;
		}
	}

	UINT ToCPUAccessFlags(r3dDX11BufferUsage usage)
	{
		return usage == R3D_DX11_BUFFER_DYNAMIC ? D3D11_CPU_ACCESS_WRITE : 0;
	}

	bool CreateDX11Buffer(
		ID3D11Device* device,
		ID3D11Buffer** buffer,
		size_t byteSize,
		UINT bindFlags,
		const void* initialData,
		r3dDX11BufferUsage usage,
		const char* debugName
	)
	{
		if (!device || !buffer || byteSize == 0)
			return false;

		*buffer = nullptr;

		D3D11_BUFFER_DESC desc{};
		desc.ByteWidth = static_cast<UINT>(byteSize);
		desc.Usage = ToD3DUsage(usage);
		desc.BindFlags = bindFlags;
		desc.CPUAccessFlags = ToCPUAccessFlags(usage);

		D3D11_SUBRESOURCE_DATA initData{};
		initData.pSysMem = initialData;

		HRESULT result = device->CreateBuffer(&desc, initialData ? &initData : nullptr, buffer);
		if (FAILED(result) || !*buffer)
			return false;

		SetDebugName(*buffer, debugName);
		return true;
	}

	bool UpdateDynamicBuffer(ID3D11DeviceContext* context, ID3D11Buffer* buffer, size_t capacity, const void* data, size_t byteSize)
	{
		if (!context || !buffer || !data || byteSize > capacity)
			return false;

		D3D11_MAPPED_SUBRESOURCE mapped{};
		HRESULT result = context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		if (FAILED(result) || !mapped.pData)
			return false;

		memcpy(mapped.pData, data, byteSize);
		context->Unmap(buffer, 0);
		return true;
	}
}

r3dDX11VertexBuffer::r3dDX11VertexBuffer()
{
}

r3dDX11VertexBuffer::~r3dDX11VertexBuffer()
{
	Shutdown();
}

bool r3dDX11VertexBuffer::Create(ID3D11Device* device, size_t byteSize, unsigned int stride, const void* initialData, r3dDX11BufferUsage usage, const char* debugName)
{
	Shutdown();

	if (stride == 0)
		return false;

	if (!CreateDX11Buffer(device, &Buffer, byteSize, D3D11_BIND_VERTEX_BUFFER, initialData, usage, debugName))
		return false;

	ByteSize = byteSize;
	Stride = stride;
	Usage = usage;
	return true;
}

void r3dDX11VertexBuffer::Shutdown()
{
	SafeReleaseDX11(Buffer);
	ByteSize = 0;
	Stride = 0;
	Usage = R3D_DX11_BUFFER_DEFAULT;
}

bool r3dDX11VertexBuffer::Update(ID3D11DeviceContext* context, const void* data, size_t byteSize)
{
	return Usage == R3D_DX11_BUFFER_DYNAMIC && UpdateDynamicBuffer(context, Buffer, ByteSize, data, byteSize);
}

void r3dDX11VertexBuffer::Bind(ID3D11DeviceContext* context, unsigned int slot, unsigned int offset)
{
	if (!context || !Buffer)
		return;

	ID3D11Buffer* buffer = Buffer;
	UINT stride = Stride;
	UINT bindOffset = offset;
	context->IASetVertexBuffers(slot, 1, &buffer, &stride, &bindOffset);
}

ID3D11Buffer* r3dDX11VertexBuffer::GetBuffer() const
{
	return Buffer;
}

size_t r3dDX11VertexBuffer::GetByteSize() const
{
	return ByteSize;
}

unsigned int r3dDX11VertexBuffer::GetStride() const
{
	return Stride;
}

bool r3dDX11VertexBuffer::IsValid() const
{
	return Buffer != nullptr;
}

r3dDX11IndexBuffer::r3dDX11IndexBuffer()
{
}

r3dDX11IndexBuffer::~r3dDX11IndexBuffer()
{
	Shutdown();
}

bool r3dDX11IndexBuffer::Create(ID3D11Device* device, size_t byteSize, DXGI_FORMAT format, const void* initialData, r3dDX11BufferUsage usage, const char* debugName)
{
	Shutdown();

	if (format != DXGI_FORMAT_R16_UINT && format != DXGI_FORMAT_R32_UINT)
		return false;

	if (!CreateDX11Buffer(device, &Buffer, byteSize, D3D11_BIND_INDEX_BUFFER, initialData, usage, debugName))
		return false;

	ByteSize = byteSize;
	Format = format;
	Usage = usage;
	return true;
}

void r3dDX11IndexBuffer::Shutdown()
{
	SafeReleaseDX11(Buffer);
	ByteSize = 0;
	Format = DXGI_FORMAT_UNKNOWN;
	Usage = R3D_DX11_BUFFER_DEFAULT;
}

bool r3dDX11IndexBuffer::Update(ID3D11DeviceContext* context, const void* data, size_t byteSize)
{
	return Usage == R3D_DX11_BUFFER_DYNAMIC && UpdateDynamicBuffer(context, Buffer, ByteSize, data, byteSize);
}

void r3dDX11IndexBuffer::Bind(ID3D11DeviceContext* context, unsigned int offset)
{
	if (context && Buffer)
		context->IASetIndexBuffer(Buffer, Format, offset);
}

ID3D11Buffer* r3dDX11IndexBuffer::GetBuffer() const
{
	return Buffer;
}

size_t r3dDX11IndexBuffer::GetByteSize() const
{
	return ByteSize;
}

DXGI_FORMAT r3dDX11IndexBuffer::GetFormat() const
{
	return Format;
}

bool r3dDX11IndexBuffer::IsValid() const
{
	return Buffer != nullptr;
}

r3dDX11InputLayout::r3dDX11InputLayout()
{
}

r3dDX11InputLayout::~r3dDX11InputLayout()
{
	Shutdown();
}

bool r3dDX11InputLayout::Create(ID3D11Device* device, const D3D11_INPUT_ELEMENT_DESC* elements, unsigned int elementCount, const void* vertexShaderBytecode, size_t bytecodeSize, const char* debugName)
{
	Shutdown();

	if (!device || !elements || elementCount == 0 || !vertexShaderBytecode || bytecodeSize == 0)
		return false;

	HRESULT result = device->CreateInputLayout(elements, elementCount, vertexShaderBytecode, bytecodeSize, &Layout);
	if (FAILED(result) || !Layout)
		return false;

	SetDebugName(Layout, debugName);
	return true;
}

void r3dDX11InputLayout::Shutdown()
{
	SafeReleaseDX11(Layout);
}

void r3dDX11InputLayout::Bind(ID3D11DeviceContext* context)
{
	if (context && Layout)
		context->IASetInputLayout(Layout);
}

ID3D11InputLayout* r3dDX11InputLayout::GetLayout() const
{
	return Layout;
}

bool r3dDX11InputLayout::IsValid() const
{
	return Layout != nullptr;
}
