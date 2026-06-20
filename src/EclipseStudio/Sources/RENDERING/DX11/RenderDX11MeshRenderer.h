#pragma once

#include "RENDERING/DX11/RenderDX11GBufferPass.h"

class r3dMesh;
class r3dDX11TextureLibrary;
class r3dSkeleton;
struct D3DXMATRIX;
struct ID3D11Device;

bool r3dDX11PrepareMeshConstants(r3dDX11MeshConstants& outConstants, const D3DXMATRIX& world, const D3DXMATRIX& viewProj);
bool r3dDX11DrawMeshGBuffer(ID3D11Device* device, r3dDX11TextureLibrary& textureLibrary, r3dDX11GBufferPass& pass, r3dMesh& mesh, const D3DXMATRIX& world, const D3DXMATRIX& viewProj, unsigned int objectColorPacked = 0xffffffff, const r3dSkeleton* skeleton = nullptr);
bool r3dDX11DrawMeshGBufferBatch(ID3D11Device* device, r3dDX11TextureLibrary& textureLibrary, r3dDX11GBufferPass& pass, r3dMesh& mesh, unsigned int batchIndex, const D3DXMATRIX& world, const D3DXMATRIX& viewProj, unsigned int objectColorPacked = 0xffffffff, const r3dSkeleton* skeleton = nullptr);
