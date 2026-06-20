#pragma once

class r3dDX11DrawContext;
class r3dDX11Texture;
class r3dDX11TextureLibrary;

struct r3dDX11MaterialConstants
{
	float MaterialParams[4];
	float MatDiffuse[4];
	float MatSpecular[4];
	float MatGlow[4];
	float Options[4];
};

class r3dDX11MaterialTextures final
{
public:
	r3dDX11MaterialTextures();

	bool Load(
		r3dDX11TextureLibrary& textureLibrary,
		const char* diffusePath,
		const char* normalPath,
		const char* glossPath,
		const char* envPowerPath,
		const char* selfIllumPath,
		const char* detailNormalPath,
		const char* sssPath,
		const char* specularPowerPath
	);

	void SetFallbacks(r3dDX11TextureLibrary& textureLibrary);
	void Bind(r3dDX11DrawContext& drawContext, unsigned int baseSlot = 0) const;
	void SetConstants(const r3dDX11MaterialConstants& constants);
	r3dDX11MaterialConstants BuildConstants(unsigned int objectColorPacked = 0xffffffff) const;
	void SetDrawInGBuffer(bool drawInGBuffer);
	void SetAlphaCut(bool alphaCut);
	void SetDoubleSided(bool doubleSided);
	bool ShouldDrawInGBuffer() const;
	bool IsDoubleSided() const;

	r3dDX11Texture* GetDiffuse() const;
	r3dDX11Texture* GetNormal() const;
	r3dDX11Texture* GetGloss() const;
	r3dDX11Texture* GetEnvPower() const;
	r3dDX11Texture* GetSelfIllum() const;
	r3dDX11Texture* GetDetailNormal() const;
	r3dDX11Texture* GetSSS() const;
	r3dDX11Texture* GetSpecularPower() const;

private:
	r3dDX11Texture* Diffuse = nullptr;
	r3dDX11Texture* Normal = nullptr;
	r3dDX11Texture* Gloss = nullptr;
	r3dDX11Texture* EnvPower = nullptr;
	r3dDX11Texture* SelfIllum = nullptr;
	r3dDX11Texture* DetailNormal = nullptr;
	r3dDX11Texture* SSS = nullptr;
	r3dDX11Texture* SpecularPower = nullptr;
	r3dDX11MaterialConstants Constants;
	bool bDrawInGBuffer = true;
	bool bAlphaCut = false;
	bool bDoubleSided = false;
};
