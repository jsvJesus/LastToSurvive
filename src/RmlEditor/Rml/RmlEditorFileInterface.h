#pragma once

#include <RmlUi/Core/FileInterface.h>

#include <string>

class RmlEditorFileInterface final : public Rml::FileInterface
{
public:
    explicit RmlEditorFileInterface(const wchar_t* DataRoot);

    Rml::FileHandle Open(const Rml::String& Path) override;
    void Close(Rml::FileHandle File) override;

    size_t Read(
        void* Buffer,
        size_t Size,
        Rml::FileHandle File
    ) override;

    bool Seek(
        Rml::FileHandle File,
        long Offset,
        int Origin
    ) override;

    size_t Tell(Rml::FileHandle File) override;
    size_t Length(Rml::FileHandle File) override;

    std::wstring ResolvePath(const Rml::String& Path) const;

private:
    std::wstring RootDirectory;

    static std::wstring Utf8ToWide(const Rml::String& Text);
    static bool IsAbsolutePath(const std::wstring& Path);
};