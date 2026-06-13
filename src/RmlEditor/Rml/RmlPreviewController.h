#pragma once

#include "RmlDocumentSession.h"
#include "RmlEditorFileInterface.h"

#include "../Rendering/RmlEditorRenderDX9.h"
#include "../Rendering/RmlEditorViewport.h"

#include <RmlUi/Core/Context.h>

#include <string>

class RmlPreviewController final
{
public:
    bool Initialize(
        Rml::Context* Context,
        RmlEditorFileInterface* FileInterface,
        RmlEditorRenderDX9* RenderInterface
    );

    void Shutdown();

    bool OpenDocument(const std::wstring& FilePath);
    bool ReloadDocument();

    void Update();
    void Render(const RmlEditorViewport& Viewport);

    bool HasDocument() const;

    Rml::Vector2i ScreenToPreview(
        const RmlEditorViewport& Viewport,
        int X,
        int Y
    ) const;

    Rml::Context* GetContext() const;
    const RmlDocumentSession& GetSession() const;
    const std::string& GetLastError() const;

private:
    Rml::Context* PreviewContext = nullptr;
    RmlEditorFileInterface* FileInterface = nullptr;
    RmlEditorRenderDX9* RenderInterface = nullptr;

    RmlDocumentSession Session;

    std::string LastError;

    void ApplyDocumentDirectory(const std::wstring& Directory);
};
