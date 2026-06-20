#include "r3dPCH.h"

#include "RENDERING/DX11/RenderDX11Draw.h"

#include "RENDERING/DX11/RenderDX11Resources.h"
#include "RENDERING/DX11/ShaderDX11.h"

r3dDX11DrawContext::r3dDX11DrawContext()
{
}

void r3dDX11DrawContext::Init(ID3D11DeviceContext* context)
{
	Context = context;
}

void r3dDX11DrawContext::Shutdown()
{
	Context = nullptr;
}

void r3dDX11DrawContext::SetViewport(int width, int height)
{
	SetViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
}

void r3dDX11DrawContext::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth)
{
	if (!Context || width <= 0.0f || height <= 0.0f)
		return;

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = x;
	viewport.TopLeftY = y;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = minDepth;
	viewport.MaxDepth = maxDepth;
	Context->RSSetViewports(1, &viewport);
}

void r3dDX11DrawContext::SetRenderTarget(ID3D11RenderTargetView* renderTarget, ID3D11DepthStencilView* depthStencil)
{
	SetRenderTargets(1, &renderTarget, depthStencil);
}

void r3dDX11DrawContext::SetRenderTargets(unsigned int count, ID3D11RenderTargetView* const* renderTargets, ID3D11DepthStencilView* depthStencil)
{
	if (Context)
		Context->OMSetRenderTargets(count, renderTargets, depthStencil);
}

void r3dDX11DrawContext::ClearRenderTarget(ID3D11RenderTargetView* renderTarget, float r, float g, float b, float a)
{
	if (!Context || !renderTarget)
		return;

	const float color[4] = { r, g, b, a };
	Context->ClearRenderTargetView(renderTarget, color);
}

void r3dDX11DrawContext::ClearDepth(ID3D11DepthStencilView* depthStencil, float depth, unsigned char stencil)
{
	if (Context && depthStencil)
		Context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
}

void r3dDX11DrawContext::SetInputLayout(r3dDX11InputLayout* inputLayout)
{
	if (inputLayout)
		inputLayout->Bind(Context);
	else if (Context)
		Context->IASetInputLayout(nullptr);
}

void r3dDX11DrawContext::SetVertexBuffer(r3dDX11VertexBuffer* vertexBuffer, unsigned int slot, unsigned int offset)
{
	if (vertexBuffer)
		vertexBuffer->Bind(Context, slot, offset);
}

void r3dDX11DrawContext::SetIndexBuffer(r3dDX11IndexBuffer* indexBuffer, unsigned int offset)
{
	if (indexBuffer)
		indexBuffer->Bind(Context, offset);
}

void r3dDX11DrawContext::SetTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
{
	if (Context)
		Context->IASetPrimitiveTopology(topology);
}

void r3dDX11DrawContext::SetShaders(r3dDX11VertexShader* vertexShader, r3dDX11PixelShader* pixelShader)
{
	if (!Context)
		return;

	Context->VSSetShader(vertexShader ? vertexShader->GetShader() : nullptr, nullptr, 0);
	Context->PSSetShader(pixelShader ? pixelShader->GetShader() : nullptr, nullptr, 0);
}

void r3dDX11DrawContext::SetShaderResource(unsigned int slot, ID3D11ShaderResourceView* resource)
{
	if (Context)
		Context->PSSetShaderResources(slot, 1, &resource);
}

void r3dDX11DrawContext::SetSampler(unsigned int slot, ID3D11SamplerState* sampler)
{
	if (Context)
		Context->PSSetSamplers(slot, 1, &sampler);
}

void r3dDX11DrawContext::Draw(unsigned int vertexCount, unsigned int startVertex)
{
	if (Context && vertexCount > 0)
		Context->Draw(vertexCount, startVertex);
}

void r3dDX11DrawContext::DrawIndexed(unsigned int indexCount, unsigned int startIndex, int baseVertex)
{
	if (Context && indexCount > 0)
		Context->DrawIndexed(indexCount, startIndex, baseVertex);
}

ID3D11DeviceContext* r3dDX11DrawContext::GetContext() const
{
	return Context;
}

bool r3dDX11DrawContext::IsValid() const
{
	return Context != nullptr;
}
