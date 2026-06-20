#pragma once

class r3dMaterial;
class r3dDX11MaterialTextures;
class r3dDX11TextureLibrary;
class r3dTexture;
struct r3dTerrainLayer;

bool r3dDX11CreateMaterialTexturesFromR3DMaterial(r3dDX11TextureLibrary& textureLibrary, const r3dMaterial& material, r3dDX11MaterialTextures& outTextures);
bool r3dDX11CreateMaterialTexturesFromTerrainLayer(r3dDX11TextureLibrary& textureLibrary, const r3dTerrainLayer& layer, float terrainSpecular, float defSSAO, r3dDX11MaterialTextures& outTextures);
bool r3dDX11CreateMaterialTexturesFromVegetationTexture(r3dDX11TextureLibrary& textureLibrary, const r3dTexture* diffuseTexture, float alphaRef, float tintStrength, bool doubleSided, r3dDX11MaterialTextures& outTextures);
