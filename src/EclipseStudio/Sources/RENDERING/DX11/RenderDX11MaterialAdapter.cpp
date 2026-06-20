#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11MaterialAdapter.h"

#include "RENDERING/DX11/RenderDX11Material.h"
#include "RENDERING/DX11/RenderDX11Texture.h"

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
}

bool r3dDX11CreateMaterialTexturesFromR3DMaterial(r3dDX11TextureLibrary& textureLibrary, const r3dMaterial& material, r3dDX11MaterialTextures& outTextures)
{
	const bool loaded = outTextures.Load(
		textureLibrary,
		GetTexturePath(material.Texture),
		GetTexturePath(material.BumpTexture),
		GetTexturePath(material.GlossTexture),
		GetTexturePath(material.imgEnvPower),
		GetTexturePath(material.IBLTexture),
		GetTexturePath(material.DetailNormalTexture),
		GetTexturePath(material.DensityTexture),
		GetTexturePath(material.SpecularPowTexture)
	);

	r3dDX11MaterialConstants constants = r3dDX11MaterialConstants();
	constants.MaterialParams[0] = material.SpecularPower;
	constants.MaterialParams[1] = material.ReflectionPower;
	constants.MaterialParams[2] = material.SpecularPower1;
	constants.MaterialParams[3] = 1.0f;

	constants.MatDiffuse[0] = material.DiffuseColor.R / 255.0f;
	constants.MatDiffuse[1] = material.DiffuseColor.G / 255.0f;
	constants.MatDiffuse[2] = material.DiffuseColor.B / 255.0f;
	constants.MatDiffuse[3] = material.lowQSelfIllum;

	constants.MatSpecular[0] = material.SpecularColor.R / 255.0f;
	constants.MatSpecular[1] = material.SpecularColor.G / 255.0f;
	constants.MatSpecular[2] = material.SpecularColor.B / 255.0f;
	constants.MatSpecular[3] = material.lowQMetallness;

	constants.MatGlow[0] = material.SelfIllumMultiplier;
	constants.MatGlow[1] = 0.0f;
	constants.MatGlow[2] = material.DetailScale;
	constants.MatGlow[3] = material.DetailAmmount;

	constants.Options[0] = (material.Flags & R3D_MAT_HASALPHA) ? 1.0f : 0.0f;
	constants.Options[1] = NormalizeAlphaRef(material.AlphaRef);
	constants.Options[2] = (material.Flags & R3D_MAT_DOUBLESIDED) ? 1.0f : 0.0f;
	constants.Options[3] = r_ssao_clear_val ? r_ssao_clear_val->GetFloat() : 1.0f;

	outTextures.SetConstants(constants);
	outTextures.SetDrawInGBuffer((material.Flags & (R3D_MAT_SKIP_DRAW | R3D_MAT_TRANSPARENT)) == 0);
	outTextures.SetAlphaCut((material.Flags & R3D_MAT_HASALPHA) != 0);
	outTextures.SetDoubleSided((material.Flags & R3D_MAT_DOUBLESIDED) != 0);
	return loaded;
}
