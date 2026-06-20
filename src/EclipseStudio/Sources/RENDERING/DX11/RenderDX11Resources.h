#pragma once

#include "RENDERING/DX11/RenderDX11Platform.h"

#include <cstddef>
#include <string>

enum r3dDX11TextureBindFlags
{
	R3D_DX11_BIND_SHADER_RESOURCE = 1 << 0,
	R3D_DX11_BIND_RENDER_TARGET = 1 << 1,
	R3D_DX11_BIND_DEPTH_STENCIL = 1 << 2
};

enum r3dDX11BufferUsage
{
	R3D_DX11_BUFFER_IMMUTABLE,
	R3D_DX11_BUFFER_DYNAMIC,
	R3D_DX11_BUFFER_DEFAULT
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

class r3dDX11ConstantBuffer final
{
public:
	r3dDX11ConstantBuffer();
	~r3dDX11ConstantBuffer();

	bool Create(ID3D11Device* device, size_t byteSize, const char* debugName = nullptr);
	void Shutdown();

	bool Update(ID3D11DeviceContext* context, const void* data, size_t byteSize);
	void BindVS(ID3D11DeviceContext* context, unsigned int slot);
	void BindPS(ID3D11DeviceContext* context, unsigned int slot);
	void BindVSPS(ID3D11DeviceContext* context, unsigned int slot);

	ID3D11Buffer* GetBuffer() const;
	size_t GetByteSize() const;
	bool IsValid() const;

private:
	ID3D11Buffer* Buffer = nullptr;
	size_t ByteSize = 0;
};

template <typename T>
bool r3dDX11UpdateConstantBuffer(ID3D11DeviceContext* context, r3dDX11ConstantBuffer& buffer, const T& value)
{
	return buffer.Update(context, &value, sizeof(T));
}

class r3dDX11VertexBuffer final
{
public:
	r3dDX11VertexBuffer();
	~r3dDX11VertexBuffer();

	bool Create(ID3D11Device* device, size_t byteSize, unsigned int stride, const void* initialData, r3dDX11BufferUsage usage, const char* debugName = nullptr);
	void Shutdown();

	bool Update(ID3D11DeviceContext* context, const void* data, size_t byteSize);
	void Bind(ID3D11DeviceContext* context, unsigned int slot = 0, unsigned int offset = 0);

	ID3D11Buffer* GetBuffer() const;
	size_t GetByteSize() const;
	unsigned int GetStride() const;
	bool IsValid() const;

private:
	ID3D11Buffer* Buffer = nullptr;
	size_t ByteSize = 0;
	unsigned int Stride = 0;
	r3dDX11BufferUsage Usage = R3D_DX11_BUFFER_DEFAULT;
};

class r3dDX11IndexBuffer final
{
public:
	r3dDX11IndexBuffer();
	~r3dDX11IndexBuffer();

	bool Create(ID3D11Device* device, size_t byteSize, DXGI_FORMAT format, const void* initialData, r3dDX11BufferUsage usage, const char* debugName = nullptr);
	void Shutdown();

	bool Update(ID3D11DeviceContext* context, const void* data, size_t byteSize);
	void Bind(ID3D11DeviceContext* context, unsigned int offset = 0);

	ID3D11Buffer* GetBuffer() const;
	size_t GetByteSize() const;
	DXGI_FORMAT GetFormat() const;
	bool IsValid() const;

private:
	ID3D11Buffer* Buffer = nullptr;
	size_t ByteSize = 0;
	DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
	r3dDX11BufferUsage Usage = R3D_DX11_BUFFER_DEFAULT;
};

class r3dDX11InputLayout final
{
public:
	r3dDX11InputLayout();
	~r3dDX11InputLayout();

	bool Create(ID3D11Device* device, const D3D11_INPUT_ELEMENT_DESC* elements, unsigned int elementCount, const void* vertexShaderBytecode, size_t bytecodeSize, const char* debugName = nullptr);
	void Shutdown();
	void Bind(ID3D11DeviceContext* context);

	ID3D11InputLayout* GetLayout() const;
	bool IsValid() const;

private:
	ID3D11InputLayout* Layout = nullptr;
};
