#pragma once

#include "../RmlContextHost.h"

class RmlFrontEndSystem final
{
public:
	RmlFrontEndSystem();
	~RmlFrontEndSystem();

	bool Init(
		HWND WindowHandle,
		IDirect3DDevice9* Device
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

	bool IsInitialized() const;

	Rml::Context* GetContext() const;

private:
	RmlContextHost ContextHost;
	bool bInitialized = false;
};