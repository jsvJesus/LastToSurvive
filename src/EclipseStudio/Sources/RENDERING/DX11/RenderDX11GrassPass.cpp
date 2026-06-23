#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11GrassPass.h"

#include "ObjectsCode/Nature/GrassMap.h"
#include "ObjectsCode/Nature/GrassLib.h"
#include "RENDERING/DX11/RenderDX11Draw.h"
#include "RENDERING/DX11/RenderDX11States.h"
#include "RENDERING/DX11/RenderDX11Texture.h"
#include "RENDERING/DX11/ShaderDX11.h"
#include "RENDERING/DX11/RenderDX11World.h"

#include <map>
#include <vector>

extern GrassMap* g_pGrassMap;
extern GrassLib* g_pGrassLib;

namespace
{
	struct GrassGpuKey
	{
		int TypeIdx;
		unsigned int ChunkIdx;

		bool operator < (const GrassGpuKey& rhs) const
		{
			if (TypeIdx != rhs.TypeIdx)
				return TypeIdx < rhs.TypeIdx;

			return ChunkIdx < rhs.ChunkIdx;
		}
	};

	std::map<GrassGpuKey, r3dDX11GrassPass::ChunkGpu*> g_DX11GrassChunks;

	int ConvertDepthBiasToD24Units(float depthBias)
	{
		const double scaled = static_cast<double>(depthBias) * 16777216.0;

		if (scaled > 2147483647.0)
			return 2147483647;

		if (scaled < -2147483648.0)
			return static_cast<int>(-2147483647 - 1);

		return static_cast<int>(scaled);
	}

	float GetGrassRuntimeTime()
	{
		static float timeValue = 0.0f;
		static DWORD prevTick = GetTickCount();

		const DWORD tick = GetTickCount();
		const float dt = static_cast<float>(tick - prevTick) * 0.001f;
		prevTick = tick;

		timeValue += dt * 3.0f;
		if (timeValue > R3D_PI * 2.0f)
			timeValue = fmodf(timeValue, R3D_PI * 2.0f);

		return timeValue;
	}

	const GrassMaskTextureEntry* FindMaskForType(const GrassTextureCell& texCell, int typeIdx)
	{
		for (uint32_t i = 0, e = texCell.MaskTextureEntries.Count(); i < e; ++i)
		{
			const GrassMaskTextureEntry& entry = texCell.MaskTextureEntries[i];
			if (entry.TypeIdx == typeIdx)
				return &entry;
		}

		return nullptr;
	}

	std::map<const GrassTextureCell*, r3dDX11GrassPass::TextureGpu*> g_DX11GrassHeightTextures;
	std::map<const GrassMaskTextureEntry*, r3dDX11GrassPass::TextureGpu*> g_DX11GrassMaskTextures;

	r3dDX11GrassPass::TextureGpu* g_DX11GrassFlatHeight = NULL;
	r3dDX11GrassPass::TextureGpu* g_DX11GrassWhiteMask = NULL;

	void ReleaseTextureGpu(r3dDX11GrassPass::TextureGpu*& gpu)
	{
		if (!gpu)
			return;

		if (gpu->SRV)
		{
			gpu->SRV->Release();
			gpu->SRV = NULL;
		}

		if (gpu->Texture)
		{
			gpu->Texture->Release();
			gpu->Texture = NULL;
		}

		delete gpu;
		gpu = NULL;
	}

	template <typename TMap>
	void ReleaseTextureMap(TMap& texMap)
	{
		for (typename TMap::iterator it = texMap.begin(); it != texMap.end(); ++it)
		{
			r3dDX11GrassPass::TextureGpu* gpu = it->second;
			ReleaseTextureGpu(gpu);
		}

		texMap.clear();
	}

	bool CreateTextureGpu(
		ID3D11Device* device,
		r3dDX11GrassPass::TextureGpu* gpu,
		int width,
		int height,
		DXGI_FORMAT format,
		const void* data,
		unsigned int rowPitch,
		const char* debugName
	)
	{
		if (!device || !gpu || width <= 0 || height <= 0 || !data || rowPitch == 0)
			return false;

		D3D11_TEXTURE2D_DESC desc;
		memset(&desc, 0, sizeof(desc));

		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData;
		memset(&initData, 0, sizeof(initData));

		initData.pSysMem = data;
		initData.SysMemPitch = rowPitch;
		initData.SysMemSlicePitch = 0;

		HRESULT hr = device->CreateTexture2D(&desc, &initData, &gpu->Texture);
		if (FAILED(hr) || !gpu->Texture)
			return false;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		memset(&srvDesc, 0, sizeof(srvDesc));

		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		hr = device->CreateShaderResourceView(gpu->Texture, &srvDesc, &gpu->SRV);
		if (FAILED(hr) || !gpu->SRV)
		{
			if (gpu->Texture)
			{
				gpu->Texture->Release();
				gpu->Texture = NULL;
			}

			return false;
		}

		gpu->Width = width;
		gpu->Height = height;

#ifndef FINAL_BUILD
		if (debugName && debugName[0])
		{
			gpu->Texture->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(debugName), debugName);
			gpu->SRV->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(debugName), debugName);
		}
#endif

		return true;
	}
}

static const int DX11_GRASS_CELL_HEIGHT_TEX_DIM = 64;
static const int DX11_GRASS_CELL_MASK_TEX_DIM = 64;
static const int DX11_GRASS_HEIGHT_TEX_FMT_SIZE = 2; // old HEIGHT_TEX_FMT = D3DFMT_L16
static const int DX11_GRASS_MASK_TEX_FMT_SIZE = 1;   // old MASK_TEX_FMT = D3DFMT_L8

r3dDX11GrassPass::r3dDX11GrassPass()
{
}

r3dDX11GrassPass::TextureGpu::TextureGpu()
	: Texture(NULL)
	, SRV(NULL)
	, Width(0)
	, Height(0)
{
}

r3dDX11GrassPass::~r3dDX11GrassPass()
{
	Shutdown();
}

bool r3dDX11GrassPass::Init(
	ID3D11Device* device,
	r3dDX11DrawContext* drawContext,
	r3dDX11ShaderLibrary* shaderLibrary,
	r3dDX11CommonStates* commonStates,
	r3dDX11TextureLibrary* textureLibrary
)
{
	if (bInitialized)
		return true;

	if (!device || !drawContext || !shaderLibrary || !commonStates || !textureLibrary)
		return false;

	Device = device;
	DrawContext = drawContext;
	ShaderLibrary = shaderLibrary;
	CommonStates = commonStates;
	TextureLibrary = textureLibrary;

	if (!CreateShadersAndLayout(device) ||
		!CreateRasterizers(device) ||
		!GrassConstants.Create(device, sizeof(r3dDX11GrassConstants), "DX11.Grass.Constants"))
	{
		Shutdown();
		return false;
	}

	bInitialized = true;
	return true;
}

void r3dDX11GrassPass::Shutdown()
{
	ReleaseChunkGpu();

	ReleaseTextureMap(g_DX11GrassHeightTextures);
	ReleaseTextureMap(g_DX11GrassMaskTextures);

	ReleaseTextureGpu(g_DX11GrassFlatHeight);
	ReleaseTextureGpu(g_DX11GrassWhiteMask);

	r3dDX11::SafeRelease(ShadowRasterizer);
	r3dDX11::SafeRelease(CullNoneRasterizer);

	InstanceBuffer.Shutdown();
	GrassConstants.Shutdown();

	delete GrassLayout;
	GrassLayout = nullptr;

	GrassVS = nullptr;
	GrassGBufferPS = nullptr;
	GrassDepthPS = nullptr;

	TextureLibrary = nullptr;
	CommonStates = nullptr;
	ShaderLibrary = nullptr;
	DrawContext = nullptr;
	Device = nullptr;

	InstanceCapacity = 0;
	bInitialized = false;
}

bool r3dDX11GrassPass::RenderGBuffer(
	const r3dCamera& camera,
	const D3DXMATRIX& viewProj,
	r3dDX11WorldRenderStats* stats
)
{
	return DrawInternal(camera, viewProj, false, stats);
}

bool r3dDX11GrassPass::RenderDepth(
	const r3dCamera& camera,
	const D3DXMATRIX& viewProj,
	r3dDX11WorldRenderStats* stats
)
{
	return DrawInternal(camera, viewProj, true, stats);
}

bool r3dDX11GrassPass::RenderShadow(
	const r3dCamera& camera,
	const D3DXMATRIX& viewProj,
	r3dDX11WorldRenderStats* stats,
	float depthBias
)
{
	if (!bInitialized || !DrawContext)
		return false;

	return DrawInternal(camera, viewProj, true, stats, ShadowRasterizer);
}

bool r3dDX11GrassPass::IsInitialized() const
{
	return bInitialized;
}

bool r3dDX11GrassPass::CreateShadersAndLayout(ID3D11Device* device)
{
	const int vsIndex = ShaderLibrary->AddVertexShader(
		"VS_DX11_GRASS",
		"DS_grass_vs.hls"
	);

	const int gbufferPsIndex = ShaderLibrary->AddPixelShader(
		"PS_DX11_GRASS_GBUFFER",
		"DS_grass_gbuffer_ps.hls"
	);

	const int depthPsIndex = ShaderLibrary->AddPixelShader(
		"PS_DX11_GRASS_DEPTH",
		"DS_grass_depth_ps.hls"
	);

	if (vsIndex < 0 || gbufferPsIndex < 0 || depthPsIndex < 0)
		return false;

	GrassVS = ShaderLibrary->GetVertexShader(vsIndex);
	GrassGBufferPS = ShaderLibrary->GetPixelShader(gbufferPsIndex);
	GrassDepthPS = ShaderLibrary->GetPixelShader(depthPsIndex);

	if (!GrassVS || !GrassGBufferPS || !GrassDepthPS)
		return false;

	if (!GrassVS->IsValid() || !GrassGBufferPS->IsValid() || !GrassDepthPS->IsValid())
		return false;

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R16G16B16A16_SNORM, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA,   0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R16G16_SNORM,       0, 8,  D3D11_INPUT_PER_VERTEX_DATA,   0 },

		{ "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 5, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 6, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
	};

	GrassLayout = new r3dDX11InputLayout();
	if (!GrassLayout->Create(
			device,
			layout,
			_countof(layout),
			GrassVS->GetBytecode(),
			GrassVS->GetBytecodeSize(),
			"DX11.Grass.Layout"))
	{
		return false;
	}

	return true;
}

bool r3dDX11GrassPass::CreateRasterizers(ID3D11Device* device)
{
	D3D11_RASTERIZER_DESC desc;
	memset(&desc, 0, sizeof(desc));

	desc.FillMode = D3D11_FILL_SOLID;
	desc.CullMode = D3D11_CULL_NONE;
	desc.FrontCounterClockwise = FALSE;
	desc.DepthClipEnable = TRUE;
	desc.ScissorEnable = FALSE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;

	if (FAILED(device->CreateRasterizerState(&desc, &CullNoneRasterizer)))
		return false;

	desc.DepthBias = ConvertDepthBiasToD24Units(0.00005f);
	desc.SlopeScaledDepthBias = 1.0f;
	desc.DepthBiasClamp = 0.0f;

	if (FAILED(device->CreateRasterizerState(&desc, &ShadowRasterizer)))
		return false;

	return true;
}

bool r3dDX11GrassPass::EnsureInstanceCapacity(unsigned int count)
{
	if (count == 0)
		return false;

	if (InstanceBuffer.IsValid() && InstanceCapacity >= count)
		return true;

	unsigned int newCapacity = InstanceCapacity ? InstanceCapacity : 256;
	while (newCapacity < count)
		newCapacity *= 2;

	if (!InstanceBuffer.Create(
			Device,
			sizeof(r3dDX11GrassInstance) * newCapacity,
			sizeof(r3dDX11GrassInstance),
			nullptr,
			R3D_DX11_BUFFER_DYNAMIC,
			"DX11.Grass.InstanceBuffer"))
	{
		InstanceCapacity = 0;
		return false;
	}

	InstanceCapacity = newCapacity;
	return true;
}

bool r3dDX11GrassPass::EnsureChunkGpu(int typeIdx, unsigned int chunkIdx, ChunkGpu** outGpu)
{
	if (outGpu)
		*outGpu = nullptr;

	if (!g_pGrassLib)
		return false;

	const GrassLibEntry& entry = g_pGrassLib->GetEntry(typeIdx);

	if (chunkIdx >= entry.Chunks.Count())
		return false;

	const GrassChunk& chunk = entry.Chunks[chunkIdx];

	GrassGpuKey key;
	key.TypeIdx = typeIdx;
	key.ChunkIdx = chunkIdx;

	std::map<GrassGpuKey, ChunkGpu*>::iterator found = g_DX11GrassChunks.find(key);
	if (found != g_DX11GrassChunks.end())
	{
		if (outGpu)
			*outGpu = found->second;

		return true;
	}

	ChunkGpu* gpu = new ChunkGpu();

	if (!chunk.SysmemIndices.Count())
	{
		delete gpu;
		return false;
	}

	gpu->IndexCount = chunk.SysmemIndices.Count();

	if (!gpu->IndexBuffer.Create(
			Device,
			chunk.SysmemIndices.Count() * sizeof(UINT16),
			DXGI_FORMAT_R16_UINT,
			&chunk.SysmemIndices[0],
			R3D_DX11_BUFFER_IMMUTABLE,
			"DX11.Grass.IndexBuffer"))
	{
		delete gpu;
		return false;
	}

	for (int variation = 0; variation < chunk.NumVariations && variation < GrassChunk::MAX_VARIATIONS; ++variation)
	{
		const GrassChunk::Vertices& vertices = chunk.SysmemVertices[variation];

		if (!vertices.Count())
			continue;

		gpu->VertexCounts[variation] = vertices.Count();

		if (!gpu->VertexBuffers[variation].Create(
				Device,
				vertices.Count() * sizeof(GrassVertex),
				sizeof(GrassVertex),
				&vertices[0],
				R3D_DX11_BUFFER_IMMUTABLE,
				"DX11.Grass.VertexBuffer"))
		{
			delete gpu;
			return false;
		}
	}

	g_DX11GrassChunks[key] = gpu;

	if (outGpu)
		*outGpu = gpu;

	return true;
}

bool r3dDX11GrassPass::EnsureHeightTextureGpu(const GrassTextureCell& texCell, TextureGpu** outGpu)
{
	if (outGpu)
		*outGpu = NULL;

	if (!Device)
		return false;

	std::map<const GrassTextureCell*, TextureGpu*>::iterator found =
		g_DX11GrassHeightTextures.find(&texCell);

	if (found != g_DX11GrassHeightTextures.end())
	{
		if (outGpu)
			*outGpu = found->second;

		return true;
	}

	TextureGpu* gpu = new TextureGpu();

	if (texCell.CpuHeightData.Count() >= DX11_GRASS_CELL_HEIGHT_TEX_DIM * DX11_GRASS_CELL_HEIGHT_TEX_DIM * DX11_GRASS_HEIGHT_TEX_FMT_SIZE)
	{
		const DXGI_FORMAT format =
			DX11_GRASS_HEIGHT_TEX_FMT_SIZE == 2
				? DXGI_FORMAT_R16_UNORM
				: DXGI_FORMAT_R8_UNORM;

		const unsigned int rowPitch =
			DX11_GRASS_CELL_HEIGHT_TEX_DIM * DX11_GRASS_HEIGHT_TEX_FMT_SIZE;

		if (!CreateTextureGpu(
				Device,
				gpu,
				DX11_GRASS_CELL_HEIGHT_TEX_DIM,
				DX11_GRASS_CELL_HEIGHT_TEX_DIM,
				format,
				&texCell.CpuHeightData[0],
				rowPitch,
				"DX11.Grass.HeightTexture"))
		{
			ReleaseTextureGpu(gpu);
			return false;
		}
	}
	else
	{
		// fallback: flat height = 0
		static const unsigned short flatHeight = 0;

		if (!CreateTextureGpu(
				Device,
				gpu,
				1,
				1,
				DXGI_FORMAT_R16_UNORM,
				&flatHeight,
				sizeof(flatHeight),
				"DX11.Grass.FlatHeight"))
		{
			ReleaseTextureGpu(gpu);
			return false;
		}
	}

	g_DX11GrassHeightTextures[&texCell] = gpu;

	if (outGpu)
		*outGpu = gpu;

	return true;
}

bool r3dDX11GrassPass::EnsureMaskTextureGpu(const GrassMaskTextureEntry* maskEntry, TextureGpu** outGpu)
{
	if (outGpu)
		*outGpu = NULL;

	if (!Device)
		return false;

	if (!maskEntry || maskEntry->CpuMaskData.Count() < DX11_GRASS_CELL_MASK_TEX_DIM * DX11_GRASS_CELL_MASK_TEX_DIM * DX11_GRASS_MASK_TEX_FMT_SIZE)
	{
		if (!g_DX11GrassWhiteMask)
		{
			unsigned char whiteMask = 255;

			g_DX11GrassWhiteMask = new TextureGpu();

			if (!CreateTextureGpu(
					Device,
					g_DX11GrassWhiteMask,
					1,
					1,
					DXGI_FORMAT_R8_UNORM,
					&whiteMask,
					sizeof(whiteMask),
					"DX11.Grass.WhiteMask"))
			{
				ReleaseTextureGpu(g_DX11GrassWhiteMask);
				return false;
			}
		}

		if (outGpu)
			*outGpu = g_DX11GrassWhiteMask;

		return true;
	}

	std::map<const GrassMaskTextureEntry*, TextureGpu*>::iterator found =
		g_DX11GrassMaskTextures.find(maskEntry);

	if (found != g_DX11GrassMaskTextures.end())
	{
		if (outGpu)
			*outGpu = found->second;

		return true;
	}

	TextureGpu* gpu = new TextureGpu();

	if (!CreateTextureGpu(
			Device,
			gpu,
			DX11_GRASS_CELL_MASK_TEX_DIM,
			DX11_GRASS_CELL_MASK_TEX_DIM,
			DXGI_FORMAT_R8_UNORM,
			&maskEntry->CpuMaskData[0],
			DX11_GRASS_CELL_MASK_TEX_DIM * DX11_GRASS_MASK_TEX_FMT_SIZE,
			"DX11.Grass.MaskTexture"))
	{
		ReleaseTextureGpu(gpu);
		return false;
	}

	g_DX11GrassMaskTextures[maskEntry] = gpu;

	if (outGpu)
		*outGpu = gpu;

	return true;
}

void r3dDX11GrassPass::SetCommonStates(bool depthOnly, ID3D11RasterizerState* rasterizerOverride)
{
	DrawContext->SetInputLayout(GrassLayout);
	DrawContext->SetTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DrawContext->SetShaders(GrassVS, depthOnly ? GrassDepthPS : GrassGBufferPS);
	DrawContext->SetBlendState(CommonStates->GetOpaqueBlendState());
	DrawContext->SetDepthStencilState(CommonStates->GetDepthReadWriteState());

	if (rasterizerOverride)
		DrawContext->SetRasterizerState(rasterizerOverride);
	else
		DrawContext->SetRasterizerState(CullNoneRasterizer ? CullNoneRasterizer : CommonStates->GetCullNoneRasterizer());

	DrawContext->SetSampler(0, CommonStates->GetLinearWrapSampler());

	ID3D11DeviceContext* context = DrawContext->GetContext();
	if (context)
	{
		ID3D11SamplerState* vsClamp = CommonStates->GetLinearClampSampler();
		ID3D11SamplerState* psClamp = CommonStates->GetLinearClampSampler();

		context->VSSetSamplers(0, 1, &vsClamp);
		context->PSSetSamplers(1, 1, &psClamp);
	}

	GrassConstants.BindVS(DrawContext->GetContext(), 0);
	GrassConstants.BindPS(DrawContext->GetContext(), 0);
}

bool r3dDX11GrassPass::DrawInternal(
	const r3dCamera& camera,
	const D3DXMATRIX& viewProj,
	bool depthOnly,
	r3dDX11WorldRenderStats* stats,
	ID3D11RasterizerState* rasterizerOverride
)
{
	if (!bInitialized || !g_pGrassMap || !g_pGrassLib)
		return false;

	const GrassMap::GrassCells& cells = g_pGrassMap->GetCells();
	const GrassMap::GrassTextureCells& textureCells = g_pGrassMap->GetTextureCells();

	if (!cells.Width() || !cells.Height() || !textureCells.Width() || !textureCells.Height())
		return true;

	SetCommonStates(depthOnly, rasterizerOverride);

	r3dDX11GrassConstants constants;
	memset(&constants, 0, sizeof(constants));
	memcpy(constants.ViewProj, &viewProj, sizeof(constants.ViewProj));
	const float timeValue = GetGrassRuntimeTime();
	const r3dPoint3D chunkScale = GetGrassChunkScale();
	const r3dPoint3D halfChunkScale = chunkScale * 0.5f;

	const float visRadius = GrassMap::GetVisRad();

	constants.CameraPos_Time[0] = camera.x;
	constants.CameraPos_Time[1] = camera.y;
	constants.CameraPos_Time[2] = camera.z;
	constants.CameraPos_Time[3] = timeValue;
	constants.Params[0] = visRadius;
	constants.Params[1] = 0.04f;
	constants.Params[2] = 0.0f;
	constants.Params[3] = 0.0f;

	if (!GrassConstants.Update(DrawContext->GetContext(), &constants, sizeof(constants)))
		return false;

	std::vector<r3dDX11GrassInstance> instances;
	instances.reserve(512);

	const float cellSize = g_pGrassMap->GetCellSize();
	const int cellsPerTextureCell = g_pGrassMap->GetCellsPerTextureCell();

	for (int z = 0, ze = cells.Height(); z < ze; ++z)
	{
		for (int x = 0, xe = cells.Width(); x < xe; ++x)
		{
			const GrassCell& cell = cells[z][x];

			if (!cell.Entries.Count())
				continue;

			const r3dPoint3D cellCenter =
				cell.Position +
				r3dPoint3D(cellSize * 0.5f, 0.0f, cellSize * 0.5f);

			const float distSq =
				(cellCenter.x - camera.x) * (cellCenter.x - camera.x) +
				(cellCenter.z - camera.z) * (cellCenter.z - camera.z);

			if (distSq > visRadius * visRadius)
				continue;

			const int texCellX = x / cellsPerTextureCell;
			const int texCellZ = z / cellsPerTextureCell;

			if (texCellX < 0 || texCellZ < 0 || texCellX >= textureCells.Width() || texCellZ >= textureCells.Height())
				continue;

			const GrassTextureCell& texCell = textureCells[texCellZ][texCellX];

			if (!texCell.HeightTexture)
				continue;

			TextureGpu* heightGpu = NULL;
			if (!EnsureHeightTextureGpu(texCell, &heightGpu) || !heightGpu || !heightGpu->SRV)
				continue;

			ID3D11DeviceContext* context = DrawContext->GetContext();
			if (context)
				context->VSSetShaderResources(1, 1, &heightGpu->SRV);

			for (uint32_t entryIndex = 0, entryCount = cell.Entries.Count(); entryIndex < entryCount; ++entryIndex)
			{
				const GrassCellEntry& cellEntry = cell.Entries[entryIndex];

				if (cellEntry.TypeIdx < 0 || cellEntry.TypeIdx >= static_cast<int>(g_pGrassLib->GetEntryCount()))
					continue;

				const GrassLibEntry& libEntry = g_pGrassLib->GetEntry(cellEntry.TypeIdx);

				const r3dPoint3D toCam = cellCenter - r3dPoint3D(camera.x, camera.y, camera.z);
				const float typeFadeRadius = libEntry.FadeDistance * visRadius + cellSize;
				if (toCam.Length() > typeFadeRadius)
					continue;

				const GrassMaskTextureEntry* maskEntry = FindMaskForType(texCell, cellEntry.TypeIdx);

				TextureGpu* maskGpu = NULL;
				if (!EnsureMaskTextureGpu(maskEntry, &maskGpu) || !maskGpu || !maskGpu->SRV)
					continue;

				DrawContext->SetShaderResource(2, maskGpu->SRV);

				for (uint32_t chunkIdx = 0, chunkCount = libEntry.Chunks.Count(); chunkIdx < chunkCount; ++chunkIdx)
				{
					const GrassChunk& chunk = libEntry.Chunks[chunkIdx];

					if (!chunk.NumVariations || !chunk.IndexBuffer)
						continue;

					ChunkGpu* gpu = nullptr;
					if (!EnsureChunkGpu(cellEntry.TypeIdx, chunkIdx, &gpu) || !gpu)
					{
						if (stats)
							++stats->VegetationSkippedFailed;

						continue;
					}

					const int variation = (x ^ z) % chunk.NumVariations;

					if (variation < 0 || variation >= GrassChunk::MAX_VARIATIONS)
						continue;

					if (!gpu->VertexBuffers[variation].IsValid() || !gpu->IndexBuffer.IsValid())
						continue;

					instances.clear();

					r3dDX11GrassInstance instance;
					memset(&instance, 0, sizeof(instance));

					instance.PositionAlphaRef[0] = cellCenter.x;
					instance.PositionAlphaRef[1] = cell.Position.y;
					instance.PositionAlphaRef[2] = cellCenter.z;
					instance.PositionAlphaRef[3] = chunk.AlphaRef;

					instance.ScaleTint[0] = halfChunkScale.x;
					instance.ScaleTint[1] = halfChunkScale.y;
					instance.ScaleTint[2] = halfChunkScale.z;
					instance.ScaleTint[3] = chunk.TintStrength;

					instance.AnimParams[0] = timeValue;
					instance.AnimParams[1] = 0.04f;
					instance.AnimParams[2] = cell.YMax - cell.Position.y;
					instance.AnimParams[3] = libEntry.FadeDistance;

					const int localCellX = x - texCellX * cellsPerTextureCell;
					const int localCellZ = z - texCellZ * cellsPerTextureCell;

					instance.CellParams[0] = static_cast<float>(localCellX);
					instance.CellParams[1] = static_cast<float>(localCellZ);
					instance.CellParams[2] = cellSize;
					instance.CellParams[3] = cellsPerTextureCell > 0 ? 1.0f / static_cast<float>(cellsPerTextureCell) : 1.0f;

					instances.push_back(instance);

					if (!EnsureInstanceCapacity(static_cast<unsigned int>(instances.size())))
						continue;

					if (!InstanceBuffer.Update(
							DrawContext->GetContext(),
							&instances[0],
							sizeof(r3dDX11GrassInstance) * instances.size()))
					{
						continue;
					}

					r3dDX11VertexBuffer* buffers[2] =
					{
						&gpu->VertexBuffers[variation],
						&InstanceBuffer
					};

					DrawContext->SetVertexBuffers(0, 2, buffers);
					DrawContext->SetIndexBuffer(&gpu->IndexBuffer);

					r3dDX11Texture* dx11Texture = nullptr;

					if (chunk.Texture && chunk.Texture->getFileLoc().FileName[0])
						dx11Texture = TextureLibrary->LoadTexture(chunk.Texture->getFileLoc().FileName, false);

					if (!dx11Texture)
						dx11Texture = TextureLibrary->GetWhiteTexture();

					if (dx11Texture)
						DrawContext->SetShaderResource(0, dx11Texture->GetSRV());

					DrawContext->DrawIndexedInstanced(
						gpu->IndexCount,
						static_cast<unsigned int>(instances.size())
					);

					if (stats)
					{
						if (depthOnly)
						{
							++stats->VegetationDepthDraws;
							stats->VegetationDepthInstances += static_cast<unsigned int>(instances.size());
						}
						else
						{
							++stats->VegetationGBufferDraws;
							stats->VegetationGBufferInstances += static_cast<unsigned int>(instances.size());
						}

						++stats->VegetationBendingDraws;
					}
				}
			}
		}
	}

	DrawContext->SetVertexBuffer(nullptr, 0);
	DrawContext->SetVertexBuffer(nullptr, 1);
	DrawContext->SetShaderResource(0, NULL);
	DrawContext->SetShaderResource(2, NULL);

	if (DrawContext->GetContext())
	{
		ID3D11ShaderResourceView* nullSrv = NULL;
		DrawContext->GetContext()->VSSetShaderResources(1, 1, &nullSrv);
	}

	return true;
}

void r3dDX11GrassPass::ReleaseChunkGpu()
{
	for (std::map<GrassGpuKey, ChunkGpu*>::iterator it = g_DX11GrassChunks.begin(); it != g_DX11GrassChunks.end(); ++it)
	{
		delete it->second;
	}

	g_DX11GrassChunks.clear();
}
