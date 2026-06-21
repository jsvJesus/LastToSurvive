#include "r3dPCH.h"
#include "r3d.h"
#include "r3dCam.h"

#include "TrueNature/ITerrain.h"
#include "TrueNature2/Terrain2.h"
#include "r3dTex.h"

#include "RENDERING/DX11/RenderDX11TerrainPass.h"
#include "RENDERING/DX11/RenderDX11Draw.h"
#include "RENDERING/DX11/RenderDX11GBufferResources.h"
#include "RENDERING/DX11/RenderDX11States.h"
#include "RENDERING/DX11/ShaderDX11.h"
#include "RENDERING/DX11/RenderDX11World.h"
#include "RENDERING/DX11/RenderDX11Texture.h"

#include <algorithm>
#include <vector>
#include <cstring>

extern r3dITerrain* Terrain;
extern r3dTerrain2* Terrain2;

namespace
{
	static const int DX11_TERRAIN_GRID_DIM = 192;

	struct DX11TerrainVertex
	{
		float Position[3];
		float Normal[3];
		float TexCoord[2];
		float LayerData[4];
	};

	struct DX11TerrainConstants
	{
		float WorldViewProj[16];

		float TerrainParams[4];
		float TerrainLayerParams[4];
		float TerrainDetailParams[4];
		float TerrainDebugParams[4];
	};

	void CopyMatrix(float outMatrix[16], const D3DXMATRIX& matrix)
	{
		std::memcpy(outMatrix, &matrix, sizeof(float) * 16);
	}

	float SaturateFloat(float value)
	{
		return std::max(0.0f, std::min(value, 1.0f));
	}

	float SafeTerrainHeight(r3dITerrain* terrain, float x, float z)
	{
		if (!terrain)
			return 0.0f;

		r3dPoint3D p;
		p.x = x;
		p.y = 0.0f;
		p.z = z;

		return terrain->GetHeight(p);
	}

	r3dPoint3D SafeTerrainNormal(r3dITerrain* terrain, float x, float z)
	{
		if (!terrain)
			return r3dPoint3D(0.0f, 1.0f, 0.0f);

		r3dPoint3D p;
		p.x = x;
		p.y = SafeTerrainHeight(terrain, x, z);
		p.z = z;

		r3dPoint3D n = terrain->GetNormal(p);

		const float lenSq =
			n.x * n.x +
			n.y * n.y +
			n.z * n.z;

		if (lenSq <= 0.000001f)
			return r3dPoint3D(0.0f, 1.0f, 0.0f);

		const float invLen = 1.0f / sqrtf(lenSq);

		n.x *= invLen;
		n.y *= invLen;
		n.z *= invLen;

		return n;
	}

	void FillTerrainLayerData(
		const r3dTerrainDesc& desc,
		float height,
		const r3dPoint3D& normal,
		float outLayerData[4]
	)
	{
		const float heightRange =
			std::max(1.0f, desc.MaxHeight - desc.MinHeight);

		const float height01 =
			SaturateFloat((height - desc.MinHeight) / heightRange);

		const float slope =
			SaturateFloat(1.0f - normal.y);

		outLayerData[0] = height01;
		outLayerData[1] = slope;
		outLayerData[2] = 1.0f - slope;
		outLayerData[3] = 1.0f;
	}

	r3dTexture* GetLegacyTerrainColorTexture()
	{
		if (Terrain2 && Terrain2->IsLoaded())
		{
			r3dTexture* colorTexture = Terrain2->GetColorTexture();

			if (colorTexture)
				return colorTexture;
		}

		if (Terrain)
		{
			const r3dTerrainDesc& desc = Terrain->GetDesc();

			if (desc.OrthoDiffuseTex)
				return desc.OrthoDiffuseTex;
		}

		return nullptr;
	}

	r3dTexture* GetLegacyTerrainNormalTexture()
	{
		if (Terrain2 && Terrain2->IsLoaded())
			return Terrain2->GetNormalTexture();

		return nullptr;
	}

	r3dTexture* GetLegacyTerrainDetailNormalTexture()
	{
		if (Terrain2 && Terrain2->IsLoaded())
			return Terrain2->GetNormalDetailTexture();

		return nullptr;
	}

	r3dDX11Texture* LoadDX11TextureFromLegacyTexture(
		r3dDX11TextureLibrary* textureLibrary,
		r3dTexture* legacyTexture,
		bool generateMips
	)
	{
		if (!textureLibrary || !legacyTexture)
			return nullptr;

		if (!legacyTexture->IsLoaded() || legacyTexture->IsMissing())
			return nullptr;

		const r3dFileLoc& location =
			legacyTexture->getFileLoc();

		if (!location.FileName[0])
			return nullptr;

		return textureLibrary->LoadTexture(
			location.FileName,
			generateMips
		);
	}
}

r3dDX11TerrainPass::r3dDX11TerrainPass()
{
}

r3dDX11TerrainPass::~r3dDX11TerrainPass()
{
	Shutdown();
}

bool r3dDX11TerrainPass::Init(
	ID3D11Device* device,
	r3dDX11DrawContext* drawContext,
	r3dDX11ShaderLibrary* shaderLibrary,
	r3dDX11CommonStates* commonStates,
	r3dDX11TextureLibrary* textureLibrary
)
{
	if (bInitialized)
		return true;

	if (!device || !drawContext || !shaderLibrary || !commonStates)
		return false;

	Device = device;
	DrawContext = drawContext;
	ShaderLibrary = shaderLibrary;
	CommonStates = commonStates;
	TextureLibrary = textureLibrary;

	if (
		!CreateShadersAndLayout(device) ||
		!CreateConstantBuffers(device)
	)
	{
		Shutdown();
		return false;
	}

	bInitialized = true;

	r3dOutToLog("[DX11] Terrain pass initialized\n");

	return true;
}

void r3dDX11TerrainPass::Shutdown()
{
	TerrainConstants.Shutdown();

	IndexBuffer.Shutdown();
	VertexBuffer.Shutdown();

	delete InputLayout;
	InputLayout = nullptr;

	TerrainVS = nullptr;
	TerrainGBufferPS = nullptr;

	VertexCount = 0;
	IndexCount = 0;
	TriangleCount = 0;

	CachedCellCountX = 0;
	CachedCellCountZ = 0;
	CachedCellSize = 0.0f;
	CachedXSize = 0.0f;
	CachedZSize = 0.0f;

	CommonStates = nullptr;
	TextureLibrary = nullptr;
	ShaderLibrary = nullptr;
	DrawContext = nullptr;
	Device = nullptr;

	bInitialized = false;
}

bool r3dDX11TerrainPass::RenderGBuffer(
	const r3dCamera& camera,
	r3dDX11GBufferResources& gbuffer,
	const D3DXMATRIX& viewProj,
	r3dDX11WorldRenderStats* stats
)
{
	(void)camera;

	if (!bInitialized || !DrawContext || !gbuffer.IsInitialized())
		return false;

	if (!EnsureTerrainMesh(Device))
	{
		if (stats)
			++stats->TerrainSkippedFailed;

		return false;
	}

	ID3D11DeviceContext* dxContext = DrawContext->GetContext();

	if (!dxContext)
		return false;

	ID3D11RenderTargetView* rtvs[] =
	{
		gbuffer.GetColor().GetRTV(),
		gbuffer.GetNormal().GetRTV(),
		gbuffer.GetLinearDepth().GetRTV(),
		gbuffer.GetAux().GetRTV()
	};

	dxContext->OMSetRenderTargets(
		_countof(rtvs),
		rtvs,
		gbuffer.GetDepthStencilView()
	);

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(gbuffer.GetWidth());
	viewport.Height = static_cast<float>(gbuffer.GetHeight());
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	dxContext->RSSetViewports(
		1,
		&viewport
	);

	return DrawTerrain(
		viewProj,
		true,
		stats
	);
}

bool r3dDX11TerrainPass::RenderDepth(
	const D3DXMATRIX& viewProj,
	ID3D11DepthStencilView* depthStencil,
	int width,
	int height,
	r3dDX11WorldRenderStats* stats
)
{
	if (!bInitialized || !DrawContext || !depthStencil || width <= 0 || height <= 0)
		return false;

	if (!EnsureTerrainMesh(Device))
	{
		if (stats)
			++stats->TerrainSkippedFailed;

		return false;
	}

	ID3D11DeviceContext* dxContext = DrawContext->GetContext();

	if (!dxContext)
		return false;

	ID3D11RenderTargetView* nullRTV = nullptr;

	dxContext->OMSetRenderTargets(
		0,
		&nullRTV,
		depthStencil
	);

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	dxContext->RSSetViewports(
		1,
		&viewport
	);

	return DrawTerrain(
		viewProj,
		false,
		stats
	);
}

bool r3dDX11TerrainPass::RenderShadow(
	const D3DXMATRIX& viewProj,
	ID3D11DepthStencilView* depthStencil,
	int width,
	int height,
	r3dDX11WorldRenderStats* stats
)
{
	const bool result =
		RenderDepth(
			viewProj,
			depthStencil,
			width,
			height,
			stats
		);

	if (result && stats)
	{
		++stats->TerrainShadowDraws;
		stats->TerrainShadowTriangles += TriangleCount;
	}

	return result;
}

bool r3dDX11TerrainPass::IsInitialized() const
{
	return bInitialized;
}

bool r3dDX11TerrainPass::CreateShadersAndLayout(ID3D11Device* device)
{
	const int vsIndex =
		ShaderLibrary->AddVertexShader(
			"VS_DX11_TERRAIN",
			"Terrain_dx11_vs.hls"
		);

	const int psIndex =
		ShaderLibrary->AddPixelShader(
			"PS_DX11_TERRAIN_GBUFFER",
			"Terrain_dx11_gbuffer_ps.hls"
		);

	if (vsIndex < 0 || psIndex < 0)
	{
		r3dOutToLog(
			"[DX11] Terrain shader registration failed: %s\n",
			ShaderLibrary->GetLastError().c_str()
		);

		return false;
	}

	TerrainVS =
		ShaderLibrary->GetVertexShader(
			vsIndex
		);

	TerrainGBufferPS =
		ShaderLibrary->GetPixelShader(
			psIndex
		);

	if (
		!TerrainVS ||
		!TerrainGBufferPS ||
		!TerrainVS->IsValid() ||
		!TerrainGBufferPS->IsValid()
	)
	{
		return false;
	}

	const D3D11_INPUT_ELEMENT_DESC inputElements[] =
	{
		{
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			offsetof(DX11TerrainVertex, Position),
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		},
		{
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			offsetof(DX11TerrainVertex, Normal),
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		},
		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			offsetof(DX11TerrainVertex, TexCoord),
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		},
		{
			"COLOR",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			offsetof(DX11TerrainVertex, LayerData),
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		}
	};

	InputLayout = new r3dDX11InputLayout();

	if (!InputLayout->Create(
			device,
			inputElements,
			static_cast<unsigned int>(_countof(inputElements)),
			TerrainVS->GetBytecode(),
			TerrainVS->GetBytecodeSize(),
			"DX11.Terrain.InputLayout"))
	{
		delete InputLayout;
		InputLayout = nullptr;
		return false;
	}

	return true;
}

bool r3dDX11TerrainPass::CreateConstantBuffers(ID3D11Device* device)
{
	return TerrainConstants.Create(
		device,
		sizeof(DX11TerrainConstants),
		"DX11.Terrain.Constants"
	);
}

bool r3dDX11TerrainPass::BuildTerrainMesh(ID3D11Device* device)
{
	if (!Terrain || !Terrain->IsLoaded())
		return false;

	const r3dTerrainDesc& desc =
		Terrain->GetDesc();

	if (desc.XSize <= 1.0f || desc.ZSize <= 1.0f)
		return false;

	const int gridX =
		std::max(
			2,
			std::min(DX11_TERRAIN_GRID_DIM, desc.CellCountX + 1)
		);

	const int gridZ =
		std::max(
			2,
			std::min(DX11_TERRAIN_GRID_DIM, desc.CellCountZ + 1)
		);

	std::vector<DX11TerrainVertex> vertices;
	std::vector<unsigned short> indices;

	vertices.resize(
		static_cast<size_t>(gridX * gridZ)
	);

	const float invX =
		1.0f / static_cast<float>(gridX - 1);

	const float invZ =
		1.0f / static_cast<float>(gridZ - 1);

	for (int z = 0; z < gridZ; ++z)
	{
		for (int x = 0; x < gridX; ++x)
		{
			const float u =
				static_cast<float>(x) * invX;

			const float v =
				static_cast<float>(z) * invZ;

			const float worldX =
				u * desc.XSize;

			const float worldZ =
				v * desc.ZSize;

			const float worldY =
				SafeTerrainHeight(
					Terrain,
					worldX,
					worldZ
				);

			const r3dPoint3D normal =
				SafeTerrainNormal(
					Terrain,
					worldX,
					worldZ
				);

			DX11TerrainVertex& vertex =
				vertices[x + z * gridX];

			vertex.Position[0] = worldX;
			vertex.Position[1] = worldY;
			vertex.Position[2] = worldZ;

			vertex.Normal[0] = normal.x;
			vertex.Normal[1] = normal.y;
			vertex.Normal[2] = normal.z;

			vertex.TexCoord[0] = u;
			vertex.TexCoord[1] = v;

			FillTerrainLayerData(
				desc,
				worldY,
				normal,
				vertex.LayerData
			);
		}
	}

	indices.reserve(
		static_cast<size_t>((gridX - 1) * (gridZ - 1) * 6)
	);

	for (int z = 0; z < gridZ - 1; ++z)
	{
		for (int x = 0; x < gridX - 1; ++x)
		{
			const unsigned short i0 =
				static_cast<unsigned short>(x + z * gridX);

			const unsigned short i1 =
				static_cast<unsigned short>((x + 1) + z * gridX);

			const unsigned short i2 =
				static_cast<unsigned short>(x + (z + 1) * gridX);

			const unsigned short i3 =
				static_cast<unsigned short>((x + 1) + (z + 1) * gridX);

			indices.push_back(i0);
			indices.push_back(i2);
			indices.push_back(i1);

			indices.push_back(i1);
			indices.push_back(i2);
			indices.push_back(i3);
		}
	}

	VertexBuffer.Shutdown();
	IndexBuffer.Shutdown();

	if (!VertexBuffer.Create(
			device,
			vertices.size() * sizeof(DX11TerrainVertex),
			sizeof(DX11TerrainVertex),
			vertices.data(),
			R3D_DX11_BUFFER_IMMUTABLE,
			"DX11.Terrain.VertexBuffer"))
	{
		return false;
	}

	if (!IndexBuffer.Create(
			device,
			indices.size() * sizeof(unsigned short),
			DXGI_FORMAT_R16_UINT,
			indices.data(),
			R3D_DX11_BUFFER_IMMUTABLE,
			"DX11.Terrain.IndexBuffer"))
	{
		VertexBuffer.Shutdown();
		return false;
	}

	VertexCount =
		static_cast<unsigned int>(vertices.size());

	IndexCount =
		static_cast<unsigned int>(indices.size());

	TriangleCount =
		IndexCount / 3;

	CachedCellCountX = desc.CellCountX;
	CachedCellCountZ = desc.CellCountZ;
	CachedCellSize = desc.CellSize;
	CachedXSize = desc.XSize;
	CachedZSize = desc.ZSize;

	r3dOutToLog(
		"[DX11][Terrain] Mesh built grid=%dx%d verts=%u tris=%u layers=%d size=%.1fx%.1f\n",
		gridX,
		gridZ,
		VertexCount,
		TriangleCount,
		desc.LayerCount,
		desc.XSize,
		desc.ZSize
	);

	return true;
}

bool r3dDX11TerrainPass::EnsureTerrainMesh(ID3D11Device* device)
{
	if (!Terrain || !Terrain->IsLoaded())
		return false;

	const r3dTerrainDesc& desc =
		Terrain->GetDesc();

	if (
		VertexBuffer.IsValid() &&
		IndexBuffer.IsValid() &&
		CachedCellCountX == desc.CellCountX &&
		CachedCellCountZ == desc.CellCountZ &&
		fabsf(CachedCellSize - desc.CellSize) < 0.0001f &&
		fabsf(CachedXSize - desc.XSize) < 0.0001f &&
		fabsf(CachedZSize - desc.ZSize) < 0.0001f
	)
	{
		return true;
	}

	return BuildTerrainMesh(
		device
	);
}

bool r3dDX11TerrainPass::DrawTerrain(
	const D3DXMATRIX& viewProj,
	bool gbufferMode,
	r3dDX11WorldRenderStats* stats
)
{
	if (
		!DrawContext ||
		!InputLayout ||
		!VertexBuffer.IsValid() ||
		!IndexBuffer.IsValid() ||
		IndexCount == 0 ||
		!Terrain ||
		!Terrain->IsLoaded()
	)
	{
		return false;
	}

	const r3dTerrainDesc& desc =
		Terrain->GetDesc();

	DX11TerrainConstants constants;
	std::memset(&constants, 0, sizeof(constants));

	CopyMatrix(
		constants.WorldViewProj,
		viewProj
	);

	constants.TerrainParams[0] = desc.XSize;
	constants.TerrainParams[1] = desc.ZSize;
	constants.TerrainParams[2] = desc.MinHeight;
	constants.TerrainParams[3] = desc.MaxHeight;

	constants.TerrainLayerParams[0] =
		static_cast<float>(std::max(1, desc.LayerCount));

	constants.TerrainLayerParams[1] =
		static_cast<float>(desc.SplatResolutionU);

	constants.TerrainLayerParams[2] =
		static_cast<float>(desc.SplatResolutionV);

	constants.TerrainLayerParams[3] =
		static_cast<float>(desc.CellSize);

	float terrainDetailAmount =
	r_dx11_terrain_detail_amount
		? SaturateFloat(r_dx11_terrain_detail_amount->GetFloat())
		: 0.08f;

	constants.TerrainDetailParams[0] = 28.0f;
	constants.TerrainDetailParams[1] = terrainDetailAmount;
	constants.TerrainDetailParams[2] = 1.0f;
	constants.TerrainDetailParams[3] = 0.0f;

	r3dDX11Texture* terrainColorTexture =
	LoadDX11TextureFromLegacyTexture(
		TextureLibrary,
		GetLegacyTerrainColorTexture(),
		true
	);

	r3dDX11Texture* terrainNormalTexture =
		LoadDX11TextureFromLegacyTexture(
			TextureLibrary,
			GetLegacyTerrainNormalTexture(),
			true
		);

	r3dDX11Texture* terrainDetailNormalTexture =
		LoadDX11TextureFromLegacyTexture(
			TextureLibrary,
			GetLegacyTerrainDetailNormalTexture(),
			true
		);

	const bool hasColorTexture =
		terrainColorTexture && terrainColorTexture->IsValid();

	const bool hasNormalTexture =
		terrainNormalTexture && terrainNormalTexture->IsValid();

	const bool hasDetailNormalTexture =
		terrainDetailNormalTexture && terrainDetailNormalTexture->IsValid();

	if (!terrainColorTexture || !terrainColorTexture->IsValid())
		terrainColorTexture = TextureLibrary ? TextureLibrary->GetWhiteTexture() : nullptr;

	if (!terrainNormalTexture || !terrainNormalTexture->IsValid())
		terrainNormalTexture = TextureLibrary ? TextureLibrary->GetFlatNormalTexture() : nullptr;

	if (!terrainDetailNormalTexture || !terrainDetailNormalTexture->IsValid())
		terrainDetailNormalTexture = TextureLibrary ? TextureLibrary->GetFlatNormalTexture() : nullptr;

	float terrainTextureBlend =
	r_dx11_terrain_texture_blend
		? SaturateFloat(r_dx11_terrain_texture_blend->GetFloat())
		: 0.18f;

	float terrainNormalBlend =
		r_dx11_terrain_normal_blend
			? SaturateFloat(r_dx11_terrain_normal_blend->GetFloat())
			: 0.10f;

	constants.TerrainDebugParams[0] = gbufferMode ? 1.0f : 0.0f;
	constants.TerrainDebugParams[1] = hasColorTexture ? terrainTextureBlend : 0.0f;
	constants.TerrainDebugParams[2] = hasNormalTexture ? terrainNormalBlend : 0.0f;
	constants.TerrainDebugParams[3] = hasDetailNormalTexture ? terrainDetailAmount : 0.0f;

	if (!TerrainConstants.Update(
			DrawContext->GetContext(),
			&constants,
			sizeof(constants)))
	{
		return false;
	}

	ID3D11DeviceContext* dxContext =
		DrawContext->GetContext();

	TerrainConstants.BindVSPS(
		dxContext,
		0
	);

	DrawContext->SetRasterizerState(
		CommonStates->GetCullBackRasterizer()
	);

	DrawContext->SetDepthStencilState(
		CommonStates->GetDepthReadWriteState()
	);

	DrawContext->SetBlendState(
		CommonStates->GetOpaqueBlendState()
	);

	DrawContext->SetInputLayout(
		InputLayout
	);

	DrawContext->SetTopology(
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);

	DrawContext->SetVertexBuffer(
		&VertexBuffer
	);

	DrawContext->SetIndexBuffer(
		&IndexBuffer
	);

	DrawContext->SetShaders(
	TerrainVS,
	gbufferMode ? TerrainGBufferPS : nullptr
);

	if (gbufferMode)
	{
		DrawContext->SetSampler(
			0,
			CommonStates->GetLinearClampSampler()
		);

		DrawContext->SetSampler(
			1,
			CommonStates->GetLinearWrapSampler()
		);

		DrawContext->SetShaderResource(
			0,
			terrainColorTexture ? terrainColorTexture->GetSRV() : nullptr
		);

		DrawContext->SetShaderResource(
			1,
			terrainNormalTexture ? terrainNormalTexture->GetSRV() : nullptr
		);

		DrawContext->SetShaderResource(
			2,
			terrainDetailNormalTexture ? terrainDetailNormalTexture->GetSRV() : nullptr
		);
	}

	DrawContext->DrawIndexed(
		IndexCount
	);

	if (gbufferMode)
	{
		ID3D11ShaderResourceView* nullSRV = nullptr;

		DrawContext->SetShaderResource(0, nullSRV);
		DrawContext->SetShaderResource(1, nullSRV);
		DrawContext->SetShaderResource(2, nullSRV);
	}

	if (stats)
	{
		stats->TerrainSplatLayers =
			static_cast<unsigned int>(std::max(1, desc.LayerCount));

		stats->TerrainDetailLayers = 1;

		if (gbufferMode)
		{
			++stats->TerrainGBufferDraws;
			stats->TerrainGBufferTriangles += TriangleCount;
		}
		else
		{
			++stats->TerrainDepthDraws;
			stats->TerrainDepthTriangles += TriangleCount;
		}
	}

	return true;
}