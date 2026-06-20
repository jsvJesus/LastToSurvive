#pragma once

#include <d3d11_1.h>

#include <map>
#include <string>
#include <vector>

class r3dDX11DrawContext;

class r3dDX11Texture final
{
public:
	r3dDX11Texture();
	~r3dDX11Texture();

	bool LoadFromFile(ID3D11Device* device, const char* path, bool generateMips, const char* debugName = nullptr);
	bool CreateFromColor(ID3D11Device* device, unsigned char r, unsigned char g, unsigned char b, unsigned char a, const char* debugName = nullptr);
	void Shutdown();
	void BindPS(r3dDX11DrawContext& drawContext, unsigned int slot) const;

	ID3D11Texture2D* GetTexture() const;
	ID3D11ShaderResourceView* GetSRV() const;
	const std::string& GetPath() const;
	int GetWidth() const;
	int GetHeight() const;
	int GetMipCount() const;
	DXGI_FORMAT GetFormat() const;
	bool IsValid() const;

private:
	bool LoadDDS(ID3D11Device* device, const char* path, bool generateMips, const char* debugName);
	bool LoadWIC(ID3D11Device* device, const char* path, bool generateMips, const char* debugName);
	bool CreateFromRGBA8(ID3D11Device* device, const unsigned char* pixels, int width, int height, bool generateMips, const char* debugName);

private:
	ID3D11Texture2D* Texture = nullptr;
	ID3D11ShaderResourceView* SRV = nullptr;
	std::string Path;
	int Width = 0;
	int Height = 0;
	int MipCount = 0;
	DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
};

class r3dDX11TextureLibrary final
{
public:
	r3dDX11TextureLibrary();
	~r3dDX11TextureLibrary();

	bool Init(ID3D11Device* device);
	void Shutdown();

	r3dDX11Texture* LoadTexture(const char* path, bool generateMips = false);
	r3dDX11Texture* FindTexture(const char* path);
	const r3dDX11Texture* FindTexture(const char* path) const;
	r3dDX11Texture* GetWhiteTexture();
	r3dDX11Texture* GetBlackTexture();
	r3dDX11Texture* GetFlatNormalTexture();
	void UnloadAll();

	const std::string& GetLastError() const;
	bool IsInitialized() const;

private:
	std::string NormalizePath(const char* path) const;
	void SetLastError(const char* format, ...);

private:
	ID3D11Device* Device = nullptr;
	r3dDX11Texture* WhiteTexture = nullptr;
	r3dDX11Texture* BlackTexture = nullptr;
	r3dDX11Texture* FlatNormalTexture = nullptr;
	std::map<std::string, r3dDX11Texture*> Textures;
	std::string LastError;
};
