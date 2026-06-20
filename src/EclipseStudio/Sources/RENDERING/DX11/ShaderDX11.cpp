#include "r3dPCH.h"

#include "RENDERING/DX11/ShaderDX11.h"

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <windows.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <memory>

#pragma comment(lib, "d3dcompiler.lib")

namespace
{
	const char ShaderCacheSignature[] = "DX11CSO1";

	template <typename T>
	void SafeReleaseDX11(T*& value)
	{
		if (value)
		{
			value->Release();
			value = nullptr;
		}
	}

	std::string NormalizeSlashes(std::string value)
	{
		for (size_t i = 0; i < value.size(); ++i)
		{
			if (value[i] == '/')
				value[i] = '\\';
		}
		return value;
	}

	std::string TrimTrailingSlash(std::string value)
	{
		value = NormalizeSlashes(value);
		while (!value.empty() && (value[value.size() - 1] == '\\' || value[value.size() - 1] == '/'))
			value.resize(value.size() - 1);
		return value;
	}

	bool IsAbsolutePath(const char* path)
	{
		return path && path[0] && path[1] == ':';
	}

	std::string JoinPath(const std::string& root, const char* child)
	{
		if (!child || !child[0])
			return root;

		if (IsAbsolutePath(child))
			return NormalizeSlashes(child);

		if (root.empty())
			return NormalizeSlashes(child);

		return root + "\\" + NormalizeSlashes(child);
	}

	std::string GetDirectory(const std::string& path)
	{
		const size_t slash = path.find_last_of("\\/");
		if (slash == std::string::npos)
			return std::string();
		return path.substr(0, slash);
	}

	std::string SanitizeForFileName(const char* text)
	{
		std::string value = text ? text : "";
		for (size_t i = 0; i < value.size(); ++i)
		{
			const char c = value[i];
			if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|' || c == ',' || c == '=')
				value[i] = '_';
		}
		return value;
	}

	std::string BuildMacroKey(const r3dDX11ShaderMacro* macros)
	{
		std::string key;
		if (!macros)
			return key;

		for (const r3dDX11ShaderMacro* macro = macros; macro->Name; ++macro)
		{
			key += macro->Name;
			key += "=";
			key += macro->Definition ? macro->Definition : "";
			key += ",";
		}

		return key;
	}

	bool ReadWholeFile(const char* path, std::vector<char>& data)
	{
		FILE* file = fopen(path, "rb");
		if (!file)
			return false;

		fseek(file, 0, SEEK_END);
		const long size = ftell(file);
		fseek(file, 0, SEEK_SET);

		if (size < 0)
		{
			fclose(file);
			return false;
		}

		data.resize(static_cast<size_t>(size));
		if (size > 0 && fread(&data[0], 1, static_cast<size_t>(size), file) != static_cast<size_t>(size))
		{
			fclose(file);
			return false;
		}

		fclose(file);
		return true;
	}

	bool GetFileWriteTime(const char* path, FILETIME& writeTime)
	{
		WIN32_FILE_ATTRIBUTE_DATA data{};
		if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data))
			return false;

		writeTime = data.ftLastWriteTime;
		return true;
	}

	bool IsNewerThan(const FILETIME& lhs, const FILETIME& rhs)
	{
		return CompareFileTime(&lhs, &rhs) > 0;
	}

	void EnsureDirectory(const std::string& path)
	{
		if (path.empty())
			return;

		std::string partial;
		for (size_t i = 0; i < path.size(); ++i)
		{
			partial += path[i];
			if (path[i] == '\\' || path[i] == '/')
			{
				if (partial.size() > 2)
					CreateDirectoryA(partial.c_str(), nullptr);
			}
		}

		CreateDirectoryA(path.c_str(), nullptr);
	}

	bool WriteString(FILE* file, const std::string& value)
	{
		const unsigned int length = static_cast<unsigned int>(value.size());
		if (fwrite(&length, sizeof(length), 1, file) != 1)
			return false;
		if (length && fwrite(value.c_str(), 1, length, file) != length)
			return false;
		return true;
	}

	bool ReadString(FILE* file, std::string& value)
	{
		unsigned int length = 0;
		if (fread(&length, sizeof(length), 1, file) != 1)
			return false;
		if (length > 64 * 1024)
			return false;

		value.resize(length);
		if (length && fread(&value[0], 1, length, file) != length)
			return false;
		return true;
	}

	class DX11IncludeHandler final : public ID3DInclude
	{
	public:
		DX11IncludeHandler(const std::string& sourceFile, const std::string& sourceRoot)
			: BaseDirectory(GetDirectory(sourceFile))
			, SourceRoot(sourceRoot)
		{
		}

		STDMETHOD(Open)(D3D_INCLUDE_TYPE, LPCSTR fileName, LPCVOID, LPCVOID* data, UINT* bytes) override
		{
			if (!data || !bytes)
				return E_INVALIDARG;

			*data = nullptr;
			*bytes = 0;

			std::string path = JoinPath(BaseDirectory, fileName);
			std::vector<char> fileData;
			if (!ReadWholeFile(path.c_str(), fileData))
			{
				path = JoinPath(SourceRoot, fileName);
				if (!ReadWholeFile(path.c_str(), fileData))
					return E_FAIL;
			}

			char* owned = new char[fileData.size() + 1];
			if (!fileData.empty())
				memcpy(owned, &fileData[0], fileData.size());
			owned[fileData.size()] = 0;

			Includes.push_back(path);
			*data = owned;
			*bytes = static_cast<UINT>(fileData.size());
			return S_OK;
		}

		STDMETHOD(Close)(LPCVOID data) override
		{
			delete[] reinterpret_cast<const char*>(data);
			return S_OK;
		}

		const std::vector<std::string>& GetIncludes() const
		{
			return Includes;
		}

	private:
		std::string BaseDirectory;
		std::string SourceRoot;
		std::vector<std::string> Includes;
	};

	void BuildD3DMacros(const r3dDX11ShaderMacro* macros, std::vector<D3D_SHADER_MACRO>& out)
	{
		if (macros)
		{
			for (const r3dDX11ShaderMacro* macro = macros; macro->Name; ++macro)
			{
				D3D_SHADER_MACRO d3dMacro{};
				d3dMacro.Name = macro->Name;
				d3dMacro.Definition = macro->Definition ? macro->Definition : "";
				out.push_back(d3dMacro);
			}
		}

		D3D_SHADER_MACRO shaderModelMacro{};
		shaderModelMacro.Name = "R3D_DX11";
		shaderModelMacro.Definition = "1";
		out.push_back(shaderModelMacro);

		D3D_SHADER_MACRO terminator{};
		out.push_back(terminator);
	}
}

const char* r3dDX11ShaderCompiler::VertexProfile()
{
	return "vs_5_0";
}

const char* r3dDX11ShaderCompiler::PixelProfile()
{
	return "ps_5_0";
}

r3dDX11ShaderCompiler::r3dDX11ShaderCompiler()
	: SourceRoot("Data\\Shaders\\DX11_P1")
	, CacheRoot("Data\\Shaders\\Cache\\DX11_P1")
{
}

void r3dDX11ShaderCompiler::SetSourceRoot(const char* path)
{
	SourceRoot = TrimTrailingSlash(path ? path : "");
}

void r3dDX11ShaderCompiler::SetCacheRoot(const char* path)
{
	CacheRoot = TrimTrailingSlash(path ? path : "");
}

const std::string& r3dDX11ShaderCompiler::GetSourceRoot() const
{
	return SourceRoot;
}

const std::string& r3dDX11ShaderCompiler::GetCacheRoot() const
{
	return CacheRoot;
}

const std::string& r3dDX11ShaderCompiler::GetLastError() const
{
	return LastError;
}

bool r3dDX11ShaderCompiler::CompileVertexShader(
	const char* relativePath,
	const char* entryPoint,
	const r3dDX11ShaderMacro* macros,
	ID3DBlob** bytecode,
	std::vector<std::string>* includes
)
{
	return CompileFromFile(relativePath, entryPoint ? entryPoint : "main", VertexProfile(), macros, bytecode, includes);
}

bool r3dDX11ShaderCompiler::CompilePixelShader(
	const char* relativePath,
	const char* entryPoint,
	const r3dDX11ShaderMacro* macros,
	ID3DBlob** bytecode,
	std::vector<std::string>* includes
)
{
	return CompileFromFile(relativePath, entryPoint ? entryPoint : "main", PixelProfile(), macros, bytecode, includes);
}

bool r3dDX11ShaderCompiler::CompileFromFile(
	const char* relativePath,
	const char* entryPoint,
	const char* profile,
	const r3dDX11ShaderMacro* macros,
	ID3DBlob** bytecode,
	std::vector<std::string>* includes
)
{
	if (!bytecode)
		return false;

	*bytecode = nullptr;
	LastError.clear();

	if (!relativePath || !relativePath[0] || !entryPoint || !entryPoint[0] || !profile || !profile[0])
	{
		SetLastError("Invalid DX11 shader compile request");
		return false;
	}

	const std::string sourcePath = BuildSourcePath(relativePath);
	const std::string cachePath = BuildCachePath(relativePath, entryPoint, profile, macros);

	if (LoadBinaryCache(sourcePath.c_str(), cachePath.c_str(), bytecode, includes))
		return true;

	std::vector<char> source;
	if (!ReadWholeFile(sourcePath.c_str(), source))
	{
		SetLastError("Missing DX11 shader file '%s'", sourcePath.c_str());
		return false;
	}

	DX11IncludeHandler includeHandler(sourcePath, SourceRoot);
	std::vector<D3D_SHADER_MACRO> d3dMacros;
	BuildD3DMacros(macros, d3dMacros);

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifndef FINAL_BUILD
	flags |= D3DCOMPILE_DEBUG;
#else
	flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	ID3DBlob* errors = nullptr;
	HRESULT result = D3DCompile(
		source.empty() ? "" : &source[0],
		source.size(),
		sourcePath.c_str(),
		&d3dMacros[0],
		&includeHandler,
		entryPoint,
		profile,
		flags,
		0,
		bytecode,
		&errors
	);

	if (FAILED(result) || !*bytecode)
	{
		SetLastError(
			"DX11 shader compilation failed '%s' [%s/%s]\n%s",
			sourcePath.c_str(),
			entryPoint,
			profile,
			errors ? reinterpret_cast<const char*>(errors->GetBufferPointer()) : "unknown error"
		);
		SafeReleaseDX11(errors);
		return false;
	}

	SafeReleaseDX11(errors);

	const std::vector<std::string>& usedIncludes = includeHandler.GetIncludes();
	if (includes)
		*includes = usedIncludes;

	SaveBinaryCache(cachePath.c_str(), *bytecode, usedIncludes);
	return true;
}

std::string r3dDX11ShaderCompiler::BuildSourcePath(const char* relativePath) const
{
	return JoinPath(SourceRoot, relativePath);
}

std::string r3dDX11ShaderCompiler::BuildCachePath(const char* relativePath, const char* entryPoint, const char* profile, const r3dDX11ShaderMacro* macros) const
{
	std::string name = SanitizeForFileName(relativePath);
	name += "_";
	name += SanitizeForFileName(entryPoint);
	name += "_";
	name += SanitizeForFileName(profile);

	const std::string macroKey = BuildMacroKey(macros);
	if (!macroKey.empty())
	{
		name += "_";
		name += SanitizeForFileName(macroKey.c_str());
	}

	return JoinPath(CacheRoot, (name + ".cso").c_str());
}

bool r3dDX11ShaderCompiler::LoadBinaryCache(const char* sourcePath, const char* cachePath, ID3DBlob** bytecode, std::vector<std::string>* includes)
{
	if (!sourcePath || !cachePath || !bytecode)
		return false;

	FILE* file = fopen(cachePath, "rb");
	if (!file)
		return false;

	char signature[sizeof(ShaderCacheSignature)]{};
	if (fread(signature, sizeof(signature), 1, file) != 1 || memcmp(signature, ShaderCacheSignature, sizeof(signature)) != 0)
	{
		fclose(file);
		return false;
	}

	unsigned int includeCount = 0;
	if (fread(&includeCount, sizeof(includeCount), 1, file) != 1 || includeCount > 4096)
	{
		fclose(file);
		return false;
	}

	std::vector<std::string> cachedIncludes;
	for (unsigned int i = 0; i < includeCount; ++i)
	{
		std::string includePath;
		if (!ReadString(file, includePath))
		{
			fclose(file);
			return false;
		}
		cachedIncludes.push_back(includePath);
	}

	if (!IsCacheCurrent(sourcePath, cachePath, cachedIncludes))
	{
		fclose(file);
		return false;
	}

	const long bytecodeOffset = ftell(file);
	fseek(file, 0, SEEK_END);
	const long bytecodeEnd = ftell(file);
	fseek(file, bytecodeOffset, SEEK_SET);

	if (bytecodeOffset < 0 || bytecodeEnd <= bytecodeOffset)
	{
		fclose(file);
		return false;
	}

	const size_t bytecodeSize = static_cast<size_t>(bytecodeEnd - bytecodeOffset);
	HRESULT result = D3DCreateBlob(bytecodeSize, bytecode);
	if (FAILED(result) || !*bytecode)
	{
		fclose(file);
		return false;
	}

	if (fread((*bytecode)->GetBufferPointer(), 1, bytecodeSize, file) != bytecodeSize)
	{
		fclose(file);
		SafeReleaseDX11(*bytecode);
		return false;
	}

	fclose(file);

	if (includes)
		*includes = cachedIncludes;

	return true;
}

void r3dDX11ShaderCompiler::SaveBinaryCache(const char* cachePath, ID3DBlob* bytecode, const std::vector<std::string>& includes)
{
	if (!cachePath || !bytecode)
		return;

	EnsureDirectory(CacheRoot);

	FILE* file = fopen(cachePath, "wb");
	if (!file)
		return;

	const unsigned int includeCount = static_cast<unsigned int>(includes.size());
	fwrite(ShaderCacheSignature, sizeof(ShaderCacheSignature), 1, file);
	fwrite(&includeCount, sizeof(includeCount), 1, file);

	for (size_t i = 0; i < includes.size(); ++i)
	{
		if (!WriteString(file, includes[i]))
		{
			fclose(file);
			return;
		}
	}

	fwrite(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), 1, file);
	fclose(file);
}

bool r3dDX11ShaderCompiler::IsCacheCurrent(const char* sourcePath, const char* cachePath, const std::vector<std::string>& includes) const
{
	FILETIME sourceTime{};
	FILETIME cacheTime{};

	if (!GetFileWriteTime(cachePath, cacheTime))
		return false;

	if (GetFileWriteTime(sourcePath, sourceTime) && IsNewerThan(sourceTime, cacheTime))
		return false;

	for (size_t i = 0; i < includes.size(); ++i)
	{
		FILETIME includeTime{};
		if (GetFileWriteTime(includes[i].c_str(), includeTime) && IsNewerThan(includeTime, cacheTime))
			return false;
	}

	return true;
}

void r3dDX11ShaderCompiler::SetLastError(const char* format, ...)
{
	char buffer[4096];

	va_list args;
	va_start(args, format);
	_vsnprintf(buffer, sizeof(buffer) - 1, format, args);
	va_end(args);

	buffer[sizeof(buffer) - 1] = 0;
	LastError = buffer;
}

r3dDX11VertexShader::r3dDX11VertexShader()
{
}

r3dDX11VertexShader::~r3dDX11VertexShader()
{
	Shutdown();
}

bool r3dDX11VertexShader::Create(
	ID3D11Device* device,
	r3dDX11ShaderCompiler& compiler,
	const char* name,
	const char* relativePath,
	const char* entryPoint,
	const r3dDX11ShaderMacro* macros
)
{
	Shutdown();

	if (!device || !name || !name[0] || !relativePath || !relativePath[0])
		return false;

	ID3DBlob* bytecode = nullptr;
	if (!compiler.CompileVertexShader(relativePath, entryPoint, macros, &bytecode))
		return false;

	HRESULT result = device->CreateVertexShader(
		bytecode->GetBufferPointer(),
		bytecode->GetBufferSize(),
		nullptr,
		&Shader
	);

	if (FAILED(result) || !Shader)
	{
		SafeReleaseDX11(bytecode);
		return false;
	}

	Name = name;
	Bytecode = bytecode;
	return true;
}

void r3dDX11VertexShader::Shutdown()
{
	SafeReleaseDX11(Shader);
	SafeReleaseDX11(Bytecode);
	Name.clear();
}

const std::string& r3dDX11VertexShader::GetName() const
{
	return Name;
}

ID3D11VertexShader* r3dDX11VertexShader::GetShader() const
{
	return Shader;
}

const void* r3dDX11VertexShader::GetBytecode() const
{
	return Bytecode ? Bytecode->GetBufferPointer() : nullptr;
}

size_t r3dDX11VertexShader::GetBytecodeSize() const
{
	return Bytecode ? Bytecode->GetBufferSize() : 0;
}

bool r3dDX11VertexShader::IsValid() const
{
	return Shader != nullptr && Bytecode != nullptr;
}

r3dDX11PixelShader::r3dDX11PixelShader()
{
}

r3dDX11PixelShader::~r3dDX11PixelShader()
{
	Shutdown();
}

bool r3dDX11PixelShader::Create(
	ID3D11Device* device,
	r3dDX11ShaderCompiler& compiler,
	const char* name,
	const char* relativePath,
	const char* entryPoint,
	const r3dDX11ShaderMacro* macros
)
{
	Shutdown();

	if (!device || !name || !name[0] || !relativePath || !relativePath[0])
		return false;

	ID3DBlob* bytecode = nullptr;
	if (!compiler.CompilePixelShader(relativePath, entryPoint, macros, &bytecode))
		return false;

	HRESULT result = device->CreatePixelShader(
		bytecode->GetBufferPointer(),
		bytecode->GetBufferSize(),
		nullptr,
		&Shader
	);

	SafeReleaseDX11(bytecode);

	if (FAILED(result) || !Shader)
		return false;

	Name = name;
	return true;
}

void r3dDX11PixelShader::Shutdown()
{
	SafeReleaseDX11(Shader);
	Name.clear();
}

const std::string& r3dDX11PixelShader::GetName() const
{
	return Name;
}

ID3D11PixelShader* r3dDX11PixelShader::GetShader() const
{
	return Shader;
}

bool r3dDX11PixelShader::IsValid() const
{
	return Shader != nullptr;
}

r3dDX11ShaderLibrary::r3dDX11ShaderLibrary()
{
}

r3dDX11ShaderLibrary::~r3dDX11ShaderLibrary()
{
	Shutdown();
}

bool r3dDX11ShaderLibrary::Init(ID3D11Device* device)
{
	if (!device)
		return false;

	Device = device;
	LastError.clear();
	return true;
}

void r3dDX11ShaderLibrary::Shutdown()
{
	for (size_t i = 0; i < VertexShaders.size(); ++i)
		delete VertexShaders[i];
	VertexShaders.clear();

	for (size_t i = 0; i < PixelShaders.size(); ++i)
		delete PixelShaders[i];
	PixelShaders.clear();

	Device = nullptr;
	LastError.clear();
}

r3dDX11ShaderCompiler& r3dDX11ShaderLibrary::GetCompiler()
{
	return Compiler;
}

const r3dDX11ShaderCompiler& r3dDX11ShaderLibrary::GetCompiler() const
{
	return Compiler;
}

int r3dDX11ShaderLibrary::AddVertexShader(const char* name, const char* relativePath, const char* entryPoint, const r3dDX11ShaderMacro* macros)
{
	LastError.clear();

	const int existing = FindVertexShader(name);
	if (existing >= 0)
		return existing;

	r3dDX11VertexShader* shader = new r3dDX11VertexShader();
	if (!shader->Create(Device, Compiler, name, relativePath, entryPoint, macros))
	{
		LastError = Compiler.GetLastError();
		if (LastError.empty())
			LastError = "DX11 vertex shader creation failed";
		delete shader;
		return -1;
	}

	VertexShaders.push_back(shader);
	return static_cast<int>(VertexShaders.size() - 1);
}

int r3dDX11ShaderLibrary::AddPixelShader(const char* name, const char* relativePath, const char* entryPoint, const r3dDX11ShaderMacro* macros)
{
	LastError.clear();

	const int existing = FindPixelShader(name);
	if (existing >= 0)
		return existing;

	r3dDX11PixelShader* shader = new r3dDX11PixelShader();
	if (!shader->Create(Device, Compiler, name, relativePath, entryPoint, macros))
	{
		LastError = Compiler.GetLastError();
		if (LastError.empty())
			LastError = "DX11 pixel shader creation failed";
		delete shader;
		return -1;
	}

	PixelShaders.push_back(shader);
	return static_cast<int>(PixelShaders.size() - 1);
}

int r3dDX11ShaderLibrary::FindVertexShader(const char* name) const
{
	if (!name)
		return -1;

	for (size_t i = 0; i < VertexShaders.size(); ++i)
	{
		if (VertexShaders[i] && VertexShaders[i]->GetName() == name)
			return static_cast<int>(i);
	}

	return -1;
}

int r3dDX11ShaderLibrary::FindPixelShader(const char* name) const
{
	if (!name)
		return -1;

	for (size_t i = 0; i < PixelShaders.size(); ++i)
	{
		if (PixelShaders[i] && PixelShaders[i]->GetName() == name)
			return static_cast<int>(i);
	}

	return -1;
}

r3dDX11VertexShader* r3dDX11ShaderLibrary::GetVertexShader(int index)
{
	if (index < 0 || index >= static_cast<int>(VertexShaders.size()))
		return nullptr;
	return VertexShaders[index];
}

r3dDX11PixelShader* r3dDX11ShaderLibrary::GetPixelShader(int index)
{
	if (index < 0 || index >= static_cast<int>(PixelShaders.size()))
		return nullptr;
	return PixelShaders[index];
}

const r3dDX11VertexShader* r3dDX11ShaderLibrary::GetVertexShader(int index) const
{
	if (index < 0 || index >= static_cast<int>(VertexShaders.size()))
		return nullptr;
	return VertexShaders[index];
}

const r3dDX11PixelShader* r3dDX11ShaderLibrary::GetPixelShader(int index) const
{
	if (index < 0 || index >= static_cast<int>(PixelShaders.size()))
		return nullptr;
	return PixelShaders[index];
}

const std::string& r3dDX11ShaderLibrary::GetLastError() const
{
	return LastError;
}

bool r3dDX11ShaderLibrary::IsInitialized() const
{
	return Device != nullptr;
}
