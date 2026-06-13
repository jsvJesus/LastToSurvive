#pragma once

#include <RmlUi/Core/ElementDocument.h>

#include <string>
#include <vector>

class RmlDocumentSession final
{
public:
    enum class LoadStatus
    {
        Empty,
        Loaded,
        Failed
    };

    bool Open(Rml::Context* Context, const std::wstring& FilePath);
    bool Reload(Rml::Context* Context);
    void Close(Rml::Context* Context);

    bool HasDocument() const;

    const std::wstring& GetDocumentPath() const;
    const std::wstring& GetDocumentDirectory() const;
    const std::wstring& GetFileName() const;
    const std::wstring& GetMainStyleSheetPath() const;

    const std::vector<std::wstring>& GetStyleSheetPaths() const;
    const std::vector<std::wstring>& GetImagePaths() const;
    const std::vector<std::wstring>& GetFontPaths() const;
    const std::vector<std::wstring>& GetIncludePaths() const;

    const std::string& GetRmlSource() const;
    const std::string& GetRcssSource() const;
    const std::string& GetLastError() const;

    Rml::ElementDocument* GetDocument() const;
    LoadStatus GetStatus() const;

    int GetLogicalWidth() const;
    int GetLogicalHeight() const;
    bool IsDirty() const;

    static std::string WideToUtf8(const std::wstring& Text);
    static std::wstring Utf8ToWide(const std::string& Text);

private:
    std::wstring DocumentPath;
    std::wstring DocumentDirectory;
    std::wstring FileName;
    std::wstring MainStyleSheetPath;

    std::vector<std::wstring> StyleSheetPaths;
    std::vector<std::wstring> ImagePaths;
    std::vector<std::wstring> FontPaths;
    std::vector<std::wstring> IncludePaths;

    std::string RmlSource;
    std::string RcssSource;
    std::string LastError;

    Rml::ElementDocument* Document = nullptr;

    LoadStatus Status = LoadStatus::Empty;
    int LogicalWidth = 1920;
    int LogicalHeight = 1080;
    bool Dirty = false;

    bool LoadSourceFiles();
    void AnalyzeLinkedResources();
    void ResetSourceState();

    std::wstring ResolveDocumentRelative(
        const std::string& Path
    ) const;

    static bool ReadFileUtf8(
        const std::wstring& Path,
        std::string& Output
    );

    static void AddUniquePath(
        std::vector<std::wstring>& Paths,
        const std::wstring& Path
    );
};
