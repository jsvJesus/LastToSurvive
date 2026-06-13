#pragma once

#include "../Rml/RmlEditorRmlHost.h"

#include <d3d9.h>
#include <windows.h>

class RmlEditorApplication final
{
public:
    RmlEditorApplication();
    ~RmlEditorApplication();

    bool Initialize(HINSTANCE Instance);
    int Run();

    void Shutdown();

private:
    static constexpr const wchar_t* WindowClassName =
        L"WarZRmlEditorWindowClass";

    static constexpr const wchar_t* WindowTitle =
        L"WarZ RML / RCSS Editor";

    HINSTANCE InstanceHandle = nullptr;
    HWND WindowHandle = nullptr;

    IDirect3D9* Direct3D = nullptr;
    IDirect3DDevice9* Device = nullptr;

    D3DPRESENT_PARAMETERS PresentParameters{};

    RmlEditorRmlHost RmlHost;

    int ClientWidth = 1600;
    int ClientHeight = 900;

    bool Initialized = false;
    bool ClassRegistered = false;
    bool Minimized = false;
    bool InSizeMove = false;
    bool DeviceResetPending = false;

    bool RegisterWindowClass();
    bool CreateMainWindow();
    bool CreateDirect3DDevice();

    bool ResetDevice();
    bool CheckDeviceState();

    void Tick(float DeltaSeconds);
    void Render();

    void UpdateClientSize();

    LRESULT HandleWindowMessage(
        HWND Window,
        UINT Message,
        WPARAM WParam,
        LPARAM LParam
    );

    static LRESULT CALLBACK StaticWindowProcedure(
        HWND Window,
        UINT Message,
        WPARAM WParam,
        LPARAM LParam
    );
};