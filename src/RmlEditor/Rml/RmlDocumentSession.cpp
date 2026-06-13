#include "RmlDocumentSession.h"

#include "../App/RmlEditorLog.h"

#include <RmlUi/Core/Context.h>

#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>

namespace
{
    bool FileExists(const std::wstring& Path)
    {
        return std::filesystem::exists(
            std::filesystem::path(Path)
        );
    }

    std::wstring NormalizePath(const std::wstring& Path)
    {
        std::error_code Error;

        std::filesystem::path Absolute =
            std::filesystem::absolute(
                std::filesystem::path(Path),
                Error
            );

        if (Error)
            Absolute = std::filesystem::path(Path);

        return Absolute.lexically_normal().wstring();
    }

    bool HasExtension(
        const std::wstring& Path,
        const wchar_t* Extension
    )
    {
        return
            _wcsicmp(
                std::filesystem::path(Path).extension().c_str(),
                Extension
            ) == 0;
    }
}

bool RmlDocumentSession::Open(
    Rml::Context* Context,
    const std::wstring& FilePath
)
{
    if (!Context)
    {
        LastError = "Preview context is not available.";
        Status = LoadStatus::Failed;
        return false;
    }

    const std::wstring NormalizedPath = NormalizePath(FilePath);

    if (!FileExists(NormalizedPath))
    {
        LastError = "Selected RML file does not exist.";
        Status = LoadStatus::Failed;

        RmlEditorLog::Write(
            "[RmlEditor] Open failed, file does not exist: %s",
            WideToUtf8(NormalizedPath).c_str()
        );

        return false;
    }

    if (!HasExtension(NormalizedPath, L".rml"))
    {
        LastError = "Selected file is not an RML document.";
        Status = LoadStatus::Failed;
        return false;
    }

    Close(Context);
    ResetSourceState();

    DocumentPath = NormalizedPath;
    DocumentDirectory =
        std::filesystem::path(DocumentPath).parent_path().wstring();

    FileName =
        std::filesystem::path(DocumentPath).filename().wstring();

    if (!LoadSourceFiles())
    {
        Status = LoadStatus::Failed;
        return false;
    }

    const std::string RmlPathUtf8 = WideToUtf8(DocumentPath);

    Document = Context->LoadDocument(RmlPathUtf8);

    if (!Document)
    {
        LastError = "RmlUi failed to parse or load the document.";
        Status = LoadStatus::Failed;

        RmlEditorLog::Write(
            "[RmlEditor] LoadDocument failed: %s",
            RmlPathUtf8.c_str()
        );

        return false;
    }

    Document->Show();
    Status = LoadStatus::Loaded;
    Dirty = false;

    RmlEditorLog::Write(
        "[RmlEditor] Preview document opened: %s",
        RmlPathUtf8.c_str()
    );

    return true;
}

bool RmlDocumentSession::Reload(Rml::Context* Context)
{
    if (DocumentPath.empty())
    {
        LastError = "No RML document is open.";
        Status = LoadStatus::Empty;
        return false;
    }

    const std::wstring PathToReload = DocumentPath;
    return Open(Context, PathToReload);
}

void RmlDocumentSession::Close(Rml::Context* Context)
{
    if (Context && Document)
    {
        Context->UnloadDocument(Document);
        Context->Update();
    }

    Document = nullptr;

    if (DocumentPath.empty())
        Status = LoadStatus::Empty;
}

bool RmlDocumentSession::HasDocument() const
{
    return Document != nullptr && Status == LoadStatus::Loaded;
}

const std::wstring& RmlDocumentSession::GetDocumentPath() const
{
    return DocumentPath;
}

const std::wstring& RmlDocumentSession::GetDocumentDirectory() const
{
    return DocumentDirectory;
}

const std::wstring& RmlDocumentSession::GetFileName() const
{
    return FileName;
}

const std::wstring& RmlDocumentSession::GetMainStyleSheetPath() const
{
    return MainStyleSheetPath;
}

const std::vector<std::wstring>&
RmlDocumentSession::GetStyleSheetPaths() const
{
    return StyleSheetPaths;
}

const std::vector<std::wstring>&
RmlDocumentSession::GetImagePaths() const
{
    return ImagePaths;
}

const std::vector<std::wstring>&
RmlDocumentSession::GetFontPaths() const
{
    return FontPaths;
}

const std::vector<std::wstring>&
RmlDocumentSession::GetIncludePaths() const
{
    return IncludePaths;
}

const std::string& RmlDocumentSession::GetRmlSource() const
{
    return RmlSource;
}

const std::string& RmlDocumentSession::GetRcssSource() const
{
    return RcssSource;
}

const std::string& RmlDocumentSession::GetLastError() const
{
    return LastError;
}

Rml::ElementDocument* RmlDocumentSession::GetDocument() const
{
    return Document;
}

RmlDocumentSession::LoadStatus
RmlDocumentSession::GetStatus() const
{
    return Status;
}

int RmlDocumentSession::GetLogicalWidth() const
{
    return LogicalWidth;
}

int RmlDocumentSession::GetLogicalHeight() const
{
    return LogicalHeight;
}

bool RmlDocumentSession::IsDirty() const
{
    return Dirty;
}

std::string RmlDocumentSession::WideToUtf8(
    const std::wstring& Text
)
{
    if (Text.empty())
        return std::string();

    const int RequiredLength = WideCharToMultiByte(
        CP_UTF8,
        0,
        Text.c_str(),
        static_cast<int>(Text.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (RequiredLength <= 0)
        return std::string();

    std::string Result;
    Result.resize(static_cast<size_t>(RequiredLength));

    WideCharToMultiByte(
        CP_UTF8,
        0,
        Text.c_str(),
        static_cast<int>(Text.size()),
        Result.data(),
        RequiredLength,
        nullptr,
        nullptr
    );

    return Result;
}

std::wstring RmlDocumentSession::Utf8ToWide(
    const std::string& Text
)
{
    if (Text.empty())
        return std::wstring();

    const int RequiredLength = MultiByteToWideChar(
        CP_UTF8,
        0,
        Text.c_str(),
        static_cast<int>(Text.size()),
        nullptr,
        0
    );

    if (RequiredLength <= 0)
        return std::wstring();

    std::wstring Result;
    Result.resize(static_cast<size_t>(RequiredLength));

    MultiByteToWideChar(
        CP_UTF8,
        0,
        Text.c_str(),
        static_cast<int>(Text.size()),
        Result.data(),
        RequiredLength
    );

    return Result;
}

bool RmlDocumentSession::ReadFileUtf8(
    const std::wstring& Path,
    std::string& Output
)
{
    Output.clear();

    std::ifstream File(
        std::filesystem::path(Path),
        std::ios::binary
    );

    if (!File)
        return false;

    File.seekg(0, std::ios::end);
    const std::streamoff Size = File.tellg();
    File.seekg(0, std::ios::beg);

    if (Size > 0)
    {
        Output.resize(static_cast<size_t>(Size));
        File.read(Output.data(), Size);
    }

    return true;
}

bool RmlDocumentSession::LoadSourceFiles()
{
    if (!ReadFileUtf8(DocumentPath, RmlSource))
    {
        LastError = "Failed to read selected RML source.";

        RmlEditorLog::Write(
            "[RmlEditor] Failed to read RML source: %s",
            WideToUtf8(DocumentPath).c_str()
        );

        return false;
    }

    AnalyzeLinkedResources();

    if (!MainStyleSheetPath.empty())
    {
        if (!ReadFileUtf8(MainStyleSheetPath, RcssSource))
        {
            RmlEditorLog::Write(
                "[RmlEditor] Failed to read RCSS source: %s",
                WideToUtf8(MainStyleSheetPath).c_str()
            );
        }
    }

    return true;
}

void RmlDocumentSession::AnalyzeLinkedResources()
{
    static const std::regex LinkPattern(
        "<link[^>]*href\\s*=\\s*['\"]([^'\"]+\\.rcss)['\"][^>]*>",
        std::regex_constants::icase
    );

    static const std::regex ImportPattern(
        "@import\\s+(?:url\\()?\\s*['\"]?([^'\"\\)]+\\.rcss)['\"]?\\s*\\)?",
        std::regex_constants::icase
    );

    static const std::regex ImagePattern(
        "['\"(]\\s*([^'\"\\)]+\\.(?:png|jpg|jpeg|tga|dds|bmp))",
        std::regex_constants::icase
    );

    static const std::regex FontPattern(
        "['\"(]\\s*([^'\"\\)]+\\.(?:ttf|otf))",
        std::regex_constants::icase
    );

    std::smatch Match;
    std::string::const_iterator SearchStart = RmlSource.begin();

    while (std::regex_search(
        SearchStart,
        RmlSource.cend(),
        Match,
        LinkPattern
    ))
    {
        const std::wstring Path =
            ResolveDocumentRelative(Match[1].str());

        AddUniquePath(StyleSheetPaths, Path);

        if (MainStyleSheetPath.empty())
            MainStyleSheetPath = Path;

        SearchStart = Match.suffix().first;
    }

    std::string ResourceText = RmlSource;

    for (const std::wstring& StylePath : StyleSheetPaths)
    {
        std::string StyleText;

        if (ReadFileUtf8(StylePath, StyleText))
            ResourceText += "\n" + StyleText;
    }

    SearchStart = ResourceText.begin();

    while (std::regex_search(
        SearchStart,
        ResourceText.cend(),
        Match,
        ImportPattern
    ))
    {
        AddUniquePath(
            IncludePaths,
            ResolveDocumentRelative(Match[1].str())
        );

        SearchStart = Match.suffix().first;
    }

    SearchStart = ResourceText.begin();

    while (std::regex_search(
        SearchStart,
        ResourceText.cend(),
        Match,
        ImagePattern
    ))
    {
        AddUniquePath(
            ImagePaths,
            ResolveDocumentRelative(Match[1].str())
        );

        SearchStart = Match.suffix().first;
    }

    SearchStart = ResourceText.begin();

    while (std::regex_search(
        SearchStart,
        ResourceText.cend(),
        Match,
        FontPattern
    ))
    {
        AddUniquePath(
            FontPaths,
            ResolveDocumentRelative(Match[1].str())
        );

        SearchStart = Match.suffix().first;
    }
}

void RmlDocumentSession::ResetSourceState()
{
    StyleSheetPaths.clear();
    ImagePaths.clear();
    FontPaths.clear();
    IncludePaths.clear();

    MainStyleSheetPath.clear();
    RmlSource.clear();
    RcssSource.clear();
    LastError.clear();
}

std::wstring RmlDocumentSession::ResolveDocumentRelative(
    const std::string& Path
) const
{
    std::wstring WidePath = Utf8ToWide(Path);

    for (wchar_t& Character : WidePath)
    {
        if (Character == L'/')
            Character = L'\\';
    }

    std::filesystem::path ResourcePath(WidePath);

    if (!ResourcePath.is_absolute())
        ResourcePath = std::filesystem::path(DocumentDirectory) / ResourcePath;

    return ResourcePath.lexically_normal().wstring();
}

void RmlDocumentSession::AddUniquePath(
    std::vector<std::wstring>& Paths,
    const std::wstring& Path
)
{
    if (Path.empty())
        return;

    const auto Found =
        std::find(Paths.begin(), Paths.end(), Path);

    if (Found == Paths.end())
        Paths.push_back(Path);
}
