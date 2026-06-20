#pragma once

class r3dDX11DrawContext;
class r3dDX11Texture;
class r3dDX11TextureLibrary;

class r3dDX11MaterialTextures final
{
public:
	r3dDX11MaterialTextures();

	bool Load(
		r3dDX11TextureLibrary& textureLibrary,
		const char* diffusePath,
		const char* normalPath,
		const char* specularPath,
		const char* glowPath
	);

	void SetFallbacks(r3dDX11TextureLibrary& textureLibrary);
	void Bind(r3dDX11DrawContext& drawContext, unsigned int baseSlot = 0) const;

	r3dDX11Texture* GetDiffuse() const;
	r3dDX11Texture* GetNormal() const;
	r3dDX11Texture* GetSpecular() const;
	r3dDX11Texture* GetGlow() const;

private:
	r3dDX11Texture* Diffuse = nullptr;
	r3dDX11Texture* Normal = nullptr;
	r3dDX11Texture* Specular = nullptr;
	r3dDX11Texture* Glow = nullptr;
};
