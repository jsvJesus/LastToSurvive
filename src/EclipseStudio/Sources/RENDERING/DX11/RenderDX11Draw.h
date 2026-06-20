#pragma once

#include <d3d11_1.h>

class r3dDX11IndexBuffer;
class r3dDX11InputLayout;
class r3dDX11PixelShader;
class r3dDX11VertexBuffer;
class r3dDX11VertexShader;

class r3dDX11DrawContext final
{
public:
	r3dDX11DrawContext();

	void Init(ID3D11DeviceContext* context);
	void Shutdown();

	void SetViewport(int width, int height);
	void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
	void SetRenderTarget(ID3D11RenderTargetView* renderTarget, ID3D11DepthStencilView* depthStencil);
	void SetRenderTargets(unsigned int count, ID3D11RenderTargetView* const* renderTargets, ID3D11DepthStencilView* depthStencil);
	void ClearRenderTarget(ID3D11RenderTargetView* renderTarget, float r, float g, float b, float a);
	void ClearDepth(ID3D11DepthStencilView* depthStencil, float depth = 1.0f, unsigned char stencil = 0);

	void SetInputLayout(r3dDX11InputLayout* inputLayout);
	void SetVertexBuffer(r3dDX11VertexBuffer* vertexBuffer, unsigned int slot = 0, unsigned int offset = 0);
	void SetIndexBuffer(r3dDX11IndexBuffer* indexBuffer, unsigned int offset = 0);
	void SetTopology(D3D11_PRIMITIVE_TOPOLOGY topology);
	void SetShaders(r3dDX11VertexShader* vertexShader, r3dDX11PixelShader* pixelShader);
	void SetShaderResource(unsigned int slot, ID3D11ShaderResourceView* resource);
	void SetSampler(unsigned int slot, ID3D11SamplerState* sampler);

	void Draw(unsigned int vertexCount, unsigned int startVertex = 0);
	void DrawIndexed(unsigned int indexCount, unsigned int startIndex = 0, int baseVertex = 0);

	ID3D11DeviceContext* GetContext() const;
	bool IsValid() const;

private:
	ID3D11DeviceContext* Context = nullptr;
};
