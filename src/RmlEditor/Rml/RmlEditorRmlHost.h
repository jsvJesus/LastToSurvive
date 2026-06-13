#pragma once

#include "RmlEditorFileInterface.h"
#include "RmlPreviewController.h"
#include "RmlEditorSystemInterface.h"

#include "../Editor/RmlEditorShellController.h"
#include "../Rendering/RmlEditorRenderDX9.h"
#include "../Rendering/RmlEditorViewport.h"

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
    RmlPreviewController PreviewController;
    RmlEditorViewport PreviewViewport;
    RmlEditorShellController ShellController;

    std::wstring DataRoot;

    int ClientWidth = 1;
    int ClientHeight = 1;

    bool Initialized = false;
    bool RmlInitialized = false;
    bool MouseTrackingEnabled = false;
    bool MouseInPreview = false;
    bool MouseCapturedByPreview = false;

    void UpdateClientSize();
    void BeginMouseTracking(HWND Window);

    bool LoadEditorShell();
    void LoadFonts();
    void AttachShellController();

    void OpenDocumentFromDialog();
    void ReloadDocument();
    bool OpenDocument(const std::wstring& FilePath);

    void UpdatePreviewViewport();
    void UpdateEditorShellForCurrentDocument();
    void UpdateEditorShellStatus(const std::string& StatusText);
    bool ExecuteShellCommandAt(int MouseX, int MouseY);

    static std::wstring GetDataRoot();
    static std::wstring GetOpenDialogInitialDirectory(
        const std::wstring& DataRoot
    );

    static int GetKeyModifiers();

    static Rml::Input::KeyIdentifier TranslateKey(
        WPARAM Key
    );

    static int TranslateMouseButton(UINT Message);
};
