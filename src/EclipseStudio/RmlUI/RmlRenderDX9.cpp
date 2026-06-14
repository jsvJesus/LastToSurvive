#include "r3dPCH.h"
#include "r3d.h"

#include "RmlRenderDX9.h"

#include <windows.h>
#include <algorithm>
#include <vector>

#ifdef SetViewport
#undef SetViewport
#endif

#ifdef SetTransform
#undef SetTransform
#endif

#ifdef SetFVF
#undef SetFVF
#endif

#ifdef SetRenderState
#undef SetRenderState
#endif

#ifdef SetTextureStageState
#undef SetTextureStageState
#endif

#ifdef SetSamplerState
#undef SetSamplerState
#endif

#ifdef SetScissorRect
#undef SetScissorRect
#endif

#ifdef SetStreamSource
#undef SetStreamSource
#endif

#ifdef SetIndices
#undef SetIndices
#endif

#ifdef SetTexture
#undef SetTexture
#endif

#ifdef DrawIndexedPrimitive
#undef DrawIndexedPrimitive
#endif

#ifdef CreateStateBlock
#undef CreateStateBlock
#endif

#ifdef CreateVertexBuffer
#undef CreateVertexBuffer
#endif

#ifdef CreateIndexBuffer
#undef CreateIndexBuffer
#endif

#ifdef D3DRS_CULLMODE
#undef D3DRS_CULLMODE
#endif

#ifdef D3DRS_SCISSORTESTENABLE
#undef D3DRS_SCISSORTESTENABLE
#endif

#ifdef D3DRS_ALPHATESTENABLE
#undef D3DRS_ALPHATESTENABLE
#endif

#ifdef D3DRS_ALPHABLENDENABLE
#undef D3DRS_ALPHABLENDENABLE
#endif

#ifdef D3DRS_SRCBLEND
#undef D3DRS_SRCBLEND
#endif

#ifdef D3DRS_DESTBLEND
#undef D3DRS_DESTBLEND
#endif

#ifdef D3DRS_BLENDOP
#undef D3DRS_BLENDOP
#endif

#ifdef D3DRS_ZENABLE
#undef D3DRS_ZENABLE
#endif

#ifdef D3DRS_ZWRITEENABLE
#undef D3DRS_ZWRITEENABLE
#endif

#ifdef D3DRS_LIGHTING
#undef D3DRS_LIGHTING
#endif

#ifdef CreateTexture
#undef CreateTexture
#endif

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

static const D3DRENDERSTATETYPE RML_D3DRS_ZENABLE = (D3DRENDERSTATETYPE)7;
static const D3DRENDERSTATETYPE RML_D3DRS_ZWRITEENABLE = (D3DRENDERSTATETYPE)14;
static const D3DRENDERSTATETYPE RML_D3DRS_ALPHATESTENABLE = (D3DRENDERSTATETYPE)15;
static const D3DRENDERSTATETYPE RML_D3DRS_SRCBLEND = (D3DRENDERSTATETYPE)19;
static const D3DRENDERSTATETYPE RML_D3DRS_DESTBLEND = (D3DRENDERSTATETYPE)20;
static const D3DRENDERSTATETYPE RML_D3DRS_CULLMODE = (D3DRENDERSTATETYPE)22;
static const D3DRENDERSTATETYPE RML_D3DRS_ALPHABLENDENABLE = (D3DRENDERSTATETYPE)27;
static const D3DRENDERSTATETYPE RML_D3DRS_LIGHTING = (D3DRENDERSTATETYPE)137;
static const D3DRENDERSTATETYPE RML_D3DRS_SCISSORTESTENABLE = (D3DRENDERSTATETYPE)174;
static const D3DRENDERSTATETYPE RML_D3DRS_BLENDOP = (D3DRENDERSTATETYPE)171;

static bool RmlFileExistsW(const std::wstring& Path)
{
	if (Path.empty())
		return false;

	const DWORD Attributes = GetFileAttributesW(Path.c_str());

	if (Attributes == INVALID_FILE_ATTRIBUTES)
		return false;

	return (Attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

RmlRenderDX9::RmlRenderDX9()
{
}

RmlRenderDX9::~RmlRenderDX9()
{
	Shutdown();
}

bool RmlRenderDX9::Init(IDirect3DDevice9* InDevice, const wchar_t* InDataRoot)
{
	if (!InDevice)
	{
		OutputDebugStringA("[RmlUI][DX9] Init failed: null IDirect3DDevice9\n");
		return false;
	}

	Device = InDevice;
	Device->AddRef();

	DataRoot = InDataRoot ? InDataRoot : L"";

	while (!DataRoot.empty() && (DataRoot.back() == L'\\' || DataRoot.back() == L'/'))
		DataRoot.pop_back();

	OutputDebugStringA("[RmlUI][DX9] Render interface initialized\n");
	return true;
}

void RmlRenderDX9::Shutdown()
{
	if (StateBlock)
	{
		StateBlock->Release();
		StateBlock = nullptr;
	}

	if (Device)
	{
		Device->Release();
		Device = nullptr;
	}
}

void RmlRenderDX9::BeginFrame(int Width, int Height)
{
	if (!Device)
		return;

	ViewWidth = std::max(1, Width);
	ViewHeight = std::max(1, Height);

	if (StateBlock)
	{
		StateBlock->Release();
		StateBlock = nullptr;
	}

	if (SUCCEEDED(Device->CreateStateBlock(D3DSBT_ALL, &StateBlock)) && StateBlock)
		StateBlock->Capture();

	SetupRenderState();
}

void RmlRenderDX9::EndFrame()
{
	if (!Device)
		return;

	if (StateBlock)
	{
		StateBlock->Apply();
		StateBlock->Release();
		StateBlock = nullptr;
	}
}

void RmlRenderDX9::OnDeviceLost()
{
	if (StateBlock)
	{
		StateBlock->Release();
		StateBlock = nullptr;
	}

	OutputDebugStringA("[RmlUI][DX9] Device lost\n");
}

void RmlRenderDX9::OnDeviceReset(int Width, int Height)
{
	ViewWidth = std::max(1, Width);
	ViewHeight = std::max(1, Height);

	OutputDebugStringA("[RmlUI][DX9] Device reset\n");
}

DWORD RmlRenderDX9::ConvertColor(const Rml::ColourbPremultiplied& Color)
{
	return D3DCOLOR_ARGB(Color.alpha, Color.red, Color.green, Color.blue);
}

D3DMATRIX RmlRenderDX9::MakeIdentity()
{
	D3DMATRIX M{};
	M._11 = 1.0f;
	M._22 = 1.0f;
	M._33 = 1.0f;
	M._44 = 1.0f;
	return M;
}

D3DMATRIX RmlRenderDX9::MakeOrthoOffCenterLH(float Left, float Right, float Bottom, float Top, float ZNear, float ZFar)
{
	D3DMATRIX M{};

	M._11 = 2.0f / (Right - Left);
	M._22 = 2.0f / (Top - Bottom);
	M._33 = 1.0f / (ZFar - ZNear);
	M._44 = 1.0f;

	M._41 = (Left + Right) / (Left - Right);
	M._42 = (Top + Bottom) / (Bottom - Top);
	M._43 = ZNear / (ZNear - ZFar);

	return M;
}

D3DMATRIX RmlRenderDX9::MakeTranslation(float X, float Y, float Z)
{
	D3DMATRIX M = MakeIdentity();

	M._41 = X;
	M._42 = Y;
	M._43 = Z;

	return M;
}

void RmlRenderDX9::SetupRenderState()
{
	if (!Device)
		return;

	D3DVIEWPORT9 Viewport{};
	Viewport.X = 0;
	Viewport.Y = 0;
	Viewport.Width = static_cast<DWORD>(ViewWidth);
	Viewport.Height = static_cast<DWORD>(ViewHeight);
	Viewport.MinZ = 0.0f;
	Viewport.MaxZ = 1.0f;

	Device->SetViewport(&Viewport);

	const D3DMATRIX World = MakeIdentity();
	const D3DMATRIX View = MakeIdentity();
	const D3DMATRIX Projection = MakeOrthoOffCenterLH(
		0.0f,
		static_cast<float>(ViewWidth),
		static_cast<float>(ViewHeight),
		0.0f,
		-1.0f,
		1.0f
	);

	Device->SetTransform(D3DTS_WORLD, &World);
	Device->SetTransform(D3DTS_VIEW, &View);
	Device->SetTransform(D3DTS_PROJECTION, &Projection);

	Device->SetFVF(VertexFVF);

	(Device->SetRenderState)(RML_D3DRS_LIGHTING, FALSE);
	(Device->SetRenderState)(RML_D3DRS_ZENABLE, FALSE);
	(Device->SetRenderState)(RML_D3DRS_ZWRITEENABLE, FALSE);
	(Device->SetRenderState)(RML_D3DRS_CULLMODE, D3DCULL_NONE);

	(Device->SetRenderState)(RML_D3DRS_ALPHATESTENABLE, FALSE);
	(Device->SetRenderState)(RML_D3DRS_ALPHABLENDENABLE, TRUE);

	(Device->SetRenderState)(RML_D3DRS_SRCBLEND, D3DBLEND_ONE);
	(Device->SetRenderState)(RML_D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	(Device->SetRenderState)(RML_D3DRS_BLENDOP, D3DBLENDOP_ADD);

	(Device->SetRenderState)(RML_D3DRS_SCISSORTESTENABLE, bScissorEnabled ? TRUE : FALSE);

	if (bScissorEnabled)
		Device->SetScissorRect(&ScissorRect);

	Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	Device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	Device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	Device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	Device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	Device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
}

Rml::CompiledGeometryHandle RmlRenderDX9::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	if (!Device || vertices.empty() || indices.empty())
		return 0;

	FCompiledGeometry* Geometry = new FCompiledGeometry();
	Geometry->NumVertices = static_cast<int>(vertices.size());
	Geometry->NumIndices = static_cast<int>(indices.size());

	const UINT VertexBytes = static_cast<UINT>(sizeof(FDx9Vertex) * Geometry->NumVertices);
	const UINT IndexBytes = static_cast<UINT>(sizeof(unsigned int) * Geometry->NumIndices);

	HRESULT Hr = Device->CreateVertexBuffer(
		VertexBytes,
		0,
		VertexFVF,
		D3DPOOL_MANAGED,
		&Geometry->VertexBuffer,
		nullptr
	);

	if (FAILED(Hr))
	{
		delete Geometry;
		OutputDebugStringA("[RmlUI][DX9] CreateVertexBuffer failed\n");
		return 0;
	}

	Hr = Device->CreateIndexBuffer(
		IndexBytes,
		0,
		D3DFMT_INDEX32,
		D3DPOOL_MANAGED,
		&Geometry->IndexBuffer,
		nullptr
	);

	if (FAILED(Hr))
	{
		Geometry->VertexBuffer->Release();
		delete Geometry;
		OutputDebugStringA("[RmlUI][DX9] CreateIndexBuffer failed\n");
		return 0;
	}

	void* VertexMemory = nullptr;
	Hr = Geometry->VertexBuffer->Lock(0, VertexBytes, &VertexMemory, 0);

	if (FAILED(Hr))
	{
		ReleaseGeometry(reinterpret_cast<Rml::CompiledGeometryHandle>(Geometry));
		OutputDebugStringA("[RmlUI][DX9] VertexBuffer lock failed\n");
		return 0;
	}

	FDx9Vertex* DstVertices = static_cast<FDx9Vertex*>(VertexMemory);

	for (int i = 0; i < Geometry->NumVertices; ++i)
	{
		const Rml::Vertex& Src = vertices[i];

		DstVertices[i].X = Src.position.x;
		DstVertices[i].Y = Src.position.y;
		DstVertices[i].Z = 0.0f;
		DstVertices[i].Color = ConvertColor(Src.colour);
		DstVertices[i].U = Src.tex_coord.x;
		DstVertices[i].V = Src.tex_coord.y;
	}

	Geometry->VertexBuffer->Unlock();

	void* IndexMemory = nullptr;
	Hr = Geometry->IndexBuffer->Lock(0, IndexBytes, &IndexMemory, 0);

	if (FAILED(Hr))
	{
		ReleaseGeometry(reinterpret_cast<Rml::CompiledGeometryHandle>(Geometry));
		OutputDebugStringA("[RmlUI][DX9] IndexBuffer lock failed\n");
		return 0;
	}

	unsigned int* DstIndices = static_cast<unsigned int*>(IndexMemory);

	for (int i = 0; i < Geometry->NumIndices; ++i)
		DstIndices[i] = static_cast<unsigned int>(indices[i]);

	Geometry->IndexBuffer->Unlock();

	return reinterpret_cast<Rml::CompiledGeometryHandle>(Geometry);
}

void RmlRenderDX9::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture)
{
	if (!Device || !geometry)
		return;

	FCompiledGeometry* Geometry = reinterpret_cast<FCompiledGeometry*>(geometry);
	IDirect3DTexture9* Texture = reinterpret_cast<IDirect3DTexture9*>(texture);
	
	// Direct3D 9 считает центр верхнего левого пикселя координатой 0,0.
	// RmlUi генерирует геометрию по современному правилу границ пикселей,
	// поэтому для точного попадания texel -> pixel нужен offset -0.5.
	constexpr float HalfPixelOffset = -0.5f;

	const D3DMATRIX World = MakeTranslation(
		translation.x + HalfPixelOffset,
		translation.y + HalfPixelOffset,
		0.0f
	);
	
	Device->SetTransform(D3DTS_WORLD, &World);

	Device->SetStreamSource(0, Geometry->VertexBuffer, 0, sizeof(FDx9Vertex));
	Device->SetIndices(Geometry->IndexBuffer);
	Device->SetFVF(VertexFVF);

	if (Texture)
	{
		Device->SetTexture(0, Texture);

		Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		Device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		Device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		Device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	}
	else
	{
		Device->SetTexture(0, nullptr);

		Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
		Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		Device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
	}

	const UINT PrimitiveCount = static_cast<UINT>(Geometry->NumIndices / 3);

	Device->DrawIndexedPrimitive(
		D3DPT_TRIANGLELIST,
		0,
		0,
		static_cast<UINT>(Geometry->NumVertices),
		0,
		PrimitiveCount
	);
}

void RmlRenderDX9::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
	if (!geometry)
		return;

	FCompiledGeometry* Geometry = reinterpret_cast<FCompiledGeometry*>(geometry);

	if (Geometry->VertexBuffer)
	{
		Geometry->VertexBuffer->Release();
		Geometry->VertexBuffer = nullptr;
	}

	if (Geometry->IndexBuffer)
	{
		Geometry->IndexBuffer->Release();
		Geometry->IndexBuffer = nullptr;
	}

	delete Geometry;
}

std::wstring RmlRenderDX9::ResolvePathW(const Rml::String& path) const
{
	if (path.empty())
		return std::wstring();

	const int Required = MultiByteToWideChar(
		CP_UTF8,
		0,
		path.c_str(),
		static_cast<int>(path.size()),
		nullptr,
		0
	);

	if (Required <= 0)
	{
		OutputDebugStringA(
			"[RmlUI][DX9] ResolvePathW failed: UTF-8 conversion failed\n"
		);

		return std::wstring();
	}

	std::wstring WidePath;
	WidePath.resize(static_cast<size_t>(Required));

	MultiByteToWideChar(
		CP_UTF8,
		0,
		path.c_str(),
		static_cast<int>(path.size()),
		&WidePath[0],
		Required
	);

	for (wchar_t& Character : WidePath)
	{
		if (Character == L'/')
			Character = L'\\';
	}

	const bool IsAbsolute =
		(WidePath.size() >= 2 && WidePath[1] == L':') ||
		(!WidePath.empty() &&
			(WidePath[0] == L'\\' || WidePath[0] == L'/'));

	if (IsAbsolute)
		return WidePath;

	if (DataRoot.empty())
		return WidePath;
	
	const std::wstring DataPath =
		DataRoot + L"\\" + WidePath;

	if (RmlFileExistsW(DataPath))
		return DataPath;
	
	const std::wstring StudioPath =
		DataRoot + L"\\Rml\\Assets\\" + WidePath;

	if (RmlFileExistsW(StudioPath))
		return StudioPath;

	wchar_t DebugText[2048]{};

	_snwprintf_s(
		DebugText,
		_countof(DebugText),
		_TRUNCATE,
		L"[RmlUI][DX9] Texture path not found. Source='%ls' Data='%ls' Studio='%ls'\n",
		WidePath.c_str(),
		DataPath.c_str(),
		StudioPath.c_str()
	);

	OutputDebugStringW(DebugText);
	
	return DataPath;
}

bool RmlRenderDX9::CreateTextureFromRGBA(const unsigned char* PixelsRGBA, int Width, int Height, IDirect3DTexture9** OutTexture)
{
	if (!Device || !PixelsRGBA || Width <= 0 || Height <= 0 || !OutTexture)
		return false;

	*OutTexture = nullptr;

	IDirect3DTexture9* Texture = nullptr;

	HRESULT Hr = Device->CreateTexture(
		static_cast<UINT>(Width),
		static_cast<UINT>(Height),
		1,
		0,
		D3DFMT_A8R8G8B8,
		D3DPOOL_MANAGED,
		&Texture,
		nullptr
	);

	if (FAILED(Hr) || !Texture)
		return false;

	D3DLOCKED_RECT Locked{};
	Hr = Texture->LockRect(0, &Locked, nullptr, 0);

	if (FAILED(Hr))
	{
		Texture->Release();
		return false;
	}

	for (int Y = 0; Y < Height; ++Y)
	{
		DWORD* Dst = reinterpret_cast<DWORD*>(reinterpret_cast<unsigned char*>(Locked.pBits) + Locked.Pitch * Y);

		for (int X = 0; X < Width; ++X)
		{
			const int SrcIndex = (Y * Width + X) * 4;

			const unsigned char R = PixelsRGBA[SrcIndex + 0];
			const unsigned char G = PixelsRGBA[SrcIndex + 1];
			const unsigned char B = PixelsRGBA[SrcIndex + 2];
			const unsigned char A = PixelsRGBA[SrcIndex + 3];

			Dst[X] = D3DCOLOR_ARGB(A, R, G, B);
		}
	}

	Texture->UnlockRect(0);

	*OutTexture = Texture;
	return true;
}

static bool PremultiplyTextureAlpha(
	IDirect3DTexture9* Texture
)
{
	if (!Texture)
		return false;

	D3DSURFACE_DESC Description{};

	HRESULT Result = Texture->GetLevelDesc(
		0,
		&Description
	);

	if (FAILED(Result))
		return false;

	if (Description.Format != D3DFMT_A8R8G8B8)
		return false;

	D3DLOCKED_RECT LockedRectangle{};

	Result = Texture->LockRect(
		0,
		&LockedRectangle,
		nullptr,
		0
	);

	if (FAILED(Result) || !LockedRectangle.pBits)
		return false;

	for (UINT Y = 0; Y < Description.Height; ++Y)
	{
		DWORD* Pixels = reinterpret_cast<DWORD*>(
			reinterpret_cast<unsigned char*>(
				LockedRectangle.pBits
			) +
			LockedRectangle.Pitch * Y
		);

		for (UINT X = 0; X < Description.Width; ++X)
		{
			const DWORD Pixel = Pixels[X];

			const unsigned int Alpha =
				(Pixel >> 24) & 0xFF;

			unsigned int Red =
				(Pixel >> 16) & 0xFF;

			unsigned int Green =
				(Pixel >> 8) & 0xFF;

			unsigned int Blue =
				Pixel & 0xFF;

			Red = (Red * Alpha + 127) / 255;
			Green = (Green * Alpha + 127) / 255;
			Blue = (Blue * Alpha + 127) / 255;

			Pixels[X] = D3DCOLOR_ARGB(
				Alpha,
				Red,
				Green,
				Blue
			);
		}
	}

	Texture->UnlockRect(0);
	return true;
}

bool RmlRenderDX9::LoadTextureD3DX(
	const std::wstring& Filename,
	Rml::Vector2i& OutDimensions,
	IDirect3DTexture9** OutTexture
)
{
	if (!Device || !OutTexture || Filename.empty())
		return false;

	*OutTexture = nullptr;
	OutDimensions = Rml::Vector2i(0, 0);

	D3DXIMAGE_INFO Information{};
	IDirect3DTexture9* Texture = nullptr;

	const HRESULT Result =
		D3DXCreateTextureFromFileExW(
			Device,
			Filename.c_str(),
			D3DX_DEFAULT,
			D3DX_DEFAULT,
			1,
			0,
			D3DFMT_A8R8G8B8,
			D3DPOOL_MANAGED,
			D3DX_FILTER_LINEAR,
			D3DX_FILTER_LINEAR,
			0,
			&Information,
			nullptr,
			&Texture
		);

	if (FAILED(Result) || !Texture)
	{
		wchar_t Text[2048]{};

		_snwprintf_s(
			Text,
			_countof(Text),
			_TRUNCATE,
			L"[RmlUI][DX9] Texture load failed: 0x%08X | %ls\n",
			static_cast<unsigned int>(Result),
			Filename.c_str()
		);

		OutputDebugStringW(Text);

		if (Texture)
			Texture->Release();

		return false;
	}

	if (!PremultiplyTextureAlpha(Texture))
	{
		wchar_t Text[2048]{};

		_snwprintf_s(
			Text,
			_countof(Text),
			_TRUNCATE,
			L"[RmlUI][DX9] Texture premultiply failed: %ls\n",
			Filename.c_str()
		);

		OutputDebugStringW(Text);

		Texture->Release();
		return false;
	}

	OutDimensions.x =
		static_cast<int>(Information.Width);

	OutDimensions.y =
		static_cast<int>(Information.Height);

	*OutTexture = Texture;

	wchar_t Text[2048]{};

	_snwprintf_s(
		Text,
		_countof(Text),
		_TRUNCATE,
		L"[RmlUI][DX9] Texture loaded: %ls (%dx%d)\n",
		Filename.c_str(),
		OutDimensions.x,
		OutDimensions.y
	);

	OutputDebugStringW(Text);

	return true;
}

Rml::TextureHandle RmlRenderDX9::LoadTexture(
	Rml::Vector2i& texture_dimensions,
	const Rml::String& source
)
{
	texture_dimensions.x = 0;
	texture_dimensions.y = 0;

	if (!Device)
	{
		OutputDebugStringA(
			"[RmlUI][DX9] LoadTexture failed: D3D9 device is null\n"
		);

		return 0;
	}

	if (source.empty())
	{
		OutputDebugStringA(
			"[RmlUI][DX9] LoadTexture failed: source is empty\n"
		);

		return 0;
	}

	const std::wstring FullPath = ResolvePathW(source);

	if (FullPath.empty())
	{
		std::string DebugText =
			"[RmlUI][DX9] LoadTexture failed to resolve: ";

		DebugText += source;
		DebugText += "\n";

		OutputDebugStringA(DebugText.c_str());
		return 0;
	}

	IDirect3DTexture9* Texture = nullptr;

	if (!LoadTextureD3DX(
			FullPath,
			texture_dimensions,
			&Texture
		))
	{
		std::string DebugText =
			"[RmlUI][DX9] Failed to load texture source: ";

		DebugText += source;
		DebugText += "\n";

		OutputDebugStringA(DebugText.c_str());
		return 0;
	}

	return reinterpret_cast<Rml::TextureHandle>(Texture);
}

Rml::TextureHandle RmlRenderDX9::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)
{
	if (source.empty() || source_dimensions.x <= 0 || source_dimensions.y <= 0)
		return 0;

	IDirect3DTexture9* Texture = nullptr;

	if (!CreateTextureFromRGBA(
		reinterpret_cast<const unsigned char*>(source.data()),
		source_dimensions.x,
		source_dimensions.y,
		&Texture
	))
	{
		OutputDebugStringA("[RmlUI][DX9] GenerateTexture failed\n");
		return 0;
	}

	return reinterpret_cast<Rml::TextureHandle>(Texture);
}

void RmlRenderDX9::ReleaseTexture(Rml::TextureHandle texture)
{
	if (!texture)
		return;

	IDirect3DTexture9* Texture = reinterpret_cast<IDirect3DTexture9*>(texture);
	Texture->Release();
}

void RmlRenderDX9::EnableScissorRegion(bool enable)
{
	bScissorEnabled = enable;

	if (Device)
		(Device->SetRenderState)(RML_D3DRS_SCISSORTESTENABLE, enable ? TRUE : FALSE);
}

void RmlRenderDX9::SetScissorRegion(Rml::Rectanglei region)
{
	ScissorRect.left = std::max(0, region.Left());
	ScissorRect.top = std::max(0, region.Top());
	ScissorRect.right = std::min(ViewWidth, region.Right());
	ScissorRect.bottom = std::min(ViewHeight, region.Bottom());

	if (ScissorRect.right < ScissorRect.left)
		ScissorRect.right = ScissorRect.left;

	if (ScissorRect.bottom < ScissorRect.top)
		ScissorRect.bottom = ScissorRect.top;

	if (Device)
		Device->SetScissorRect(&ScissorRect);
}

void RmlRenderDX9::SetTransform(const Rml::Matrix4f* transform)
{
	// Для первого Studio AppSelect CSS transform не нужен.
	// Метод оставлен, чтобы RmlUI не падал при элементах без advanced transform.
	// Если позже понадобятся анимации transform/rotate/scale — сюда добавляется конвертация Matrix4f -> D3DMATRIX.
	(void)transform;
}