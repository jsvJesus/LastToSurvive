#include "r3dPCH.h"
#include "r3d.h"

#include "RmlRenderDX11.h"

#include <RmlUi/Core/Core.h>

#include "RENDERING/DX11/RenderDX11Platform.h"
#include <wincodec.h>

#include <algorithm>
#include <cstring>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace
{
	const char* RmlDx11ShaderSource =
		"cbuffer Constants : register(b0)\n"
		"{\n"
		"	row_major float4x4 Transform;\n"
		"	float4 Translate;\n"
		"	float4 TextureEnabled;\n"
		"};\n"
		"Texture2D SourceTexture : register(t0);\n"
		"SamplerState SourceSampler : register(s0);\n"
		"struct VSInput\n"
		"{\n"
		"	float3 Position : POSITION;\n"
		"	float4 Color : COLOR0;\n"
		"	float2 TexCoord : TEXCOORD0;\n"
		"};\n"
		"struct VSOutput\n"
		"{\n"
		"	float4 Position : SV_POSITION;\n"
		"	float4 Color : COLOR0;\n"
		"	float2 TexCoord : TEXCOORD0;\n"
		"};\n"
		"VSOutput VSMain(VSInput input)\n"
		"{\n"
		"	VSOutput output;\n"
		"	float4 position = float4(input.Position.xy + Translate.xy, input.Position.z, 1.0f);\n"
		"	output.Position = mul(position, Transform);\n"
		"	output.Color = input.Color;\n"
		"	output.TexCoord = input.TexCoord;\n"
		"	return output;\n"
		"}\n"
		"float4 PSMain(VSOutput input) : SV_TARGET\n"
		"{\n"
		"	float4 color = input.Color;\n"
		"	if (TextureEnabled.x > 0.5f)\n"
		"		color *= SourceTexture.Sample(SourceSampler, input.TexCoord);\n"
		"	return color;\n"
		"}\n";

	template <typename T>
	void SafeReleaseDX11(T*& value)
	{
		if (value)
		{
			value->Release();
			value = nullptr;
		}
	}

	bool FileExistsW(const std::wstring& path)
	{
		const DWORD attributes = GetFileAttributesW(path.c_str());
		return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}

	std::wstring Utf8ToWide(const Rml::String& text)
	{
		if (text.empty())
			return std::wstring();

		const int required = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
		if (required <= 0)
			return std::wstring(text.begin(), text.end());

		std::wstring result;
		result.resize(static_cast<size_t>(required));
		MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &result[0], required);
		return result;
	}

	void PremultiplyRGBA(unsigned char* pixels, size_t byteCount)
	{
		if (!pixels)
			return;

		const size_t pixelCount = byteCount / 4;
		for (size_t i = 0; i < pixelCount; ++i)
		{
			unsigned char* pixel = pixels + i * 4;
			const unsigned int alpha = pixel[3];
			pixel[0] = static_cast<unsigned char>((static_cast<unsigned int>(pixel[0]) * alpha + 127) / 255);
			pixel[1] = static_cast<unsigned char>((static_cast<unsigned int>(pixel[1]) * alpha + 127) / 255);
			pixel[2] = static_cast<unsigned char>((static_cast<unsigned int>(pixel[2]) * alpha + 127) / 255);
		}
	}
}

RmlRenderDX11::RmlRenderDX11()
{
	SetIdentity(CurrentTransform);
}

RmlRenderDX11::~RmlRenderDX11()
{
	Shutdown();
}

bool RmlRenderDX11::Init(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* dataRoot)
{
	if (!device || !context)
		return false;

	Device = device;
	Context = context;

	Device->AddRef();
	Context->AddRef();

	DataRoot = dataRoot ? dataRoot : L"";

	if (!CreateDeviceObjects())
	{
		Shutdown();
		return false;
	}

	bInitialized = true;
	return true;
}

void RmlRenderDX11::Shutdown()
{
	ReleaseStateBackup();
	ReleaseDeviceObjects();

	SafeReleaseDX11(Context);
	SafeReleaseDX11(Device);

	CharacterPreviewTexture = nullptr;
	CharacterPortraitTexture = nullptr;
	ActiveRenderTarget = nullptr;
	ActiveDepthStencil = nullptr;
	DataRoot.clear();

	bInitialized = false;
	bFrameOpen = false;
}

void RmlRenderDX11::BeginFrame(ID3D11RenderTargetView* renderTarget, ID3D11DepthStencilView* depthStencil, int width, int height)
{
	if (!bInitialized || !Context || !renderTarget || bFrameOpen)
		return;

	ActiveRenderTarget = renderTarget;
	ActiveDepthStencil = depthStencil;
	ViewWidth = std::max(1, width);
	ViewHeight = std::max(1, height);

	ViewportTopLeftX = 0.0f;
	ViewportTopLeftY = 0.0f;
	ViewportWidth = static_cast<float>(ViewWidth);
	ViewportHeight = static_cast<float>(ViewHeight);

	ScissorLeft = 0;
	ScissorTop = 0;
	ScissorRight = ViewWidth;
	ScissorBottom = ViewHeight;
	bScissorEnabled = false;

	CaptureState();

	Context->OMSetRenderTargets(1, &ActiveRenderTarget, ActiveDepthStencil);
	SetupPipeline(false, false);

	bFrameOpen = true;
}

void RmlRenderDX11::EndFrame()
{
	if (!Context || !bFrameOpen)
		return;

	ID3D11ShaderResourceView* nullSRV = nullptr;
	Context->PSSetShaderResources(0, 1, &nullSRV);

	RestoreState();
	ReleaseStateBackup();

	bFrameOpen = false;
	ActiveRenderTarget = nullptr;
	ActiveDepthStencil = nullptr;
}

void RmlRenderDX11::SetCharacterPreviewTexture(ID3D11ShaderResourceView* texture, int width, int height)
{
	CharacterPreviewTexture = texture;
	CharacterPreviewWidth = std::max(0, width);
	CharacterPreviewHeight = std::max(0, height);
}

void RmlRenderDX11::SetCharacterPortraitTexture(ID3D11ShaderResourceView* texture, int width, int height)
{
	CharacterPortraitTexture = texture;
	CharacterPortraitWidth = std::max(0, width);
	CharacterPortraitHeight = std::max(0, height);
}

Rml::CompiledGeometryHandle RmlRenderDX11::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	if (!Device || vertices.empty() || indices.empty())
		return 0;

	FCompiledGeometry* geometry = new FCompiledGeometry();
	geometry->NumIndices = static_cast<unsigned int>(indices.size());

	std::vector<FVertex> vertexData;
	vertexData.resize(vertices.size());

	for (size_t i = 0; i < vertices.size(); ++i)
	{
		const Rml::Vertex& src = vertices[i];
		vertexData[i].Position[0] = src.position.x;
		vertexData[i].Position[1] = src.position.y;
		vertexData[i].Position[2] = 0.0f;
		vertexData[i].Color = ConvertColor(src.colour);
		vertexData[i].TexCoord[0] = src.tex_coord.x;
		vertexData[i].TexCoord[1] = src.tex_coord.y;
	}

	std::vector<unsigned int> indexData;
	indexData.resize(indices.size());

	for (size_t i = 0; i < indices.size(); ++i)
		indexData[i] = static_cast<unsigned int>(indices[i]);

	D3D11_BUFFER_DESC vertexDesc{};
	vertexDesc.ByteWidth = static_cast<UINT>(sizeof(FVertex) * vertexData.size());
	vertexDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexInit{};
	vertexInit.pSysMem = vertexData.data();

	HRESULT result = Device->CreateBuffer(&vertexDesc, &vertexInit, &geometry->VertexBuffer);
	if (FAILED(result))
	{
		delete geometry;
		return 0;
	}

	D3D11_BUFFER_DESC indexDesc{};
	indexDesc.ByteWidth = static_cast<UINT>(sizeof(unsigned int) * indexData.size());
	indexDesc.Usage = D3D11_USAGE_DEFAULT;
	indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexInit{};
	indexInit.pSysMem = indexData.data();

	result = Device->CreateBuffer(&indexDesc, &indexInit, &geometry->IndexBuffer);
	if (FAILED(result))
	{
		ReleaseGeometry(reinterpret_cast<Rml::CompiledGeometryHandle>(geometry));
		return 0;
	}

	return reinterpret_cast<Rml::CompiledGeometryHandle>(geometry);
}

void RmlRenderDX11::RenderGeometry(Rml::CompiledGeometryHandle geometryHandle, Rml::Vector2f translation, Rml::TextureHandle textureHandle)
{
	if (!Context || !geometryHandle || !bFrameOpen)
		return;

	FCompiledGeometry* geometry = reinterpret_cast<FCompiledGeometry*>(geometryHandle);
	if (!geometry->VertexBuffer || !geometry->IndexBuffer || geometry->NumIndices == 0)
		return;

	FTextureHandle* texture = reinterpret_cast<FTextureHandle*>(textureHandle);
	ID3D11ShaderResourceView* srv = nullptr;
	bool straightAlphaBlend = false;

	if (texture)
	{
		if (texture->bExternalCharacterPreview)
		{
			srv = CharacterPreviewTexture;
			straightAlphaBlend = true;
		}
		else if (texture->bExternalCharacterPortrait)
		{
			srv = CharacterPortraitTexture;
			straightAlphaBlend = true;
		}
		else
		{
			srv = texture->SRV;
			straightAlphaBlend = texture->bStraightAlpha;
		}
	}

	SetupPipeline(srv != nullptr, straightAlphaBlend);

	FConstants constants{};

	float projection[16]{};
	SetIdentity(projection);
	projection[0] = 2.0f / static_cast<float>(ViewWidth);
	projection[5] = -2.0f / static_cast<float>(ViewHeight);
	projection[12] = -1.0f;
	projection[13] = 1.0f;

	for (int row = 0; row < 4; ++row)
	{
		for (int column = 0; column < 4; ++column)
		{
			float value = 0.0f;

			for (int k = 0; k < 4; ++k)
				value += CurrentTransform[row * 4 + k] * projection[k * 4 + column];

			constants.Transform[row * 4 + column] = value;
		}
	}

	constants.Translate[0] = translation.x;
	constants.Translate[1] = translation.y;
	constants.Translate[2] = 0.0f;
	constants.Translate[3] = 0.0f;
	constants.TextureEnabled[0] = srv ? 1.0f : 0.0f;

	Context->UpdateSubresource(ConstantBuffer, 0, nullptr, &constants, 0, 0);

	const UINT stride = sizeof(FVertex);
	const UINT offset = 0;
	Context->IASetVertexBuffers(0, 1, &geometry->VertexBuffer, &stride, &offset);
	Context->IASetIndexBuffer(geometry->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Context->PSSetShaderResources(0, 1, &srv);
	Context->DrawIndexed(geometry->NumIndices, 0, 0);
}

void RmlRenderDX11::ReleaseGeometry(Rml::CompiledGeometryHandle geometryHandle)
{
	if (!geometryHandle)
		return;

	FCompiledGeometry* geometry = reinterpret_cast<FCompiledGeometry*>(geometryHandle);
	SafeReleaseDX11(geometry->IndexBuffer);
	SafeReleaseDX11(geometry->VertexBuffer);
	delete geometry;
}

Rml::TextureHandle RmlRenderDX11::LoadTexture(Rml::Vector2i& textureDimensions, const Rml::String& source)
{
	textureDimensions = Rml::Vector2i(0, 0);

	if (!Device)
		return 0;

	if (source == "rml://character-preview" || source == "rml:/character-preview")
	{
		FTextureHandle* handle = new FTextureHandle();
		handle->bExternalCharacterPreview = true;
		handle->Width = CharacterPreviewWidth > 0 ? CharacterPreviewWidth : ViewWidth;
		handle->Height = CharacterPreviewHeight > 0 ? CharacterPreviewHeight : ViewHeight;
		textureDimensions = Rml::Vector2i(handle->Width, handle->Height);
		return reinterpret_cast<Rml::TextureHandle>(handle);
	}

	if (source == "rml://character-portrait" || source == "rml:/character-portrait")
	{
		FTextureHandle* handle = new FTextureHandle();
		handle->bExternalCharacterPortrait = true;
		handle->Width = CharacterPortraitWidth > 0 ? CharacterPortraitWidth : 512;
		handle->Height = CharacterPortraitHeight > 0 ? CharacterPortraitHeight : 512;
		textureDimensions = Rml::Vector2i(handle->Width, handle->Height);
		return reinterpret_cast<Rml::TextureHandle>(handle);
	}

	ID3D11ShaderResourceView* srv = nullptr;
	const std::wstring fullPath = ResolvePathW(source);

	if (!LoadTextureFromFile(fullPath, textureDimensions, &srv))
		return 0;

	FTextureHandle* handle = new FTextureHandle();
	handle->SRV = srv;
	handle->Width = textureDimensions.x;
	handle->Height = textureDimensions.y;
	return reinterpret_cast<Rml::TextureHandle>(handle);
}

Rml::TextureHandle RmlRenderDX11::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i sourceDimensions)
{
	if (!Device || source.empty() || sourceDimensions.x <= 0 || sourceDimensions.y <= 0)
		return 0;

	const size_t expectedSize =
		static_cast<size_t>(sourceDimensions.x) *
		static_cast<size_t>(sourceDimensions.y) *
		4;

	if (source.size() < expectedSize)
		return 0;

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = static_cast<UINT>(sourceDimensions.x);
	desc.Height = static_cast<UINT>(sourceDimensions.y);
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA init{};
	std::vector<unsigned char> premultipliedSource;
	premultipliedSource.resize(expectedSize);
	std::memcpy(premultipliedSource.data(), source.data(), premultipliedSource.size());
	PremultiplyRGBA(premultipliedSource.data(), premultipliedSource.size());

	init.pSysMem = premultipliedSource.data();
	init.SysMemPitch = static_cast<UINT>(sourceDimensions.x * 4);

	ID3D11Texture2D* texture = nullptr;
	HRESULT result = Device->CreateTexture2D(&desc, &init, &texture);
	if (FAILED(result) || !texture)
		return 0;

	ID3D11ShaderResourceView* srv = nullptr;
	result = Device->CreateShaderResourceView(texture, nullptr, &srv);
	texture->Release();

	if (FAILED(result) || !srv)
		return 0;

	FTextureHandle* handle = new FTextureHandle();
	handle->SRV = srv;
	handle->Width = sourceDimensions.x;
	handle->Height = sourceDimensions.y;
	return reinterpret_cast<Rml::TextureHandle>(handle);
}

void RmlRenderDX11::ReleaseTexture(Rml::TextureHandle textureHandle)
{
	if (!textureHandle)
		return;

	FTextureHandle* texture = reinterpret_cast<FTextureHandle*>(textureHandle);
	if (!texture->bExternalCharacterPreview && !texture->bExternalCharacterPortrait)
		SafeReleaseDX11(texture->SRV);

	delete texture;
}

void RmlRenderDX11::EnableScissorRegion(bool enable)
{
	bScissorEnabled = enable;

	if (Context)
		Context->RSSetState(enable ? ScissorRasterizerState : RasterizerState);
}

void RmlRenderDX11::SetScissorRegion(Rml::Rectanglei region)
{
	ScissorLeft = std::max(0, region.Left());
	ScissorTop = std::max(0, region.Top());
	ScissorRight = std::min(ViewWidth, region.Right());
	ScissorBottom = std::min(ViewHeight, region.Bottom());

	if (ScissorRight < ScissorLeft)
		ScissorRight = ScissorLeft;

	if (ScissorBottom < ScissorTop)
		ScissorBottom = ScissorTop;

	if (Context)
	{
		const D3D11_RECT rect = { ScissorLeft, ScissorTop, ScissorRight, ScissorBottom };
		Context->RSSetScissorRects(1, &rect);
	}
}

void RmlRenderDX11::SetTransform(const Rml::Matrix4f* transform)
{
	if (transform)
		CopyTransform(CurrentTransform, *transform);
	else
		SetIdentity(CurrentTransform);
}

bool RmlRenderDX11::CreateDeviceObjects()
{
	ID3DBlob* vertexShaderBlob = nullptr;
	ID3DBlob* pixelShaderBlob = nullptr;
	ID3DBlob* errors = nullptr;

	HRESULT result = D3DCompile(
		RmlDx11ShaderSource,
		std::strlen(RmlDx11ShaderSource),
		"RmlRenderDX11",
		nullptr,
		nullptr,
		"VSMain",
		"vs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&vertexShaderBlob,
		&errors
	);

	SafeReleaseDX11(errors);

	if (FAILED(result) || !vertexShaderBlob)
		return false;

	result = D3DCompile(
		RmlDx11ShaderSource,
		std::strlen(RmlDx11ShaderSource),
		"RmlRenderDX11",
		nullptr,
		nullptr,
		"PSMain",
		"ps_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&pixelShaderBlob,
		&errors
	);

	SafeReleaseDX11(errors);

	if (FAILED(result) || !pixelShaderBlob)
	{
		SafeReleaseDX11(vertexShaderBlob);
		return false;
	}

	result = Device->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &VertexShader);
	if (FAILED(result))
	{
		SafeReleaseDX11(vertexShaderBlob);
		SafeReleaseDX11(pixelShaderBlob);
		return false;
	}

	result = Device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &PixelShader);
	if (FAILED(result))
	{
		SafeReleaseDX11(vertexShaderBlob);
		SafeReleaseDX11(pixelShaderBlob);
		return false;
	}

	const D3D11_INPUT_ELEMENT_DESC inputElements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(FVertex, Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, offsetof(FVertex, Color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FVertex, TexCoord), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	result = Device->CreateInputLayout(
		inputElements,
		static_cast<UINT>(_countof(inputElements)),
		vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(),
		&InputLayout
	);

	SafeReleaseDX11(vertexShaderBlob);
	SafeReleaseDX11(pixelShaderBlob);

	if (FAILED(result))
		return false;

	D3D11_BUFFER_DESC constantDesc{};
	constantDesc.ByteWidth = sizeof(FConstants);
	constantDesc.Usage = D3D11_USAGE_DEFAULT;
	constantDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	result = Device->CreateBuffer(&constantDesc, nullptr, &ConstantBuffer);
	if (FAILED(result))
		return false;

	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	result = Device->CreateSamplerState(&samplerDesc, &SamplerState);
	if (FAILED(result))
		return false;

	D3D11_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	result = Device->CreateBlendState(&blendDesc, &BlendState);
	if (FAILED(result))
		return false;

	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	result = Device->CreateBlendState(&blendDesc, &StraightAlphaBlendState);
	if (FAILED(result))
		return false;

	D3D11_DEPTH_STENCIL_DESC depthDesc{};
	depthDesc.DepthEnable = FALSE;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	depthDesc.StencilEnable = FALSE;

	result = Device->CreateDepthStencilState(&depthDesc, &DepthStencilState);
	if (FAILED(result))
		return false;

	D3D11_RASTERIZER_DESC rasterDesc{};
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthClipEnable = TRUE;

	result = Device->CreateRasterizerState(&rasterDesc, &RasterizerState);
	if (FAILED(result))
		return false;

	rasterDesc.ScissorEnable = TRUE;
	result = Device->CreateRasterizerState(&rasterDesc, &ScissorRasterizerState);

	return SUCCEEDED(result);
}

void RmlRenderDX11::ReleaseDeviceObjects()
{
	SafeReleaseDX11(ScissorRasterizerState);
	SafeReleaseDX11(RasterizerState);
	SafeReleaseDX11(DepthStencilState);
	SafeReleaseDX11(StraightAlphaBlendState);
	SafeReleaseDX11(BlendState);
	SafeReleaseDX11(SamplerState);
	SafeReleaseDX11(ConstantBuffer);
	SafeReleaseDX11(InputLayout);
	SafeReleaseDX11(PixelShader);
	SafeReleaseDX11(VertexShader);
}

void RmlRenderDX11::SetupPipeline(bool textureEnabled, bool straightAlphaBlend)
{
	if (!Context)
		return;

	const float blendFactor[4] = { 0, 0, 0, 0 };
	Context->IASetInputLayout(InputLayout);
	Context->VSSetShader(VertexShader, nullptr, 0);
	Context->PSSetShader(PixelShader, nullptr, 0);
	Context->VSSetConstantBuffers(0, 1, &ConstantBuffer);
	Context->PSSetConstantBuffers(0, 1, &ConstantBuffer);
	Context->PSSetSamplers(0, 1, &SamplerState);
	Context->OMSetBlendState(straightAlphaBlend ? StraightAlphaBlendState : BlendState, blendFactor, 0xFFFFFFFF);
	Context->OMSetDepthStencilState(DepthStencilState, 0);
	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = ViewportTopLeftX;
	viewport.TopLeftY = ViewportTopLeftY;
	viewport.Width = ViewportWidth;
	viewport.Height = ViewportHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	const D3D11_RECT rect = { ScissorLeft, ScissorTop, ScissorRight, ScissorBottom };

	Context->RSSetViewports(1, &viewport);
	Context->RSSetState(bScissorEnabled ? ScissorRasterizerState : RasterizerState);
	Context->RSSetScissorRects(1, &rect);

	if (!textureEnabled)
	{
		ID3D11ShaderResourceView* nullSRV = nullptr;
		Context->PSSetShaderResources(0, 1, &nullSRV);
	}
}

void RmlRenderDX11::CaptureState()
{
	if (!Context || bStateCaptured)
		return;

	ReleaseStateBackup();

	Context->OMGetRenderTargets(1, &StateBackup.RenderTarget, &StateBackup.DepthStencil);
	Context->IAGetInputLayout(&StateBackup.InputLayout);
	Context->IAGetPrimitiveTopology(&StateBackup.Topology);
	Context->IAGetVertexBuffers(0, 1, &StateBackup.VertexBuffer, &StateBackup.VertexStride, &StateBackup.VertexOffset);
	Context->IAGetIndexBuffer(&StateBackup.IndexBuffer, &StateBackup.IndexFormat, &StateBackup.IndexOffset);
	Context->VSGetShader(&StateBackup.VertexShader, nullptr, nullptr);
	Context->PSGetShader(&StateBackup.PixelShader, nullptr, nullptr);
	Context->VSGetConstantBuffers(0, 1, &StateBackup.VSConstantBuffer);
	Context->PSGetConstantBuffers(0, 1, &StateBackup.PSConstantBuffer);
	Context->PSGetShaderResources(0, 1, &StateBackup.PSSRV);
	Context->PSGetSamplers(0, 1, &StateBackup.PSSampler);
	Context->OMGetBlendState(&StateBackup.BlendState, StateBackup.BlendFactor, &StateBackup.SampleMask);
	Context->OMGetDepthStencilState(&StateBackup.DepthStencilState, &StateBackup.StencilRef);
	Context->RSGetState(&StateBackup.RasterizerState);

	StateBackup.ViewportCount = static_cast<unsigned int>(_countof(StateBackup.Viewports));
	Context->RSGetViewports(&StateBackup.ViewportCount, StateBackup.Viewports);

	StateBackup.ScissorCount = static_cast<unsigned int>(_countof(StateBackup.ScissorRects));
	Context->RSGetScissorRects(&StateBackup.ScissorCount, StateBackup.ScissorRects);

	bStateCaptured = true;
}

void RmlRenderDX11::RestoreState()
{
	if (!Context || !bStateCaptured)
		return;

	Context->OMSetRenderTargets(1, &StateBackup.RenderTarget, StateBackup.DepthStencil);
	Context->IASetInputLayout(StateBackup.InputLayout);
	Context->IASetPrimitiveTopology(StateBackup.Topology);
	Context->IASetVertexBuffers(0, 1, &StateBackup.VertexBuffer, &StateBackup.VertexStride, &StateBackup.VertexOffset);
	Context->IASetIndexBuffer(StateBackup.IndexBuffer, StateBackup.IndexFormat, StateBackup.IndexOffset);
	Context->VSSetShader(StateBackup.VertexShader, nullptr, 0);
	Context->PSSetShader(StateBackup.PixelShader, nullptr, 0);
	Context->VSSetConstantBuffers(0, 1, &StateBackup.VSConstantBuffer);
	Context->PSSetConstantBuffers(0, 1, &StateBackup.PSConstantBuffer);
	Context->PSSetShaderResources(0, 1, &StateBackup.PSSRV);
	Context->PSSetSamplers(0, 1, &StateBackup.PSSampler);
	Context->OMSetBlendState(StateBackup.BlendState, StateBackup.BlendFactor, StateBackup.SampleMask);
	Context->OMSetDepthStencilState(StateBackup.DepthStencilState, StateBackup.StencilRef);
	Context->RSSetState(StateBackup.RasterizerState);

	if (StateBackup.ViewportCount > 0)
		Context->RSSetViewports(StateBackup.ViewportCount, StateBackup.Viewports);

	if (StateBackup.ScissorCount > 0)
		Context->RSSetScissorRects(StateBackup.ScissorCount, StateBackup.ScissorRects);
}

void RmlRenderDX11::ReleaseStateBackup()
{
	SafeReleaseDX11(StateBackup.RenderTarget);
	SafeReleaseDX11(StateBackup.DepthStencil);
	SafeReleaseDX11(StateBackup.InputLayout);
	SafeReleaseDX11(StateBackup.VertexBuffer);
	SafeReleaseDX11(StateBackup.IndexBuffer);
	SafeReleaseDX11(StateBackup.VertexShader);
	SafeReleaseDX11(StateBackup.PixelShader);
	SafeReleaseDX11(StateBackup.VSConstantBuffer);
	SafeReleaseDX11(StateBackup.PSConstantBuffer);
	SafeReleaseDX11(StateBackup.PSSRV);
	SafeReleaseDX11(StateBackup.PSSampler);
	SafeReleaseDX11(StateBackup.BlendState);
	SafeReleaseDX11(StateBackup.DepthStencilState);
	SafeReleaseDX11(StateBackup.RasterizerState);

	StateBackup = FStateBackup();
	bStateCaptured = false;
}

bool RmlRenderDX11::LoadTextureFromFile(const std::wstring& filename, Rml::Vector2i& dimensions, ID3D11ShaderResourceView** outSRV)
{
	if (!Device || filename.empty() || !outSRV)
		return false;

	*outSRV = nullptr;
	dimensions = Rml::Vector2i(0, 0);

	const HRESULT coResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	const bool coInitialized = SUCCEEDED(coResult);
	if (FAILED(coResult) && coResult != RPC_E_CHANGED_MODE)
		return false;

	IWICImagingFactory* factory = nullptr;
	IWICBitmapDecoder* decoder = nullptr;
	IWICBitmapFrameDecode* frame = nullptr;
	IWICFormatConverter* converter = nullptr;
	ID3D11Texture2D* texture = nullptr;

	auto finish = [&](bool ok) -> bool
	{
		SafeReleaseDX11(texture);
		SafeReleaseDX11(converter);
		SafeReleaseDX11(frame);
		SafeReleaseDX11(decoder);
		SafeReleaseDX11(factory);
		if (coInitialized)
			CoUninitialize();
		return ok;
	};

	HRESULT result = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		__uuidof(IWICImagingFactory),
		reinterpret_cast<void**>(&factory)
	);
	if (FAILED(result) || !factory)
		return finish(false);

	result = factory->CreateDecoderFromFilename(
		filename.c_str(),
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&decoder
	);
	if (FAILED(result) || !decoder)
		return finish(false);

	result = decoder->GetFrame(0, &frame);
	if (FAILED(result) || !frame)
		return finish(false);

	UINT width = 0;
	UINT height = 0;
	result = frame->GetSize(&width, &height);
	if (FAILED(result) || width == 0 || height == 0 || width > static_cast<UINT>(INT_MAX) || height > static_cast<UINT>(INT_MAX))
		return finish(false);

	result = factory->CreateFormatConverter(&converter);
	if (FAILED(result) || !converter)
		return finish(false);

	result = converter->Initialize(
		frame,
		GUID_WICPixelFormat32bppRGBA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0,
		WICBitmapPaletteTypeCustom
	);
	if (FAILED(result))
		return finish(false);

	if (width > UINT_MAX / 4)
		return finish(false);

	const UINT stride = width * 4;
	if (height > UINT_MAX / stride)
		return finish(false);

	const UINT imageSize = stride * height;
	std::vector<unsigned char> pixels(imageSize);

	result = converter->CopyPixels(nullptr, stride, imageSize, pixels.data());
	if (FAILED(result))
		return finish(false);

	PremultiplyRGBA(pixels.data(), pixels.size());

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA init{};
	init.pSysMem = pixels.data();
	init.SysMemPitch = stride;

	result = Device->CreateTexture2D(&desc, &init, &texture);
	if (FAILED(result) || !texture)
		return finish(false);

	result = Device->CreateShaderResourceView(texture, nullptr, outSRV);
	if (FAILED(result) || !*outSRV)
		return finish(false);

	dimensions.x = static_cast<int>(width);
	dimensions.y = static_cast<int>(height);
	return finish(true);
}

std::wstring RmlRenderDX11::ResolvePathW(const Rml::String& path) const
{
	const std::wstring widePath = Utf8ToWide(path);
	if (widePath.empty())
		return widePath;

	const bool isAbsolute =
		widePath.size() > 2 &&
		widePath[1] == L':';

	if (isAbsolute)
		return widePath;

	if (DataRoot.empty())
		return widePath;

	const std::wstring dataPath = DataRoot + L"\\" + widePath;
	if (FileExistsW(dataPath))
		return dataPath;

	const std::wstring assetPath = DataRoot + L"\\Rml\\Assets\\" + widePath;
	if (FileExistsW(assetPath))
		return assetPath;

	return dataPath;
}

unsigned int RmlRenderDX11::ConvertColor(const Rml::ColourbPremultiplied& color)
{
	return
		(static_cast<unsigned int>(color.alpha) << 24) |
		(static_cast<unsigned int>(color.blue) << 16) |
		(static_cast<unsigned int>(color.green) << 8) |
		static_cast<unsigned int>(color.red);
}

void RmlRenderDX11::SetIdentity(float outMatrix[16])
{
	for (int i = 0; i < 16; ++i)
		outMatrix[i] = 0.0f;

	outMatrix[0] = 1.0f;
	outMatrix[5] = 1.0f;
	outMatrix[10] = 1.0f;
	outMatrix[15] = 1.0f;
}

void RmlRenderDX11::CopyTransform(float outMatrix[16], const Rml::Matrix4f& transform)
{
	const float* source = transform.data();

	for (int i = 0; i < 16; ++i)
		outMatrix[i] = source[i];
}
