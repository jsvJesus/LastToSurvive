#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11MeshAdapter.h"

bool r3dDX11CreateMeshResourceFromR3DMesh(ID3D11Device* device, const r3dMesh& mesh, r3dDX11MeshResource& outResource, const char* debugName)
{
	if (!device || !mesh.IsLoaded() || mesh.NumVertices <= 0 || mesh.NumIndices <= 0)
		return false;

	if (!mesh.VertexPositions || !mesh.VertexNormals || !mesh.VertexUVs || !mesh.VertexTangents || !mesh.Indices)
		return false;

	std::vector<r3dDX11MeshBatch> batches;
	if (mesh.NumMatChunks > 0)
	{
		batches.reserve(mesh.NumMatChunks);
		for (int i = 0; i < mesh.NumMatChunks; ++i)
		{
			const r3dTriBatch& sourceBatch = mesh.MatChunks[i];
			if (sourceBatch.EndIndex <= sourceBatch.StartIndex)
				continue;

			r3dDX11MeshBatch batch;
			batch.StartIndex = static_cast<unsigned int>(sourceBatch.StartIndex);
			batch.IndexCount = static_cast<unsigned int>(sourceBatch.EndIndex - sourceBatch.StartIndex);
			batch.MaterialIndex = static_cast<unsigned int>(i);
			batches.push_back(batch);
		}
	}

	r3dDX11MeshBuildDesc desc;
	desc.PositionsXYZ = reinterpret_cast<const float*>(mesh.VertexPositions);
	desc.NormalsXYZ = reinterpret_cast<const float*>(mesh.VertexNormals);
	desc.TexcoordsUV = reinterpret_cast<const float*>(mesh.VertexUVs);
	desc.TangentsXYZ = reinterpret_cast<const float*>(mesh.VertexTangents);
	desc.TangentSigns = mesh.VertexTangentWs;
	desc.Indices = mesh.Indices;
	desc.NumVertices = static_cast<unsigned int>(mesh.NumVertices);
	desc.NumIndices = static_cast<unsigned int>(mesh.NumIndices);
	desc.Batches = batches.empty() ? nullptr : &batches[0];
	desc.NumBatches = static_cast<unsigned int>(batches.size());
	desc.PositionScale[0] = mesh.unpackScale.x;
	desc.PositionScale[1] = mesh.unpackScale.y;
	desc.PositionScale[2] = mesh.unpackScale.z;
	desc.TexcoordScale[0] = mesh.texcUnpackScale.x;
	desc.TexcoordScale[1] = mesh.texcUnpackScale.y;
	if (mesh.pWeights)
	{
		desc.BlendIndices = reinterpret_cast<const unsigned char*>(&mesh.pWeights[0].BoneID[0]);
		desc.BlendWeights = reinterpret_cast<const float*>(&mesh.pWeights[0].Weight[0]);
		desc.BlendIndexStride = sizeof(mesh.pWeights[0]);
		desc.BlendWeightStride = sizeof(mesh.pWeights[0]);
	}

	return outResource.Create(device, desc, debugName ? debugName : mesh.FileName.c_str());
}
