#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11MaterialAdapter.h"

#include "RENDERING/DX11/RenderDX11Material.h"
#include "RENDERING/DX11/RenderDX11Texture.h"
#include "TrueNature2/Terrain2.h"

namespace
{
	const char* GetTexturePath(const r3dTexture* texture)
	{
		if (!texture || !texture->IsLoaded() || texture->IsMissing())
			return nullptr;

		const char* path = texture->getFileLoc().FileName;
		return path && path[0] ? path : nullptr;
	}

	float NormalizeAlphaRef(float alphaRef)
	{
		if (alphaRef <= 0.0f)
			return 0.15f;
		if (alphaRef > 1.0f)
			alphaRef /= 255.0f;
		if (alphaRef < 0.0f)
			return 0.0f;
		if (alphaRef > 1.0f)
			return 1.0f;
		return alphaRef;
	}

	void SetTexturePathsFromR3DMaterial(const r3dMaterial& material, r3dDX11MaterialDesc& desc)
	{
		desc.TexturePaths[0] = GetTexturePath(material.Texture);
		desc.TexturePaths[1] = GetTexturePath(material.BumpTexture);
		desc.TexturePaths[2] = GetTexturePath(material.GlossTexture);
		desc.TexturePaths[3] = GetTexturePath(material.imgEnvPower);
		desc.TexturePaths[4] = GetTexturePath(material.IBLTexture);
		desc.TexturePaths[5] = GetTexturePath(material.DetailNormalTexture);
		desc.TexturePaths[6] = GetTexturePath(material.DensityTexture);
		desc.TexturePaths[7] = GetTexturePath(material.SpecularPowTexture);
	}

	r3dDX11MaterialDesc BuildMeshDescFromR3DMaterial(const r3dMaterial& material)
	{
		r3dDX11MaterialDesc desc;
		SetTexturePathsFromR3DMaterial(material, desc);

		desc.Constants.MaterialParams[0] = material.SpecularPower;
		desc.Constants.MaterialParams[1] = material.ReflectionPower;
		desc.Constants.MaterialParams[2] = material.SpecularPower1;
		desc.Constants.MaterialParams[3] = 1.0f;

		desc.Constants.MatDiffuse[0] = material.DiffuseColor.R / 255.0f;
		desc.Constants.MatDiffuse[1] = material.DiffuseColor.G / 255.0f;
		desc.Constants.MatDiffuse[2] = material.DiffuseColor.B / 255.0f;
		desc.Constants.MatDiffuse[3] = material.lowQSelfIllum;

		desc.Constants.MatSpecular[0] = material.SpecularColor.R / 255.0f;
		desc.Constants.MatSpecular[1] = material.SpecularColor.G / 255.0f;
		desc.Constants.MatSpecular[2] = material.SpecularColor.B / 255.0f;
		desc.Constants.MatSpecular[3] = material.lowQMetallness;

		desc.Constants.MatGlow[0] = material.SelfIllumMultiplier;
		desc.Constants.MatGlow[1] = 0.0f;
		desc.Constants.MatGlow[2] = material.DetailScale;
		desc.Constants.MatGlow[3] = material.DetailAmmount;

		desc.Constants.Options[0] = (material.Flags & R3D_MAT_HASALPHA) ? 1.0f : 0.0f;
		desc.Constants.Options[1] = NormalizeAlphaRef(material.AlphaRef);
		desc.Constants.Options[2] = (material.Flags & R3D_MAT_DOUBLESIDED) ? 1.0f : 0.0f;
		desc.Constants.Options[3] = r_ssao_clear_val ? r_ssao_clear_val->GetFloat() : 1.0f;

		desc.Constants.Emissive[0] = material.EmissiveColor.R / 255.0f;
		desc.Constants.Emissive[1] = material.EmissiveColor.G / 255.0f;
		desc.Constants.Emissive[2] = material.EmissiveColor.B / 255.0f;
		desc.Constants.Emissive[3] = material.TransparencyMultiplier;

		if (material.Flags & R3D_MAT_HASALPHA)
			desc.Flags |= R3D_DX11_MATERIAL_ALPHA_CUT;
		if (material.Flags & R3D_MAT_DOUBLESIDED)
			desc.Flags |= R3D_DX11_MATERIAL_DOUBLE_SIDED;
		if (material.Flags & R3D_MAT_TRANSPARENT)
			desc.Flags |= R3D_DX11_MATERIAL_TRANSPARENT;
		if (material.Flags & R3D_MAT_SKIP_DRAW)
			desc.Flags |= R3D_DX11_MATERIAL_SKIP_DRAW;
		if (material.Flags & (R3D_MAT_TRANSPARENT | R3D_MAT_SKIP_DRAW))
			desc.Flags &= ~R3D_DX11_MATERIAL_DRAW_GBUFFER;

		return desc;
	}
}

bool r3dDX11CreateMaterialTexturesFromR3DMaterial(r3dDX11TextureLibrary& textureLibrary, const r3dMaterial& material, r3dDX11MaterialTextures& outTextures)
{
	return outTextures.ApplyDesc(textureLibrary, BuildMeshDescFromR3DMaterial(material));
}

bool r3dDX11CreateMaterialTexturesFromTerrainLayer(r3dDX11TextureLibrary& textureLibrary, const r3dTerrainLayer& layer, float terrainSpecular, float defSSAO, r3dDX11MaterialTextures& outTextures)
{
	r3dDX11MaterialDesc desc;
	desc.Domain = R3D_DX11_MATERIAL_TERRAIN;
	desc.TexturePaths[0] = GetTexturePath(layer.DiffuseTex);
	desc.TexturePaths[1] = GetTexturePath(layer.NormalTex);
	desc.TexturePaths[2] = nullptr;
	desc.TexturePaths[3] = nullptr;
	desc.TexturePaths[4] = nullptr;
	desc.TexturePaths[5] = nullptr;
	desc.TexturePaths[6] = nullptr;
	desc.TexturePaths[7] = nullptr;

	desc.Constants.MaterialParams[0] = terrainSpecular;
	desc.Constants.MaterialParams[1] = 0.0f;
	desc.Constants.MaterialParams[2] = layer.SpecularPow;
	desc.Constants.MaterialParams[3] = 1.0f;
	desc.Constants.MatDiffuse[0] = 1.0f;
	desc.Constants.MatDiffuse[1] = 1.0f;
	desc.Constants.MatDiffuse[2] = 1.0f;
	desc.Constants.MatDiffuse[3] = 0.0f;
	desc.Constants.MatSpecular[0] = 1.0f;
	desc.Constants.MatSpecular[1] = 1.0f;
	desc.Constants.MatSpecular[2] = 1.0f;
	desc.Constants.MatSpecular[3] = 0.0f;
	desc.Constants.MatGlow[0] = 0.0f;
	desc.Constants.MatGlow[1] = 0.0f;
	desc.Constants.MatGlow[2] = layer.ShaderScaleU;
	desc.Constants.MatGlow[3] = layer.ShaderScaleV;
	desc.Constants.Options[0] = 0.0f;
	desc.Constants.Options[1] = 0.15f;
	desc.Constants.Options[2] = 0.0f;
	desc.Constants.Options[3] = defSSAO;
	return outTextures.ApplyDesc(textureLibrary, desc);
}

bool r3dDX11CreateMaterialTexturesFromVegetationTexture(r3dDX11TextureLibrary& textureLibrary, const r3dTexture* diffuseTexture, float alphaRef, float tintStrength, bool doubleSided, r3dDX11MaterialTextures& outTextures)
{
	r3dDX11MaterialDesc desc;
	desc.Domain = R3D_DX11_MATERIAL_VEGETATION;
	desc.TexturePaths[0] = GetTexturePath(diffuseTexture);
	desc.Constants.MaterialParams[0] = 0.0f;
	desc.Constants.MaterialParams[1] = 0.0f;
	desc.Constants.MaterialParams[2] = 0.5f;
	desc.Constants.MaterialParams[3] = 1.0f;
	desc.Constants.MatDiffuse[0] = 1.0f;
	desc.Constants.MatDiffuse[1] = 1.0f;
	desc.Constants.MatDiffuse[2] = 1.0f;
	desc.Constants.MatDiffuse[3] = 0.0f;
	desc.Constants.MatGlow[0] = 0.0f;
	desc.Constants.MatGlow[1] = tintStrength;
	desc.Constants.MatGlow[2] = 1.0f;
	desc.Constants.MatGlow[3] = 0.0f;
	desc.Constants.Options[0] = 1.0f;
	desc.Constants.Options[1] = NormalizeAlphaRef(alphaRef);
	desc.Constants.Options[2] = doubleSided ? 1.0f : 0.0f;
	desc.Constants.Options[3] = r_ssao_clear_val ? r_ssao_clear_val->GetFloat() : 1.0f;
	desc.Flags |= R3D_DX11_MATERIAL_ALPHA_CUT;
	if (doubleSided)
		desc.Flags |= R3D_DX11_MATERIAL_DOUBLE_SIDED;
	return outTextures.ApplyDesc(textureLibrary, desc);
}
