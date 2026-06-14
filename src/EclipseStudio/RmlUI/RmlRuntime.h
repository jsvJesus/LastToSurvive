#pragma once

#include <RmlUi/Core.h>

#include <d3d9.h>
#include <windows.h>

#include <memory>
#include <string>
#include <unordered_set>

class RmlRenderDX9;
class RmlSystemInterface;
class RmlFileInterface;

class RmlRuntime final
{
public:
	static RmlRuntime& Get();

	bool Acquire(
		HWND WindowHandle,
		IDirect3DDevice9* Device
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

	void OnDeviceLost();

	void OnDeviceReset(
		int Width,
		int Height
	);

	bool IsInitialized() const;
	int GetReferenceCount() const;

	const std::wstring& GetDataRoot() const;

private:
	RmlRuntime() = default;
	~RmlRuntime() = default;

	RmlRuntime(const RmlRuntime&) = delete;
	RmlRuntime& operator=(const RmlRuntime&) = delete;

	bool InitializeCore(
		HWND WindowHandle,
		IDirect3DDevice9* Device
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

	std::unique_ptr<RmlSystemInterface> SystemInterface;
	std::unique_ptr<RmlFileInterface> FileInterface;
	std::unique_ptr<RmlRenderDX9> RenderInterface;

	std::unordered_set<Rml::Context*> Contexts;

	Rml::Context* ActiveContext = nullptr;

	std::wstring DataRoot;

	unsigned int ContextSerial = 0;
	int ReferenceCount = 0;

	bool bInitialized = false;
	bool bCoreInitialized = false;
	bool bRenderFrameOpen = false;
};