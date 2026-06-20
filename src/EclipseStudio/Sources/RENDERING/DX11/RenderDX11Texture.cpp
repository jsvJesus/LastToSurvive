#include "r3dPCH.h"

#include "RENDERING/DX11/RenderDX11Texture.h"

#include "RENDERING/DX11/RenderDX11Draw.h"

#include <wincodec.h>
#include <windows.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "windowscodecs.lib")

namespace
{
	template <typename T>
	void SafeReleaseDX11(T*& value)
	{
		if (value)
		{
			value->Release();
			value = nullptr;
		}
	}

	void SetDebugName(ID3D11DeviceChild* object, const char* name)
	{
		if (object && name && name[0])
			object->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(name)), name);
	}

	bool ReadFileBytes(const char* path, std::vector<unsigned char>& data)
	{
		FILE* file = fopen(path, "rb");
		if (!file)
			return false;

		fseek(file, 0, SEEK_END);
		const long size = ftell(file);
		fseek(file, 0, SEEK_SET);
		if (size <= 0)
		{
			fclose(file);
			return false;
		}

		data.resize(static_cast<size_t>(size));
		const bool ok = fread(&data[0], 1, data.size(), file) == data.size();
		fclose(file);
		return ok;
	}

	bool HasExtension(const char* path, const char* ext)
	{
		if (!path || !ext)
			return false;

		const char* dot = strrchr(path, '.');
		return dot && _stricmp(dot, ext) == 0;
	}

	std::wstring Utf8ToWide(const char* text)
	{
		if (!text || !text[0])
			return std::wstring();

		const int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
		if (length <= 0)
			return std::wstring();

		std::wstring result;
		result.resize(static_cast<size_t>(length - 1));
		MultiByteToWideChar(CP_UTF8, 0, text, -1, &result[0], length);
		return result;
	}

	DWORD MakeFourCC(char a, char b, char c, char d)
	{
		return
			(static_cast<DWORD>(static_cast<unsigned char>(a))) |
			(static_cast<DWORD>(static_cast<unsigned char>(b)) << 8) |
			(static_cast<DWORD>(static_cast<unsigned char>(c)) << 16) |
			(static_cast<DWORD>(static_cast<unsigned char>(d)) << 24);
	}

	const DWORD DDS_MAGIC = MakeFourCC('D', 'D', 'S', ' ');
	const DWORD DDS_FOURCC = 0x00000004;
	const DWORD DDS_RGB = 0x00000040;
	const DWORD DDS_RGBA = 0x00000041;
	const DWORD DDS_LUMINANCE = 0x00020000;
	const DWORD DDS_HEADER_FLAGS_VOLUME = 0x00800000;
	const DWORD DDS_RESOURCE_MISC_TEXTURECUBE = 0x4;
	const DWORD DDSCAPS2_CUBEMAP = 0x00000200;

#pragma pack(push, 1)
	struct DDS_PIXELFORMAT
	{
		DWORD size;
		DWORD flags;
		DWORD fourCC;
		DWORD rgbBitCount;
		DWORD rBitMask;
		DWORD gBitMask;
		DWORD bBitMask;
		DWORD aBitMask;
	};

	struct DDS_HEADER
	{
		DWORD size;
		DWORD flags;
		DWORD height;
		DWORD width;
		DWORD pitchOrLinearSize;
		DWORD depth;
		DWORD mipMapCount;
		DWORD reserved1[11];
		DDS_PIXELFORMAT ddspf;
		DWORD caps;
		DWORD caps2;
		DWORD caps3;
		DWORD caps4;
		DWORD reserved2;
	};

	struct DDS_HEADER_DXT10
	{
		DXGI_FORMAT dxgiFormat;
		DWORD resourceDimension;
		DWORD miscFlag;
		DWORD arraySize;
		DWORD miscFlags2;
	};
#pragma pack(pop)

	bool IsBlockCompressed(DXGI_FORMAT format)
	{
		return format == DXGI_FORMAT_BC1_UNORM ||
			format == DXGI_FORMAT_BC2_UNORM ||
			format == DXGI_FORMAT_BC3_UNORM ||
			format == DXGI_FORMAT_BC4_UNORM ||
			format == DXGI_FORMAT_BC5_UNORM;
	}

	size_t BitsPerPixel(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R8_UNORM:
			return 8;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return 32;
		default:
			return 0;
		}
	}

	size_t BlockBytes(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC4_UNORM:
			return 8;
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC5_UNORM:
			return 16;
		default:
			return 0;
		}
	}

	bool GetSurfaceInfo(size_t width, size_t height, DXGI_FORMAT format, size_t& rowBytes, size_t& numRows, size_t& numBytes)
	{
		if (IsBlockCompressed(format))
		{
			const size_t blockBytes = BlockBytes(format);
			if (!blockBytes)
				return false;

			const size_t blocksWide = std::max<size_t>(1, (width + 3) / 4);
			const size_t blocksHigh = std::max<size_t>(1, (height + 3) / 4);
			rowBytes = blocksWide * blockBytes;
			numRows = blocksHigh;
			numBytes = rowBytes * numRows;
			return true;
		}

		const size_t bpp = BitsPerPixel(format);
		if (!bpp)
			return false;

		rowBytes = (width * bpp + 7) / 8;
		numRows = height;
		numBytes = rowBytes * numRows;
		return true;
	}

	DXGI_FORMAT GetDXGIFormat(const DDS_PIXELFORMAT& ddpf)
	{
		if (ddpf.flags & DDS_FOURCC)
		{
			if (ddpf.fourCC == MakeFourCC('D', 'X', 'T', '1'))
				return DXGI_FORMAT_BC1_UNORM;
			if (ddpf.fourCC == MakeFourCC('D', 'X', 'T', '3'))
				return DXGI_FORMAT_BC2_UNORM;
			if (ddpf.fourCC == MakeFourCC('D', 'X', 'T', '5'))
				return DXGI_FORMAT_BC3_UNORM;
			if (ddpf.fourCC == MakeFourCC('B', 'C', '4', 'U') || ddpf.fourCC == MakeFourCC('A', 'T', 'I', '1'))
				return DXGI_FORMAT_BC4_UNORM;
			if (ddpf.fourCC == MakeFourCC('B', 'C', '5', 'U') || ddpf.fourCC == MakeFourCC('A', 'T', 'I', '2'))
				return DXGI_FORMAT_BC5_UNORM;
			return DXGI_FORMAT_UNKNOWN;
		}

		if ((ddpf.flags & DDS_RGBA) && ddpf.rgbBitCount == 32)
		{
			if (ddpf.rBitMask == 0x000000ff && ddpf.gBitMask == 0x0000ff00 && ddpf.bBitMask == 0x00ff0000 && ddpf.aBitMask == 0xff000000)
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			if (ddpf.rBitMask == 0x00ff0000 && ddpf.gBitMask == 0x0000ff00 && ddpf.bBitMask == 0x000000ff && ddpf.aBitMask == 0xff000000)
				return DXGI_FORMAT_B8G8R8A8_UNORM;
		}

		if ((ddpf.flags & DDS_RGB) && ddpf.rgbBitCount == 32)
		{
			if (ddpf.rBitMask == 0x00ff0000 && ddpf.gBitMask == 0x0000ff00 && ddpf.bBitMask == 0x000000ff)
				return DXGI_FORMAT_B8G8R8A8_UNORM;
		}

		if ((ddpf.flags & DDS_LUMINANCE) && ddpf.rgbBitCount == 8)
			return DXGI_FORMAT_R8_UNORM;

		return DXGI_FORMAT_UNKNOWN;
	}
}

r3dDX11Texture::r3dDX11Texture()
{
}

r3dDX11Texture::~r3dDX11Texture()
{
	Shutdown();
}

bool r3dDX11Texture::LoadFromFile(ID3D11Device* device, const char* path, bool generateMips, const char* debugName)
{
	Shutdown();

	if (!device || !path || !path[0])
		return false;

	Path = path;
	if (HasExtension(path, ".dds"))
		return LoadDDS(device, path, generateMips, debugName);

	return LoadWIC(device, path, generateMips, debugName);
}

bool r3dDX11Texture::CreateFromColor(ID3D11Device* device, unsigned char r, unsigned char g, unsigned char b, unsigned char a, const char* debugName)
{
	Shutdown();

	const unsigned char pixel[4] = { r, g, b, a };
	Path = debugName ? debugName : "";
	return CreateFromRGBA8(device, pixel, 1, 1, false, debugName);
}

void r3dDX11Texture::Shutdown()
{
	SafeReleaseDX11(SRV);
	SafeReleaseDX11(Texture);
	Path.clear();
	Width = 0;
	Height = 0;
	MipCount = 0;
	Format = DXGI_FORMAT_UNKNOWN;
}

void r3dDX11Texture::BindPS(r3dDX11DrawContext& drawContext, unsigned int slot) const
{
	drawContext.SetShaderResource(slot, SRV);
}

ID3D11Texture2D* r3dDX11Texture::GetTexture() const
{
	return Texture;
}

ID3D11ShaderResourceView* r3dDX11Texture::GetSRV() const
{
	return SRV;
}

const std::string& r3dDX11Texture::GetPath() const
{
	return Path;
}

int r3dDX11Texture::GetWidth() const
{
	return Width;
}

int r3dDX11Texture::GetHeight() const
{
	return Height;
}

int r3dDX11Texture::GetMipCount() const
{
	return MipCount;
}

DXGI_FORMAT r3dDX11Texture::GetFormat() const
{
	return Format;
}

bool r3dDX11Texture::IsValid() const
{
	return Texture != nullptr && SRV != nullptr;
}

bool r3dDX11Texture::LoadDDS(ID3D11Device* device, const char* path, bool, const char* debugName)
{
	std::vector<unsigned char> data;
	if (!ReadFileBytes(path, data) || data.size() < sizeof(DWORD) + sizeof(DDS_HEADER))
		return false;

	const DWORD magic = *reinterpret_cast<const DWORD*>(&data[0]);
	if (magic != DDS_MAGIC)
		return false;

	const DDS_HEADER* header = reinterpret_cast<const DDS_HEADER*>(&data[sizeof(DWORD)]);
	if (header->size != sizeof(DDS_HEADER) || header->ddspf.size != sizeof(DDS_PIXELFORMAT))
		return false;

	const bool hasDX10Header = (header->ddspf.flags & DDS_FOURCC) && header->ddspf.fourCC == MakeFourCC('D', 'X', '1', '0');
	size_t offset = sizeof(DWORD) + sizeof(DDS_HEADER);
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	UINT arraySize = 1;
	bool isCube = (header->caps2 & DDSCAPS2_CUBEMAP) != 0;

	if (hasDX10Header)
	{
		if (data.size() < offset + sizeof(DDS_HEADER_DXT10))
			return false;

		const DDS_HEADER_DXT10* dx10 = reinterpret_cast<const DDS_HEADER_DXT10*>(&data[offset]);
		offset += sizeof(DDS_HEADER_DXT10);
		format = dx10->dxgiFormat;
		arraySize = dx10->arraySize;
		isCube = isCube || (dx10->miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE) != 0;
	}
	else
	{
		format = GetDXGIFormat(header->ddspf);
	}

	if (format == DXGI_FORMAT_UNKNOWN || arraySize != 1 || isCube || (header->flags & DDS_HEADER_FLAGS_VOLUME) || header->depth > 1)
		return false;

	const UINT width = std::max<DWORD>(1, header->width);
	const UINT height = std::max<DWORD>(1, header->height);
	const UINT mipCount = std::max<DWORD>(1, header->mipMapCount);

	std::vector<D3D11_SUBRESOURCE_DATA> subresources;
	subresources.reserve(mipCount);

	size_t sourceOffset = offset;
	UINT mipWidth = width;
	UINT mipHeight = height;
	for (UINT mip = 0; mip < mipCount; ++mip)
	{
		size_t rowBytes = 0;
		size_t numRows = 0;
		size_t numBytes = 0;
		if (!GetSurfaceInfo(mipWidth, mipHeight, format, rowBytes, numRows, numBytes))
			return false;

		if (sourceOffset + numBytes > data.size())
			return false;

		D3D11_SUBRESOURCE_DATA subresource{};
		subresource.pSysMem = &data[sourceOffset];
		subresource.SysMemPitch = static_cast<UINT>(rowBytes);
		subresource.SysMemSlicePitch = static_cast<UINT>(numBytes);
		subresources.push_back(subresource);

		sourceOffset += numBytes;
		mipWidth = std::max<UINT>(1, mipWidth >> 1);
		mipHeight = std::max<UINT>(1, mipHeight >> 1);
	}

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = mipCount;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	HRESULT result = device->CreateTexture2D(&desc, &subresources[0], &Texture);
	if (FAILED(result) || !Texture)
	{
		Shutdown();
		return false;
	}

	result = device->CreateShaderResourceView(Texture, nullptr, &SRV);
	if (FAILED(result) || !SRV)
	{
		Shutdown();
		return false;
	}

	SetDebugName(Texture, debugName ? debugName : path);
	Width = static_cast<int>(width);
	Height = static_cast<int>(height);
	MipCount = static_cast<int>(mipCount);
	Format = format;
	return true;
}

bool r3dDX11Texture::LoadWIC(ID3D11Device* device, const char* path, bool generateMips, const char* debugName)
{
	HRESULT coResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(coResult) && coResult != RPC_E_CHANGED_MODE)
		return false;

	IWICImagingFactory* factory = nullptr;
	IWICBitmapDecoder* decoder = nullptr;
	IWICBitmapFrameDecode* frame = nullptr;
	IWICFormatConverter* converter = nullptr;

	auto finish = [&](bool ok) -> bool
	{
		SafeReleaseDX11(converter);
		SafeReleaseDX11(frame);
		SafeReleaseDX11(decoder);
		SafeReleaseDX11(factory);
		return ok;
	};

	HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, __uuidof(IWICImagingFactory), reinterpret_cast<void**>(&factory));
	if (FAILED(result) || !factory)
		return finish(false);

	const std::wstring widePath = Utf8ToWide(path);
	result = factory->CreateDecoderFromFilename(widePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
	if (FAILED(result) || !decoder)
		return finish(false);

	result = decoder->GetFrame(0, &frame);
	if (FAILED(result) || !frame)
		return finish(false);

	UINT width = 0;
	UINT height = 0;
	result = frame->GetSize(&width, &height);
	if (FAILED(result) || width == 0 || height == 0)
		return finish(false);

	result = factory->CreateFormatConverter(&converter);
	if (FAILED(result) || !converter)
		return finish(false);

	result = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
	if (FAILED(result))
		return finish(false);

	if (width > UINT_MAX / 4)
		return finish(false);

	const UINT stride = width * 4;
	const UINT imageSize = stride * height;
	std::vector<unsigned char> pixels(imageSize);
	result = converter->CopyPixels(nullptr, stride, imageSize, &pixels[0]);
	if (FAILED(result))
		return finish(false);

	return finish(CreateFromRGBA8(device, &pixels[0], static_cast<int>(width), static_cast<int>(height), generateMips, debugName ? debugName : path));
}

bool r3dDX11Texture::CreateFromRGBA8(ID3D11Device* device, const unsigned char* pixels, int width, int height, bool generateMips, const char* debugName)
{
	if (!device || !pixels || width <= 0 || height <= 0)
		return false;

	ID3D11DeviceContext* context = nullptr;
	if (generateMips)
		device->GetImmediateContext(&context);

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = static_cast<UINT>(width);
	desc.Height = static_cast<UINT>(height);
	desc.MipLevels = generateMips ? 0 : 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | (generateMips ? D3D11_BIND_RENDER_TARGET : 0);
	desc.MiscFlags = generateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

	D3D11_SUBRESOURCE_DATA init{};
	init.pSysMem = pixels;
	init.SysMemPitch = static_cast<UINT>(width * 4);

	HRESULT result = device->CreateTexture2D(&desc, generateMips ? nullptr : &init, &Texture);
	if (FAILED(result) || !Texture)
	{
		SafeReleaseDX11(context);
		Shutdown();
		return false;
	}

	result = device->CreateShaderResourceView(Texture, nullptr, &SRV);
	if (FAILED(result) || !SRV)
	{
		SafeReleaseDX11(context);
		Shutdown();
		return false;
	}

	if (generateMips && context)
	{
		context->UpdateSubresource(Texture, 0, nullptr, pixels, static_cast<UINT>(width * 4), 0);
		context->GenerateMips(SRV);
	}

	SetDebugName(Texture, debugName);
	Width = width;
	Height = height;
	MipCount = generateMips ? 0 : 1;
	Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	SafeReleaseDX11(context);
	return true;
}

r3dDX11TextureLibrary::r3dDX11TextureLibrary()
{
}

r3dDX11TextureLibrary::~r3dDX11TextureLibrary()
{
	Shutdown();
}

bool r3dDX11TextureLibrary::Init(ID3D11Device* device)
{
	if (!device)
		return false;

	Device = device;
	LastError.clear();

	WhiteTexture = new r3dDX11Texture();
	BlackTexture = new r3dDX11Texture();
	FlatNormalTexture = new r3dDX11Texture();

	if (!WhiteTexture->CreateFromColor(Device, 255, 255, 255, 255, "DX11.Default.White") ||
		!BlackTexture->CreateFromColor(Device, 0, 0, 0, 255, "DX11.Default.Black") ||
		!FlatNormalTexture->CreateFromColor(Device, 128, 128, 255, 255, "DX11.Default.FlatNormal"))
	{
		Shutdown();
		return false;
	}

	return true;
}

void r3dDX11TextureLibrary::Shutdown()
{
	UnloadAll();
	delete FlatNormalTexture;
	delete BlackTexture;
	delete WhiteTexture;
	FlatNormalTexture = nullptr;
	BlackTexture = nullptr;
	WhiteTexture = nullptr;
	Device = nullptr;
	LastError.clear();
}

r3dDX11Texture* r3dDX11TextureLibrary::LoadTexture(const char* path, bool generateMips)
{
	LastError.clear();

	const std::string normalized = NormalizePath(path);
	if (normalized.empty())
	{
		SetLastError("Invalid DX11 texture path");
		return nullptr;
	}

	std::map<std::string, r3dDX11Texture*>::iterator existing = Textures.find(normalized);
	if (existing != Textures.end())
		return existing->second;

	r3dDX11Texture* texture = new r3dDX11Texture();
	if (!texture->LoadFromFile(Device, normalized.c_str(), generateMips, normalized.c_str()))
	{
		SetLastError("DX11 texture load failed '%s'", normalized.c_str());
		delete texture;
		return nullptr;
	}

	Textures[normalized] = texture;
	return texture;
}

r3dDX11Texture* r3dDX11TextureLibrary::FindTexture(const char* path)
{
	const std::string normalized = NormalizePath(path);
	std::map<std::string, r3dDX11Texture*>::iterator found = Textures.find(normalized);
	return found != Textures.end() ? found->second : nullptr;
}

const r3dDX11Texture* r3dDX11TextureLibrary::FindTexture(const char* path) const
{
	const std::string normalized = NormalizePath(path);
	std::map<std::string, r3dDX11Texture*>::const_iterator found = Textures.find(normalized);
	return found != Textures.end() ? found->second : nullptr;
}

r3dDX11Texture* r3dDX11TextureLibrary::GetWhiteTexture()
{
	return WhiteTexture;
}

r3dDX11Texture* r3dDX11TextureLibrary::GetBlackTexture()
{
	return BlackTexture;
}

r3dDX11Texture* r3dDX11TextureLibrary::GetFlatNormalTexture()
{
	return FlatNormalTexture;
}

void r3dDX11TextureLibrary::UnloadAll()
{
	for (std::map<std::string, r3dDX11Texture*>::iterator it = Textures.begin(); it != Textures.end(); ++it)
		delete it->second;
	Textures.clear();
}

const std::string& r3dDX11TextureLibrary::GetLastError() const
{
	return LastError;
}

bool r3dDX11TextureLibrary::IsInitialized() const
{
	return Device != nullptr;
}

std::string r3dDX11TextureLibrary::NormalizePath(const char* path) const
{
	if (!path || !path[0])
		return std::string();

	char fullPath[MAX_PATH] = {};
	if (GetFullPathNameA(path, _countof(fullPath), fullPath, nullptr) == 0)
		return path;

	for (char* c = fullPath; *c; ++c)
	{
		if (*c == '/')
			*c = '\\';
		else
			*c = static_cast<char>(tolower(static_cast<unsigned char>(*c)));
	}

	return fullPath;
}

void r3dDX11TextureLibrary::SetLastError(const char* format, ...)
{
	char buffer[2048];
	va_list args;
	va_start(args, format);
	_vsnprintf(buffer, sizeof(buffer) - 1, format, args);
	va_end(args);
	buffer[sizeof(buffer) - 1] = 0;
	LastError = buffer;
}
