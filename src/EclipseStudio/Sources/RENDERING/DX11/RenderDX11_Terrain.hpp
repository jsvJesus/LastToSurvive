#pragma once

bool RenderDX11_UpdateTerrainVertices(
    ID3D11Buffer* VertexBuffer,
    int TileX,
    int TileZ,
    float PatchSize
);

void RenderDX11_BeginTerrainCacheFrame()
	{
		++gDX11TerrainCacheFrameId;

		if (gDX11TerrainCacheFrameId == 0)
		{
			gDX11TerrainCacheFrameId = 1;

			for (int i = 0; i < DX11_TERRAIN_PATCH_COUNT; ++i)
			{
				gDX11TerrainPatchCache[i].LastUsedFrame = 0;
			}
		}

		gDX11TerrainPatchDrawCount = 0;
		gDX11TerrainPatchUpdateCount = 0;
		gDX11TerrainPatchCullCount = 0;
	}

	bool RenderDX11_CreateTerrainResources()
	{
		bool bAllVBsReady = true;

		for (int i = 0; i < DX11_TERRAIN_PATCH_COUNT; ++i)
		{
			if (!gDX11TerrainPatchCache[i].VertexBuffer)
			{
				bAllVBsReady = false;
				break;
			}
		}

		if (bAllVBsReady && gDX11TerrainIB)
			return true;

		RenderDX11_ReleaseTerrainResources();

		for (int i = 0; i < DX11_TERRAIN_PATCH_COUNT; ++i)
		{
			D3D11_BUFFER_DESC VBDesc = {};
			VBDesc.ByteWidth =
				sizeof(WorldDX11TerrainVertex) *
				DX11_TERRAIN_VERTEX_COUNT;
			VBDesc.Usage = D3D11_USAGE_DYNAMIC;
			VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			VBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			HRESULT Hr =
				gDX11Device->CreateBuffer(
					&VBDesc,
					0,
					&gDX11TerrainPatchCache[i].VertexBuffer
				);

			if (FAILED(Hr))
			{
				char Text[256] = {};
				sprintf_s(
					Text,
					"[RenderDX11] Create terrain cache VB[%d] failed. HRESULT=0x%08X\n",
					i,
					static_cast<unsigned int>(Hr)
				);

				OutputDebugStringA(Text);
				RenderDX11_ReleaseTerrainResources();
				return false;
			}

			gDX11TerrainPatchCache[i].TileX = 0;
			gDX11TerrainPatchCache[i].TileZ = 0;
			gDX11TerrainPatchCache[i].Valid = false;
			gDX11TerrainPatchCache[i].LastUsedFrame = 0;
		}

		unsigned short Indices[DX11_TERRAIN_INDEX_COUNT] = {};
		int Index = 0;

		for (int z = 0; z < DX11_TERRAIN_GRID_DIM - 1; ++z)
		{
			for (int x = 0; x < DX11_TERRAIN_GRID_DIM - 1; ++x)
			{
				const unsigned short I0 =
					static_cast<unsigned short>(
						z * DX11_TERRAIN_GRID_DIM + x
					);
				const unsigned short I1 =
					static_cast<unsigned short>(
						z * DX11_TERRAIN_GRID_DIM + x + 1
					);
				const unsigned short I2 =
					static_cast<unsigned short>(
						(z + 1) * DX11_TERRAIN_GRID_DIM + x
					);
				const unsigned short I3 =
					static_cast<unsigned short>(
						(z + 1) * DX11_TERRAIN_GRID_DIM + x + 1
					);

				Indices[Index++] = I0;
				Indices[Index++] = I1;
				Indices[Index++] = I2;
				Indices[Index++] = I1;
				Indices[Index++] = I3;
				Indices[Index++] = I2;
			}
		}

		D3D11_BUFFER_DESC IBDesc = {};
		IBDesc.ByteWidth = sizeof(Indices);
		IBDesc.Usage = D3D11_USAGE_DEFAULT;
		IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA InitData = {};
		InitData.pSysMem = Indices;

		HRESULT Hr =
			gDX11Device->CreateBuffer(
				&IBDesc,
				&InitData,
				&gDX11TerrainIB
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Create terrain IB failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			RenderDX11_ReleaseTerrainResources();
			return false;
		}

		OutputDebugStringA(
			"[RenderDX11] Terrain DX11 3x3 cache buffers created\n"
		);

		return true;
	}

	bool RenderDX11_TerrainTileInRequest(
		const int* TileXs,
		const int* TileZs,
		int TileCount,
		int TileX,
		int TileZ
	)
	{
		for (int i = 0; i < TileCount; ++i)
		{
			if (
				TileXs[i] == TileX &&
				TileZs[i] == TileZ
			)
			{
				return true;
			}
		}

		return false;
	}

	int RenderDX11_FindTerrainCacheSlot(
		int TileX,
		int TileZ
	)
	{
		for (int i = 0; i < DX11_TERRAIN_PATCH_COUNT; ++i)
		{
			const WorldDX11TerrainPatchCacheEntry& Entry =
				gDX11TerrainPatchCache[i];

			if (
				Entry.Valid &&
				Entry.TileX == TileX &&
				Entry.TileZ == TileZ
			)
			{
				return i;
			}
		}

		return -1;
	}

	int RenderDX11_FindTerrainCacheSlotForWrite(
		const int* RequestTileXs,
		const int* RequestTileZs,
		int RequestTileCount
	)
	{
		for (int i = 0; i < DX11_TERRAIN_PATCH_COUNT; ++i)
		{
			if (!gDX11TerrainPatchCache[i].Valid)
				return i;
		}

		for (int i = 0; i < DX11_TERRAIN_PATCH_COUNT; ++i)
		{
			const WorldDX11TerrainPatchCacheEntry& Entry =
				gDX11TerrainPatchCache[i];

			if (!RenderDX11_TerrainTileInRequest(
				RequestTileXs,
				RequestTileZs,
				RequestTileCount,
				Entry.TileX,
				Entry.TileZ
			))
			{
				return i;
			}
		}

		return -1;
	}

	bool RenderDX11_EnsureTerrainCacheTile(
		int CacheIndex,
		int TileX,
		int TileZ,
		float PatchSize
	)
	{
		if (
			CacheIndex < 0 ||
			CacheIndex >= DX11_TERRAIN_PATCH_COUNT
		)
		{
			return false;
		}

		WorldDX11TerrainPatchCacheEntry& Entry =
			gDX11TerrainPatchCache[CacheIndex];

		if (!Entry.VertexBuffer)
			return false;

		if (
			Entry.Valid &&
			Entry.TileX == TileX &&
			Entry.TileZ == TileZ
		)
		{
			Entry.LastUsedFrame = gDX11TerrainCacheFrameId;
			return true;
		}

		if (!RenderDX11_UpdateTerrainVertices(
			Entry.VertexBuffer,
			TileX,
			TileZ,
			PatchSize
		))
		{
			Entry.Valid = false;
			Entry.LastUsedFrame = 0;
			return false;
		}

		Entry.TileX = TileX;
		Entry.TileZ = TileZ;
		Entry.Valid = true;
		Entry.LastUsedFrame = gDX11TerrainCacheFrameId;

		++gDX11TerrainPatchUpdateCount;

		return true;
	}

	void RenderDX11_NormalizeFrustumPlane(
		WorldDX11FrustumPlane& Plane
	)
	{
		const float Len =
			sqrtf(
				Plane.A * Plane.A +
				Plane.B * Plane.B +
				Plane.C * Plane.C
			);

		if (Len <= 0.00001f)
			return;

		const float InvLen = 1.0f / Len;

		Plane.A *= InvLen;
		Plane.B *= InvLen;
		Plane.C *= InvLen;
		Plane.D *= InvLen;
	}

	bool RenderDX11_BuildFrameFrustum(
		WorldDX11FrustumPlane* OutPlanes
	)
	{
		if (!OutPlanes || !r3dRenderer)
			return false;

		const D3DXMATRIX& M =
			r3dRenderer->ViewProjMatrix;

		// Left
		OutPlanes[0].A = M._14 + M._11;
		OutPlanes[0].B = M._24 + M._21;
		OutPlanes[0].C = M._34 + M._31;
		OutPlanes[0].D = M._44 + M._41;

		// Right
		OutPlanes[1].A = M._14 - M._11;
		OutPlanes[1].B = M._24 - M._21;
		OutPlanes[1].C = M._34 - M._31;
		OutPlanes[1].D = M._44 - M._41;

		// Bottom
		OutPlanes[2].A = M._14 + M._12;
		OutPlanes[2].B = M._24 + M._22;
		OutPlanes[2].C = M._34 + M._32;
		OutPlanes[2].D = M._44 + M._42;

		// Top
		OutPlanes[3].A = M._14 - M._12;
		OutPlanes[3].B = M._24 - M._22;
		OutPlanes[3].C = M._34 - M._32;
		OutPlanes[3].D = M._44 - M._42;

		// Near, D3D clip space z >= 0
		OutPlanes[4].A = M._13;
		OutPlanes[4].B = M._23;
		OutPlanes[4].C = M._33;
		OutPlanes[4].D = M._43;

		// Far, D3D clip space z <= w
		OutPlanes[5].A = M._14 - M._13;
		OutPlanes[5].B = M._24 - M._23;
		OutPlanes[5].C = M._34 - M._33;
		OutPlanes[5].D = M._44 - M._43;

		for (int i = 0; i < 6; ++i)
		{
			RenderDX11_NormalizeFrustumPlane(
				OutPlanes[i]
			);
		}

		return true;
	}

	bool RenderDX11_BuildTerrainPatchBounds(
		const r3dTerrainDesc& TerrainDesc,
		int TileX,
		int TileZ,
		float PatchSize,
		WorldDX11TerrainPatchBounds* OutBounds
	)
	{
		if (!OutBounds || !Terrain)
			return false;

		const float MinX =
			static_cast<float>(TileX) *
			PatchSize;

		const float MinZ =
			static_cast<float>(TileZ) *
			PatchSize;

		const float MaxX =
			R3D_MIN(
				MinX + PatchSize,
				TerrainDesc.XSize
			);

		const float MaxZ =
			R3D_MIN(
				MinZ + PatchSize,
				TerrainDesc.ZSize
			);

		float MinY =
			TerrainDesc.MinHeight;

		float MaxY =
			TerrainDesc.MaxHeight;

		Terrain->GetHeightRange(
			&MinY,
			&MaxY,
			r3dPoint2D(MinX, MinZ),
			r3dPoint2D(MaxX, MaxZ)
		);

		if (MinY > MaxY)
		{
			const float Tmp = MinY;
			MinY = MaxY;
			MaxY = Tmp;
		}

		// Small safety pad for terrain normal/height interpolation.
		MinY -= 4.0f;
		MaxY += 4.0f;

		OutBounds->MinX = MinX;
		OutBounds->MinY = MinY;
		OutBounds->MinZ = MinZ;
		OutBounds->MaxX = MaxX;
		OutBounds->MaxY = MaxY;
		OutBounds->MaxZ = MaxZ;

		return true;
	}

	bool RenderDX11_IsTerrainPatchVisible(
		const WorldDX11FrustumPlane* Planes,
		const WorldDX11TerrainPatchBounds& Bounds
	)
	{
		if (!Planes)
			return true;

		for (int i = 0; i < 6; ++i)
		{
			const WorldDX11FrustumPlane& Plane =
				Planes[i];

			const float X =
				Plane.A >= 0.0f
				? Bounds.MaxX
				: Bounds.MinX;

			const float Y =
				Plane.B >= 0.0f
				? Bounds.MaxY
				: Bounds.MinY;

			const float Z =
				Plane.C >= 0.0f
				? Bounds.MaxZ
				: Bounds.MinZ;

			const float Distance =
				Plane.A * X +
				Plane.B * Y +
				Plane.C * Z +
				Plane.D;

			if (Distance < 0.0f)
				return false;
		}

		return true;
	}

	bool RenderDX11_UpdateTerrainVertices(
		ID3D11Buffer* VertexBuffer,
		int TileX,
		int TileZ,
		float PatchSize
	)
	{
		if (
			!gDX11Context ||
			!VertexBuffer ||
			!Terrain
		)
		{
			return false;
		}

		if (!Terrain->IsLoaded())
			return false;

		D3D11_MAPPED_SUBRESOURCE Mapped = {};

		HRESULT Hr =
			gDX11Context->Map(
				VertexBuffer,
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&Mapped
			);

		if (FAILED(Hr))
		{
			char Text[256] = {};
			sprintf_s(
				Text,
				"[RenderDX11] Map terrain VB tile(%d,%d) failed. HRESULT=0x%08X\n",
				TileX,
				TileZ,
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			return false;
		}

		WorldDX11TerrainVertex* Vertices =
			reinterpret_cast<WorldDX11TerrainVertex*>(
				Mapped.pData
			);

		const float BaseX =
			static_cast<float>(TileX) *
			PatchSize;

		const float BaseZ =
			static_cast<float>(TileZ) *
			PatchSize;

		const float Step =
			PatchSize /
			static_cast<float>(DX11_TERRAIN_GRID_DIM - 1);

		const r3dTerrainDesc& TerrainDesc =
			Terrain->GetDesc();

		float NormalSampleStep =
			TerrainDesc.CellSize;

		if (NormalSampleStep <= 0.01f)
			NormalSampleStep = Step;

		int VertexIndex = 0;

		for (int z = 0; z < DX11_TERRAIN_GRID_DIM; ++z)
		{
			for (int x = 0; x < DX11_TERRAIN_GRID_DIM; ++x)
			{
				const float WorldX =
					BaseX +
					static_cast<float>(x) * Step;

				const float WorldZ =
					BaseZ +
					static_cast<float>(z) * Step;

				r3dPoint3D Pos(
					WorldX,
					0.0f,
					WorldZ
				);

				const float Height =
					Terrain->GetHeight(Pos);

				Pos.y = Height;

				r3dPoint3D Normal;

				if (!RenderDX11_ComputeTerrainNormalFromHeights(
					TerrainDesc,
					WorldX,
					WorldZ,
					NormalSampleStep,
					&Normal
				))
				{
					Normal =
						Terrain->GetNormal(
							Pos
						);

					if (!RenderDX11_NormalizeTerrainNormal(Normal))
					{
						Normal.Assign(
							0.0f,
							1.0f,
							0.0f
						);
					}
				}

				Vertices[VertexIndex].Position[0] = WorldX;
				Vertices[VertexIndex].Position[1] = Height;
				Vertices[VertexIndex].Position[2] = WorldZ;

				Vertices[VertexIndex].Normal[0] = Normal.x;
				Vertices[VertexIndex].Normal[1] = Normal.y;
				Vertices[VertexIndex].Normal[2] = Normal.z;

				++VertexIndex;
			}
		}

		gDX11Context->Unmap(
			VertexBuffer,
			0
		);

		return true;
	}

	bool RenderDX11_BindTerrainGeometry(
		ID3D11Buffer* VertexBuffer
	)
	{
		if (
			!gDX11Context ||
			!gDX11TerrainInputLayout ||
			!VertexBuffer ||
			!gDX11TerrainIB
		)
		{
			return false;
		}

		UINT Stride = sizeof(WorldDX11TerrainVertex);
		UINT Offset = 0;

		gDX11Context->IASetInputLayout(
			gDX11TerrainInputLayout
		);

		gDX11Context->IASetPrimitiveTopology(
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
		);

		gDX11Context->IASetVertexBuffers(
			0,
			1,
			&VertexBuffer,
			&Stride,
			&Offset
		);

		gDX11Context->IASetIndexBuffer(
			gDX11TerrainIB,
			DXGI_FORMAT_R16_UINT,
			0
		);

		return true;
	}

	bool RenderDX11_DrawTerrainPatchSet()
	{
		if (
			!gDX11Context ||
			!Terrain ||
			!Terrain->IsLoaded()
		)
		{
			return false;
		}

		const r3dTerrainDesc& TerrainDesc =
			Terrain->GetDesc();

		float CellSize = TerrainDesc.CellSize;

		if (CellSize <= 0.01f)
			CellSize = 1.0f;

		int CellsPerPatch = TerrainDesc.CellCountPerTile;

		if (CellsPerPatch <= 0)
			CellsPerPatch = DX11_TERRAIN_GRID_DIM - 1;

		float PatchSize =
			CellSize *
			static_cast<float>(CellsPerPatch);

		if (PatchSize <= CellSize)
		{
			PatchSize =
				CellSize *
				static_cast<float>(DX11_TERRAIN_GRID_DIM - 1);
		}

		const float MaxBaseX =
			R3D_MAX(0.0f, TerrainDesc.XSize - PatchSize);

		const float MaxBaseZ =
			R3D_MAX(0.0f, TerrainDesc.ZSize - PatchSize);

		const int MaxTileX =
			RenderDX11_ClampInt(
				static_cast<int>(floorf(MaxBaseX / PatchSize)),
				0,
				0x7fffffff
			);

		const int MaxTileZ =
			RenderDX11_ClampInt(
				static_cast<int>(floorf(MaxBaseZ / PatchSize)),
				0,
				0x7fffffff
			);

		const int TileCountX =
			RenderDX11_ClampInt(
				DX11_TERRAIN_PATCH_SIDE,
				1,
				MaxTileX + 1
			);

		const int TileCountZ =
			RenderDX11_ClampInt(
				DX11_TERRAIN_PATCH_SIDE,
				1,
				MaxTileZ + 1
			);

		const int CenterTileX =
			RenderDX11_ClampInt(
				static_cast<int>(floorf(gCam.x / PatchSize)),
				0,
				MaxTileX
			);

		const int CenterTileZ =
			RenderDX11_ClampInt(
				static_cast<int>(floorf(gCam.z / PatchSize)),
				0,
				MaxTileZ
			);

		int StartTileX =
			CenterTileX -
			TileCountX / 2;

		int StartTileZ =
			CenterTileZ -
			TileCountZ / 2;

		StartTileX =
			RenderDX11_ClampInt(
				StartTileX,
				0,
				MaxTileX - TileCountX + 1
			);

		StartTileZ =
			RenderDX11_ClampInt(
				StartTileZ,
				0,
				MaxTileZ - TileCountZ + 1
			);

		int RequestTileXs[DX11_TERRAIN_PATCH_COUNT] = {};
		int RequestTileZs[DX11_TERRAIN_PATCH_COUNT] = {};
		int RequestTileCount = 0;

		for (int z = 0; z < TileCountZ; ++z)
		{
			for (int x = 0; x < TileCountX; ++x)
			{
				if (RequestTileCount >= DX11_TERRAIN_PATCH_COUNT)
					return false;

				RequestTileXs[RequestTileCount] =
					StartTileX + x;

				RequestTileZs[RequestTileCount] =
					StartTileZ + z;

				++RequestTileCount;
			}
		}

		WorldDX11FrustumPlane FrustumPlanes[6] = {};
		const bool bHasFrustum =
			RenderDX11_BuildFrameFrustum(
				FrustumPlanes
			);

		gDX11TerrainPatchDrawCount = 0;
		gDX11TerrainPatchCullCount = 0;

		for (int i = 0; i < RequestTileCount; ++i)
		{
			const int TileX =
				RequestTileXs[i];

			const int TileZ =
				RequestTileZs[i];

			WorldDX11TerrainPatchBounds Bounds = {};

			if (RenderDX11_BuildTerrainPatchBounds(
				TerrainDesc,
				TileX,
				TileZ,
				PatchSize,
				&Bounds
			))
			{
				if (
					bHasFrustum &&
					!RenderDX11_IsTerrainPatchVisible(
						FrustumPlanes,
						Bounds
					)
				)
				{
					++gDX11TerrainPatchCullCount;
					continue;
				}
			}

			int CacheIndex =
				RenderDX11_FindTerrainCacheSlot(
					TileX,
					TileZ
				);

			if (CacheIndex < 0)
			{
				CacheIndex =
					RenderDX11_FindTerrainCacheSlotForWrite(
						RequestTileXs,
						RequestTileZs,
						RequestTileCount
					);
			}

			if (CacheIndex < 0)
				return false;

			if (!RenderDX11_EnsureTerrainCacheTile(
				CacheIndex,
				TileX,
				TileZ,
				PatchSize
			))
			{
				return false;
			}

			WorldDX11TerrainPatchCacheEntry& Entry =
				gDX11TerrainPatchCache[CacheIndex];

			if (!RenderDX11_BindTerrainGeometry(
				Entry.VertexBuffer
			))
			{
				return false;
			}

			gDX11Context->DrawIndexed(
				DX11_TERRAIN_INDEX_COUNT,
				0,
				0
			);

			++gDX11TerrainPatchDrawCount;
		}

		return RequestTileCount > 0;
	}

	bool RenderDX11_DrawTerrainDepth()
	{
		if (
			!gDX11Context ||
			!gDX11TerrainVS ||
			!RenderDX11_CreateTerrainResources()
		)
		{
			return false;
		}

		RenderDX11_UpdateTerrainCB();

		gDX11Context->VSSetShader(
			gDX11TerrainVS,
			0,
			0
		);

		gDX11Context->RSSetState(
			gDX11RasterSolidNoCull
		);

		gDX11Context->PSSetShader(
			0,
			0,
			0
		);

		return RenderDX11_DrawTerrainPatchSet();
	}