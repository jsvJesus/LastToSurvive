#include "r3dPCH.h"
#include "r3d.h"

#include "RmlFileInterface.h"

#include <windows.h>
#include <cstdio>

RmlFileInterface::RmlFileInterface(const wchar_t* InDataRoot)
	: DataRoot(InDataRoot ? InDataRoot : L"")
{
	while (!DataRoot.empty() && (DataRoot.back() == L'\\' || DataRoot.back() == L'/'))
		DataRoot.pop_back();
}

std::wstring RmlFileInterface::Utf8ToWide(const Rml::String& text)
{
	if (text.empty())
		return std::wstring();

	const int Required = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
	if (Required <= 0)
		return std::wstring();

	std::wstring Result;
	Result.resize(Required);

	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &Result[0], Required);
	return Result;
}

bool RmlFileInterface::IsAbsolutePathW(const std::wstring& path)
{
	if (path.size() >= 2 && path[1] == L':')
		return true;

	if (!path.empty() && (path[0] == L'\\' || path[0] == L'/'))
		return true;

	return false;
}

std::wstring RmlFileInterface::ResolvePathW(const Rml::String& path) const
{
	std::wstring WidePath = Utf8ToWide(path);

	for (wchar_t& Ch : WidePath)
	{
		if (Ch == L'/')
			Ch = L'\\';
	}

	if (IsAbsolutePathW(WidePath))
		return WidePath;

	if (DataRoot.empty())
		return WidePath;

	return DataRoot + L"\\" + WidePath;
}

Rml::FileHandle RmlFileInterface::Open(const Rml::String& path)
{
	const std::wstring FullPath = ResolvePathW(path);

	FILE* File = nullptr;
	_wfopen_s(&File, FullPath.c_str(), L"rb");

	if (!File)
	{
		std::string Text = "[RmlUI][File] Failed to open: ";
		Text += path;
		Text += "\n";
		OutputDebugStringA(Text.c_str());
		return nullptr;
	}

	return reinterpret_cast<Rml::FileHandle>(File);
}

void RmlFileInterface::Close(Rml::FileHandle file)
{
	if (!file)
		return;

	FILE* File = reinterpret_cast<FILE*>(file);
	fclose(File);
}

size_t RmlFileInterface::Read(void* buffer, size_t size, Rml::FileHandle file)
{
	if (!file || !buffer || size == 0)
		return 0;

	FILE* File = reinterpret_cast<FILE*>(file);
	return fread(buffer, 1, size, File);
}

bool RmlFileInterface::Seek(Rml::FileHandle file, long offset, int origin)
{
	if (!file)
		return false;

	FILE* File = reinterpret_cast<FILE*>(file);
	return fseek(File, offset, origin) == 0;
}

size_t RmlFileInterface::Tell(Rml::FileHandle file)
{
	if (!file)
		return 0;

	FILE* File = reinterpret_cast<FILE*>(file);
	const long Pos = ftell(File);

	return Pos < 0 ? 0 : static_cast<size_t>(Pos);
}

size_t RmlFileInterface::Length(Rml::FileHandle file)
{
	if (!file)
		return 0;

	FILE* File = reinterpret_cast<FILE*>(file);

	const long Current = ftell(File);
	fseek(File, 0, SEEK_END);
	const long End = ftell(File);
	fseek(File, Current, SEEK_SET);

	return End < 0 ? 0 : static_cast<size_t>(End);
}