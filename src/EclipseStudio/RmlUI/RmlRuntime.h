#pragma once

#include <RmlUi/Core.h>

#include <windows.h>

#include <memory>
#include <string>
#include <unordered_set>

class RmlRenderDX9;
class RmlRenderDX11;
class RmlSystemInterface;
class RmlFileInterface;

struct IDirect3DDevice9;
struct ID3D11DepthStencilView;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

class RmlRuntime final
{
public:
	static RmlRuntime& Get();

	~RmlRuntime();

	bool Acquire(
		HWND WindowHandle,
		IDirect3DDevice9* Device
	);

	bool Acquire(
		HWND WindowHandle,
		ID3D11Device* Device,
		ID3D11DeviceContext* Context
	);

	void Release();

	Rml::Context* CreateContext(
		const char* BaseName,
		const Rml::Vector2i& Dimensions
	);

	void DestroyContext(
		Rml::Context*& Context
	);

	void SetActiveContext(
		Rml::Context* Context
	);

	void ClearActiveContext(
		Rml::Context* Context
	);

	bool IsActiveContext(
		const Rml::Context* Context
	) const;

	bool ProcessWin32Message(
		Rml::Context* Context,
		HWND WindowHandle,
		UINT Message,
		WPARAM WParam,
		LPARAM LParam,
		LRESULT* OutResult
	);

	void RenderContext(
		Rml::Context* Context,
		int Width,
		int Height
	);

	void RenderContextDX11(
		Rml::Context* Context,
		ID3D11RenderTargetView* RenderTarget,
		ID3D11DepthStencilView* DepthStencil,
		int Width,
		int Height
	);

	void SetCharacterPreviewTexture(
		IDirect3DTexture9* Texture
	);

	void SetCharacterPortraitTexture(
		IDirect3DTexture9* Texture
	);

	void SetCharacterPreviewTextureDX11(
		ID3D11ShaderResourceView* Texture,
		int Width,
		int Height
	);

	void SetCharacterPortraitTextureDX11(
		ID3D11ShaderResourceView* Texture,
		int Width,
		int Height
	);

	void OnDeviceLost();

	void OnDeviceReset(
		int Width,
		int Height
	);

	void OnDeviceResetDX11(
		int Width,
		int Height
	);

	bool EnsureDebugger(
		Rml::Context* Context
	);

	void SetDebuggerVisible(
		Rml::Context* Context,
		bool bVisible
	);

	bool IsDebuggerVisible(
		const Rml::Context* Context
	) const;

	bool IsInitialized() const;
	bool IsUsingDX9() const;
	bool IsUsingDX11() const;
	int GetReferenceCount() const;

	const std::wstring& GetDataRoot() const;

private:
	RmlRuntime();

	RmlRuntime(const RmlRuntime&) = delete;
	RmlRuntime& operator=(const RmlRuntime&) = delete;

	bool InitializeCore(
		HWND WindowHandle,
		IDirect3DDevice9* Device
	);

	bool InitializeCore(
		HWND WindowHandle,
		ID3D11Device* Device,
		ID3D11DeviceContext* Context
	);

	void ShutdownCore();

	bool IsRegisteredContext(
		const Rml::Context* Context
	) const;

	static std::wstring BuildDataRoot();

	static int GetKeyModifiers();

	static Rml::Input::KeyIdentifier TranslateKey(
		WPARAM WParam
	);

	static int TranslateMouseButton(
		UINT Message
	);

private:
	HWND Hwnd = nullptr;
	IDirect3DDevice9* Device = nullptr;
	ID3D11Device* Device11 = nullptr;
	ID3D11DeviceContext* Context11 = nullptr;

	std::unique_ptr<RmlSystemInterface> SystemInterface;
	std::unique_ptr<RmlFileInterface> FileInterface;
	std::unique_ptr<RmlRenderDX9> RenderInterface;
	std::unique_ptr<RmlRenderDX11> RenderInterface11;

	std::unordered_set<Rml::Context*> Contexts;

	Rml::Context* ActiveContext = nullptr;
	Rml::Context* DebuggerContext = nullptr;

	std::wstring DataRoot;

	unsigned int ContextSerial = 0;
	int ReferenceCount = 0;

	bool bInitialized = false;
	bool bCoreInitialized = false;
	bool bRenderFrameOpen = false;
	bool bDebuggerInitialized = false;
};
