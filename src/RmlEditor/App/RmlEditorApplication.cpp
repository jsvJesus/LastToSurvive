#include "RmlEditorApplication.h"

#include "RmlEditorLog.h"

#include <algorithm>
#include <chrono>

#pragma comment(lib, "d3d9.lib")

RmlEditorApplication::RmlEditorApplication() = default;

RmlEditorApplication::~RmlEditorApplication()
{
	Shutdown();
}

bool RmlEditorApplication::Initialize(HINSTANCE Instance)
{
	if (Initialized)
		return true;

	RmlEditorLog::Initialize();
	RmlEditorLog::Write("[RmlEditor] Starting");

	if (!SetProcessDpiAwarenessContext(
		DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
	))
	{
		SetProcessDPIAware();
	}

	InstanceHandle = Instance;

	if (!RegisterWindowClass())
	{
		Shutdown();
		return false;
	}

	if (!CreateMainWindow())
	{
		Shutdown();
		return false;
	}

	if (!CreateDirect3DDevice())
	{
		MessageBoxW(
			WindowHandle,
			L"DirectX 9 device creation failed.",
			WindowTitle,
			MB_OK | MB_ICONERROR
		);

		Shutdown();
		return false;
	}

	if (!RmlHost.Initialize(WindowHandle, Device))
	{
		MessageBoxW(
			WindowHandle,
			L"RmlUi initialization failed.\n"
			L"Check RmlEditor.log and runtime DLL files.",
			WindowTitle,
			MB_OK | MB_ICONERROR
		);

		Shutdown();
		return false;
	}

	ShowWindow(WindowHandle, SW_SHOW);
	UpdateWindow(WindowHandle);

	Initialized = true;

	RmlEditorLog::Write(
		"[RmlEditor] Application initialized"
	);

	return true;
}

bool RmlEditorApplication::RegisterWindowClass()
{
	WNDCLASSEXW WindowClass{};

	WindowClass.cbSize = sizeof(WindowClass);
	WindowClass.style =
		CS_HREDRAW |
		CS_VREDRAW |
		CS_DBLCLKS;

	WindowClass.lpfnWndProc =
		&RmlEditorApplication::StaticWindowProcedure;

	WindowClass.hInstance = InstanceHandle;
	WindowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);

	WindowClass.hbrBackground =
		reinterpret_cast<HBRUSH>(
			GetStockObject(BLACK_BRUSH)
		);

	WindowClass.lpszClassName = WindowClassName;

	if (!RegisterClassExW(&WindowClass))
	{
		RmlEditorLog::Write(
			"[RmlEditor] RegisterClassExW failed: %lu",
			GetLastError()
		);

		return false;
	}

	ClassRegistered = true;
	return true;
}

bool RmlEditorApplication::CreateMainWindow()
{
	RECT WindowRectangle{
		0,
		0,
		ClientWidth,
		ClientHeight
	};

	const DWORD WindowStyle =
		WS_OVERLAPPEDWINDOW;

	AdjustWindowRect(
		&WindowRectangle,
		WindowStyle,
		FALSE
	);

	const int WindowWidth =
		WindowRectangle.right -
		WindowRectangle.left;

	const int WindowHeight =
		WindowRectangle.bottom -
		WindowRectangle.top;

	const int ScreenWidth =
		GetSystemMetrics(SM_CXSCREEN);

	const int ScreenHeight =
		GetSystemMetrics(SM_CYSCREEN);

	const int PositionX =
		std::max(0, (ScreenWidth - WindowWidth) / 2);

	const int PositionY =
		std::max(0, (ScreenHeight - WindowHeight) / 2);

	WindowHandle = CreateWindowExW(
		0,
		WindowClassName,
		WindowTitle,
		WindowStyle,
		PositionX,
		PositionY,
		WindowWidth,
		WindowHeight,
		nullptr,
		nullptr,
		InstanceHandle,
		this
	);

	if (!WindowHandle)
	{
		RmlEditorLog::Write(
			"[RmlEditor] CreateWindowExW failed: %lu",
			GetLastError()
		);

		return false;
	}

	UpdateClientSize();
	return true;
}

bool RmlEditorApplication::CreateDirect3DDevice()
{
	Direct3D = Direct3DCreate9(D3D_SDK_VERSION);

	if (!Direct3D)
	{
		RmlEditorLog::Write(
			"[RmlEditor][DX9] Direct3DCreate9 failed"
		);

		return false;
	}

	ZeroMemory(
		&PresentParameters,
		sizeof(PresentParameters)
	);

	PresentParameters.Windowed = TRUE;
	PresentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	PresentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	PresentParameters.BackBufferCount = 1;

	PresentParameters.BackBufferWidth =
		static_cast<UINT>(ClientWidth);

	PresentParameters.BackBufferHeight =
		static_cast<UINT>(ClientHeight);

	PresentParameters.hDeviceWindow = WindowHandle;

	PresentParameters.EnableAutoDepthStencil = FALSE;

	PresentParameters.PresentationInterval =
		D3DPRESENT_INTERVAL_ONE;

	const DWORD BaseFlags =
		D3DCREATE_FPU_PRESERVE;

	const DWORD DeviceFlags[] =
	{
		BaseFlags | D3DCREATE_HARDWARE_VERTEXPROCESSING,
		BaseFlags | D3DCREATE_MIXED_VERTEXPROCESSING,
		BaseFlags | D3DCREATE_SOFTWARE_VERTEXPROCESSING
	};

	HRESULT Result = E_FAIL;

	for (DWORD Flags : DeviceFlags)
	{
		Result = Direct3D->CreateDevice(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			WindowHandle,
			Flags,
			&PresentParameters,
			&Device
		);

		if (SUCCEEDED(Result))
			break;
	}

	if (FAILED(Result) || !Device)
	{
		RmlEditorLog::Write(
			"[RmlEditor][DX9] CreateDevice failed: 0x%08X",
			static_cast<unsigned int>(Result)
		);

		return false;
	}

	RmlEditorLog::Write(
		"[RmlEditor][DX9] Device created: %dx%d",
		ClientWidth,
		ClientHeight
	);

	return true;
}

void RmlEditorApplication::UpdateClientSize()
{
	if (!WindowHandle)
		return;

	RECT ClientRectangle{};
	GetClientRect(WindowHandle, &ClientRectangle);

	ClientWidth = std::max(
		1,
		static_cast<int>(
			ClientRectangle.right -
			ClientRectangle.left
		)
	);

	ClientHeight = std::max(
		1,
		static_cast<int>(
			ClientRectangle.bottom -
			ClientRectangle.top
		)
	);
}

bool RmlEditorApplication::ResetDevice()
{
	if (!Device || Minimized)
		return false;

	UpdateClientSize();

	if (ClientWidth <= 0 || ClientHeight <= 0)
		return false;

	RmlHost.OnDeviceLost();

	PresentParameters.BackBufferWidth =
		static_cast<UINT>(ClientWidth);

	PresentParameters.BackBufferHeight =
		static_cast<UINT>(ClientHeight);

	const HRESULT Result =
		Device->Reset(&PresentParameters);

	if (FAILED(Result))
	{
		RmlEditorLog::Write(
			"[RmlEditor][DX9] Reset failed: 0x%08X",
			static_cast<unsigned int>(Result)
		);

		return false;
	}

	DeviceResetPending = false;

	RmlHost.OnDeviceReset();

	RmlEditorLog::Write(
		"[RmlEditor][DX9] Reset completed"
	);

	return true;
}

bool RmlEditorApplication::CheckDeviceState()
{
	if (!Device)
		return false;

	const HRESULT Result =
		Device->TestCooperativeLevel();

	if (Result == D3DERR_DEVICELOST)
		return false;

	if (Result == D3DERR_DEVICENOTRESET)
		return ResetDevice();

	if (FAILED(Result))
		return false;

	if (DeviceResetPending && !InSizeMove)
		return ResetDevice();

	return true;
}

void RmlEditorApplication::Tick(float DeltaSeconds)
{
	if (!CheckDeviceState())
		return;

	RmlHost.Update(DeltaSeconds);
	Render();
}

void RmlEditorApplication::Render()
{
	if (!Device)
		return;

	const D3DCOLOR ClearColor =
		D3DCOLOR_ARGB(255, 11, 14, 18);

	Device->Clear(
		0,
		nullptr,
		D3DCLEAR_TARGET,
		ClearColor,
		1.0f,
		0
	);

	if (SUCCEEDED(Device->BeginScene()))
	{
		RmlHost.Render();
		Device->EndScene();
	}

	const HRESULT PresentResult =
		Device->Present(
			nullptr,
			nullptr,
			nullptr,
			nullptr
		);

	if (PresentResult == D3DERR_DEVICELOST)
		DeviceResetPending = true;
}

int RmlEditorApplication::Run()
{
	if (!Initialized)
		return -1;

	MSG Message{};

	auto PreviousTime =
		std::chrono::steady_clock::now();

	while (Message.message != WM_QUIT)
	{
		if (PeekMessageW(
			&Message,
			nullptr,
			0,
			0,
			PM_REMOVE
		))
		{
			TranslateMessage(&Message);
			DispatchMessageW(&Message);
			continue;
		}

		if (Minimized)
		{
			WaitMessage();

			PreviousTime =
				std::chrono::steady_clock::now();

			continue;
		}

		const auto CurrentTime =
			std::chrono::steady_clock::now();

		float DeltaSeconds =
			std::chrono::duration<float>(
				CurrentTime -
				PreviousTime
			).count();

		PreviousTime = CurrentTime;

		if (DeltaSeconds < 0.0f)
			DeltaSeconds = 0.0f;

		if (DeltaSeconds > 0.1f)
			DeltaSeconds = 0.1f;

		Tick(DeltaSeconds);
	}

	return static_cast<int>(Message.wParam);
}

void RmlEditorApplication::Shutdown()
{
	if (!InstanceHandle &&
		!WindowHandle &&
		!Device &&
		!Direct3D &&
		!ClassRegistered)
	{
		return;
	}

	Initialized = false;

	RmlHost.Shutdown();

	if (Device)
	{
		Device->Release();
		Device = nullptr;
	}

	if (Direct3D)
	{
		Direct3D->Release();
		Direct3D = nullptr;
	}

	if (WindowHandle && IsWindow(WindowHandle))
	{
		DestroyWindow(WindowHandle);
	}

	WindowHandle = nullptr;

	if (ClassRegistered && InstanceHandle)
	{
		UnregisterClassW(
			WindowClassName,
			InstanceHandle
		);

		ClassRegistered = false;
	}

	InstanceHandle = nullptr;

	RmlEditorLog::Write(
		"[RmlEditor] Application shutdown"
	);

	RmlEditorLog::Shutdown();
}

LRESULT RmlEditorApplication::HandleWindowMessage(
	HWND Window,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam
)
{
	switch (Message)
	{
	case WM_CLOSE:
		DestroyWindow(Window);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_ERASEBKGND:
		return 1;

	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* MinMaxInformation =
			reinterpret_cast<MINMAXINFO*>(LParam);

		MinMaxInformation->ptMinTrackSize.x = 960;
		MinMaxInformation->ptMinTrackSize.y = 640;

		return 0;
	}

	case WM_ENTERSIZEMOVE:
		InSizeMove = true;
		return 0;

	case WM_EXITSIZEMOVE:
		InSizeMove = false;
		UpdateClientSize();
		DeviceResetPending = true;
		return 0;

	case WM_SIZE:
	{
		if (WParam == SIZE_MINIMIZED)
		{
			Minimized = true;
			return 0;
		}

		Minimized = false;

		ClientWidth = std::max(
			1,
			static_cast<int>(LOWORD(LParam))
		);

		ClientHeight = std::max(
			1,
			static_cast<int>(HIWORD(LParam))
		);

		if (!InSizeMove)
			DeviceResetPending = true;

		return 0;
	}

	case WM_SYSKEYDOWN:
		if (WParam == VK_F4 &&
			(GetKeyState(VK_MENU) & 0x8000))
		{
			DestroyWindow(Window);
			return 0;
		}
		break;

	default:
		break;
	}

	if (RmlHost.IsInitialized())
	{
		LRESULT RmlResult = 0;

		if (RmlHost.ProcessWindowMessage(
			Window,
			Message,
			WParam,
			LParam,
			&RmlResult
		))
		{
			return RmlResult;
		}
	}

	return DefWindowProcW(
		Window,
		Message,
		WParam,
		LParam
	);
}

LRESULT CALLBACK
RmlEditorApplication::StaticWindowProcedure(
	HWND Window,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam
)
{
	RmlEditorApplication* Application =
		reinterpret_cast<RmlEditorApplication*>(
			GetWindowLongPtrW(
				Window,
				GWLP_USERDATA
			)
		);

	if (Message == WM_NCCREATE)
	{
		const CREATESTRUCTW* CreateData =
			reinterpret_cast<CREATESTRUCTW*>(
				LParam
			);

		Application =
			static_cast<RmlEditorApplication*>(
				CreateData->lpCreateParams
			);

		SetWindowLongPtrW(
			Window,
			GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>(
				Application
			)
		);
	}

	if (Application)
	{
		return Application->HandleWindowMessage(
			Window,
			Message,
			WParam,
			LParam
		);
	}

	return DefWindowProcW(
		Window,
		Message,
		WParam,
		LParam
	);
}