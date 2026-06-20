#pragma once

#include "RENDERING/DX11/RenderDX11Platform.h"

namespace r3dDX11VertexLayouts
{
	const D3D11_INPUT_ELEMENT_DESC* Mesh(unsigned int* count);
	const D3D11_INPUT_ELEMENT_DESC* BendingMesh(unsigned int* count);
	const D3D11_INPUT_ELEMENT_DESC* PreciseMesh(unsigned int* count);
	const D3D11_INPUT_ELEMENT_DESC* SkinnedMesh(unsigned int* count);
	const D3D11_INPUT_ELEMENT_DESC* InstancedMesh(unsigned int* count);
	const D3D11_INPUT_ELEMENT_DESC* InstancedSkinnedMesh(unsigned int* count);
}
