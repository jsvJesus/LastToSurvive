#include "r3dPCH.h"
#include "r3d.h"

#include "RmlContextHost.h"
#include "RmlRuntime.h"

#include <algorithm>

RmlContextHost::RmlContextHost()
{
}

RmlContextHost::~RmlContextHost()
{
	Shutdown();
}

bool RmlContextHost::Init(
	HWND WindowHandle,
	IDirect3DDevice9* Device,
	const char* ContextBaseName
)
{
	if (bInitialized)
		return true;

	if (!WindowHandle || !Device)
	{
		r3dOutToLog(
			"[RmlUI][ContextHost][Init] Invalid window or device\n"
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
		Hwnd = nullptr;
		return false;
	}

	Context = Runtime.CreateContext(
		ContextBaseName,
		Rml::Vector2i(Width, Height)
	);

	if (!Context)
	{
		Runtime.Release();

		Hwnd = nullptr;
		return false;
	}

	bInitialized = true;
	bInputEnabled = false;
	bRenderEnabled = false;

	r3dOutToLog(
		"[RmlUI][ContextHost][Init] Context ready: %s\n",
		Context->GetName().c_str()
	);

	return true;
}

void RmlContextHost::Shutdown()
{
	if (!bInitialized && !Context)
		return;

	SetInputEnabled(false);
	bRenderEnabled = false;

	RmlRuntime& Runtime =
		RmlRuntime::Get();

	Runtime.DestroyContext(Context);
	Runtime.Release();

	Hwnd = nullptr;

	Width = 1;
	Height = 1;

	bInitialized = false;
	bInputEnabled = false;
	bRenderEnabled = false;

	r3dOutToLog(
		"[RmlUI][ContextHost][Shutdown] Complete\n"
	);
}

void RmlContextHost::Update(
	float DeltaSeconds
)
{
	(void)DeltaSeconds;

	if (!bInitialized || !Context)
		return;

	RefreshDimensions();

	Context->Update();
}

void RmlContextHost::Render()
{
	if (
		!bInitialized ||
		!Context ||
		!bRenderEnabled
	)
	{
		return;
	}

	RmlRuntime::Get().RenderContext(
		Context,
		Width,
		Height
	);
}

bool RmlContextHost::ProcessWin32Message(
	HWND WindowHandle,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam,
	LRESULT* OutResult
)
{
	if (Message == WM_SIZE)
		RefreshDimensions();

	if (
		!bInitialized ||
		!Context ||
		!bInputEnabled
	)
	{
		if (OutResult)
			*OutResult = 0;

		return false;
	}

	return RmlRuntime::Get().ProcessWin32Message(
		Context,
		WindowHandle,
		Message,
		WParam,
		LParam,
		OutResult
	);
}

void RmlContextHost::OnDeviceLost()
{
	if (!bInitialized)
		return;

	RmlRuntime::Get().OnDeviceLost();
}

void RmlContextHost::OnDeviceReset()
{
	if (!bInitialized)
		return;

	RefreshDimensions();

	RmlRuntime::Get().OnDeviceReset(
		Width,
		Height
	);
}

void RmlContextHost::RefreshDimensions()
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

void RmlContextHost::SetInputEnabled(
	bool Enabled
)
{
	if (!bInitialized || !Context)
	{
		bInputEnabled = false;
		return;
	}

	bInputEnabled = Enabled;

	if (Enabled)
	{
		RmlRuntime::Get().SetActiveContext(
			Context
		);
	}
	else
	{
		RmlRuntime::Get().ClearActiveContext(
			Context
		);
	}
}

void RmlContextHost::SetRenderEnabled(
	bool Enabled
)
{
	bRenderEnabled =
		bInitialized &&
		Context &&
		Enabled;
}

bool RmlContextHost::IsInitialized() const
{
	return bInitialized;
}

bool RmlContextHost::IsInputEnabled() const
{
	return bInputEnabled;
}

bool RmlContextHost::IsRenderEnabled() const
{
	return bRenderEnabled;
}

int RmlContextHost::GetWidth() const
{
	return Width;
}

int RmlContextHost::GetHeight() const
{
	return Height;
}

Rml::Context* RmlContextHost::GetContext() const
{
	return Context;
}