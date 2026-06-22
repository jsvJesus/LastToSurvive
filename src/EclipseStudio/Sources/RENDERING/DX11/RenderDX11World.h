#pragma once

class r3dCamera;
class r3dDX11Renderer;

struct r3dDX11WorldRenderStats
{
	unsigned int TotalRenderables;
	unsigned int MeshRenderables;

	unsigned int DepthTotalRenderables;
	unsigned int DepthMeshRenderables;
	unsigned int DepthStaticMeshes;
	unsigned int DepthSkinnedMeshes;
	unsigned int DepthAlphaTestedMeshes;
	unsigned int DepthFirstPersonMeshes;
	unsigned int DepthDrawnMeshes;
	unsigned int DepthSkippedUnsupported;
	unsigned int DepthSkippedFailed;

	unsigned int DrawnMeshes;
	unsigned int SkippedUnsupported;
	unsigned int SkippedFailed;

	unsigned int ShadowRenderables;
	unsigned int ShadowMeshRenderables;
	unsigned int ShadowStaticMeshes;
	unsigned int ShadowSkinnedMeshes;
	unsigned int ShadowDrawnMeshes;
	unsigned int ShadowAlphaTested;
	unsigned int ShadowSkippedUnsupported;
	unsigned int ShadowSkippedFailed;
	unsigned int ShadowSlicesRendered;

	unsigned int TransparentShadowRenderables;
	unsigned int TransparentShadowMeshRenderables;
	unsigned int TransparentShadowStaticMeshes;
	unsigned int TransparentShadowSkinnedMeshes;
	unsigned int TransparentShadowDrawnMeshes;
	unsigned int TransparentShadowAlphaTested;
	unsigned int TransparentShadowSkippedUnsupported;
	unsigned int TransparentShadowSkippedFailed;
	unsigned int TransparentShadowCasesRendered;

	unsigned int LightingPasses;
	unsigned int LightingDirectionalLights;
	unsigned int LightingPointLights;
	unsigned int LightingSpotLights;
	unsigned int LightingShadowed;
	unsigned int LightingGBufferDecoded;
	unsigned int LightingSpecGlossDecoded;
	unsigned int LightingFogApplied;
	unsigned int LightingAmbientApplied;
	unsigned int LightingProbeApplied;
	unsigned int LightingSkippedFailed;

	unsigned int TerrainGBufferDraws;
	unsigned int TerrainGBufferTriangles;
	unsigned int TerrainDepthDraws;
	unsigned int TerrainDepthTriangles;
	unsigned int TerrainShadowDraws;
	unsigned int TerrainShadowTriangles;
	unsigned int TerrainSplatLayers;
	unsigned int TerrainDetailLayers;
	unsigned int TerrainSkippedFailed;

	unsigned int VegetationGBufferInstances;
	unsigned int VegetationGBufferDraws;
	unsigned int VegetationDepthInstances;
	unsigned int VegetationDepthDraws;
	unsigned int VegetationShadowInstances;
	unsigned int VegetationShadowDraws;
	unsigned int VegetationBendingDraws;
	unsigned int VegetationSkippedFailed;

	unsigned int TransparentDepthRenderables;
	unsigned int TransparentDepthMeshRenderables;
	unsigned int TransparentDepthDrawnMeshes;
	unsigned int TransparentDepthAlphaTestedMeshes;
	unsigned int TransparentDepthCamouflageMeshes;
	unsigned int TransparentDepthSkippedUnsupported;
	unsigned int TransparentDepthSkippedFailed;
};

void r3dDX11ResetWorldRenderStats(r3dDX11WorldRenderStats& stats);

bool r3dDX11RenderWorldDepthOnly(
	r3dDX11Renderer& renderer,
	const r3dCamera& camera,
	r3dDX11WorldRenderStats* stats = 0,
	bool clearDepth = true
);

bool r3dDX11RenderWorldGBuffer(
	r3dDX11Renderer& renderer,
	const r3dCamera& camera,
	r3dDX11WorldRenderStats* stats = 0
);
