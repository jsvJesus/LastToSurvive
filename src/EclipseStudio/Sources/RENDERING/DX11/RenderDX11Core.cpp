#include "r3dPCH.h"
#include "r3d.h"

#include "r3dRenderDX11.h"

#include "../../../../Eternity/Source/r3dRenderDX11.inl"

#include "rendering/DX11/RenderDX11Core.h"

#if LTS_STUDIO_DX11

namespace
{
	template <typename T>
	void RenderDX11Core_SafeRelease(
		T*& Object
	)
	{
		if (!Object)
			return;

		Object->Release();
		Object = 0;
	}

	void RenderDX11Core_Log(
		const char* Text
	)
	{
		if (!Text)
			return;

		OutputDebugStringA(Text);
		r3dOutToLog("%s", Text);
	}
}

RenderDX11Core::RenderDX11Core()
	: Device_(0)
	, Context_(0)
	, FeatureLevel_(D3D_FEATURE_LEVEL_10_0)
	, EngineDeviceInitialized_(false)
{
}

bool RenderDX11Core::Initialize(
	const RenderDX11CoreCreateDesc& Desc
)
{
	if (IsReady())
		return true;

	/*
	 * Очищаем возможное незавершённое состояние после
	 * предыдущей неудачной инициализации.
	 */
	Shutdown();

	r3dDX11DeviceCreateParams Params;

	Params.Window = Desc.Window;
	Params.Width = Desc.Width > 0 ? Desc.Width : 1;
	Params.Height = Desc.Height > 0 ? Desc.Height : 1;
	Params.Windowed = Desc.Windowed;
	Params.EnableDebugLayer = Desc.EnableDebugLayer;

	r3dRenderDX11& GraphicsDevice =
		r3dGetRenderDX11();

	if (!GraphicsDevice.Initialize(Params))
	{
		RenderDX11Core_Log(
			"[DX11][Core] r3dRenderDX11 initialization failed\n"
		);

		return false;
	}

	EngineDeviceInitialized_ = true;

	Device_ = GraphicsDevice.GetDevice();
	Context_ = GraphicsDevice.GetContext();
	FeatureLevel_ = GraphicsDevice.GetFeatureLevel();

	if (!Device_ || !Context_)
	{
		RenderDX11Core_Log(
			"[DX11][Core] Device or immediate context is null\n"
		);

		Shutdown();
		return false;
	}

	/*
	 * r3dRenderDX11 остаётся реальным владельцем устройства.
	 * RenderDX11Core держит собственные ссылки, пока живы
	 * ресурсы нового world renderer.
	 */
	Device_->AddRef();
	Context_->AddRef();

	RenderDX11Core_Log(
		"[DX11][Core] Device and immediate context acquired\n"
	);

	return true;
}

void RenderDX11Core::Shutdown()
{
	if (Context_)
	{
		/*
		 * Снимаем привязки ресурсов перед уничтожением
		 * renderer states, shaders и render targets.
		 */
		Context_->ClearState();
		Context_->Flush();
	}

	RenderDX11Core_SafeRelease(Context_);
	RenderDX11Core_SafeRelease(Device_);

	FeatureLevel_ =
		D3D_FEATURE_LEVEL_10_0;

	if (EngineDeviceInitialized_)
	{
		EngineDeviceInitialized_ = false;

		r3dGetRenderDX11().Shutdown();

		RenderDX11Core_Log(
			"[DX11][Core] Graphics device shutdown\n"
		);
	}
}

bool RenderDX11Core::IsReady() const
{
	return
		EngineDeviceInitialized_ &&
		Device_ != 0 &&
		Context_ != 0;
}

ID3D11Device* RenderDX11Core::GetDevice() const
{
	return Device_;
}

ID3D11DeviceContext* RenderDX11Core::GetContext() const
{
	return Context_;
}

D3D_FEATURE_LEVEL RenderDX11Core::GetFeatureLevel() const
{
	return FeatureLevel_;
}

bool RenderDX11Core::Resize(
	unsigned int Width,
	unsigned int Height
)
{
	if (!IsReady())
		return false;

	if (Width < 1)
		Width = 1;

	if (Height < 1)
		Height = 1;

	return r3dGetRenderDX11().Resize(
		Width,
		Height
	);
}

bool RenderDX11Core::CopyToBackBuffer(
	ID3D11Texture2D* SourceTexture
)
{
	if (!IsReady() || !SourceTexture)
		return false;

	return r3dGetRenderDX11().CopyToBackBuffer(
		SourceTexture
	);
}

bool RenderDX11Core::Present()
{
	if (!IsReady())
		return false;

	return r3dGetRenderDX11().Present();
}

RenderDX11Core& RenderDX11_GetCore()
{
	static RenderDX11Core Core;
	return Core;
}

#endif // LTS_STUDIO_DX11