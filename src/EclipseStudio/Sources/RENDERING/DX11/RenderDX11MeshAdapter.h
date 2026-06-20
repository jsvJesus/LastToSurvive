#pragma once

#include "RENDERING/DX11/RenderDX11Mesh.h"

class r3dMesh;

bool r3dDX11CreateMeshResourceFromR3DMesh(ID3D11Device* device, const r3dMesh& mesh, r3dDX11MeshResource& outResource, const char* debugName = nullptr);
