#pragma once

class r3dDX11DrawContext;
class r3dDX11Texture;
class r3dDX11TextureLibrary;

enum r3dDX11MaterialDomain
{
	R3D_DX11_MATERIAL_MESH = 0,
	R3D_DX11_MATERIAL_VEGETATION,
	R3D_DX11_MATERIAL_TERRAIN
};

enum r3dDX11MaterialFlag
{
	R3D_DX11_MATERIAL_DRAW_GBUFFER = 1 << 0,
	R3D_DX11_MATERIAL_ALPHA_CUT = 1 << 1,
	R3D_DX11_MATERIAL_DOUBLE_SIDED = 1 << 2,
	R3D_DX11_MATERIAL_TRANSPARENT = 1 << 3,
	R3D_DX11_MATERIAL_SKIP_DRAW = 1 << 4,
	R3D_DX11_MATERIAL_CAMOUFLAGE = 1 << 5
};

struct r3dDX11MaterialConstants
{
	float MaterialParams[4];
	float MatDiffuse[4];
	float MatSpecular[4];
	float MatGlow[4];
	float Options[4];
	float Emissive[4];
};

struct r3dDX11MaterialDesc
{
	r3dDX11MaterialDesc();

	const char* TexturePaths[8];
	r3dDX11MaterialConstants Constants;
	unsigned int Flags;
	r3dDX11MaterialDomain Domain;
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
	bool ApplyDesc(r3dDX11TextureLibrary& textureLibrary, const r3dDX11MaterialDesc& desc);
	void SetConstants(const r3dDX11MaterialConstants& constants);
	r3dDX11MaterialConstants BuildConstants(unsigned int objectColorPacked = 0xffffffff) const;
	void SetDomain(r3dDX11MaterialDomain domain);
	void SetDrawInGBuffer(bool drawInGBuffer);
	void SetAlphaCut(bool alphaCut);
	void SetDoubleSided(bool doubleSided);
	void SetTransparent(bool transparent);
	void SetSkipDraw(bool skipDraw);
	void SetCamouflage(bool camouflage);
	bool IsCamouflage() const;
	r3dDX11MaterialDomain GetDomain() const;
	bool ShouldDrawInGBuffer() const;
	bool IsDoubleSided() const;
	bool IsAlphaCut() const;
	bool IsTransparent() const;
	bool IsSkipDraw() const;

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
	r3dDX11MaterialDomain Domain = R3D_DX11_MATERIAL_MESH;
	bool bDrawInGBuffer = true;
	bool bAlphaCut = false;
	bool bDoubleSided = false;
	bool bTransparent = false;
	bool bSkipDraw = false;
	bool bCamouflage = false;
};
