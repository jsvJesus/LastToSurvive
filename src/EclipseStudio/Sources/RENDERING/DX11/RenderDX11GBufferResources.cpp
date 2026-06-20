#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11GBufferResources.h"

#include <algorithm>

namespace
{
	int ClampDim(int value)
	{
		return std::max(1, value);
	}

	void ClearTarget(ID3D11DeviceContext* context, r3dDX11RenderTarget& target, const float color[4])
	{
		if (context && target.GetRTV())
			context->ClearRenderTargetView(target.GetRTV(), color);
	}
}

r3dDX11GBufferDesc::r3dDX11GBufferDesc()
	: CreateDoubleDepth(false)
	, CreateHalfResParticles(true)
	, CreateTemporalSSAO(false)
	, HdrFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
{
}

r3dDX11GBufferResources::r3dDX11GBufferResources()
{
}

r3dDX11GBufferResources::~r3dDX11GBufferResources()
{
	Shutdown();
}

bool r3dDX11GBufferResources::Init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height, const r3dDX11GBufferDesc& desc)
{
	if (bInitialized)
		return true;

	if (!device || !context || width <= 0 || height <= 0)
		return false;

	Device = device;
	Context = context;
	Width = ClampDim(width);
	Height = ClampDim(height);
	Desc = desc;

	if (!CreateTargets())
	{
		Shutdown();
		return false;
	}

	bInitialized = true;
	r3dOutToLog("[DX11] GBuffer resources initialized %dx%d\n", Width, Height);
	return true;
}

void r3dDX11GBufferResources::Shutdown()
{
	CurrentSSAO.Shutdown();
	PrevSSAO.Shutdown();
	PrevDepth.Shutdown();
	Flashbang.Shutdown();
	One64_1.Shutdown();
	One64_0.Shutdown();
	One32_1.Shutdown();
	One32_0.Shutdown();
	One16_1.Shutdown();
	One16_0.Shutdown();
	One8_1.Shutdown();
	One8_0.Shutdown();
	TempQuarter.Shutdown();
	BlurQuarter.Shutdown();
	ScreenSmall1.Shutdown();
	ScreenSmall0.Shutdown();
	Particles.Shutdown();
	HalfDepth1.Shutdown();
	HalfDepth0.Shutdown();
	SecondaryDepth.Shutdown();
	PrimaryDepth.Shutdown();
	Aux.Shutdown();
	LinearDepth.Shutdown();
	Normal.Shutdown();
	Color.Shutdown();
	DepthStencil.Shutdown();

	Device = nullptr;
	Context = nullptr;
	Width = 0;
	Height = 0;
	bInitialized = false;
}

bool r3dDX11GBufferResources::Resize(int width, int height)
{
	if (!Device || !Context)
		return false;

	width = ClampDim(width);
	height = ClampDim(height);

	if (bInitialized && width == Width && height == Height)
		return true;

	DepthStencil.Shutdown();
	Color.Shutdown();
	Normal.Shutdown();
	LinearDepth.Shutdown();
	Aux.Shutdown();
	PrimaryDepth.Shutdown();
	SecondaryDepth.Shutdown();
	HalfDepth0.Shutdown();
	HalfDepth1.Shutdown();
	Particles.Shutdown();
	ScreenSmall0.Shutdown();
	ScreenSmall1.Shutdown();
	BlurQuarter.Shutdown();
	TempQuarter.Shutdown();
	One8_0.Shutdown();
	One8_1.Shutdown();
	One16_0.Shutdown();
	One16_1.Shutdown();
	One32_0.Shutdown();
	One32_1.Shutdown();
	One64_0.Shutdown();
	One64_1.Shutdown();
	Flashbang.Shutdown();
	PrevDepth.Shutdown();
	PrevSSAO.Shutdown();
	CurrentSSAO.Shutdown();

	Width = width;
	Height = height;

	if (!CreateTargets())
	{
		Shutdown();
		return false;
	}

	bInitialized = true;
	return true;
}

void r3dDX11GBufferResources::BeginGBuffer()
{
	if (!bInitialized || !Context)
		return;

	ID3D11RenderTargetView* views[] =
	{
		Color.GetRTV(),
		Normal.GetRTV(),
		LinearDepth.GetRTV(),
		Aux.GetRTV()
	};

	Context->OMSetRenderTargets(_countof(views), views, DepthStencil.GetDSV());

	D3D11_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(Width);
	viewport.Height = static_cast<float>(Height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	Context->RSSetViewports(1, &viewport);

	ClearGBuffer();
}

void r3dDX11GBufferResources::EndGBuffer()
{
	if (!Context)
		return;

	ID3D11RenderTargetView* nullViews[4] = {};
	Context->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);
}

void r3dDX11GBufferResources::ClearGBuffer()
{
	if (!Context)
		return;

	const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const float clearDepthValue[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	ClearTarget(Context, Color, clearColor);
	ClearTarget(Context, Normal, clearColor);
	ClearTarget(Context, LinearDepth, clearDepthValue);
	ClearTarget(Context, Aux, clearColor);

	if (DepthStencil.GetDSV())
		Context->ClearDepthStencilView(DepthStencil.GetDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

r3dDX11RenderTarget& r3dDX11GBufferResources::GetColor()
{
	return Color;
}

r3dDX11RenderTarget& r3dDX11GBufferResources::GetNormal()
{
	return Normal;
}

r3dDX11RenderTarget& r3dDX11GBufferResources::GetLinearDepth()
{
	return LinearDepth;
}

r3dDX11RenderTarget& r3dDX11GBufferResources::GetAux()
{
	return Aux;
}

r3dDX11RenderTarget& r3dDX11GBufferResources::GetPrimaryDepth()
{
	return PrimaryDepth;
}

r3dDX11RenderTarget& r3dDX11GBufferResources::GetSecondaryDepth()
{
	return SecondaryDepth;
}

r3dDX11RenderTarget& r3dDX11GBufferResources::GetHalfDepth0()
{
	return HalfDepth0;
}

r3dDX11RenderTarget& r3dDX11GBufferResources::GetHalfDepth1()
{
	return HalfDepth1;
}

r3dDX11RenderTarget& r3dDX11GBufferResources::GetParticles()
{
	return Particles;
}

r3dDX11RenderTarget& r3dDX11GBufferResources::GetScreenSmall0()
{
	return ScreenSmall0;
}

r3dDX11RenderTarget& r3dDX11GBufferResources::GetScreenSmall1()
{
	return ScreenSmall1;
}

r3dDX11RenderTarget& r3dDX11GBufferResources::GetPrevDepth()
{
	return PrevDepth;
}

r3dDX11RenderTarget& r3dDX11GBufferResources::GetPrevSSAO()
{
	return PrevSSAO;
}

r3dDX11RenderTarget& r3dDX11GBufferResources::GetCurrentSSAO()
{
	return CurrentSSAO;
}

const r3dDX11RenderTarget& r3dDX11GBufferResources::GetColor() const
{
	return Color;
}

const r3dDX11RenderTarget& r3dDX11GBufferResources::GetNormal() const
{
	return Normal;
}

const r3dDX11RenderTarget& r3dDX11GBufferResources::GetLinearDepth() const
{
	return LinearDepth;
}

const r3dDX11RenderTarget& r3dDX11GBufferResources::GetAux() const
{
	return Aux;
}

const r3dDX11RenderTarget& r3dDX11GBufferResources::GetPrimaryDepth() const
{
	return PrimaryDepth;
}

const r3dDX11RenderTarget& r3dDX11GBufferResources::GetSecondaryDepth() const
{
	return SecondaryDepth;
}

const r3dDX11RenderTarget& r3dDX11GBufferResources::GetHalfDepth0() const
{
	return HalfDepth0;
}

const r3dDX11RenderTarget& r3dDX11GBufferResources::GetHalfDepth1() const
{
	return HalfDepth1;
}

const r3dDX11RenderTarget& r3dDX11GBufferResources::GetParticles() const
{
	return Particles;
}

const r3dDX11RenderTarget& r3dDX11GBufferResources::GetScreenSmall0() const
{
	return ScreenSmall0;
}

const r3dDX11RenderTarget& r3dDX11GBufferResources::GetScreenSmall1() const
{
	return ScreenSmall1;
}

const r3dDX11RenderTarget& r3dDX11GBufferResources::GetPrevDepth() const
{
	return PrevDepth;
}

const r3dDX11RenderTarget& r3dDX11GBufferResources::GetPrevSSAO() const
{
	return PrevSSAO;
}

const r3dDX11RenderTarget& r3dDX11GBufferResources::GetCurrentSSAO() const
{
	return CurrentSSAO;
}

ID3D11DepthStencilView* r3dDX11GBufferResources::GetDepthStencilView() const
{
	return DepthStencil.GetDSV();
}

int r3dDX11GBufferResources::GetWidth() const
{
	return Width;
}

int r3dDX11GBufferResources::GetHeight() const
{
	return Height;
}

bool r3dDX11GBufferResources::IsInitialized() const
{
	return bInitialized;
}

bool r3dDX11GBufferResources::CreateTargets()
{
	const DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const DXGI_FORMAT depthFloatFormat = DXGI_FORMAT_R32_FLOAT;
	const DXGI_FORMAT hdrFormat = SelectSupportedRenderTargetFormat(Desc.HdrFormat, DXGI_FORMAT_R16G16B16A16_FLOAT);
	const DXGI_FORMAT screenSmallFormat = SelectSupportedRenderTargetFormat(DXGI_FORMAT_R10G10B10A2_UNORM, colorFormat);
	const DXGI_FORMAT flashbangFormat = SelectSupportedRenderTargetFormat(DXGI_FORMAT_B5G6R5_UNORM, colorFormat);

	if (!DepthStencil.Create(Device, Width, Height, DXGI_FORMAT_D24_UNORM_S8_UINT, R3D_DX11_BIND_DEPTH_STENCIL, "DX11.GBuffer.DepthStencil"))
		return false;

	if (!CreateRenderTarget(Color, "DX11.GBuffer.Color", Width, Height, colorFormat))
		return false;

	if (!CreateRenderTarget(Normal, "DX11.GBuffer.Normal", Width, Height, colorFormat))
		return false;

	if (!CreateRenderTarget(LinearDepth, "DX11.GBuffer.LinearDepth", Width, Height, depthFloatFormat))
		return false;

	if (!CreateRenderTarget(Aux, "DX11.GBuffer.Aux", Width, Height, colorFormat))
		return false;

	if (!CreateRenderTarget(PrimaryDepth, "DX11.GBuffer.PrimaryDepth", Width, Height, depthFloatFormat))
		return false;

	if (Desc.CreateDoubleDepth && !CreateRenderTarget(SecondaryDepth, "DX11.GBuffer.SecondaryDepth", Width, Height, depthFloatFormat))
		return false;

	if (!CreateRenderTarget(HalfDepth0, "DX11.GBuffer.HalfDepth0", Width / 2, Height / 2, depthFloatFormat))
		return false;

	if (Desc.CreateDoubleDepth && !CreateRenderTarget(HalfDepth1, "DX11.GBuffer.HalfDepth1", Width / 2, Height / 2, depthFloatFormat))
		return false;

	const int particleWidth = Desc.CreateHalfResParticles ? Width / 2 : Width;
	const int particleHeight = Desc.CreateHalfResParticles ? Height / 2 : Height;
	if (!CreateRenderTarget(Particles, "DX11.GBuffer.Particles", particleWidth, particleHeight, DXGI_FORMAT_R16G16B16A16_FLOAT))
		return false;

	if (!CreateRenderTarget(ScreenSmall0, "DX11.ScreenSmall0", Width / 2, Height / 2, screenSmallFormat))
		return false;

	if (!CreateRenderTarget(ScreenSmall1, "DX11.ScreenSmall1", Width / 2, Height / 2, screenSmallFormat))
		return false;

	const int quarterWidth = Width / 4;
	const int quarterHeight = Height / 4;
	if (!CreateRenderTarget(BlurQuarter, "DX11.BlurQuarter", quarterWidth, quarterHeight, hdrFormat))
		return false;

	if (!CreateRenderTarget(TempQuarter, "DX11.TempQuarter", quarterWidth, quarterHeight, hdrFormat))
		return false;

	if (!CreateRenderTarget(One8_0, "DX11.One8_0", quarterWidth / 2, quarterHeight / 2, hdrFormat))
		return false;

	if (!CreateRenderTarget(One8_1, "DX11.One8_1", quarterWidth / 2, quarterHeight / 2, hdrFormat))
		return false;

	if (!CreateRenderTarget(One16_0, "DX11.One16_0", quarterWidth / 4, quarterHeight / 4, hdrFormat))
		return false;

	if (!CreateRenderTarget(One16_1, "DX11.One16_1", quarterWidth / 4, quarterHeight / 4, hdrFormat))
		return false;

	if (!CreateRenderTarget(One32_0, "DX11.One32_0", quarterWidth / 8, quarterHeight / 8, hdrFormat))
		return false;

	if (!CreateRenderTarget(One32_1, "DX11.One32_1", quarterWidth / 8, quarterHeight / 8, hdrFormat))
		return false;

	if (!CreateRenderTarget(One64_0, "DX11.One64_0", quarterWidth / 16, quarterHeight / 16, hdrFormat))
		return false;

	if (!CreateRenderTarget(One64_1, "DX11.One64_1", quarterWidth / 16, quarterHeight / 16, hdrFormat))
		return false;

	if (!CreateRenderTarget(Flashbang, "DX11.Flashbang", Width / 2, Height / 2, flashbangFormat))
		return false;

	if (Desc.CreateTemporalSSAO)
	{
		if (!CreateRenderTarget(PrevDepth, "DX11.PrevDepth", Width, Height, depthFloatFormat))
			return false;

		if (!CreateRenderTarget(PrevSSAO, "DX11.PrevSSAO", Width, Height, colorFormat))
			return false;

		if (!CreateRenderTarget(CurrentSSAO, "DX11.CurrentSSAO", Width, Height, colorFormat))
			return false;
	}

	return true;
}

bool r3dDX11GBufferResources::CreateRenderTarget(r3dDX11RenderTarget& target, const char* name, int width, int height, DXGI_FORMAT format)
{
	return target.Create(Device, name, ClampDim(width), ClampDim(height), format, false, DXGI_FORMAT_UNKNOWN);
}

DXGI_FORMAT r3dDX11GBufferResources::SelectSupportedRenderTargetFormat(DXGI_FORMAT preferred, DXGI_FORMAT fallback) const
{
	UINT support = 0;

	if (Device && SUCCEEDED(Device->CheckFormatSupport(preferred, &support)))
	{
		const UINT required = D3D11_FORMAT_SUPPORT_RENDER_TARGET | D3D11_FORMAT_SUPPORT_SHADER_SAMPLE;
		if ((support & required) == required)
			return preferred;
	}

	return fallback;
}
