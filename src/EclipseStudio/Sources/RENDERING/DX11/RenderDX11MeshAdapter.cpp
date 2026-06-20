#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11MeshAdapter.h"

namespace
{
	bool BuildBatches(const r3dMesh& mesh, std::vector<r3dDX11MeshBatch>& batches)
	{
		batches.clear();
		if (mesh.NumMatChunks <= 0)
			return true;

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

		return true;
	}

	bool CanBuildFromCpuStreams(const r3dMesh& mesh)
	{
		return mesh.VertexPositions &&
			mesh.VertexNormals &&
			mesh.VertexUVs &&
			mesh.VertexTangents &&
			mesh.Indices;
	}

	bool TryCreateFromCpuStreams(ID3D11Device* device, const r3dMesh& mesh, r3dDX11MeshResource& outResource, const std::vector<r3dDX11MeshBatch>& batches, const char* debugName)
	{
		if (!CanBuildFromCpuStreams(mesh))
			return false;

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

	bool TryCreateFromD3D9PackedBuffers(ID3D11Device* device, const r3dMesh& mesh, r3dDX11MeshResource& outResource, const std::vector<r3dDX11MeshBatch>& batches, const char* debugName)
	{
		if (mesh.VertexFlags & r3dMesh::vfPrecise)
		{
			static bool logged = false;
			if (!logged)
			{
				r3dOutToLog("[DX11][Mesh] precise D3D9 mesh fallback is not implemented yet: %s\n", mesh.FileName.c_str());
				logged = true;
			}
			return false;
		}

		MeshGlobalBuffer::Entry& bufferEntry = const_cast<MeshGlobalBuffer::Entry&>(mesh.buffers);

		if (!bufferEntry.VB.Valid() || !bufferEntry.IB.Valid() || bufferEntry.vCount == 0 || bufferEntry.iCount == 0)
			return false;

		void* vertexData = nullptr;
		void* indexData = nullptr;

		bufferEntry.Lock(vertexData, indexData);
		const bool skinned =
			mesh.pWeights != nullptr ||
			bufferEntry.vbStride >= sizeof(r3dSkinnedMeshVertex);

		const float positionScale[3] =
		{
			mesh.unpackScale.x,
			mesh.unpackScale.y,
			mesh.unpackScale.z
		};

		const float texcoordScale[2] =
		{
			mesh.texcUnpackScale.x,
			mesh.texcUnpackScale.y
		};

		const bool created = outResource.CreateFromPacked(
			device,
			vertexData,
			bufferEntry.vbStride,
			bufferEntry.vCount,
			static_cast<const unsigned int*>(indexData),
			bufferEntry.iCount,
			batches.empty() ? nullptr : &batches[0],
			static_cast<unsigned int>(batches.size()),
			skinned,
			positionScale,
			texcoordScale,
			debugName ? debugName : mesh.FileName.c_str()
		);

		bufferEntry.Unlock();
		return created;
	}
}

bool r3dDX11CreateMeshResourceFromR3DMesh(ID3D11Device* device, const r3dMesh& mesh, r3dDX11MeshResource& outResource, const char* debugName)
{
	if (!device || !mesh.IsLoaded() || mesh.NumVertices <= 0 || mesh.NumIndices <= 0)
		return false;

	std::vector<r3dDX11MeshBatch> batches;
	BuildBatches(mesh, batches);

	if (TryCreateFromCpuStreams(device, mesh, outResource, batches, debugName))
		return true;

	return TryCreateFromD3D9PackedBuffers(device, mesh, outResource, batches, debugName);
}
