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
}

bool r3dDX11MaterialTextures::Load(
	r3dDX11TextureLibrary& textureLibrary,
	const char* diffusePath,
	const char* normalPath,
	const char* specularPath,
	const char* glowPath
)
{
	Diffuse = LoadOrFallback(textureLibrary, diffusePath, textureLibrary.GetWhiteTexture());
	Normal = LoadOrFallback(textureLibrary, normalPath, textureLibrary.GetFlatNormalTexture());
	Specular = LoadOrFallback(textureLibrary, specularPath, textureLibrary.GetBlackTexture());
	Glow = LoadOrFallback(textureLibrary, glowPath, textureLibrary.GetBlackTexture());

	return Diffuse && Normal && Specular && Glow;
}

void r3dDX11MaterialTextures::SetFallbacks(r3dDX11TextureLibrary& textureLibrary)
{
	Diffuse = textureLibrary.GetWhiteTexture();
	Normal = textureLibrary.GetFlatNormalTexture();
	Specular = textureLibrary.GetBlackTexture();
	Glow = textureLibrary.GetBlackTexture();
}

void r3dDX11MaterialTextures::Bind(r3dDX11DrawContext& drawContext, unsigned int baseSlot) const
{
	if (Diffuse)
		Diffuse->BindPS(drawContext, baseSlot + 0);
	if (Normal)
		Normal->BindPS(drawContext, baseSlot + 1);
	if (Specular)
		Specular->BindPS(drawContext, baseSlot + 2);
	if (Glow)
		Glow->BindPS(drawContext, baseSlot + 3);
}

r3dDX11Texture* r3dDX11MaterialTextures::GetDiffuse() const
{
	return Diffuse;
}

r3dDX11Texture* r3dDX11MaterialTextures::GetNormal() const
{
	return Normal;
}

r3dDX11Texture* r3dDX11MaterialTextures::GetSpecular() const
{
	return Specular;
}

r3dDX11Texture* r3dDX11MaterialTextures::GetGlow() const
{
	return Glow;
}
