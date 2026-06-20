#include "r3dPCH.h"

#include "RENDERING/DX11/RenderDX11Material.h"

#include "RENDERING/DX11/RenderDX11Draw.h"
#include "RENDERING/DX11/RenderDX11Texture.h"

namespace
{
	r3dDX11Texture* LoadOrFallback(r3dDX11TextureLibrary& textureLibrary, const char* path, r3dDX11Texture* fallback)
	{
		if (!path || !path[0])
			return fallback;

		r3dDX11Texture* texture = textureLibrary.LoadTexture(path);
		return texture ? texture : fallback;
	}
}

r3dDX11MaterialTextures::r3dDX11MaterialTextures()
{
	SetConstants(r3dDX11MaterialConstants());
	Constants.MaterialParams[2] = 0.5f;
	Constants.MaterialParams[3] = 1.0f;
	Constants.MatDiffuse[0] = 1.0f;
	Constants.MatDiffuse[1] = 1.0f;
	Constants.MatDiffuse[2] = 1.0f;
	Constants.MatSpecular[0] = 1.0f;
	Constants.MatSpecular[1] = 1.0f;
	Constants.MatSpecular[2] = 1.0f;
	Constants.MatGlow[2] = 1.0f;
	Constants.Options[1] = 0.15f;
	Constants.Options[3] = 1.0f;
}

bool r3dDX11MaterialTextures::Load(
	r3dDX11TextureLibrary& textureLibrary,
	const char* diffusePath,
	const char* normalPath,
	const char* glossPath,
	const char* envPowerPath,
	const char* selfIllumPath,
	const char* detailNormalPath,
	const char* sssPath,
	const char* specularPowerPath
)
{
	Diffuse = LoadOrFallback(textureLibrary, diffusePath, textureLibrary.GetWhiteTexture());
	Normal = LoadOrFallback(textureLibrary, normalPath, textureLibrary.GetFlatNormalTexture());
	Gloss = LoadOrFallback(textureLibrary, glossPath, textureLibrary.GetBlackTexture());
	EnvPower = LoadOrFallback(textureLibrary, envPowerPath, textureLibrary.GetBlackTexture());
	SelfIllum = LoadOrFallback(textureLibrary, selfIllumPath, textureLibrary.GetBlackTexture());
	DetailNormal = LoadOrFallback(textureLibrary, detailNormalPath, textureLibrary.GetFlatNormalTexture());
	SSS = LoadOrFallback(textureLibrary, sssPath, textureLibrary.GetBlackTexture());
	SpecularPower = LoadOrFallback(textureLibrary, specularPowerPath, textureLibrary.GetWhiteTexture());

	return Diffuse && Normal && Gloss && EnvPower && SelfIllum && DetailNormal && SSS && SpecularPower;
}

void r3dDX11MaterialTextures::SetFallbacks(r3dDX11TextureLibrary& textureLibrary)
{
	Diffuse = textureLibrary.GetWhiteTexture();
	Normal = textureLibrary.GetFlatNormalTexture();
	Gloss = textureLibrary.GetBlackTexture();
	EnvPower = textureLibrary.GetBlackTexture();
	SelfIllum = textureLibrary.GetBlackTexture();
	DetailNormal = textureLibrary.GetFlatNormalTexture();
	SSS = textureLibrary.GetBlackTexture();
	SpecularPower = textureLibrary.GetWhiteTexture();
}

void r3dDX11MaterialTextures::Bind(r3dDX11DrawContext& drawContext, unsigned int baseSlot) const
{
	if (Diffuse)
		Diffuse->BindPS(drawContext, baseSlot + 0);
	if (Normal)
		Normal->BindPS(drawContext, baseSlot + 1);
	if (Gloss)
		Gloss->BindPS(drawContext, baseSlot + 2);
	if (EnvPower)
		EnvPower->BindPS(drawContext, baseSlot + 3);
	if (SelfIllum)
		SelfIllum->BindPS(drawContext, baseSlot + 4);
	if (DetailNormal)
		DetailNormal->BindPS(drawContext, baseSlot + 5);
	if (SSS)
		SSS->BindPS(drawContext, baseSlot + 6);
	if (SpecularPower)
		SpecularPower->BindPS(drawContext, baseSlot + 7);
}

void r3dDX11MaterialTextures::SetConstants(const r3dDX11MaterialConstants& constants)
{
	Constants = constants;
}

r3dDX11MaterialConstants r3dDX11MaterialTextures::BuildConstants(unsigned int objectColorPacked) const
{
	r3dDX11MaterialConstants constants = Constants;
	const float inv255 = 1.0f / 255.0f;
	const float objectR = static_cast<float>((objectColorPacked >> 16) & 0xff) * inv255;
	const float objectG = static_cast<float>((objectColorPacked >> 8) & 0xff) * inv255;
	const float objectB = static_cast<float>(objectColorPacked & 0xff) * inv255;

	constants.MatDiffuse[0] *= objectR;
	constants.MatDiffuse[1] *= objectG;
	constants.MatDiffuse[2] *= objectB;
	constants.Options[0] = bAlphaCut ? 1.0f : 0.0f;
	constants.Options[2] = bDoubleSided ? 1.0f : 0.0f;
	return constants;
}

void r3dDX11MaterialTextures::SetDrawInGBuffer(bool drawInGBuffer)
{
	bDrawInGBuffer = drawInGBuffer;
}

void r3dDX11MaterialTextures::SetAlphaCut(bool alphaCut)
{
	bAlphaCut = alphaCut;
}

void r3dDX11MaterialTextures::SetDoubleSided(bool doubleSided)
{
	bDoubleSided = doubleSided;
}

bool r3dDX11MaterialTextures::ShouldDrawInGBuffer() const
{
	return bDrawInGBuffer;
}

bool r3dDX11MaterialTextures::IsDoubleSided() const
{
	return bDoubleSided;
}

r3dDX11Texture* r3dDX11MaterialTextures::GetDiffuse() const
{
	return Diffuse;
}

r3dDX11Texture* r3dDX11MaterialTextures::GetNormal() const
{
	return Normal;
}

r3dDX11Texture* r3dDX11MaterialTextures::GetGloss() const
{
	return Gloss;
}

r3dDX11Texture* r3dDX11MaterialTextures::GetEnvPower() const
{
	return EnvPower;
}

r3dDX11Texture* r3dDX11MaterialTextures::GetSelfIllum() const
{
	return SelfIllum;
}

r3dDX11Texture* r3dDX11MaterialTextures::GetDetailNormal() const
{
	return DetailNormal;
}

r3dDX11Texture* r3dDX11MaterialTextures::GetSSS() const
{
	return SSS;
}

r3dDX11Texture* r3dDX11MaterialTextures::GetSpecularPower() const
{
	return SpecularPower;
}
