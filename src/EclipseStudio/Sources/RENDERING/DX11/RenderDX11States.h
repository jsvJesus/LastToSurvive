#pragma once

#include "RENDERING/DX11/RenderDX11Platform.h"

class r3dDX11CommonStates final
{
public:
	r3dDX11CommonStates();
	~r3dDX11CommonStates();

	bool Init(ID3D11Device* device);
	void Shutdown();

	ID3D11SamplerState* GetLinearClampSampler() const;
	ID3D11SamplerState* GetLinearWrapSampler() const;
	ID3D11BlendState* GetOpaqueBlendState() const;
	ID3D11BlendState* GetAlphaBlendState() const;
	ID3D11DepthStencilState* GetDepthDisabledState() const;
	ID3D11DepthStencilState* GetDepthReadWriteState() const;
	ID3D11RasterizerState* GetCullBackRasterizer() const;
	ID3D11RasterizerState* GetCullNoneRasterizer() const;

	bool IsInitialized() const;

private:
	ID3D11SamplerState* LinearClampSampler = nullptr;
	ID3D11SamplerState* LinearWrapSampler = nullptr;
	ID3D11BlendState* OpaqueBlendState = nullptr;
	ID3D11BlendState* AlphaBlendState = nullptr;
	ID3D11DepthStencilState* DepthDisabledState = nullptr;
	ID3D11DepthStencilState* DepthReadWriteState = nullptr;
	ID3D11RasterizerState* CullBackRasterizer = nullptr;
	ID3D11RasterizerState* CullNoneRasterizer = nullptr;
	bool bInitialized = false;
};
