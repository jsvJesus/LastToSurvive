#pragma once

#include <string>
#include <vector>

struct ID3D11Device;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D10Blob;
typedef ID3D10Blob ID3DBlob;

struct r3dDX11ShaderMacro
{
	const char* Name = nullptr;
	const char* Definition = nullptr;
};

class r3dDX11ShaderCompiler final
{
public:
	static const char* VertexProfile();
	static const char* PixelProfile();

	r3dDX11ShaderCompiler();

	void SetSourceRoot(const char* path);
	void SetCacheRoot(const char* path);

	const std::string& GetSourceRoot() const;
	const std::string& GetCacheRoot() const;
	const std::string& GetLastError() const;

	bool CompileVertexShader(
		const char* relativePath,
		const char* entryPoint,
		const r3dDX11ShaderMacro* macros,
		ID3DBlob** bytecode,
		std::vector<std::string>* includes = nullptr
	);

	bool CompilePixelShader(
		const char* relativePath,
		const char* entryPoint,
		const r3dDX11ShaderMacro* macros,
		ID3DBlob** bytecode,
		std::vector<std::string>* includes = nullptr
	);

	bool CompileFromFile(
		const char* relativePath,
		const char* entryPoint,
		const char* profile,
		const r3dDX11ShaderMacro* macros,
		ID3DBlob** bytecode,
		std::vector<std::string>* includes = nullptr
	);

private:
	std::string BuildSourcePath(const char* relativePath) const;
	std::string BuildCachePath(const char* relativePath, const char* entryPoint, const char* profile, const r3dDX11ShaderMacro* macros) const;

	bool LoadBinaryCache(const char* sourcePath, const char* cachePath, ID3DBlob** bytecode, std::vector<std::string>* includes);
	void SaveBinaryCache(const char* cachePath, ID3DBlob* bytecode, const std::vector<std::string>& includes);
	bool IsCacheCurrent(const char* sourcePath, const char* cachePath, const std::vector<std::string>& includes) const;

	void SetLastError(const char* format, ...);

private:
	std::string SourceRoot;
	std::string CacheRoot;
	std::string LastError;
};

class r3dDX11VertexShader final
{
public:
	r3dDX11VertexShader();
	~r3dDX11VertexShader();

	bool Create(
		ID3D11Device* device,
		r3dDX11ShaderCompiler& compiler,
		const char* name,
		const char* relativePath,
		const char* entryPoint,
		const r3dDX11ShaderMacro* macros
	);

	void Shutdown();

	const std::string& GetName() const;
	ID3D11VertexShader* GetShader() const;
	const void* GetBytecode() const;
	size_t GetBytecodeSize() const;
	bool IsValid() const;

private:
	std::string Name;
	ID3D11VertexShader* Shader = nullptr;
	ID3DBlob* Bytecode = nullptr;
};

class r3dDX11PixelShader final
{
public:
	r3dDX11PixelShader();
	~r3dDX11PixelShader();

	bool Create(
		ID3D11Device* device,
		r3dDX11ShaderCompiler& compiler,
		const char* name,
		const char* relativePath,
		const char* entryPoint,
		const r3dDX11ShaderMacro* macros
	);

	void Shutdown();

	const std::string& GetName() const;
	ID3D11PixelShader* GetShader() const;
	bool IsValid() const;

private:
	std::string Name;
	ID3D11PixelShader* Shader = nullptr;
};

class r3dDX11ShaderLibrary final
{
public:
	r3dDX11ShaderLibrary();
	~r3dDX11ShaderLibrary();

	bool Init(ID3D11Device* device);
	void Shutdown();

	r3dDX11ShaderCompiler& GetCompiler();
	const r3dDX11ShaderCompiler& GetCompiler() const;

	int AddVertexShader(const char* name, const char* relativePath, const char* entryPoint = "main", const r3dDX11ShaderMacro* macros = nullptr);
	int AddPixelShader(const char* name, const char* relativePath, const char* entryPoint = "main", const r3dDX11ShaderMacro* macros = nullptr);

	int FindVertexShader(const char* name) const;
	int FindPixelShader(const char* name) const;

	r3dDX11VertexShader* GetVertexShader(int index);
	r3dDX11PixelShader* GetPixelShader(int index);
	const r3dDX11VertexShader* GetVertexShader(int index) const;
	const r3dDX11PixelShader* GetPixelShader(int index) const;

	const std::string& GetLastError() const;
	bool IsInitialized() const;

private:
	ID3D11Device* Device = nullptr;
	r3dDX11ShaderCompiler Compiler;
	std::string LastError;
	std::vector<r3dDX11VertexShader*> VertexShaders;
	std::vector<r3dDX11PixelShader*> PixelShaders;
};
