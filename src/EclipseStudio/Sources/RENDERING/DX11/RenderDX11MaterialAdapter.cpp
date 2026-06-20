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
}

bool r3dDX11CreateMaterialTexturesFromR3DMaterial(r3dDX11TextureLibrary& textureLibrary, const r3dMaterial& material, r3dDX11MaterialTextures& outTextures)
{
	return outTextures.Load(
		textureLibrary,
		GetTexturePath(material.Texture),
		GetTexturePath(material.BumpTexture),
		GetTexturePath(material.GlossTexture),
		GetTexturePath(material.IBLTexture)
	);
}
