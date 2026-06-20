#pragma once

#include "RENDERING/DX11/RenderDX11Device.h"
#include "RENDERING/DX11/RenderDX11DepthOnlyPass.h"
#include "RENDERING/DX11/RenderDX11Draw.h"
#include "RENDERING/DX11/RenderDX11FrameResources.h"
#include "RENDERING/DX11/RenderDX11GBufferPass.h"
#include "RENDERING/DX11/RenderDX11GBufferResources.h"
#include "RENDERING/DX11/RenderDX11States.h"
#include "RENDERING/DX11/RenderDX11Texture.h"
#include "RENDERING/DX11/ShaderDX11.h"

#ifndef _WINDEF_
struct HWND__;
typedef HWND__* HWND;
#endif

namespace Rml
{
	class Context;
}

class r3dCamera;
struct r3dDX11WorldRenderStats;

class r3dDX11Renderer final
{
public:
	r3dDX11Renderer();
	~r3dDX11Renderer();

	bool Init(HWND windowHandle, int width, int height, bool fullscreen, bool enableDebug, bool acquireRmlRuntime = true);
	void Shutdown();
	bool Resize(int width, int height);

	void BeginFrame(float clearR, float clearG, float clearB, float clearA);
	bool RenderWorldGBuffer(const r3dCamera& camera, r3dDX11WorldRenderStats* stats = nullptr);
	bool RenderWorldDepthOnly(const r3dCamera& camera, r3dDX11WorldRenderStats* stats = nullptr, bool clearDepth = true);
	bool ResolveSceneToBackBuffer();
	void RenderRmlContext(Rml::Context* context);
	void Present(bool vsync);
	bool EndFrame(bool vsync, Rml::Context* rmlContext = nullptr);

	r3dDX11Device& GetDevice();
	r3dDX11DrawContext& GetDrawContext();
	r3dDX11FrameResources& GetFrameResources();
	r3dDX11DepthOnlyPass& GetDepthOnlyPass();
	r3dDX11GBufferPass& GetGBufferPass();
	r3dDX11GBufferResources& GetGBufferResources();
	r3dDX11CommonStates& GetCommonStates();
	r3dDX11TextureLibrary& GetTextureLibrary();
	r3dDX11ShaderLibrary& GetShaderLibrary();
	const r3dDX11Device& GetDevice() const;
	const r3dDX11DrawContext& GetDrawContext() const;
	const r3dDX11FrameResources& GetFrameResources() const;
	const r3dDX11DepthOnlyPass& GetDepthOnlyPass() const;
	const r3dDX11GBufferPass& GetGBufferPass() const;
	const r3dDX11GBufferResources& GetGBufferResources() const;
	const r3dDX11CommonStates& GetCommonStates() const;
	const r3dDX11TextureLibrary& GetTextureLibrary() const;
	const r3dDX11ShaderLibrary& GetShaderLibrary() const;

	int GetWidth() const;
	int GetHeight() const;
	bool IsInitialized() const;

private:
	r3dDX11Device Device;
	r3dDX11DrawContext DrawContext;
	r3dDX11ShaderLibrary ShaderLibrary;
	r3dDX11CommonStates CommonStates;
	r3dDX11TextureLibrary TextureLibrary;
	r3dDX11FrameResources FrameResources;
	r3dDX11DepthOnlyPass DepthOnlyPass;
	r3dDX11GBufferPass GBufferPass;
	r3dDX11GBufferResources GBufferResources;
	HWND WindowHandle = nullptr;
	bool bInitialized = false;
	bool bRmlRuntimeAcquired = false;
};
