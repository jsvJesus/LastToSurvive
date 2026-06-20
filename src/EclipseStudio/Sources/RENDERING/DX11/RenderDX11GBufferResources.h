#pragma once

#include "RENDERING/DX11/RenderDX11Resources.h"
#include "RENDERING/DX11/RenderDX11Platform.h"

struct r3dDX11GBufferDesc
{
	r3dDX11GBufferDesc();

	bool CreateDoubleDepth;
	bool CreateHalfResParticles;
	bool CreateTemporalSSAO;
	DXGI_FORMAT HdrFormat;
};

class r3dDX11GBufferResources final
{
public:
	r3dDX11GBufferResources();
	~r3dDX11GBufferResources();

	bool Init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height, const r3dDX11GBufferDesc& desc);
	void Shutdown();
	bool Resize(int width, int height);

	void BeginGBuffer(bool clearDepth = true);
	void EndGBuffer();
	void ClearGBuffer(bool clearDepth = true);

	r3dDX11RenderTarget& GetColor();
	r3dDX11RenderTarget& GetNormal();
	r3dDX11RenderTarget& GetLinearDepth();
	r3dDX11RenderTarget& GetAux();
	r3dDX11RenderTarget& GetPrimaryDepth();
	r3dDX11RenderTarget& GetSecondaryDepth();
	r3dDX11RenderTarget& GetHalfDepth0();
	r3dDX11RenderTarget& GetHalfDepth1();
	r3dDX11RenderTarget& GetParticles();
	r3dDX11RenderTarget& GetScreenSmall0();
	r3dDX11RenderTarget& GetScreenSmall1();
	r3dDX11RenderTarget& GetPrevDepth();
	r3dDX11RenderTarget& GetPrevSSAO();
	r3dDX11RenderTarget& GetCurrentSSAO();

	const r3dDX11RenderTarget& GetColor() const;
	const r3dDX11RenderTarget& GetNormal() const;
	const r3dDX11RenderTarget& GetLinearDepth() const;
	const r3dDX11RenderTarget& GetAux() const;
	const r3dDX11RenderTarget& GetPrimaryDepth() const;
	const r3dDX11RenderTarget& GetSecondaryDepth() const;
	const r3dDX11RenderTarget& GetHalfDepth0() const;
	const r3dDX11RenderTarget& GetHalfDepth1() const;
	const r3dDX11RenderTarget& GetParticles() const;
	const r3dDX11RenderTarget& GetScreenSmall0() const;
	const r3dDX11RenderTarget& GetScreenSmall1() const;
	const r3dDX11RenderTarget& GetPrevDepth() const;
	const r3dDX11RenderTarget& GetPrevSSAO() const;
	const r3dDX11RenderTarget& GetCurrentSSAO() const;

	ID3D11DepthStencilView* GetDepthStencilView() const;

	int GetWidth() const;
	int GetHeight() const;
	bool IsInitialized() const;

private:
	bool CreateTargets();
	bool CreateRenderTarget(r3dDX11RenderTarget& target, const char* name, int width, int height, DXGI_FORMAT format);
	DXGI_FORMAT SelectSupportedRenderTargetFormat(DXGI_FORMAT preferred, DXGI_FORMAT fallback) const;

private:
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* Context = nullptr;

	r3dDX11GBufferDesc Desc;
	r3dDX11Texture2D DepthStencil;
	r3dDX11RenderTarget Color;
	r3dDX11RenderTarget Normal;
	r3dDX11RenderTarget LinearDepth;
	r3dDX11RenderTarget Aux;
	r3dDX11RenderTarget PrimaryDepth;
	r3dDX11RenderTarget SecondaryDepth;
	r3dDX11RenderTarget HalfDepth0;
	r3dDX11RenderTarget HalfDepth1;
	r3dDX11RenderTarget Particles;
	r3dDX11RenderTarget ScreenSmall0;
	r3dDX11RenderTarget ScreenSmall1;
	r3dDX11RenderTarget BlurQuarter;
	r3dDX11RenderTarget TempQuarter;
	r3dDX11RenderTarget One8_0;
	r3dDX11RenderTarget One8_1;
	r3dDX11RenderTarget One16_0;
	r3dDX11RenderTarget One16_1;
	r3dDX11RenderTarget One32_0;
	r3dDX11RenderTarget One32_1;
	r3dDX11RenderTarget One64_0;
	r3dDX11RenderTarget One64_1;
	r3dDX11RenderTarget Flashbang;
	r3dDX11RenderTarget PrevDepth;
	r3dDX11RenderTarget PrevSSAO;
	r3dDX11RenderTarget CurrentSSAO;

	int Width = 0;
	int Height = 0;
	bool bInitialized = false;
};
