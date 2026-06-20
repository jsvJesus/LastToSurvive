#include "r3dPCH.h"
#include "r3d.h"

#include "RmlFileInterface.h"

#include <windows.h>
#include <cstdio>
#include <vector>

RmlFileInterface::RmlFileInterface(
	const wchar_t* InDataRoot
)
	: DataRoot(InDataRoot ? InDataRoot : L"")
{
	while (
		!DataRoot.empty() &&
		(
			DataRoot.back() == L'\\' ||
			DataRoot.back() == L'/'
		)
	)
	{
		DataRoot.pop_back();
	}

	DataRoot =
		NormalizePathW(
			DataRoot
		);
}

std::wstring RmlFileInterface::Utf8ToWide(
	const Rml::String& text
)
{
	if (text.empty())
		return std::wstring();

	const int Required =
		MultiByteToWideChar(
			CP_UTF8,
			0,
			text.c_str(),
			static_cast<int>(text.size()),
			nullptr,
			0
		);

	if (Required <= 0)
		return std::wstring();

	std::wstring Result;
	Result.resize(
		Required
	);

	MultiByteToWideChar(
		CP_UTF8,
		0,
		text.c_str(),
		static_cast<int>(text.size()),
		&Result[0],
		Required
	);

	return Result;
}

std::string RmlFileInterface::WideToUtf8(
	const std::wstring& text
)
{
	if (text.empty())
		return std::string();

	const int Required =
		WideCharToMultiByte(
			CP_UTF8,
			0,
			text.c_str(),
			static_cast<int>(text.size()),
			nullptr,
			0,
			nullptr,
			nullptr
		);

	if (Required <= 0)
		return std::string();

	std::string Result;
	Result.resize(
		Required
	);

	WideCharToMultiByte(
		CP_UTF8,
		0,
		text.c_str(),
		static_cast<int>(text.size()),
		&Result[0],
		Required,
		nullptr,
		nullptr
	);

	return Result;
}

bool RmlFileInterface::IsAbsolutePathW(
	const std::wstring& path
)
{
	if (
		path.size() >= 2 &&
		path[1] == L':'
	)
	{
		return true;
	}

	if (
		!path.empty() &&
		(
			path[0] == L'\\' ||
			path[0] == L'/'
		)
	)
	{
		return true;
	}

	return false;
}

std::wstring RmlFileInterface::NormalizePathW(
	const std::wstring& path
)
{
	if (path.empty())
		return std::wstring();

	std::wstring FixedPath =
		path;

	for (wchar_t& Ch : FixedPath)
	{
		if (Ch == L'/')
			Ch = L'\\';
	}

	std::wstring Prefix;
	size_t ReadIndex = 0;
	bool bAbsoluteRoot = false;

	if (
		FixedPath.size() >= 2 &&
		FixedPath[1] == L':'
	)
	{
		Prefix =
			FixedPath.substr(
				0,
				2
			);

		ReadIndex = 2;

		if (
			ReadIndex < FixedPath.size() &&
			FixedPath[ReadIndex] == L'\\'
		)
		{
			Prefix += L"\\";
			++ReadIndex;
			bAbsoluteRoot = true;
		}
	}
	else if (
		FixedPath.size() >= 2 &&
		FixedPath[0] == L'\\' &&
		FixedPath[1] == L'\\'
	)
	{
		Prefix = L"\\\\";
		ReadIndex = 2;
		bAbsoluteRoot = true;
	}
	else if (
		!FixedPath.empty() &&
		FixedPath[0] == L'\\'
	)
	{
		Prefix = L"\\";
		ReadIndex = 1;
		bAbsoluteRoot = true;
	}

	std::vector<std::wstring> Parts;
	std::wstring CurrentPart;

	for (
		size_t Index = ReadIndex;
		Index <= FixedPath.size();
		++Index
	)
	{
		const wchar_t Ch =
			Index < FixedPath.size()
				? FixedPath[Index]
				: L'\\';

		if (Ch == L'\\')
		{
			if (
				CurrentPart.empty() ||
				CurrentPart == L"."
			)
			{
				CurrentPart.clear();
				continue;
			}

			if (CurrentPart == L"..")
			{
				if (!Parts.empty())
				{
					Parts.pop_back();
				}
				else if (!bAbsoluteRoot)
				{
					Parts.push_back(
						CurrentPart
					);
				}
			}
			else
			{
				Parts.push_back(
					CurrentPart
				);
			}

			CurrentPart.clear();
		}
		else
		{
			CurrentPart.push_back(
				Ch
			);
		}
	}

	std::wstring Result =
		Prefix;

	for (size_t Index = 0; Index < Parts.size(); ++Index)
	{
		if (
			!Result.empty() &&
			Result.back() != L'\\'
		)
		{
			Result += L"\\";
		}

		Result +=
			Parts[Index];
	}

	if (Result.empty())
		return L".";

	return Result;
}

std::wstring RmlFileInterface::ResolvePathW(
	const Rml::String& path
) const
{
	std::wstring WidePath =
		Utf8ToWide(
			path
		);

	WidePath =
		NormalizePathW(
			WidePath
		);

	if (
		IsAbsolutePathW(
			WidePath
		)
	)
	{
		return WidePath;
	}

	if (DataRoot.empty())
	{
		return WidePath;
	}

	return NormalizePathW(
		DataRoot + L"\\" + WidePath
	);
}

Rml::FileHandle RmlFileInterface::Open(
	const Rml::String& path
)
{
	const std::wstring FullPath =
		ResolvePathW(
			path
		);

	FILE* File = nullptr;

	_wfopen_s(
		&File,
		FullPath.c_str(),
		L"rb"
	);

	if (!File)
	{
		std::string Text =
			"[RmlUI][File] Failed to open: ";

		Text +=
			WideToUtf8(
				FullPath
			);

		Text += "\n";

		OutputDebugStringA(
			Text.c_str()
		);

		return 0;
	}

	return reinterpret_cast<Rml::FileHandle>(
		File
	);
}

void RmlFileInterface::Close(
	Rml::FileHandle file
)
{
	if (!file)
		return;

	FILE* File =
		reinterpret_cast<FILE*>(
			file
		);

	fclose(
		File
	);
}

size_t RmlFileInterface::Read(
	void* buffer,
	size_t size,
	Rml::FileHandle file
)
{
	if (
		!file ||
		!buffer ||
		size == 0
	)
	{
		return 0;
	}

	FILE* File =
		reinterpret_cast<FILE*>(
			file
		);

	return fread(
		buffer,
		1,
		size,
		File
	);
}

bool RmlFileInterface::Seek(
	Rml::FileHandle file,
	long offset,
	int origin
)
{
	if (!file)
		return false;

	FILE* File =
		reinterpret_cast<FILE*>(
			file
		);

	return fseek(
		File,
		offset,
		origin
	) == 0;
}

size_t RmlFileInterface::Tell(
	Rml::FileHandle file
)
{
	if (!file)
		return 0;

	FILE* File =
		reinterpret_cast<FILE*>(
			file
		);

	const long Pos =
		ftell(
			File
		);

	return Pos < 0
		? 0
		: static_cast<size_t>(
			Pos
		);
}

size_t RmlFileInterface::Length(
	Rml::FileHandle file
)
{
	if (!file)
		return 0;

	FILE* File =
		reinterpret_cast<FILE*>(
			file
		);

	const long Current =
		ftell(
			File
		);

	fseek(
		File,
		0,
		SEEK_END
	);

	const long End =
		ftell(
			File
		);

	fseek(
		File,
		Current,
		SEEK_SET
	);

	return End < 0
		? 0
		: static_cast<size_t>(
			End
		);
}