#include "r3dPCH.h"
#include "r3d.h"

#include "RmlFrontEndSystem.h"

RmlFrontEndSystem::RmlFrontEndSystem()
{
}

RmlFrontEndSystem::~RmlFrontEndSystem()
{
	Shutdown();
}

bool RmlFrontEndSystem::Init(
	HWND WindowHandle,
	IDirect3DDevice9* Device
)
{
	if (bInitialized)
		return true;

	r3dOutToLog(
		"[RmlUI][FrontEnd][Init] Creating empty GameFrontEnd context\n"
	);

	if (!ContextHost.Init(
		WindowHandle,
		Device,
		"GameFrontEnd"
	))
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd][Fallback] Context creation failed; "
			"Scaleform remains active\n"
		);

		return false;
	}

	// Этап 1:
	// context существует, но ничего не рисует
	// и не получает input.
	ContextHost.SetInputEnabled(false);
	ContextHost.SetRenderEnabled(false);

	bInitialized = true;

	r3dOutToLog(
		"[RmlUI][FrontEnd][Init] Empty context created: %s\n",
		ContextHost.GetContext()->GetName().c_str()
	);

	return true;
}

void RmlFrontEndSystem::Shutdown()
{
	if (!bInitialized)
		return;

	r3dOutToLog(
		"[RmlUI][FrontEnd][Shutdown] Begin\n"
	);

	ContextHost.SetInputEnabled(false);
	ContextHost.SetRenderEnabled(false);
	ContextHost.Shutdown();

	bInitialized = false;

	r3dOutToLog(
		"[RmlUI][FrontEnd][Shutdown] Complete\n"
	);
}

void RmlFrontEndSystem::Update(
	float DeltaSeconds
)
{
	if (!bInitialized)
		return;

	ContextHost.Update(
		DeltaSeconds
	);
}

void RmlFrontEndSystem::Render()
{
	if (!bInitialized)
		return;

	ContextHost.Render();
}

bool RmlFrontEndSystem::ProcessWin32Message(
	HWND WindowHandle,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam,
	LRESULT* OutResult
)
{
	if (!bInitialized)
	{
		if (OutResult)
			*OutResult = 0;

		return false;
	}

	return ContextHost.ProcessWin32Message(
		WindowHandle,
		Message,
		WParam,
		LParam,
		OutResult
	);
}

void RmlFrontEndSystem::OnDeviceLost()
{
	if (!bInitialized)
		return;

	ContextHost.OnDeviceLost();
}

void RmlFrontEndSystem::OnDeviceReset()
{
	if (!bInitialized)
		return;

	ContextHost.OnDeviceReset();
}

bool RmlFrontEndSystem::IsInitialized() const
{
	return bInitialized;
}

Rml::Context* RmlFrontEndSystem::GetContext() const
{
	return ContextHost.GetContext();
}