#pragma once

#include "r3dRendererConfig.h"

#if LTS_STUDIO_DX11

#include <D3D11.h>

class RenderDX11FrameTargets
{
public:
	RenderDX11FrameTargets();

	bool Ensure(
		int Width,
		int Height,
		bool CreateSmokeReadback
	);

	void Release();

	bool IsReady(
		bool SmokeReadbackRequired
	) const;

	void BindGBuffer();
	void Clear();
	void Unbind();

	/*
	 * Transitional accessors.
	 *
	 * Они возвращают ссылки на указатели, чтобы старый код
	 * временно продолжал поддерживать конструкции вроде:
	 *
	 *     &gDX11SceneColorRTV
	 *
	 * После разделения render passes эти accessors станут
	 * обычными read-only getters.
	 */

	ID3D11Texture2D*& GBufferColorTexture()
	{
		return GBufferColorTexture_;
	}

	ID3D11Texture2D*& GBufferNormalTexture()
	{
		return GBufferNormalTexture_;
	}

	ID3D11Texture2D*& GBufferDepthLinearTexture()
	{
		return GBufferDepthLinearTexture_;
	}

	ID3D11Texture2D*& GBufferAuxTexture()
	{
		return GBufferAuxTexture_;
	}

	ID3D11Texture2D*& SceneColorTexture()
	{
		return SceneColorTexture_;
	}

	ID3D11Texture2D*& FinalColorTexture()
	{
		return FinalColorTexture_;
	}

	ID3D11Texture2D*& DepthTexture()
	{
		return DepthTexture_;
	}

	ID3D11Texture2D*& SmokeReadbackTexture()
	{
		return SmokeReadbackTexture_;
	}

	ID3D11RenderTargetView*& GBufferColorRTV()
	{
		return GBufferColorRTV_;
	}

	ID3D11RenderTargetView*& GBufferNormalRTV()
	{
		return GBufferNormalRTV_;
	}

	ID3D11RenderTargetView*& GBufferDepthLinearRTV()
	{
		return GBufferDepthLinearRTV_;
	}

	ID3D11RenderTargetView*& GBufferAuxRTV()
	{
		return GBufferAuxRTV_;
	}

	ID3D11RenderTargetView*& SceneColorRTV()
	{
		return SceneColorRTV_;
	}

	ID3D11RenderTargetView*& FinalColorRTV()
	{
		return FinalColorRTV_;
	}

	ID3D11DepthStencilView*& DepthDSV()
	{
		return DepthDSV_;
	}

	ID3D11ShaderResourceView*& GBufferColorSRV()
	{
		return GBufferColorSRV_;
	}

	ID3D11ShaderResourceView*& GBufferNormalSRV()
	{
		return GBufferNormalSRV_;
	}

	ID3D11ShaderResourceView*& GBufferDepthLinearSRV()
	{
		return GBufferDepthLinearSRV_;
	}

	ID3D11ShaderResourceView*& SceneColorSRV()
	{
		return SceneColorSRV_;
	}

	D3D11_VIEWPORT& Viewport()
	{
		return Viewport_;
	}

	int& FrameWidth()
	{
		return FrameWidth_;
	}

	int& FrameHeight()
	{
		return FrameHeight_;
	}

private:
	RenderDX11FrameTargets(
		const RenderDX11FrameTargets&
	);

	RenderDX11FrameTargets& operator=(
		const RenderDX11FrameTargets&
	);

	bool CreateRenderTarget(
		int Width,
		int Height,
		DXGI_FORMAT Format,
		const char* DebugName,
		ID3D11Texture2D** OutTexture,
		ID3D11RenderTargetView** OutRTV,
		ID3D11ShaderResourceView** OutSRV
	);

	bool CreateDepthTarget(
		int Width,
		int Height
	);

	bool CreateSmokeReadbackTarget(
		int Width,
		int Height
	);

	void LogFailureOnce(
		const char* Stage,
		HRESULT Result
	);

	static int ClampSize(
		int Value
	);

private:
	ID3D11Texture2D* GBufferColorTexture_;
	ID3D11Texture2D* GBufferNormalTexture_;
	ID3D11Texture2D* GBufferDepthLinearTexture_;
	ID3D11Texture2D* GBufferAuxTexture_;

	ID3D11Texture2D* SceneColorTexture_;
	ID3D11Texture2D* FinalColorTexture_;

	ID3D11Texture2D* DepthTexture_;
	ID3D11Texture2D* SmokeReadbackTexture_;

	ID3D11RenderTargetView* GBufferColorRTV_;
	ID3D11RenderTargetView* GBufferNormalRTV_;
	ID3D11RenderTargetView* GBufferDepthLinearRTV_;
	ID3D11RenderTargetView* GBufferAuxRTV_;

	ID3D11RenderTargetView* SceneColorRTV_;
	ID3D11RenderTargetView* FinalColorRTV_;

	ID3D11DepthStencilView* DepthDSV_;

	ID3D11ShaderResourceView* GBufferColorSRV_;
	ID3D11ShaderResourceView* GBufferNormalSRV_;
	ID3D11ShaderResourceView* GBufferDepthLinearSRV_;
	ID3D11ShaderResourceView* SceneColorSRV_;

	D3D11_VIEWPORT Viewport_;

	int FrameWidth_;
	int FrameHeight_;

	bool FailureLogged_;
};

RenderDX11FrameTargets& RenderDX11_GetFrameTargets();

#endif // LTS_STUDIO_DX11