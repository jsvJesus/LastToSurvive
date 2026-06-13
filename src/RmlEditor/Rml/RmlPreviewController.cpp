#include "RmlPreviewController.h"

#include "../App/RmlEditorLog.h"

#include <filesystem>

bool RmlPreviewController::Initialize(
    Rml::Context* Context,
    RmlEditorFileInterface* InFileInterface,
    RmlEditorRenderDX9* InRenderInterface
)
{
    if (!Context || !InFileInterface || !InRenderInterface)
    {
        LastError = "Preview controller initialization failed.";
        return false;
    }

    PreviewContext = Context;
    FileInterface = InFileInterface;
    RenderInterface = InRenderInterface;

    PreviewContext->SetDimensions(
        Rml::Vector2i(
            Session.GetLogicalWidth(),
            Session.GetLogicalHeight()
        )
    );

    return true;
}

void RmlPreviewController::Shutdown()
{
    Session.Close(PreviewContext);

    PreviewContext = nullptr;
    FileInterface = nullptr;
    RenderInterface = nullptr;
    LastError.clear();
}

bool RmlPreviewController::OpenDocument(
    const std::wstring& FilePath
)
{
    if (!PreviewContext || !FileInterface || !RenderInterface)
    {
        LastError = "Preview controller is not initialized.";
        return false;
    }

    const std::wstring Directory =
        std::filesystem::path(FilePath).parent_path().wstring();

    ApplyDocumentDirectory(Directory);

    PreviewContext->SetDimensions(
        Rml::Vector2i(
            Session.GetLogicalWidth(),
            Session.GetLogicalHeight()
        )
    );

    const bool Loaded = Session.Open(PreviewContext, FilePath);

    LastError = Session.GetLastError();

    if (!Loaded)
    {
        RmlEditorLog::Write(
            "[RmlEditor] Preview open failed: %s",
            LastError.c_str()
        );
    }

    return Loaded;
}

bool RmlPreviewController::ReloadDocument()
{
    if (!PreviewContext)
    {
        LastError = "Preview context is not available.";
        return false;
    }

    ApplyDocumentDirectory(Session.GetDocumentDirectory());

    const bool Loaded = Session.Reload(PreviewContext);

    LastError = Session.GetLastError();

    if (!Loaded)
    {
        RmlEditorLog::Write(
            "[RmlEditor] Preview reload failed: %s",
            LastError.c_str()
        );
    }

    return Loaded;
}

void RmlPreviewController::Update()
{
    if (PreviewContext)
        PreviewContext->Update();
}

void RmlPreviewController::Render(
    const RmlEditorViewport& Viewport
)
{
    if (!PreviewContext ||
        !RenderInterface ||
        !Session.HasDocument() ||
        !Viewport.IsValid())
    {
        return;
    }

    RenderInterface->BeginViewportFrame(Viewport);
    PreviewContext->Render();
    RenderInterface->EndViewportFrame();
}

bool RmlPreviewController::HasDocument() const
{
    return Session.HasDocument();
}

Rml::Vector2i RmlPreviewController::ScreenToPreview(
    const RmlEditorViewport& Viewport,
    int X,
    int Y
) const
{
    return Viewport.ScreenToLogical(X, Y);
}

Rml::Context* RmlPreviewController::GetContext() const
{
    return PreviewContext;
}

const RmlDocumentSession& RmlPreviewController::GetSession() const
{
    return Session;
}

const std::string& RmlPreviewController::GetLastError() const
{
    return LastError;
}

void RmlPreviewController::ApplyDocumentDirectory(
    const std::wstring& Directory
)
{
    if (FileInterface)
        FileInterface->SetDocumentDirectory(Directory.c_str());

    if (RenderInterface)
        RenderInterface->SetDocumentDirectory(Directory.c_str());
}
