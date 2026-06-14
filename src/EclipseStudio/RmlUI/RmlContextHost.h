#pragma once

#include <RmlUi/Core.h>

#include <d3d9.h>
#include <windows.h>

class RmlContextHost final
{
public:
	RmlContextHost();
	~RmlContextHost();

	bool Init(
		HWND WindowHandle,
		IDirect3DDevice9* Device,
		const char* ContextBaseName
	);

	void Shutdown();

	void Update(float DeltaSeconds);
	void Render();

	bool ProcessWin32Message(
		HWND WindowHandle,
		UINT Message,
		WPARAM WParam,
		LPARAM LParam,
		LRESULT* OutResult
	);

	void OnDeviceLost();
	void OnDeviceReset();

	void RefreshDimensions();

	void SetInputEnabled(bool Enabled);
	void SetRenderEnabled(bool Enabled);

	bool IsInitialized() const;
	bool IsInputEnabled() const;
	bool IsRenderEnabled() const;

	int GetWidth() const;
	int GetHeight() const;

	Rml::Context* GetContext() const;

private:
	HWND Hwnd = nullptr;
	Rml::Context* Context = nullptr;

	int Width = 1;
	int Height = 1;

	bool bInitialized = false;
	bool bInputEnabled = false;
	bool bRenderEnabled = false;
};