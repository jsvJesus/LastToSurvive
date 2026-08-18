#pragma once

#include "r3dRendererConfig.h"

#if LTS_STUDIO_DX11

#include <D3D11.h>

class RenderDX11States
{
public:
	RenderDX11States();

	bool Initialize();
	void Shutdown();

	bool IsReady() const;

	void ApplyDefaults();

	/*
	 * Transitional reference getters.
	 *
	 * Нужны, чтобы существующий код мог продолжать использовать:
	 *
	 *     gDX11DepthWriteLessEqual
	 *     &gDX11SamplerLinearWrap
	 *
	 * Через compatibility macros в RenderDX11.cpp.
	 */

	ID3D11DepthStencilState*& DepthWriteLessEqual()
	{
		return DepthWriteLessEqual_;
	}

	ID3D11DepthStencilState*& DepthReadLessEqual()
	{
		return DepthReadLessEqual_;
	}

	ID3D11DepthStencilState*& DepthDisabled()
	{
		return DepthDisabled_;
	}

	ID3D11RasterizerState*& RasterSolidBackCull()
	{
		return RasterSolidBackCull_;
	}

	ID3D11RasterizerState*& RasterSolidNoCull()
	{
		return RasterSolidNoCull_;
	}

	ID3D11BlendState*& BlendOpaque()
	{
		return BlendOpaque_;
	}

	ID3D11BlendState*& BlendAlpha()
	{
		return BlendAlpha_;
	}

	ID3D11SamplerState*& SamplerLinearWrap()
	{
		return SamplerLinearWrap_;
	}

	ID3D11SamplerState*& SamplerLinearClamp()
	{
		return SamplerLinearClamp_;
	}

	ID3D11SamplerState*& SunGlareBorderSampler()
	{
		return SunGlareBorderSampler_;
	}

private:
	RenderDX11States(
		const RenderDX11States&
	);

	RenderDX11States& operator=(
		const RenderDX11States&
	);

	bool CreateDepthStates();
	bool CreateRasterizerStates();
	bool CreateBlendStates();
	bool CreateSamplerStates();

	void LogFailure(
		const char* Stage,
		HRESULT Result
	);

private:
	ID3D11DepthStencilState* DepthWriteLessEqual_;
	ID3D11DepthStencilState* DepthReadLessEqual_;
	ID3D11DepthStencilState* DepthDisabled_;

	ID3D11RasterizerState* RasterSolidBackCull_;
	ID3D11RasterizerState* RasterSolidNoCull_;

	ID3D11BlendState* BlendOpaque_;
	ID3D11BlendState* BlendAlpha_;

	ID3D11SamplerState* SamplerLinearWrap_;
	ID3D11SamplerState* SamplerLinearClamp_;
	ID3D11SamplerState* SunGlareBorderSampler_;
};

RenderDX11States& RenderDX11_GetStates();

#endif // LTS_STUDIO_DX11