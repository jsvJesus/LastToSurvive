#pragma once

#include "RmlEditorFileInterface.h"
#include "RmlEditorSystemInterface.h"

#include "../Rendering/RmlEditorRenderDX9.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementDocument.h>

#include <d3d9.h>
#include <windows.h>

#include <memory>
#include <string>

class RmlEditorRmlHost final
{
public:
    RmlEditorRmlHost();
    ~RmlEditorRmlHost();

    bool Initialize(
        HWND Window,
        IDirect3DDevice9* Device
    );

    void Shutdown();

    void Update(float DeltaSeconds);
    void Render();

    bool ProcessWindowMessage(
        HWND Window,
        UINT Message,
        WPARAM WParam,
        LPARAM LParam,
        LRESULT* Result
    );

    void OnDeviceLost();
    void OnDeviceReset();

    bool IsInitialized() const;

    Rml::Context* GetEditorContext() const;
    Rml::Context* GetPreviewContext() const;

private:
    HWND WindowHandle = nullptr;

    std::unique_ptr<RmlEditorSystemInterface>
        SystemInterface;

    std::unique_ptr<RmlEditorFileInterface>
        FileInterface;

    std::unique_ptr<RmlEditorRenderDX9>
        RenderInterface;

    Rml::Context* EditorContext = nullptr;
    Rml::Context* PreviewContext = nullptr;

    Rml::ElementDocument* EditorDocument = nullptr;

    std::wstring DataRoot;

    int ClientWidth = 1;
    int ClientHeight = 1;

    bool Initialized = false;
    bool RmlInitialized = false;
    bool MouseTrackingEnabled = false;

    void UpdateClientSize();
    void BeginMouseTracking(HWND Window);

    bool LoadEditorShell();
    void LoadFonts();

    static std::wstring GetDataRoot();

    static int GetKeyModifiers();

    static Rml::Input::KeyIdentifier TranslateKey(
        WPARAM Key
    );

    static int TranslateMouseButton(UINT Message);
};