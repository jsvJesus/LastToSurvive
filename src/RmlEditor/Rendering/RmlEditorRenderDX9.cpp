#include "RmlEditorRenderDX9.h"

#include "../App/RmlEditorLog.h"
#include "RmlEditorViewport.h"

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <cmath>
#include <new>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

RmlEditorRenderDX9::RmlEditorRenderDX9() = default;

RmlEditorRenderDX9::~RmlEditorRenderDX9()
{
	Shutdown();
}

bool RmlEditorRenderDX9::Initialize(
	IDirect3DDevice9* InDevice,
	const wchar_t* DataRoot
)
{
	if (!InDevice)
	{
		RmlEditorLog::Write(
			"[RmlEditor][DX9] Initialize failed: device is null"
		);

		return false;
	}

	Device = InDevice;
	Device->AddRef();

	RootDirectory = DataRoot ? DataRoot : L"";

	while (!RootDirectory.empty())
	{
		const wchar_t LastCharacter = RootDirectory.back();

		if (LastCharacter != L'\\' && LastCharacter != L'/')
			break;

		RootDirectory.pop_back();
	}

	RmlEditorLog::Write(
		"[RmlEditor][DX9] Render interface initialized"
	);

	return true;
}

void RmlEditorRenderDX9::Shutdown()
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

	RootDirectory.clear();
	DocumentDirectory.clear();
}

void RmlEditorRenderDX9::SetDocumentDirectory(
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

void RmlEditorRenderDX9::BeginFrame(int Width, int Height)
{
	if (!Device)
		return;

	ViewWidth = std::max(1, Width);
	ViewHeight = std::max(1, Height);
	FullViewWidth = ViewWidth;
	FullViewHeight = ViewHeight;
	ViewportRendering = false;

	if (StateBlock)
	{
		StateBlock->Release();
		StateBlock = nullptr;
	}

	if (SUCCEEDED(Device->CreateStateBlock(D3DSBT_ALL, &StateBlock)))
	{
		if (StateBlock)
			StateBlock->Capture();
	}

	SetupRenderState();
}

void RmlEditorRenderDX9::BeginViewportFrame(
	const RmlEditorViewport& Viewport
)
{
	if (!Device || !Viewport.IsValid())
		return;

	const RmlEditorViewport::Rectangle& Rectangle =
		Viewport.GetPhysicalRectangle();

	ViewportRendering = true;
	ViewportX = Rectangle.X;
	ViewportY = Rectangle.Y;
	ViewportWidth = std::max(1, Rectangle.Width);
	ViewportHeight = std::max(1, Rectangle.Height);
	ViewportScaleX = Viewport.GetScaleX();
	ViewportScaleY = Viewport.GetScaleY();

	SetupRenderStateForViewport(
		ViewportX,
		ViewportY,
		ViewportWidth,
		ViewportHeight,
		Viewport.GetLogicalWidth(),
		Viewport.GetLogicalHeight()
	);
}

void RmlEditorRenderDX9::EndViewportFrame()
{
	if (!Device)
		return;

	ViewportRendering = false;
	ViewportX = 0;
	ViewportY = 0;
	ViewportWidth = std::max(1, FullViewWidth);
	ViewportHeight = std::max(1, FullViewHeight);
	ViewportScaleX = 1.0f;
	ViewportScaleY = 1.0f;

	ViewWidth = std::max(1, FullViewWidth);
	ViewHeight = std::max(1, FullViewHeight);

	SetupRenderState();
}

void RmlEditorRenderDX9::EndFrame()
{
	if (!StateBlock)
		return;

	StateBlock->Apply();
	StateBlock->Release();
	StateBlock = nullptr;
}

void RmlEditorRenderDX9::OnDeviceLost()
{
	if (StateBlock)
	{
		StateBlock->Release();
		StateBlock = nullptr;
	}

	RmlEditorLog::Write(
		"[RmlEditor][DX9] Device lost"
	);
}

void RmlEditorRenderDX9::OnDeviceReset(
	int Width,
	int Height
)
{
	ViewWidth = std::max(1, Width);
	ViewHeight = std::max(1, Height);

	RmlEditorLog::Write(
		"[RmlEditor][DX9] Device reset: %dx%d",
		ViewWidth,
		ViewHeight
	);
}

DWORD RmlEditorRenderDX9::ConvertColor(
	const Rml::ColourbPremultiplied& Color
)
{
	return D3DCOLOR_ARGB(
		Color.alpha,
		Color.red,
		Color.green,
		Color.blue
	);
}

D3DMATRIX RmlEditorRenderDX9::IdentityMatrix()
{
	D3DMATRIX Result{};

	Result._11 = 1.0f;
	Result._22 = 1.0f;
	Result._33 = 1.0f;
	Result._44 = 1.0f;

	return Result;
}

D3DMATRIX RmlEditorRenderDX9::OrthographicMatrix(
	float Left,
	float Right,
	float Bottom,
	float Top,
	float NearPlane,
	float FarPlane
)
{
	D3DMATRIX Result{};

	Result._11 = 2.0f / (Right - Left);
	Result._22 = 2.0f / (Top - Bottom);
	Result._33 = 1.0f / (FarPlane - NearPlane);
	Result._44 = 1.0f;

	Result._41 = (Left + Right) / (Left - Right);
	Result._42 = (Top + Bottom) / (Bottom - Top);
	Result._43 = NearPlane / (NearPlane - FarPlane);

	return Result;
}

D3DMATRIX RmlEditorRenderDX9::TranslationMatrix(
	float X,
	float Y,
	float Z
)
{
	D3DMATRIX Result = IdentityMatrix();

	Result._41 = X;
	Result._42 = Y;
	Result._43 = Z;

	return Result;
}

void RmlEditorRenderDX9::SetupRenderState()
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

	const D3DMATRIX World = IdentityMatrix();
	const D3DMATRIX View = IdentityMatrix();

	const D3DMATRIX Projection = OrthographicMatrix(
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

	Device->SetFVF(VertexFormat);

	Device->SetRenderState(D3DRS_LIGHTING, FALSE);
	Device->SetRenderState(D3DRS_ZENABLE, FALSE);
	Device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	Device->SetRenderState(D3DRS_FOGENABLE, FALSE);

	Device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

	Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	Device->SetRenderState(
		D3DRS_DESTBLEND,
		D3DBLEND_INVSRCALPHA
	);

	Device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);

	Device->SetRenderState(
		D3DRS_SCISSORTESTENABLE,
		ScissorEnabled ? TRUE : FALSE
	);

	if (ScissorEnabled)
		Device->SetScissorRect(&ScissorRectangle);

	Device->SetTextureStageState(
		0,
		D3DTSS_COLOROP,
		D3DTOP_MODULATE
	);

	Device->SetTextureStageState(
		0,
		D3DTSS_COLORARG1,
		D3DTA_TEXTURE
	);

	Device->SetTextureStageState(
		0,
		D3DTSS_COLORARG2,
		D3DTA_DIFFUSE
	);

	Device->SetTextureStageState(
		0,
		D3DTSS_ALPHAOP,
		D3DTOP_MODULATE
	);

	Device->SetTextureStageState(
		0,
		D3DTSS_ALPHAARG1,
		D3DTA_TEXTURE
	);

	Device->SetTextureStageState(
		0,
		D3DTSS_ALPHAARG2,
		D3DTA_DIFFUSE
	);

	Device->SetTextureStageState(
		1,
		D3DTSS_COLOROP,
		D3DTOP_DISABLE
	);

	Device->SetTextureStageState(
		1,
		D3DTSS_ALPHAOP,
		D3DTOP_DISABLE
	);

	Device->SetSamplerState(
		0,
		D3DSAMP_MINFILTER,
		D3DTEXF_LINEAR
	);

	Device->SetSamplerState(
		0,
		D3DSAMP_MAGFILTER,
		D3DTEXF_LINEAR
	);

	Device->SetSamplerState(
		0,
		D3DSAMP_MIPFILTER,
		D3DTEXF_NONE
	);

	Device->SetSamplerState(
		0,
		D3DSAMP_ADDRESSU,
		D3DTADDRESS_CLAMP
	);

	Device->SetSamplerState(
		0,
		D3DSAMP_ADDRESSV,
		D3DTADDRESS_CLAMP
	);
}

void RmlEditorRenderDX9::SetupRenderStateForViewport(
	int PhysicalX,
	int PhysicalY,
	int PhysicalWidth,
	int PhysicalHeight,
	int LogicalWidth,
	int LogicalHeight
)
{
	if (!Device)
		return;

	ViewWidth = std::max(1, LogicalWidth);
	ViewHeight = std::max(1, LogicalHeight);

	D3DVIEWPORT9 Viewport{};

	Viewport.X = static_cast<DWORD>(std::max(0, PhysicalX));
	Viewport.Y = static_cast<DWORD>(std::max(0, PhysicalY));
	Viewport.Width = static_cast<DWORD>(std::max(1, PhysicalWidth));
	Viewport.Height = static_cast<DWORD>(std::max(1, PhysicalHeight));
	Viewport.MinZ = 0.0f;
	Viewport.MaxZ = 1.0f;

	Device->SetViewport(&Viewport);

	const D3DMATRIX World = IdentityMatrix();
	const D3DMATRIX View = IdentityMatrix();

	const D3DMATRIX Projection = OrthographicMatrix(
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

	Device->SetFVF(VertexFormat);

	Device->SetRenderState(D3DRS_LIGHTING, FALSE);
	Device->SetRenderState(D3DRS_ZENABLE, FALSE);
	Device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	Device->SetRenderState(D3DRS_FOGENABLE, FALSE);
	Device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	Device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	Device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

	RECT PhysicalScissor{};
	PhysicalScissor.left = ViewportX;
	PhysicalScissor.top = ViewportY;
	PhysicalScissor.right = ViewportX + ViewportWidth;
	PhysicalScissor.bottom = ViewportY + ViewportHeight;
	Device->SetScissorRect(&PhysicalScissor);

	Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	Device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	Device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	Device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	Device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	Device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	Device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	Device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
}

Rml::CompiledGeometryHandle
RmlEditorRenderDX9::CompileGeometry(
	Rml::Span<const Rml::Vertex> Vertices,
	Rml::Span<const int> Indices
)
{
	if (!Device || Vertices.empty() || Indices.empty())
		return 0;

	CompiledGeometry* Geometry =
		new (std::nothrow) CompiledGeometry();

	if (!Geometry)
		return 0;

	Geometry->VertexCount =
		static_cast<int>(Vertices.size());

	Geometry->IndexCount =
		static_cast<int>(Indices.size());

	const UINT VertexBufferSize =
		static_cast<UINT>(
			sizeof(DX9Vertex) *
			Geometry->VertexCount
		);

	const UINT IndexBufferSize =
		static_cast<UINT>(
			sizeof(uint32_t) *
			Geometry->IndexCount
		);

	HRESULT Result = Device->CreateVertexBuffer(
		VertexBufferSize,
		0,
		VertexFormat,
		D3DPOOL_MANAGED,
		&Geometry->VertexBuffer,
		nullptr
	);

	if (FAILED(Result))
	{
		delete Geometry;

		RmlEditorLog::Write(
			"[RmlEditor][DX9] CreateVertexBuffer failed: 0x%08X",
			static_cast<unsigned int>(Result)
		);

		return 0;
	}

	Result = Device->CreateIndexBuffer(
		IndexBufferSize,
		0,
		D3DFMT_INDEX32,
		D3DPOOL_MANAGED,
		&Geometry->IndexBuffer,
		nullptr
	);

	if (FAILED(Result))
	{
		Geometry->VertexBuffer->Release();
		delete Geometry;

		RmlEditorLog::Write(
			"[RmlEditor][DX9] CreateIndexBuffer failed: 0x%08X",
			static_cast<unsigned int>(Result)
		);

		return 0;
	}

	void* VertexMemory = nullptr;

	Result = Geometry->VertexBuffer->Lock(
		0,
		VertexBufferSize,
		&VertexMemory,
		0
	);

	if (FAILED(Result) || !VertexMemory)
	{
		ReleaseGeometry(
			reinterpret_cast<Rml::CompiledGeometryHandle>(
				Geometry
			)
		);

		return 0;
	}

	DX9Vertex* DestinationVertices =
		static_cast<DX9Vertex*>(VertexMemory);

	for (int Index = 0;
		 Index < Geometry->VertexCount;
		 ++Index)
	{
		const Rml::Vertex& SourceVertex = Vertices[Index];

		DestinationVertices[Index].X =
			SourceVertex.position.x;

		DestinationVertices[Index].Y =
			SourceVertex.position.y;

		DestinationVertices[Index].Z = 0.0f;

		DestinationVertices[Index].Color =
			ConvertColor(SourceVertex.colour);

		DestinationVertices[Index].U =
			SourceVertex.tex_coord.x;

		DestinationVertices[Index].V =
			SourceVertex.tex_coord.y;
	}

	Geometry->VertexBuffer->Unlock();

	void* IndexMemory = nullptr;

	Result = Geometry->IndexBuffer->Lock(
		0,
		IndexBufferSize,
		&IndexMemory,
		0
	);

	if (FAILED(Result) || !IndexMemory)
	{
		ReleaseGeometry(
			reinterpret_cast<Rml::CompiledGeometryHandle>(
				Geometry
			)
		);

		return 0;
	}

	uint32_t* DestinationIndices =
		static_cast<uint32_t*>(IndexMemory);

	for (int Index = 0;
		 Index < Geometry->IndexCount;
		 ++Index)
	{
		DestinationIndices[Index] =
			static_cast<uint32_t>(Indices[Index]);
	}

	Geometry->IndexBuffer->Unlock();

	return reinterpret_cast<Rml::CompiledGeometryHandle>(
		Geometry
	);
}

void RmlEditorRenderDX9::RenderGeometry(
	Rml::CompiledGeometryHandle GeometryHandle,
	Rml::Vector2f Translation,
	Rml::TextureHandle TextureHandle
)
{
	if (!Device || !GeometryHandle)
		return;

	CompiledGeometry* Geometry =
		reinterpret_cast<CompiledGeometry*>(
			GeometryHandle
		);

	IDirect3DTexture9* Texture =
		reinterpret_cast<IDirect3DTexture9*>(
			TextureHandle
		);

	// Direct3D 9 считает центр верхнего левого пикселя координатой 0,0.
	// RmlUi генерирует геометрию по современному правилу границ пикселей,
	// поэтому для точного попадания texel -> pixel нужен offset -0.5.
	constexpr float HalfPixelOffset = -0.5f;

	const D3DMATRIX World = TranslationMatrix(
		Translation.x + HalfPixelOffset,
		Translation.y + HalfPixelOffset,
		0.0f
	);

	Device->SetTransform(D3DTS_WORLD, &World);

	Device->SetStreamSource(
		0,
		Geometry->VertexBuffer,
		0,
		sizeof(DX9Vertex)
	);

	Device->SetIndices(Geometry->IndexBuffer);
	Device->SetFVF(VertexFormat);

	if (Texture)
	{
		Device->SetTexture(0, Texture);

		Device->SetTextureStageState(
			0,
			D3DTSS_COLOROP,
			D3DTOP_MODULATE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_COLORARG1,
			D3DTA_TEXTURE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_COLORARG2,
			D3DTA_DIFFUSE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_ALPHAOP,
			D3DTOP_MODULATE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_ALPHAARG1,
			D3DTA_TEXTURE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_ALPHAARG2,
			D3DTA_DIFFUSE
		);
	}
	else
	{
		Device->SetTexture(0, nullptr);

		Device->SetTextureStageState(
			0,
			D3DTSS_COLOROP,
			D3DTOP_SELECTARG1
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_COLORARG1,
			D3DTA_DIFFUSE
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_ALPHAOP,
			D3DTOP_SELECTARG1
		);

		Device->SetTextureStageState(
			0,
			D3DTSS_ALPHAARG1,
			D3DTA_DIFFUSE
		);
	}

	const UINT PrimitiveCount =
		static_cast<UINT>(
			Geometry->IndexCount / 3
		);

	Device->DrawIndexedPrimitive(
		D3DPT_TRIANGLELIST,
		0,
		0,
		static_cast<UINT>(Geometry->VertexCount),
		0,
		PrimitiveCount
	);
}

void RmlEditorRenderDX9::ReleaseGeometry(
	Rml::CompiledGeometryHandle GeometryHandle
)
{
	if (!GeometryHandle)
		return;

	CompiledGeometry* Geometry =
		reinterpret_cast<CompiledGeometry*>(
			GeometryHandle
		);

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

std::wstring RmlEditorRenderDX9::ResolvePath(
	const Rml::String& Path
) const
{
	if (Path.empty())
		return std::wstring();

	const int RequiredLength = MultiByteToWideChar(
		CP_UTF8,
		0,
		Path.c_str(),
		static_cast<int>(Path.size()),
		nullptr,
		0
	);

	if (RequiredLength <= 0)
		return std::wstring();

	std::wstring WidePath;
	WidePath.resize(static_cast<size_t>(RequiredLength));

	MultiByteToWideChar(
		CP_UTF8,
		0,
		Path.c_str(),
		static_cast<int>(Path.size()),
		WidePath.data(),
		RequiredLength
	);

	for (wchar_t& Character : WidePath)
	{
		if (Character == L'/')
			Character = L'\\';
	}

	const bool Absolute =
		(WidePath.size() >= 2 && WidePath[1] == L':') ||
		(!WidePath.empty() && WidePath[0] == L'\\');

	if (Absolute || RootDirectory.empty())
		return WidePath;

	if (!DocumentDirectory.empty())
	{
		const std::wstring DocumentPath =
			DocumentDirectory + L"\\" + WidePath;

		if (FileExists(DocumentPath))
			return DocumentPath;
	}

	const std::wstring DataPath =
		RootDirectory + L"\\" + WidePath;

	if (FileExists(DataPath))
		return DataPath;

	return DataPath;
}

bool RmlEditorRenderDX9::FileExists(
	const std::wstring& Path
)
{
	return std::filesystem::exists(std::filesystem::path(Path));
}

bool RmlEditorRenderDX9::CreateTextureFromRGBA(
	const unsigned char* Source,
	int Width,
	int Height,
	IDirect3DTexture9** OutputTexture
)
{
	if (!Device ||
		!Source ||
		!OutputTexture ||
		Width <= 0 ||
		Height <= 0)
	{
		return false;
	}

	*OutputTexture = nullptr;

	IDirect3DTexture9* Texture = nullptr;

	const HRESULT CreateResult = Device->CreateTexture(
		static_cast<UINT>(Width),
		static_cast<UINT>(Height),
		1,
		0,
		D3DFMT_A8R8G8B8,
		D3DPOOL_MANAGED,
		&Texture,
		nullptr
	);

	if (FAILED(CreateResult) || !Texture)
		return false;

	D3DLOCKED_RECT LockedRectangle{};

	const HRESULT LockResult = Texture->LockRect(
		0,
		&LockedRectangle,
		nullptr,
		0
	);

	if (FAILED(LockResult))
	{
		Texture->Release();
		return false;
	}

	for (int Y = 0; Y < Height; ++Y)
	{
		DWORD* Destination =
			reinterpret_cast<DWORD*>(
				reinterpret_cast<unsigned char*>(
					LockedRectangle.pBits
				) +
				LockedRectangle.Pitch * Y
			);

		for (int X = 0; X < Width; ++X)
		{
			const int SourceIndex =
				(Y * Width + X) * 4;

			const unsigned char Red =
				Source[SourceIndex + 0];

			const unsigned char Green =
				Source[SourceIndex + 1];

			const unsigned char Blue =
				Source[SourceIndex + 2];

			const unsigned char Alpha =
				Source[SourceIndex + 3];

			Destination[X] = D3DCOLOR_ARGB(
				Alpha,
				Red,
				Green,
				Blue
			);
		}
	}

	Texture->UnlockRect(0);

	*OutputTexture = Texture;
	return true;
}

Rml::TextureHandle RmlEditorRenderDX9::LoadTexture(
	Rml::Vector2i& TextureDimensions,
	const Rml::String& Source
)
{
	if (!Device)
		return 0;

	const std::wstring FullPath = ResolvePath(Source);

	D3DXIMAGE_INFO ImageInformation{};
	IDirect3DTexture9* Texture = nullptr;

	const HRESULT Result = D3DXCreateTextureFromFileExW(
		Device,
		FullPath.c_str(),
		D3DX_DEFAULT,
		D3DX_DEFAULT,
		1,
		0,
		D3DFMT_A8R8G8B8,
		D3DPOOL_MANAGED,
		D3DX_FILTER_LINEAR,
		D3DX_FILTER_LINEAR,
		0,
		&ImageInformation,
		nullptr,
		&Texture
	);

	if (FAILED(Result) || !Texture)
	{
		RmlEditorLog::Write(
			"[RmlEditor][DX9] Failed to load texture: %s",
			Source.c_str()
		);

		return 0;
	}

	TextureDimensions.x =
		static_cast<int>(ImageInformation.Width);

	TextureDimensions.y =
		static_cast<int>(ImageInformation.Height);

	return reinterpret_cast<Rml::TextureHandle>(Texture);
}

Rml::TextureHandle RmlEditorRenderDX9::GenerateTexture(
	Rml::Span<const Rml::byte> Source,
	Rml::Vector2i SourceDimensions
)
{
	if (Source.empty() ||
		SourceDimensions.x <= 0 ||
		SourceDimensions.y <= 0)
	{
		return 0;
	}

	IDirect3DTexture9* Texture = nullptr;

	if (!CreateTextureFromRGBA(
			reinterpret_cast<const unsigned char*>(
				Source.data()
			),
			SourceDimensions.x,
			SourceDimensions.y,
			&Texture
		))
	{
		RmlEditorLog::Write(
			"[RmlEditor][DX9] GenerateTexture failed"
		);

		return 0;
	}

	return reinterpret_cast<Rml::TextureHandle>(Texture);
}

void RmlEditorRenderDX9::ReleaseTexture(
	Rml::TextureHandle TextureHandle
)
{
	if (!TextureHandle)
		return;

	IDirect3DTexture9* Texture =
		reinterpret_cast<IDirect3DTexture9*>(
			TextureHandle
		);

	Texture->Release();
}

void RmlEditorRenderDX9::EnableScissorRegion(bool Enable)
{
	ScissorEnabled = Enable;

	if (Device)
	{
		if (ViewportRendering)
		{
			Device->SetRenderState(
				D3DRS_SCISSORTESTENABLE,
				TRUE
			);

			if (!Enable)
			{
				RECT PhysicalScissor{};
				PhysicalScissor.left = ViewportX;
				PhysicalScissor.top = ViewportY;
				PhysicalScissor.right = ViewportX + ViewportWidth;
				PhysicalScissor.bottom = ViewportY + ViewportHeight;
				Device->SetScissorRect(&PhysicalScissor);
			}

			return;
		}

		Device->SetRenderState(
			D3DRS_SCISSORTESTENABLE,
			Enable ? TRUE : FALSE
		);
	}
}

void RmlEditorRenderDX9::SetScissorRegion(
	Rml::Rectanglei Region
)
{
	ScissorRectangle.left =
		std::max(0, Region.Left());

	ScissorRectangle.top =
		std::max(0, Region.Top());

	ScissorRectangle.right =
		std::min(ViewWidth, Region.Right());

	ScissorRectangle.bottom =
		std::min(ViewHeight, Region.Bottom());

	if (ScissorRectangle.right < ScissorRectangle.left)
	{
		ScissorRectangle.right =
			ScissorRectangle.left;
	}

	if (ScissorRectangle.bottom < ScissorRectangle.top)
	{
		ScissorRectangle.bottom =
			ScissorRectangle.top;
	}

	if (!Device)
		return;

	if (!ViewportRendering)
	{
		Device->SetScissorRect(&ScissorRectangle);
		return;
	}

	RECT PhysicalScissor{};

	PhysicalScissor.left =
		ViewportX +
		static_cast<LONG>(
			std::floor(
				static_cast<float>(ScissorRectangle.left) *
				ViewportScaleX
			)
		);

	PhysicalScissor.top =
		ViewportY +
		static_cast<LONG>(
			std::floor(
				static_cast<float>(ScissorRectangle.top) *
				ViewportScaleY
			)
		);

	PhysicalScissor.right =
		ViewportX +
		static_cast<LONG>(
			std::ceil(
				static_cast<float>(ScissorRectangle.right) *
				ViewportScaleX
			)
		);

	PhysicalScissor.bottom =
		ViewportY +
		static_cast<LONG>(
			std::ceil(
				static_cast<float>(ScissorRectangle.bottom) *
				ViewportScaleY
			)
		);

	PhysicalScissor.left =
		std::max<LONG>(ViewportX, PhysicalScissor.left);

	PhysicalScissor.top =
		std::max<LONG>(ViewportY, PhysicalScissor.top);

	PhysicalScissor.right =
		std::min<LONG>(
			ViewportX + ViewportWidth,
			PhysicalScissor.right
		);

	PhysicalScissor.bottom =
		std::min<LONG>(
			ViewportY + ViewportHeight,
			PhysicalScissor.bottom
		);

	if (PhysicalScissor.right < PhysicalScissor.left)
		PhysicalScissor.right = PhysicalScissor.left;

	if (PhysicalScissor.bottom < PhysicalScissor.top)
		PhysicalScissor.bottom = PhysicalScissor.top;

	Device->SetScissorRect(&PhysicalScissor);
}

void RmlEditorRenderDX9::SetTransform(
	const Rml::Matrix4f* Transform
)
{
	// Поддержка CSS transform будет добавлена отдельно.
	// Для текущей оболочки редактора transform не используется.
	(void)Transform;
}
