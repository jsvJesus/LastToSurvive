#include "RmlEditorFileInterface.h"

#include "../App/RmlEditorLog.h"

#include <windows.h>

#include <cstdio>
#include <filesystem>

RmlEditorFileInterface::RmlEditorFileInterface(const wchar_t* DataRoot)
	: RootDirectory(DataRoot ? DataRoot : L"")
{
	while (!RootDirectory.empty())
	{
		const wchar_t LastCharacter = RootDirectory.back();

		if (LastCharacter != L'\\' && LastCharacter != L'/')
			break;

		RootDirectory.pop_back();
	}

	RmlEditorLog::Write("[RmlEditor][File] File interface initialized");
}

std::wstring RmlEditorFileInterface::Utf8ToWide(const Rml::String& Text)
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

bool RmlEditorFileInterface::IsAbsolutePath(const std::wstring& Path)
{
	if (Path.size() >= 2 && Path[1] == L':')
		return true;

	if (!Path.empty() && (Path[0] == L'\\' || Path[0] == L'/'))
		return true;

	return false;
}

bool RmlEditorFileInterface::FileExists(const std::wstring& Path)
{
	return std::filesystem::exists(std::filesystem::path(Path));
}

void RmlEditorFileInterface::SetDocumentDirectory(
	const wchar_t* Directory
)
{
	DocumentDirectory = Directory ? Directory : L"";

	while (!DocumentDirectory.empty())
	{
		const wchar_t LastCharacter = DocumentDirectory.back();

		if (LastCharacter != L'\\' && LastCharacter != L'/')
			break;

		DocumentDirectory.pop_back();
	}
}

std::wstring RmlEditorFileInterface::ResolvePath(
	const Rml::String& Path
) const
{
	std::wstring WidePath = Utf8ToWide(Path);

	for (wchar_t& Character : WidePath)
	{
		if (Character == L'/')
			Character = L'\\';
	}

	if (IsAbsolutePath(WidePath))
		return WidePath;

	if (!DocumentDirectory.empty())
	{
		const std::wstring DocumentPath =
			DocumentDirectory + L"\\" + WidePath;

		if (FileExists(DocumentPath))
			return DocumentPath;
	}

	if (!RootDirectory.empty())
	{
		const std::wstring DataPath =
			RootDirectory + L"\\" + WidePath;

		if (FileExists(DataPath))
			return DataPath;
	}

	if (RootDirectory.empty())
	{
		if (!DocumentDirectory.empty())
			return DocumentDirectory + L"\\" + WidePath;

		return WidePath;
	}

	if (!DocumentDirectory.empty())
		return DocumentDirectory + L"\\" + WidePath;

	return RootDirectory + L"\\" + WidePath;
}

Rml::FileHandle RmlEditorFileInterface::Open(const Rml::String& Path)
{
	const std::wstring FullPath = ResolvePath(Path);

	FILE* File = nullptr;

	if (_wfopen_s(&File, FullPath.c_str(), L"rb") != 0 || !File)
	{
		RmlEditorLog::Write(
			"[RmlEditor][File] Failed to open: %s",
			Path.c_str()
		);

		return Rml::FileHandle{};
	}

	return reinterpret_cast<Rml::FileHandle>(File);
}

void RmlEditorFileInterface::Close(Rml::FileHandle File)
{
	if (!File)
		return;

	fclose(reinterpret_cast<FILE*>(File));
}

size_t RmlEditorFileInterface::Read(
	void* Buffer,
	size_t Size,
	Rml::FileHandle File
)
{
	if (!File || !Buffer || Size == 0)
		return 0;

	return fread(
		Buffer,
		1,
		Size,
		reinterpret_cast<FILE*>(File)
	);
}

bool RmlEditorFileInterface::Seek(
	Rml::FileHandle File,
	long Offset,
	int Origin
)
{
	if (!File)
		return false;

	return fseek(
		reinterpret_cast<FILE*>(File),
		Offset,
		Origin
	) == 0;
}

size_t RmlEditorFileInterface::Tell(Rml::FileHandle File)
{
	if (!File)
		return 0;

	const long Position = ftell(
		reinterpret_cast<FILE*>(File)
	);

	return Position < 0
		? 0
		: static_cast<size_t>(Position);
}

size_t RmlEditorFileInterface::Length(Rml::FileHandle File)
{
	if (!File)
		return 0;

	FILE* FilePointer = reinterpret_cast<FILE*>(File);

	const long CurrentPosition = ftell(FilePointer);

	if (CurrentPosition < 0)
		return 0;

	if (fseek(FilePointer, 0, SEEK_END) != 0)
		return 0;

	const long EndPosition = ftell(FilePointer);

	fseek(FilePointer, CurrentPosition, SEEK_SET);

	return EndPosition < 0
		? 0
		: static_cast<size_t>(EndPosition);
}
