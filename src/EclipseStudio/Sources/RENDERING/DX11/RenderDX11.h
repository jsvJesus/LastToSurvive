#pragma once

#include "RENDERING/DX11/RenderDX11Device.h"
#include "RENDERING/DX11/RenderDX11FrameResources.h"
#include "RENDERING/DX11/RenderDX11GBufferResources.h"
#include "RENDERING/DX11/ShaderDX11.h"

#ifndef _WINDEF_
struct HWND__;
typedef HWND__* HWND;
#endif

namespace Rml
{
	class Context;
}

class r3dDX11Renderer final
{
public:
	r3dDX11Renderer();
	~r3dDX11Renderer();

	bool Init(HWND windowHandle, int width, int height, bool fullscreen, bool enableDebug);
	void Shutdown();
	bool Resize(int width, int height);

	void BeginFrame(float clearR, float clearG, float clearB, float clearA);
	bool ResolveSceneToBackBuffer();
	void RenderRmlContext(Rml::Context* context);
	void Present(bool vsync);
	bool EndFrame(bool vsync, Rml::Context* rmlContext = nullptr);

	r3dDX11Device& GetDevice();
	r3dDX11FrameResources& GetFrameResources();
	r3dDX11GBufferResources& GetGBufferResources();
	r3dDX11ShaderLibrary& GetShaderLibrary();
	const r3dDX11Device& GetDevice() const;
	const r3dDX11FrameResources& GetFrameResources() const;
	const r3dDX11GBufferResources& GetGBufferResources() const;
	const r3dDX11ShaderLibrary& GetShaderLibrary() const;

	int GetWidth() const;
	int GetHeight() const;
	bool IsInitialized() const;

private:
	r3dDX11Device Device;
	r3dDX11ShaderLibrary ShaderLibrary;
	r3dDX11FrameResources FrameResources;
	r3dDX11GBufferResources GBufferResources;
	HWND WindowHandle = nullptr;
	bool bInitialized = false;
	bool bRmlRuntimeAcquired = false;
};
