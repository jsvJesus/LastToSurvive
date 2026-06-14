#include "r3dPCH.h"
#include "r3d.h"

#include "RmlFrontEndContext.h"
#include "../RmlRuntime.h"

#include <algorithm>

RmlFrontEndContext::RmlFrontEndContext()
{
}

RmlFrontEndContext::~RmlFrontEndContext()
{
	Shutdown();
}

bool RmlFrontEndContext::Init(
	HWND WindowHandle,
	IDirect3DDevice9* Device
)
{
	if (bInitialized)
		return true;

	if (!WindowHandle || !Device)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd][Init] Invalid window or device\n"
		);

		return false;
	}

	Hwnd = WindowHandle;

	RefreshDimensions();

	RmlRuntime& Runtime =
		RmlRuntime::Get();

	if (!Runtime.Acquire(
		WindowHandle,
		Device
	))
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd][Fallback] Shared runtime failed\n"
		);

		Hwnd = nullptr;
		return false;
	}

	bRuntimeAcquired = true;

	Context = Runtime.CreateContext(
		"GameFrontEnd",
		Rml::Vector2i(
			Width,
			Height
		)
	);

	if (!Context)
	{
		Runtime.Release();

		bRuntimeAcquired = false;
		Hwnd = nullptr;

		r3dOutToLog(
			"[RmlUI][FrontEnd][Fallback] Context creation failed\n"
		);

		return false;
	}

	// Этап 1:
	// context пустой, ничего не рисует и input не принимает.
	Runtime.ClearActiveContext(
		Context
	);

	bInitialized = true;

	r3dOutToLog(
		"[RmlUI][FrontEnd][Init] Empty context created: %s\n",
		Context->GetName().c_str()
	);

	return true;
}

void RmlFrontEndContext::Shutdown()
{
	if (!bInitialized && !bRuntimeAcquired)
		return;

	RmlRuntime& Runtime =
		RmlRuntime::Get();

	Runtime.ClearActiveContext(
		Context
	);

	Runtime.DestroyContext(
		Context
	);

	if (bRuntimeAcquired)
	{
		Runtime.Release();
		bRuntimeAcquired = false;
	}

	Hwnd = nullptr;

	Width = 1;
	Height = 1;

	bInitialized = false;

	r3dOutToLog(
		"[RmlUI][FrontEnd][Shutdown] Empty context destroyed\n"
	);
}

void RmlFrontEndContext::Update()
{
	if (!bInitialized || !Context)
		return;

	RefreshDimensions();

	Context->Update();
}

bool RmlFrontEndContext::IsInitialized() const
{
	return bInitialized;
}

void RmlFrontEndContext::RefreshDimensions()
{
	if (!Hwnd)
	{
		Width = 1;
		Height = 1;
		return;
	}

	RECT ClientRectangle{};

	GetClientRect(
		Hwnd,
		&ClientRectangle
	);

	Width = std::max(
		1,
		static_cast<int>(
			ClientRectangle.right -
			ClientRectangle.left
		)
	);

	Height = std::max(
		1,
		static_cast<int>(
			ClientRectangle.bottom -
			ClientRectangle.top
		)
	);

	if (Context)
	{
		Context->SetDimensions(
			Rml::Vector2i(
				Width,
				Height
			)
		);
	}
}