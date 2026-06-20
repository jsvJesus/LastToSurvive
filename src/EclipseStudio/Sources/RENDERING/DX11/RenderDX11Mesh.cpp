#include "r3dPCH.h"

#include "RENDERING/DX11/RenderDX11Mesh.h"

#include "RENDERING/DX11/RenderDX11Draw.h"
#include "RENDERING/DX11/RenderDX11GBufferPass.h"

#include <algorithm>
#include <cmath>

namespace
{
	float ClampFloat(float value, float minValue, float maxValue)
	{
		return std::max(minValue, std::min(maxValue, value));
	}

	short PackSnorm16(float value)
	{
		value = ClampFloat(value, -1.0f, 1.0f);
		return static_cast<short>(value >= 0.0f ? value * 32767.0f + 0.5f : value * 32767.0f - 0.5f);
	}

	unsigned char PackUnorm8(float value)
	{
		value = ClampFloat(value, 0.0f, 1.0f);
		return static_cast<unsigned char>(value * 255.0f + 0.5f);
	}

	float SafeScale(float value)
	{
		return std::fabs(value) > 0.000001f ? value : 1.0f;
	}
}

r3dDX11MeshResource::r3dDX11MeshResource()
{
}

r3dDX11MeshResource::~r3dDX11MeshResource()
{
	Shutdown();
}

bool r3dDX11MeshResource::Create(ID3D11Device* device, const r3dDX11MeshBuildDesc& desc, const char* debugName)
{
	Shutdown();

	if (!device || !desc.PositionsXYZ || !desc.NormalsXYZ || !desc.TexcoordsUV || !desc.TangentsXYZ || !desc.Indices || desc.NumVertices == 0 || desc.NumIndices == 0)
		return false;

	PositionScale[0] = SafeScale(desc.PositionScale[0]);
	PositionScale[1] = SafeScale(desc.PositionScale[1]);
	PositionScale[2] = SafeScale(desc.PositionScale[2]);
	TexcoordScale[0] = SafeScale(desc.TexcoordScale[0]);
	TexcoordScale[1] = SafeScale(desc.TexcoordScale[1]);

	std::vector<r3dDX11PackedMeshVertex> vertices;
	vertices.resize(desc.NumVertices);
	for (unsigned int i = 0; i < desc.NumVertices; ++i)
		vertices[i] = PackVertex(desc, i);

	if (!VertexBuffer.Create(device, vertices.size() * sizeof(vertices[0]), sizeof(vertices[0]), &vertices[0], R3D_DX11_BUFFER_IMMUTABLE, debugName ? debugName : "DX11.Mesh.VB"))
	{
		Shutdown();
		return false;
	}

	if (!IndexBuffer.Create(device, desc.NumIndices * sizeof(desc.Indices[0]), DXGI_FORMAT_R32_UINT, desc.Indices, R3D_DX11_BUFFER_IMMUTABLE, debugName ? debugName : "DX11.Mesh.IB"))
	{
		Shutdown();
		return false;
	}

	if (desc.NumBatches && desc.Batches)
	{
		Batches.assign(desc.Batches, desc.Batches + desc.NumBatches);
	}
	else
	{
		r3dDX11MeshBatch batch;
		batch.StartIndex = 0;
		batch.IndexCount = desc.NumIndices;
		batch.MaterialIndex = 0;
		Batches.push_back(batch);
	}

	VertexCount = desc.NumVertices;
	IndexCount = desc.NumIndices;
	return true;
}

void r3dDX11MeshResource::Shutdown()
{
	IndexBuffer.Shutdown();
	VertexBuffer.Shutdown();
	Batches.clear();
	VertexCount = 0;
	IndexCount = 0;
	PositionScale[0] = PositionScale[1] = PositionScale[2] = 1.0f;
	TexcoordScale[0] = TexcoordScale[1] = 1.0f;
}

void r3dDX11MeshResource::Bind(r3dDX11DrawContext& drawContext)
{
	drawContext.SetVertexBuffer(&VertexBuffer);
	drawContext.SetIndexBuffer(&IndexBuffer);
}

void r3dDX11MeshResource::Draw(r3dDX11DrawContext& drawContext)
{
	Bind(drawContext);
	drawContext.DrawIndexed(IndexCount);
}

void r3dDX11MeshResource::DrawBatch(r3dDX11DrawContext& drawContext, unsigned int batchIndex)
{
	const r3dDX11MeshBatch* batch = GetBatch(batchIndex);
	if (!batch)
		return;

	Bind(drawContext);
	drawContext.DrawIndexed(batch->IndexCount, batch->StartIndex);
}

void r3dDX11MeshResource::DrawBatch(r3dDX11GBufferPass& pass, unsigned int batchIndex)
{
	const r3dDX11MeshBatch* batch = GetBatch(batchIndex);
	if (batch)
		pass.DrawMesh(VertexBuffer, IndexBuffer, batch->IndexCount, batch->StartIndex);
}

const r3dDX11MeshBatch* r3dDX11MeshResource::GetBatch(unsigned int batchIndex) const
{
	if (batchIndex >= Batches.size())
		return nullptr;

	return &Batches[batchIndex];
}

unsigned int r3dDX11MeshResource::GetBatchCount() const
{
	return static_cast<unsigned int>(Batches.size());
}

unsigned int r3dDX11MeshResource::GetVertexCount() const
{
	return VertexCount;
}

unsigned int r3dDX11MeshResource::GetIndexCount() const
{
	return IndexCount;
}

const float* r3dDX11MeshResource::GetPositionScale() const
{
	return PositionScale;
}

const float* r3dDX11MeshResource::GetTexcoordScale() const
{
	return TexcoordScale;
}

bool r3dDX11MeshResource::IsValid() const
{
	return VertexBuffer.IsValid() && IndexBuffer.IsValid() && VertexCount > 0 && IndexCount > 0;
}

r3dDX11PackedMeshVertex r3dDX11MeshResource::PackVertex(const r3dDX11MeshBuildDesc& desc, unsigned int index)
{
	r3dDX11PackedMeshVertex vertex = {};

	const float* position = desc.PositionsXYZ + index * 3;
	const float* normal = desc.NormalsXYZ + index * 3;
	const float* texcoord = desc.TexcoordsUV + index * 2;
	const float* tangent = desc.TangentsXYZ + index * 3;

	vertex.Position[0] = PackSnorm16(position[0] / SafeScale(desc.PositionScale[0]));
	vertex.Position[1] = PackSnorm16(position[1] / SafeScale(desc.PositionScale[1]));
	vertex.Position[2] = PackSnorm16(position[2] / SafeScale(desc.PositionScale[2]));
	vertex.Position[3] = PackSnorm16(1.0f);

	vertex.TexCoord[0] = PackSnorm16(texcoord[0] / SafeScale(desc.TexcoordScale[0]));
	vertex.TexCoord[1] = PackSnorm16(texcoord[1] / SafeScale(desc.TexcoordScale[1]));

	vertex.Normal[0] = PackUnorm8(normal[0] * 0.5f + 0.5f);
	vertex.Normal[1] = PackUnorm8(normal[1] * 0.5f + 0.5f);
	vertex.Normal[2] = PackUnorm8(normal[2] * 0.5f + 0.5f);
	vertex.Normal[3] = 255;

	vertex.Tangent[0] = PackUnorm8(tangent[0] * 0.5f + 0.5f);
	vertex.Tangent[1] = PackUnorm8(tangent[1] * 0.5f + 0.5f);
	vertex.Tangent[2] = PackUnorm8(tangent[2] * 0.5f + 0.5f);
	vertex.Tangent[3] = 255;

	return vertex;
}
