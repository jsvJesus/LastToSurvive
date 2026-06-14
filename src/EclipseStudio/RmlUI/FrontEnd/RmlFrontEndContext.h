#pragma once

#include <RmlUi/Core.h>

#include <d3d9.h>
#include <windows.h>

class RmlFrontEndContext final
{
public:
    RmlFrontEndContext();
    ~RmlFrontEndContext();

    bool Init(
        HWND WindowHandle,
        IDirect3DDevice9* Device
    );

    void Shutdown();

    void Update();

    bool IsInitialized() const;

private:
    void RefreshDimensions();

private:
    HWND Hwnd = nullptr;
    Rml::Context* Context = nullptr;

    int Width = 1;
    int Height = 1;

    bool bInitialized = false;
    bool bRuntimeAcquired = false;
};