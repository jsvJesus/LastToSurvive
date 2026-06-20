#pragma once

class r3dMaterial;
class r3dDX11MaterialTextures;
class r3dDX11TextureLibrary;

bool r3dDX11CreateMaterialTexturesFromR3DMaterial(r3dDX11TextureLibrary& textureLibrary, const r3dMaterial& material, r3dDX11MaterialTextures& outTextures);
