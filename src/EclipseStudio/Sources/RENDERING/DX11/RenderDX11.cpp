#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11.h"

#include "../../../RmlUI/RmlRuntime.h"

namespace
{
	const char* FindExistingTextureSmokePath()
	{
		static const char* candidates[] =
		{
			"Data\\Shaders\\Texture\\MissingTexture.dds",
			"bin\\Data\\Shaders\\Texture\\MissingTexture.dds",
			"Data\\Shaders\\Texture\\White.dds",
			"bin\\Data\\Shaders\\Texture\\White.dds"
		};

		for (int i = 0; i < _countof(candidates); ++i)
		{
			const DWORD attributes = GetFileAttributesA(candidates[i]);
			if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				return candidates[i];
		}

		return nullptr;
	}
}

r3dDX11Renderer::r3dDX11Renderer()
{
}

r3dDX11Renderer::~r3dDX11Renderer()
{
	Shutdown();
}

bool r3dDX11Renderer::Init(HWND windowHandle, int width, int height, bool fullscreen, bool enableDebug)
{
	if (bInitialized)
		return true;

	if (!windowHandle || width <= 0 || height <= 0)
		return false;

	WindowHandle = windowHandle;

	if (!Device.Init(windowHandle, width, height, fullscreen, enableDebug))
	{
		r3dOutToLog("[DX11] Device initialization failed\n");
		Shutdown();
		return false;
	}

	DrawContext.Init(Device.GetContext());

	if (!ShaderLibrary.Init(Device.GetDevice()))
	{
		r3dOutToLog("[DX11] Shader library initialization failed\n");
		Shutdown();
		return false;
	}

	if (ShaderLibrary.AddVertexShader("VS_FULLSCREEN", "Fullscreen_vs.hls") < 0 ||
		ShaderLibrary.AddPixelShader("PS_COPY", "copy_ps.hls") < 0)
	{
		r3dOutToLog("[DX11] Built-in shader registration failed: %s\n", ShaderLibrary.GetLastError().c_str());
		Shutdown();
		return false;
	}

	if (!CommonStates.Init(Device.GetDevice()))
	{
		r3dOutToLog("[DX11] Common state initialization failed\n");
		Shutdown();
		return false;
	}

	if (!TextureLibrary.Init(Device.GetDevice()))
	{
		r3dOutToLog("[DX11] Texture library initialization failed\n");
		Shutdown();
		return false;
	}

	if (const char* textureSmokePath = FindExistingTextureSmokePath())
	{
		r3dDX11Texture* textureSmoke = TextureLibrary.LoadTexture(textureSmokePath);
		if (textureSmoke)
		{
			r3dOutToLog(
				"[DX11] Texture smoke loaded '%s' %dx%d mips=%d format=%d\n",
				textureSmokePath,
				textureSmoke->GetWidth(),
				textureSmoke->GetHeight(),
				textureSmoke->GetMipCount(),
				static_cast<int>(textureSmoke->GetFormat())
			);
		}
		else
		{
			r3dOutToLog("[DX11] Texture smoke failed '%s': %s\n", textureSmokePath, TextureLibrary.GetLastError().c_str());
		}
	}

	if (!FrameResources.Init(Device.GetDevice(), Device.GetContext(), &DrawContext, &ShaderLibrary, &CommonStates, width, height))
	{
		r3dOutToLog("[DX11] Frame resources initialization failed\n");
		Shutdown();
		return false;
	}

	r3dDX11GBufferDesc gbufferDesc;
	if (!GBufferResources.Init(Device.GetDevice(), Device.GetContext(), width, height, gbufferDesc))
	{
		r3dOutToLog("[DX11] GBuffer resources initialization failed\n");
		Shutdown();
		return false;
	}

	if (!GBufferPass.Init(Device.GetDevice(), &DrawContext, &ShaderLibrary, &CommonStates))
	{
		r3dOutToLog("[DX11] GBuffer pass initialization failed\n");
		Shutdown();
		return false;
	}

	if (!RmlRuntime::Get().Acquire(windowHandle, Device.GetDevice(), Device.GetContext()))
	{
		r3dOutToLog("[DX11] Rml runtime initialization failed\n");
		Shutdown();
		return false;
	}

	bRmlRuntimeAcquired = true;
	bInitialized = true;

	r3dOutToLog("[DX11] Renderer initialized %dx%d\n", width, height);
	return true;
}

void r3dDX11Renderer::Shutdown()
{
	if (bRmlRuntimeAcquired)
	{
		RmlRuntime::Get().Release();
		bRmlRuntimeAcquired = false;
	}

	GBufferPass.Shutdown();
	GBufferResources.Shutdown();
	FrameResources.Shutdown();
	TextureLibrary.Shutdown();
	CommonStates.Shutdown();
	ShaderLibrary.Shutdown();
	DrawContext.Shutdown();
	Device.Shutdown();

	WindowHandle = nullptr;
	bInitialized = false;
}

bool r3dDX11Renderer::Resize(int width, int height)
{
	if (!bInitialized || width <= 0 || height <= 0)
		return false;

	if (!Device.Resize(width, height))
	{
		r3dOutToLog("[DX11] Device resize failed: %dx%d\n", width, height);
		return false;
	}

	if (!FrameResources.Resize(width, height))
	{
		r3dOutToLog("[DX11] Frame resources resize failed: %dx%d\n", width, height);
		return false;
	}

	if (!GBufferResources.Resize(width, height))
	{
		r3dOutToLog("[DX11] GBuffer resources resize failed: %dx%d\n", width, height);
		return false;
	}

	RmlRuntime::Get().OnDeviceResetDX11(width, height);
	return true;
}

void r3dDX11Renderer::BeginFrame(float clearR, float clearG, float clearB, float clearA)
{
	if (!bInitialized)
		return;

	FrameResources.BeginScene(clearR, clearG, clearB, clearA);
}

bool r3dDX11Renderer::ResolveSceneToBackBuffer()
{
	if (!bInitialized)
		return false;

	return FrameResources.CopySceneToBackBuffer(
		Device.GetBackBufferRTV(),
		Device.GetDepthStencilView()
	);
}

void r3dDX11Renderer::RenderRmlContext(Rml::Context* context)
{
	if (!bInitialized || !context)
		return;

	RmlRuntime::Get().RenderContextDX11(
		context,
		Device.GetBackBufferRTV(),
		Device.GetDepthStencilView(),
		Device.GetWidth(),
		Device.GetHeight()
	);
}

void r3dDX11Renderer::Present(bool vsync)
{
	if (bInitialized)
		Device.Present(vsync);
}

bool r3dDX11Renderer::EndFrame(bool vsync, Rml::Context* rmlContext)
{
	if (!bInitialized)
		return false;

	const bool resolved = ResolveSceneToBackBuffer();

	if (rmlContext)
		RenderRmlContext(rmlContext);

	Present(vsync);
	return resolved;
}

r3dDX11Device& r3dDX11Renderer::GetDevice()
{
	return Device;
}

r3dDX11DrawContext& r3dDX11Renderer::GetDrawContext()
{
	return DrawContext;
}

r3dDX11FrameResources& r3dDX11Renderer::GetFrameResources()
{
	return FrameResources;
}

r3dDX11GBufferPass& r3dDX11Renderer::GetGBufferPass()
{
	return GBufferPass;
}

r3dDX11GBufferResources& r3dDX11Renderer::GetGBufferResources()
{
	return GBufferResources;
}

r3dDX11CommonStates& r3dDX11Renderer::GetCommonStates()
{
	return CommonStates;
}

r3dDX11TextureLibrary& r3dDX11Renderer::GetTextureLibrary()
{
	return TextureLibrary;
}

r3dDX11ShaderLibrary& r3dDX11Renderer::GetShaderLibrary()
{
	return ShaderLibrary;
}

const r3dDX11Device& r3dDX11Renderer::GetDevice() const
{
	return Device;
}

const r3dDX11DrawContext& r3dDX11Renderer::GetDrawContext() const
{
	return DrawContext;
}

const r3dDX11FrameResources& r3dDX11Renderer::GetFrameResources() const
{
	return FrameResources;
}

const r3dDX11GBufferPass& r3dDX11Renderer::GetGBufferPass() const
{
	return GBufferPass;
}

const r3dDX11GBufferResources& r3dDX11Renderer::GetGBufferResources() const
{
	return GBufferResources;
}

const r3dDX11CommonStates& r3dDX11Renderer::GetCommonStates() const
{
	return CommonStates;
}

const r3dDX11TextureLibrary& r3dDX11Renderer::GetTextureLibrary() const
{
	return TextureLibrary;
}

const r3dDX11ShaderLibrary& r3dDX11Renderer::GetShaderLibrary() const
{
	return ShaderLibrary;
}

int r3dDX11Renderer::GetWidth() const
{
	return Device.GetWidth();
}

int r3dDX11Renderer::GetHeight() const
{
	return Device.GetHeight();
}

bool r3dDX11Renderer::IsInitialized() const
{
	return bInitialized;
}
