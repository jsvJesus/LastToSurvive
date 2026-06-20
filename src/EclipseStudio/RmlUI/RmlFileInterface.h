#pragma once

#include <RmlUi/Core/FileInterface.h>
#include <string>

class RmlFileInterface final :
    public Rml::FileInterface
{
public:
    explicit RmlFileInterface(
        const wchar_t* InDataRoot
    );

    Rml::FileHandle Open(
        const Rml::String& path
    ) override;

    void Close(
        Rml::FileHandle file
    ) override;

    size_t Read(
        void* buffer,
        size_t size,
        Rml::FileHandle file
    ) override;

    bool Seek(
        Rml::FileHandle file,
        long offset,
        int origin
    ) override;

    size_t Tell(
        Rml::FileHandle file
    ) override;

    size_t Length(
        Rml::FileHandle file
    ) override;

    std::wstring ResolvePathW(
        const Rml::String& path
    ) const;

private:
    std::wstring DataRoot;

    static std::wstring Utf8ToWide(
        const Rml::String& text
    );

    static std::string WideToUtf8(
        const std::wstring& text
    );

    static bool IsAbsolutePathW(
        const std::wstring& path
    );

    static std::wstring NormalizePathW(
        const std::wstring& path
    );
};