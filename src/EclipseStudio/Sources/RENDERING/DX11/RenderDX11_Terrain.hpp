#pragma once

bool RenderDX11_DrawSelectedTerrain(
	bool GBufferPass
);

bool RenderDX11_NormalizeTerrainNormal(
	r3dPoint3D& Normal
);

bool RenderDX11_ComputeTerrainNormalFromHeights(
	const r3dTerrainDesc& TerrainDesc,
	float WorldX,
	float WorldZ,
	float SampleStep,
	r3dPoint3D* OutNormal
);

bool RenderDX11_UpdateTerrainVertices(
	ID3D11Buffer* VertexBuffer,
	int TileX,
	int TileZ,
	float PatchSize
);

bool RenderDX11_NormalizeTerrainNormal(
	r3dPoint3D& Normal
)
{
	const float LenSq =
		Normal.x * Normal.x +
		Normal.y * Normal.y +
		Normal.z * Normal.z;

	if (!(LenSq > 0.000001f && LenSq < 1000000.0f))
		return false;

	const float InvLen =
		1.0f / sqrtf(LenSq);

	Normal.x *= InvLen;
	Normal.y *= InvLen;
	Normal.z *= InvLen;

	if (Normal.y < 0.0f)
	{
		Normal.x = -Normal.x;
		Normal.y = -Normal.y;
		Normal.z = -Normal.z;
	}

	return true;
}

bool RenderDX11_ComputeTerrainNormalFromHeights(
	const r3dTerrainDesc& TerrainDesc,
	float WorldX,
	float WorldZ,
	float SampleStep,
	r3dPoint3D* OutNormal
)
{
	if (!Terrain || !OutNormal)
		return false;

	if (SampleStep <= 0.01f)
		SampleStep = TerrainDesc.CellSize;

	if (SampleStep <= 0.01f)
		SampleStep = 1.0f;

	const float LeftX =
		R3D_MAX(
			0.0f,
			WorldX - SampleStep
		);

	const float RightX =
		R3D_MIN(
			TerrainDesc.XSize,
			WorldX + SampleStep
		);

	const float BackZ =
		R3D_MAX(
			0.0f,
			WorldZ - SampleStep
		);

	const float FrontZ =
		R3D_MIN(
			TerrainDesc.ZSize,
			WorldZ + SampleStep
		);

	const float DeltaX =
		RightX - LeftX;

	const float DeltaZ =
		FrontZ - BackZ;

	if (
		DeltaX <= 0.001f ||
		DeltaZ <= 0.001f
	)
	{
		return false;
	}

	r3dPoint3D PosLeft(
		LeftX,
		0.0f,
		WorldZ
	);

	r3dPoint3D PosRight(
		RightX,
		0.0f,
		WorldZ
	);

	r3dPoint3D PosBack(
		WorldX,
		0.0f,
		BackZ
	);

	r3dPoint3D PosFront(
		WorldX,
		0.0f,
		FrontZ
	);

	const float HeightLeft =
		Terrain->GetHeight(
			PosLeft
		);

	const float HeightRight =
		Terrain->GetHeight(
			PosRight
		);

	const float HeightBack =
		Terrain->GetHeight(
			PosBack
		);

	const float HeightFront =
		Terrain->GetHeight(
			PosFront
		);

	const float SlopeX =
		(HeightRight - HeightLeft) /
		DeltaX;

	const float SlopeZ =
		(HeightFront - HeightBack) /
		DeltaZ;

	r3dPoint3D Normal(
		-SlopeX,
		1.0f,
		-SlopeZ
	);

	if (!RenderDX11_NormalizeTerrainNormal(Normal))
		return false;

	*OutNormal = Normal;

	return true;
}

void RenderDX11_BeginTerrainCacheFrame()
	{
		++gDX11TerrainCacheFrameId;

		if (gDX11TerrainCacheFrameId == 0)
		{
			gDX11TerrainCacheFrameId = 1;

			for (int i = 0; i < DX11_TERRAIN_PATCH_CACHE_COUNT; ++i)
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

		for (int i = 0; i < DX11_TERRAIN_PATCH_CACHE_COUNT; ++i)
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

		for (int i = 0; i < DX11_TERRAIN_PATCH_CACHE_COUNT; ++i)
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
					"[DX11][Render] Create terrain cache VB[%d] failed. HRESULT=0x%08X\n",
					i,
					static_cast<unsigned int>(Hr)
				);

				OutputDebugStringA(Text);
				RenderDX11_ReleaseTerrainResources();
				return false;
			}

			gDX11TerrainPatchCache[i].TileX = 0;
			gDX11TerrainPatchCache[i].TileZ = 0;
			gDX11TerrainPatchCache[i].L = 0;
			gDX11TerrainPatchCache[i].ConFlags = 0;
			gDX11TerrainPatchCache[i].VertexDim = 0;
			gDX11TerrainPatchCache[i].PatchSize = 0.0f;
			gDX11TerrainPatchCache[i].VertexCapacity =
				DX11_TERRAIN_VERTEX_COUNT;
			gDX11TerrainPatchCache[i].IndexCapacity = 0;
			gDX11TerrainPatchCache[i].IndexCount = 0;
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
				"[DX11][Render] Create terrain IB failed. HRESULT=0x%08X\n",
				static_cast<unsigned int>(Hr)
			);

			OutputDebugStringA(Text);
			RenderDX11_ReleaseTerrainResources();
			return false;
		}

		OutputDebugStringA(
			"[DX11][Render] Terrain DX11 patch cache buffers created\n"
		);

		return true;
	}

	bool RenderDX11_TerrainTileInRequest(
		const int* TileXs,
		const int* TileZs,
		const int* TileLs,
		int TileCount,
		int TileX,
		int TileZ,
		int TileL
	)
	{
		for (int i = 0; i < TileCount; ++i)
		{
			if (
				TileXs[i] == TileX &&
				TileZs[i] == TileZ &&
				(!TileLs || TileLs[i] == TileL)
			)
			{
				return true;
			}
		}

		return false;
	}

	int RenderDX11_FindTerrainCacheSlot(
		int TileX,
		int TileZ,
		int TileL
	)
	{
		for (int i = 0; i < DX11_TERRAIN_PATCH_CACHE_COUNT; ++i)
		{
			const WorldDX11TerrainPatchCacheEntry& Entry =
				gDX11TerrainPatchCache[i];

			if (
				Entry.Valid &&
				Entry.TileX == TileX &&
				Entry.TileZ == TileZ &&
				Entry.L == TileL
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
		const int* RequestTileLs,
		int RequestTileCount
	)
	{
		for (int i = 0; i < DX11_TERRAIN_PATCH_CACHE_COUNT; ++i)
		{
			if (!gDX11TerrainPatchCache[i].Valid)
				return i;
		}

		for (int i = 0; i < DX11_TERRAIN_PATCH_CACHE_COUNT; ++i)
		{
			const WorldDX11TerrainPatchCacheEntry& Entry =
				gDX11TerrainPatchCache[i];

			if (!RenderDX11_TerrainTileInRequest(
				RequestTileXs,
				RequestTileZs,
				RequestTileLs,
				RequestTileCount,
				Entry.TileX,
				Entry.TileZ,
				Entry.L
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
		int TileL,
		float PatchSize
	)
	{
		if (
			CacheIndex < 0 ||
			CacheIndex >= DX11_TERRAIN_PATCH_CACHE_COUNT
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
			Entry.TileZ == TileZ &&
			Entry.L == TileL &&
			fabsf(Entry.PatchSize - PatchSize) <= 0.01f
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
		Entry.L = TileL;
		Entry.PatchSize = PatchSize;
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

		const float HorizontalPad =
			R3D_MAX(
				PatchSize * 0.20f,
				TerrainDesc.CellSize * 8.0f
			);

		const float HeightSpan =
			R3D_MAX(
				TerrainDesc.MaxHeight - TerrainDesc.MinHeight,
				0.0f
			);

		const float VerticalPad =
			R3D_MIN(
				R3D_MAX(
					HeightSpan * 0.08f,
					24.0f
				),
				160.0f
			);

		OutBounds->MinX = MinX - HorizontalPad;
		OutBounds->MinY = MinY - VerticalPad;
		OutBounds->MinZ = MinZ - HorizontalPad;
		OutBounds->MaxX = MaxX + HorizontalPad;
		OutBounds->MaxY = MaxY + VerticalPad;
		OutBounds->MaxZ = MaxZ + HorizontalPad;

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

	void RenderDX11_ReleaseTerrainV3HeightSamples()
	{
		delete [] gDX11TerrainV3HeightSamples;
		gDX11TerrainV3HeightSamples = 0;
		gDX11TerrainV3HeightSampleCountX = 0;
		gDX11TerrainV3HeightSampleCountZ = 0;
	}

	bool RenderDX11_LoadTerrainV3HeightSamples(
		const char* HomeDir
	)
	{
		RenderDX11_ReleaseTerrainV3HeightSamples();

		if (
			!gDX11TerrainV3Desc.UseRawHeightGrid ||
			!gDX11TerrainV3Desc.HeightFile[0]
		)
		{
			return false;
		}

		char HeightPath[512] = {};
		sprintf_s(
			HeightPath,
			"%s\\TerrainV3\\%s",
			HomeDir,
			gDX11TerrainV3Desc.HeightFile
		);

		FILE* HeightFile = fopen(HeightPath, "rb");
		if (!HeightFile)
			return false;

		unsigned int Magic = 0;
		unsigned int Version = 0;
		unsigned int Width = 0;
		unsigned int Height = 0;
		float CellSize = 0.0f;
		float HeightAmplitude = 0.0f;

		if (
			fread(&Magic, sizeof(Magic), 1, HeightFile) != 1 ||
			fread(&Version, sizeof(Version), 1, HeightFile) != 1 ||
			fread(&Width, sizeof(Width), 1, HeightFile) != 1 ||
			fread(&Height, sizeof(Height), 1, HeightFile) != 1 ||
			fread(&CellSize, sizeof(CellSize), 1, HeightFile) != 1 ||
			fread(&HeightAmplitude, sizeof(HeightAmplitude), 1, HeightFile) != 1
		)
		{
			fclose(HeightFile);
			return false;
		}

		if (
			Magic != 0x33485654 ||
			Version != 1 ||
			Width < 2 ||
			Height < 2 ||
			Width > 16385 ||
			Height > 16385
		)
		{
			fclose(HeightFile);
			r3dOutToLog(
				"[DX11][TerrainV3] Invalid height file header: %s\n",
				HeightPath
			);
			return false;
		}

		const unsigned long long SampleCount64 =
			static_cast<unsigned long long>(Width) *
			static_cast<unsigned long long>(Height);
		const size_t SampleCount =
			static_cast<size_t>(SampleCount64);

		float* Samples = new float[SampleCount];
		if (!Samples)
		{
			fclose(HeightFile);
			return false;
		}

		if (fread(Samples, sizeof(float), SampleCount, HeightFile) != SampleCount)
		{
			fclose(HeightFile);
			delete [] Samples;
			r3dOutToLog(
				"[DX11][TerrainV3] Failed to read height samples: %s\n",
				HeightPath
			);
			return false;
		}

		fclose(HeightFile);

		gDX11TerrainV3HeightSamples = Samples;
		gDX11TerrainV3HeightSampleCountX = static_cast<int>(Width);
		gDX11TerrainV3HeightSampleCountZ = static_cast<int>(Height);

		gDX11TerrainV3Desc.HeightWidth = static_cast<int>(Width);
		gDX11TerrainV3Desc.HeightHeight = static_cast<int>(Height);

		r3dOutToLog(
			"[DX11][TerrainV3] Loaded raw height grid: %s (%ux%u)\n",
			HeightPath,
			Width,
			Height
		);
		return true;
	}

	void RenderDX11_EnsureTerrainV3Desc()
	{
		const char* HomeDir =
			r3dGameLevel::GetHomeDir();

		if (!HomeDir)
			HomeDir = "";

		if (
			gDX11TerrainV3Desc.Initialized &&
			strcmp(gDX11TerrainV3Desc.SourceDir, HomeDir) == 0
		)
		{
			return;
		}

		RenderDX11_ReleaseTerrainV3HeightSamples();

		memset(
			&gDX11TerrainV3Desc,
			0,
			sizeof(gDX11TerrainV3Desc)
		);
		gDX11TerrainV3Desc.Initialized = true;
		r3dscpy(
			gDX11TerrainV3Desc.SourceDir,
			HomeDir
		);
		gDX11TerrainV3PathLogged = false;
		gDX11TerrainV3Desc.SizeX =
			DX11_TERRAIN_V3_DEFAULT_WORLD_SIZE;
		gDX11TerrainV3Desc.SizeZ =
			DX11_TERRAIN_V3_DEFAULT_WORLD_SIZE;
		gDX11TerrainV3Desc.CellSize =
			DX11_TERRAIN_V3_DEFAULT_CELL_SIZE;
		gDX11TerrainV3Desc.ChunkCells =
			DX11_TERRAIN_V3_DEFAULT_CHUNK_CELLS;
		gDX11TerrainV3Desc.BaseHeight = 0.0f;
		gDX11TerrainV3Desc.HeightAmplitude = 42.0f;
		gDX11TerrainV3Desc.MinHeight = -80.0f;
		gDX11TerrainV3Desc.MaxHeight = 120.0f;
		r3dscpy(
			gDX11TerrainV3Desc.HeightFile,
			"height.v3"
		);

		char DescPath[512] = {};
		sprintf_s(
			DescPath,
			"%s\\TerrainV3\\terrain_v3.desc",
			HomeDir
		);

		FILE* DescFile = fopen(DescPath, "rt");
		gDX11TerrainV3Desc.DescriptorFound = DescFile != 0;

		if (DescFile)
		{
			char Key[128] = {};
			char Value[256] = {};

			while (fscanf(DescFile, "%127s %255s", Key, Value) == 2)
			{
				if (strcmp(Key, "size_x") == 0)
					gDX11TerrainV3Desc.SizeX = static_cast<float>(atof(Value));
				else if (strcmp(Key, "size_z") == 0)
					gDX11TerrainV3Desc.SizeZ = static_cast<float>(atof(Value));
				else if (strcmp(Key, "cell_size") == 0)
					gDX11TerrainV3Desc.CellSize = static_cast<float>(atof(Value));
				else if (strcmp(Key, "chunk_cells") == 0)
					gDX11TerrainV3Desc.ChunkCells = atoi(Value);
				else if (strcmp(Key, "base_height") == 0)
					gDX11TerrainV3Desc.BaseHeight = static_cast<float>(atof(Value));
				else if (strcmp(Key, "height_amplitude") == 0)
					gDX11TerrainV3Desc.HeightAmplitude = static_cast<float>(atof(Value));
				else if (strcmp(Key, "min_height") == 0)
					gDX11TerrainV3Desc.MinHeight = static_cast<float>(atof(Value));
				else if (strcmp(Key, "max_height") == 0)
					gDX11TerrainV3Desc.MaxHeight = static_cast<float>(atof(Value));
				else if (strcmp(Key, "height_source") == 0)
					gDX11TerrainV3Desc.UseRawHeightGrid = strcmp(Value, "raw_f32_grid") == 0;
				else if (strcmp(Key, "height_file") == 0)
					r3dscpy(gDX11TerrainV3Desc.HeightFile, Value);
				else if (strcmp(Key, "height_width") == 0)
					gDX11TerrainV3Desc.HeightWidth = atoi(Value);
				else if (strcmp(Key, "height_height") == 0)
					gDX11TerrainV3Desc.HeightHeight = atoi(Value);
			}

			fclose(DescFile);
		}

		if (gDX11TerrainV3Desc.SizeX <= 1.0f)
			gDX11TerrainV3Desc.SizeX =
				DX11_TERRAIN_V3_DEFAULT_WORLD_SIZE;

		if (gDX11TerrainV3Desc.SizeZ <= 1.0f)
			gDX11TerrainV3Desc.SizeZ =
				DX11_TERRAIN_V3_DEFAULT_WORLD_SIZE;

		if (gDX11TerrainV3Desc.CellSize <= 0.01f)
			gDX11TerrainV3Desc.CellSize =
				DX11_TERRAIN_V3_DEFAULT_CELL_SIZE;

		if (gDX11TerrainV3Desc.ChunkCells <= 0)
			gDX11TerrainV3Desc.ChunkCells =
				DX11_TERRAIN_V3_DEFAULT_CHUNK_CELLS;

		RenderDX11_LoadTerrainV3HeightSamples(HomeDir);

		for (int i = 0; i < DX11_TERRAIN_PATCH_CACHE_COUNT; ++i)
			gDX11TerrainPatchCache[i].Valid = false;

		OutputDebugStringA(
			"[DX11][TerrainV3] Initialized independent TerrainV3 "
			"descriptor: no TerrainV2/Terrain2 data provider\n"
		);
	}

	float RenderDX11_TerrainV3Height(
		float WorldX,
		float WorldZ
	)
	{
		RenderDX11_EnsureTerrainV3Desc();

		if (
			gDX11TerrainV3HeightSamples &&
			gDX11TerrainV3HeightSampleCountX > 1 &&
			gDX11TerrainV3HeightSampleCountZ > 1 &&
			gDX11TerrainV3Desc.CellSize > 0.01f
		)
		{
			const float GridX =
				R3D_MAX(
					0.0f,
					R3D_MIN(
						WorldX / gDX11TerrainV3Desc.CellSize,
						static_cast<float>(gDX11TerrainV3HeightSampleCountX - 1)
					)
				);
			const float GridZ =
				R3D_MAX(
					0.0f,
					R3D_MIN(
						WorldZ / gDX11TerrainV3Desc.CellSize,
						static_cast<float>(gDX11TerrainV3HeightSampleCountZ - 1)
					)
				);

			const int X0 =
				RenderDX11_ClampInt(
					static_cast<int>(floorf(GridX)),
					0,
					gDX11TerrainV3HeightSampleCountX - 1
				);
			const int Z0 =
				RenderDX11_ClampInt(
					static_cast<int>(floorf(GridZ)),
					0,
					gDX11TerrainV3HeightSampleCountZ - 1
				);
			const int X1 =
				RenderDX11_ClampInt(
					X0 + 1,
					0,
					gDX11TerrainV3HeightSampleCountX - 1
				);
			const int Z1 =
				RenderDX11_ClampInt(
					Z0 + 1,
					0,
					gDX11TerrainV3HeightSampleCountZ - 1
				);

			const float Fx = GridX - static_cast<float>(X0);
			const float Fz = GridZ - static_cast<float>(Z0);

			const int Row0 =
				Z0 * gDX11TerrainV3HeightSampleCountX;
			const int Row1 =
				Z1 * gDX11TerrainV3HeightSampleCountX;

			const float H00 =
				gDX11TerrainV3HeightSamples[Row0 + X0];
			const float H10 =
				gDX11TerrainV3HeightSamples[Row0 + X1];
			const float H01 =
				gDX11TerrainV3HeightSamples[Row1 + X0];
			const float H11 =
				gDX11TerrainV3HeightSamples[Row1 + X1];

			const float H0 =
				H00 + (H10 - H00) * Fx;
			const float H1 =
				H01 + (H11 - H01) * Fx;

			return H0 + (H1 - H0) * Fz;
		}

		const float Amp =
			gDX11TerrainV3Desc.HeightAmplitude;

		const float Low =
			sinf(WorldX * 0.0017f) *
			cosf(WorldZ * 0.0013f);

		const float Mid =
			sinf((WorldX + WorldZ) * 0.0031f) * 0.35f;

		const float Detail =
			cosf(WorldX * 0.009f + WorldZ * 0.004f) * 0.08f;

		return
			gDX11TerrainV3Desc.BaseHeight +
			(Low + Mid + Detail) * Amp;
	}

	r3dPoint3D RenderDX11_TerrainV3Normal(
		float WorldX,
		float WorldZ
	)
	{
		RenderDX11_EnsureTerrainV3Desc();

		float Step = gDX11TerrainV3Desc.CellSize;
		if (Step <= 0.01f)
			Step = 1.0f;

		const float HeightL =
			RenderDX11_TerrainV3Height(
				WorldX - Step,
				WorldZ
			);
		const float HeightR =
			RenderDX11_TerrainV3Height(
				WorldX + Step,
				WorldZ
			);
		const float HeightB =
			RenderDX11_TerrainV3Height(
				WorldX,
				WorldZ - Step
			);
		const float HeightF =
			RenderDX11_TerrainV3Height(
				WorldX,
				WorldZ + Step
			);

		r3dPoint3D Normal(
			HeightL - HeightR,
			Step * 2.0f,
			HeightB - HeightF
		);

		if (!RenderDX11_NormalizeTerrainNormal(Normal))
		{
			Normal.Assign(
				0.0f,
				1.0f,
				0.0f
			);
		}

		return Normal;
	}

	bool RenderDX11_WriteTerrainV3CB()
	{
		if (!gDX11Context || !gDX11TerrainCB)
			return false;

		RenderDX11_EnsureTerrainV3Desc();

		WorldDX11TerrainCB TerrainCB = {};

		TerrainCB.BaseColor[0] = 0.12f;
		TerrainCB.BaseColor[1] = 0.28f;
		TerrainCB.BaseColor[2] = 0.10f;
		TerrainCB.BaseColor[3] = 1.0f;

		TerrainCB.ColorScale[0] = 0.22f;
		TerrainCB.ColorScale[1] = 0.18f;
		TerrainCB.ColorScale[2] = 0.18f;
		TerrainCB.ColorScale[3] = 1.0f;

		const float HeightRange =
			R3D_MAX(
				gDX11TerrainV3Desc.MaxHeight -
				gDX11TerrainV3Desc.MinHeight,
				1.0f
			);

		TerrainCB.DebugParams[0] =
			-gDX11TerrainV3Desc.MinHeight;
		TerrainCB.DebugParams[1] =
			1.0f / HeightRange;
		TerrainCB.DebugParams[2] = 0.0f;
		TerrainCB.DebugParams[3] = 0.0f;

		TerrainCB.TerrainSize[0] =
			gDX11TerrainV3Desc.SizeX;
		TerrainCB.TerrainSize[1] =
			gDX11TerrainV3Desc.SizeZ;
		TerrainCB.TerrainSize[2] =
			gDX11TerrainV3Desc.SizeX > 0.01f
			? 1.0f / gDX11TerrainV3Desc.SizeX
			: 1.0f;
		TerrainCB.TerrainSize[3] =
			gDX11TerrainV3Desc.SizeZ > 0.01f
			? 1.0f / gDX11TerrainV3Desc.SizeZ
			: 1.0f;

		D3D11_MAPPED_SUBRESOURCE Mapped = {};
		HRESULT Hr =
			gDX11Context->Map(
				gDX11TerrainCB,
				0,
				D3D11_MAP_WRITE_DISCARD,
				0,
				&Mapped
			);

		if (FAILED(Hr))
			return false;

		memcpy(
			Mapped.pData,
			&TerrainCB,
			sizeof(TerrainCB)
		);

		gDX11Context->Unmap(
			gDX11TerrainCB,
			0
		);

		RenderDX11_BindTerrainCB();
		return true;
	}

	bool RenderDX11_EnsureTerrainV3VertexBuffer(
		WorldDX11TerrainPatchCacheEntry& Entry
	)
	{
		if (!gDX11Device)
			return false;

		if (
			Entry.VertexBuffer &&
			Entry.VertexCapacity >= DX11_TERRAIN_VERTEX_COUNT
		)
		{
			return true;
		}

		RenderDX11_SafeRelease(Entry.VertexBuffer);

		D3D11_BUFFER_DESC VBDesc = {};
		VBDesc.ByteWidth =
			sizeof(WorldDX11TerrainVertex) *
			DX11_TERRAIN_VERTEX_COUNT;
		VBDesc.Usage = D3D11_USAGE_DYNAMIC;
		VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		VBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		if (FAILED(gDX11Device->CreateBuffer(
			&VBDesc,
			0,
			&Entry.VertexBuffer
		)))
		{
			Entry.VertexCapacity = 0;
			return false;
		}

		Entry.VertexCapacity = DX11_TERRAIN_VERTEX_COUNT;
		return true;
	}

	bool RenderDX11_UpdateTerrainV3ChunkVertices(
		ID3D11Buffer* VertexBuffer,
		int ChunkX,
		int ChunkZ,
		float ChunkSize
	)
	{
		if (!gDX11Context || !VertexBuffer)
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
			return false;

		WorldDX11TerrainVertex* Vertices =
			reinterpret_cast<WorldDX11TerrainVertex*>(
				Mapped.pData
			);

		const float BaseX =
			static_cast<float>(ChunkX) *
			ChunkSize;

		const float BaseZ =
			static_cast<float>(ChunkZ) *
			ChunkSize;

		const float Step =
			ChunkSize /
			static_cast<float>(DX11_TERRAIN_GRID_DIM - 1);

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

				const float Height =
					RenderDX11_TerrainV3Height(
						WorldX,
						WorldZ
					);

				const r3dPoint3D Normal =
					RenderDX11_TerrainV3Normal(
						WorldX,
						WorldZ
					);

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

	bool RenderDX11_EnsureTerrainV3Chunk(
		int CacheIndex,
		int ChunkX,
		int ChunkZ,
		float ChunkSize
	)
	{
		if (
			CacheIndex < 0 ||
			CacheIndex >= DX11_TERRAIN_PATCH_CACHE_COUNT
		)
		{
			return false;
		}

		WorldDX11TerrainPatchCacheEntry& Entry =
			gDX11TerrainPatchCache[CacheIndex];

		if (!RenderDX11_EnsureTerrainV3VertexBuffer(Entry))
			return false;

		if (
			Entry.Valid &&
			Entry.TileX == ChunkX &&
			Entry.TileZ == ChunkZ &&
			Entry.L == DX11_TERRAIN_V3_CACHE_L &&
			fabsf(Entry.PatchSize - ChunkSize) <= 0.01f
		)
		{
			Entry.LastUsedFrame = gDX11TerrainCacheFrameId;
			return true;
		}

		if (!RenderDX11_UpdateTerrainV3ChunkVertices(
			Entry.VertexBuffer,
			ChunkX,
			ChunkZ,
			ChunkSize
		))
		{
			Entry.Valid = false;
			return false;
		}

		Entry.TileX = ChunkX;
		Entry.TileZ = ChunkZ;
		Entry.L = DX11_TERRAIN_V3_CACHE_L;
		Entry.ConFlags = 0;
		Entry.VertexDim = DX11_TERRAIN_GRID_DIM - 1;
		Entry.PatchSize = ChunkSize;
		Entry.IndexCount = DX11_TERRAIN_INDEX_COUNT;
		Entry.Valid = true;
		Entry.LastUsedFrame = gDX11TerrainCacheFrameId;

		++gDX11TerrainPatchUpdateCount;
		return true;
	}

	bool RenderDX11_BuildTerrainV3ChunkBounds(
		int ChunkX,
		int ChunkZ,
		float ChunkSize,
		WorldDX11TerrainPatchBounds* OutBounds
	)
	{
		if (!OutBounds)
			return false;

		RenderDX11_EnsureTerrainV3Desc();

		const float MinX =
			static_cast<float>(ChunkX) *
			ChunkSize;
		const float MinZ =
			static_cast<float>(ChunkZ) *
			ChunkSize;

		OutBounds->MinX = MinX;
		OutBounds->MinY = gDX11TerrainV3Desc.MinHeight;
		OutBounds->MinZ = MinZ;
		OutBounds->MaxX = MinX + ChunkSize;
		OutBounds->MaxY = gDX11TerrainV3Desc.MaxHeight;
		OutBounds->MaxZ = MinZ + ChunkSize;

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
				"[DX11][Render] Map terrain VB tile(%d,%d) failed. HRESULT=0x%08X\n",
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

		const float CellSize =
			TerrainDesc.CellSize > 0.01f
			? TerrainDesc.CellSize
			: 1.0f;

		const int BaseCellX =
			static_cast<int>(
				floorf(BaseX / CellSize + 0.5f)
			);

		const int BaseCellZ =
			static_cast<int>(
				floorf(BaseZ / CellSize + 0.5f)
			);

		const int CellStep =
			R3D_MAX(
				static_cast<int>(
					floorf(Step / CellSize + 0.5f)
				),
				1
			);

		float NormalSampleStep =
			TerrainDesc.CellSize;

		if (NormalSampleStep <= 0.01f)
			NormalSampleStep = Step;

		int VertexIndex = 0;

		for (int z = 0; z < DX11_TERRAIN_GRID_DIM; ++z)
		{
			for (int x = 0; x < DX11_TERRAIN_GRID_DIM; ++x)
			{
				const int HeightCellX =
					R3D_MIN(
						BaseCellX + x * CellStep,
						TerrainDesc.CellCountX - 1
					);

				const int HeightCellZ =
					R3D_MIN(
						BaseCellZ + z * CellStep,
						TerrainDesc.CellCountZ - 1
					);

				const float WorldX =
					BaseX +
					static_cast<float>(x) * Step;

				const float WorldZ =
					BaseZ +
					static_cast<float>(z) * Step;

				const float Height =
					Terrain->GetHeight(
						HeightCellX,
						HeightCellZ
					);

				r3dPoint3D Pos(
					WorldX,
					Height,
					WorldZ
				);

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

	bool RenderDX11_EnsureTerrain2HeightSamples()
	{
		if (!Terrain2)
			return false;

		const r3dTerrainDesc& TerrainDesc =
			Terrain2->GetDesc();

		const int SampleCountX =
			TerrainDesc.CellCountX;
		const int SampleCountZ =
			TerrainDesc.CellCountZ;
		const int SampleCount =
			SampleCountX * SampleCountZ;

		if (
			SampleCountX <= 0 ||
			SampleCountZ <= 0 ||
			SampleCount <= 0
		)
		{
			return false;
		}

		if (
			gDX11Terrain2HeightSource == Terrain2 &&
			gDX11Terrain2HeightSampleCountX == SampleCountX &&
			gDX11Terrain2HeightSampleCountZ == SampleCountZ &&
			gDX11Terrain2HeightSamples.Count() == SampleCount
		)
		{
			return true;
		}

		gDX11Terrain2HeightSamples.Resize(SampleCount);
		Terrain2->SaveHeightField(
			&gDX11Terrain2HeightSamples
		);

		if (gDX11Terrain2HeightSamples.Count() != SampleCount)
		{
			gDX11Terrain2HeightSamples.Clear();
			gDX11Terrain2HeightSource = 0;
			gDX11Terrain2HeightSampleCountX = 0;
			gDX11Terrain2HeightSampleCountZ = 0;
			return false;
		}

		gDX11Terrain2HeightSource = Terrain2;
		gDX11Terrain2HeightSampleCountX = SampleCountX;
		gDX11Terrain2HeightSampleCountZ = SampleCountZ;

		char Text[256] = {};
		sprintf_s(
			Text,
			"[DX11][Render] Terrain2 height field cached: %dx%d "
			"(native world-space samples)\n",
			SampleCountX,
			SampleCountZ
		);
		OutputDebugStringA(Text);

		return true;
	}

	float RenderDX11_GetTerrain2HeightSample(
		int CellX,
		int CellZ
	)
	{
		CellX = RenderDX11_ClampInt(
			CellX,
			0,
			gDX11Terrain2HeightSampleCountX - 1
		);
		CellZ = RenderDX11_ClampInt(
			CellZ,
			0,
			gDX11Terrain2HeightSampleCountZ - 1
		);

		return gDX11Terrain2HeightSamples[
			CellZ * gDX11Terrain2HeightSampleCountX +
			CellX
		];
	}

	void RenderDX11_WriteTerrain2NativeVertex(
		WorldDX11TerrainVertex* Vertex,
		int CellX,
		int CellZ,
		float CellSize
	)
	{
		const float HeightL =
			RenderDX11_GetTerrain2HeightSample(
				CellX - 1,
				CellZ
			);
		const float HeightR =
			RenderDX11_GetTerrain2HeightSample(
				CellX + 1,
				CellZ
			);
		const float HeightB =
			RenderDX11_GetTerrain2HeightSample(
				CellX,
				CellZ - 1
			);
		const float HeightF =
			RenderDX11_GetTerrain2HeightSample(
				CellX,
				CellZ + 1
			);

		float NormalX = HeightL - HeightR;
		float NormalY = CellSize * 2.0f;
		float NormalZ = HeightB - HeightF;
		const float NormalLength =
			sqrtf(
				NormalX * NormalX +
				NormalY * NormalY +
				NormalZ * NormalZ
			);

		if (NormalLength > 0.00001f)
		{
			const float InvNormalLength =
				1.0f / NormalLength;
			NormalX *= InvNormalLength;
			NormalY *= InvNormalLength;
			NormalZ *= InvNormalLength;
		}
		else
		{
			NormalX = 0.0f;
			NormalY = 1.0f;
			NormalZ = 0.0f;
		}

		Vertex->Position[0] = CellX * CellSize;
		Vertex->Position[1] =
			RenderDX11_GetTerrain2HeightSample(
				CellX,
				CellZ
			);
		Vertex->Position[2] = CellZ * CellSize;
		Vertex->Normal[0] = NormalX;
		Vertex->Normal[1] = NormalY;
		Vertex->Normal[2] = NormalZ;
	}

	void RenderDX11_WriteTerrain2SkirtVertex(
		WorldDX11TerrainVertex* Vertex,
		int CellX,
		int CellZ,
		float CellSize,
		float SkirtBottomY
	)
	{
		Vertex->Position[0] = CellX * CellSize;
		Vertex->Position[1] = SkirtBottomY;
		Vertex->Position[2] = CellZ * CellSize;

		// Negative Y is used by the DX11 terrain VS to keep skirt vertices
		// from being snapped back to the height texture surface.
		Vertex->Normal[0] = 0.0f;
		Vertex->Normal[1] = -1.0f;
		Vertex->Normal[2] = 0.0f;
	}

	int RenderDX11_GetTerrain2SkirtIndexCount(
		int VertexDim
	)
	{
		return VertexDim > 0
			? VertexDim * 4 * 6
			: 0;
	}

	int RenderDX11_GetTerrain2SkirtVertexCount(
		int VertexDim
	)
	{
		return VertexDim > 0
			? (VertexDim + 1) * 4
			: 0;
	}

	void RenderDX11_AppendTerrain2SkirtQuad(
		unsigned short* Indices,
		int& IndexWrite,
		int IndexCapacity,
		int Top0,
		int Top1,
		int Bottom0,
		int Bottom1
	)
	{
		if (IndexWrite + 6 > IndexCapacity)
			return;

		Indices[IndexWrite++] =
			static_cast<unsigned short>(Top0);
		Indices[IndexWrite++] =
			static_cast<unsigned short>(Bottom0);
		Indices[IndexWrite++] =
			static_cast<unsigned short>(Top1);

		Indices[IndexWrite++] =
			static_cast<unsigned short>(Top1);
		Indices[IndexWrite++] =
			static_cast<unsigned short>(Bottom0);
		Indices[IndexWrite++] =
			static_cast<unsigned short>(Bottom1);
	}

	void RenderDX11_AppendTerrain2SkirtIndices(
		unsigned short* Indices,
		int& IndexWrite,
		int IndexCapacity,
		int VertexDim,
		int SkirtVertexStart
	)
	{
		if (
			!Indices ||
			VertexDim <= 0 ||
			SkirtVertexStart <= 0
		)
		{
			return;
		}

		const int Side = VertexDim + 1;
		int Bottom = SkirtVertexStart;

		// North edge: z = 0
		for (int x = 0; x < VertexDim; ++x)
		{
			RenderDX11_AppendTerrain2SkirtQuad(
				Indices,
				IndexWrite,
				IndexCapacity,
				x,
				x + 1,
				Bottom + x,
				Bottom + x + 1
			);
		}

		// South edge: z = VertexDim
		Bottom += Side;
		for (int x = 0; x < VertexDim; ++x)
		{
			RenderDX11_AppendTerrain2SkirtQuad(
				Indices,
				IndexWrite,
				IndexCapacity,
				VertexDim * Side + x,
				VertexDim * Side + x + 1,
				Bottom + x,
				Bottom + x + 1
			);
		}

		// West edge: x = 0
		Bottom += Side;
		for (int z = 0; z < VertexDim; ++z)
		{
			RenderDX11_AppendTerrain2SkirtQuad(
				Indices,
				IndexWrite,
				IndexCapacity,
				z * Side,
				(z + 1) * Side,
				Bottom + z,
				Bottom + z + 1
			);
		}

		// East edge: x = VertexDim
		Bottom += Side;
		for (int z = 0; z < VertexDim; ++z)
		{
			RenderDX11_AppendTerrain2SkirtQuad(
				Indices,
				IndexWrite,
				IndexCapacity,
				z * Side + VertexDim,
				(z + 1) * Side + VertexDim,
				Bottom + z,
				Bottom + z + 1
			);
		}
	}

	bool RenderDX11_UpdateTerrain2NativeTile(
		int CacheIndex,
		int VisibleTileIndex,
		const r3dTerrain2::DX11AtlasTileInfo& TileInfo
	)
	{
		if (
			!Terrain2 ||
			!gDX11Device ||
			!gDX11Context ||
			CacheIndex < 0 ||
			CacheIndex >= DX11_TERRAIN_PATCH_CACHE_COUNT ||
			TileInfo.VertexDim <= 0 ||
			TileInfo.VertexDim > 128 ||
			TileInfo.TileSizeCells <= 0 ||
			TileInfo.CellSize <= 0.0f
		)
		{
			return false;
		}

		if (!RenderDX11_EnsureTerrain2HeightSamples())
			return false;

		WorldDX11TerrainPatchCacheEntry& Entry =
			gDX11TerrainPatchCache[CacheIndex];

		const int Side = TileInfo.VertexDim + 1;
		const bool UseSkirts = false;
		const int SkirtVertexCount =
			UseSkirts
			? RenderDX11_GetTerrain2SkirtVertexCount(
				TileInfo.VertexDim
			)
			: 0;
		const int VertexCount =
			Side * Side +
			(TileInfo.ConFlags ? TileInfo.VertexDim * 4 : 0) +
			SkirtVertexCount;

		const int NativeIndexCount =
			Terrain2->BuildDX11TileIndices(
				TileInfo,
				0,
				0
			);
		const int SkirtIndexCount =
			UseSkirts
			? RenderDX11_GetTerrain2SkirtIndexCount(
				TileInfo.VertexDim
			)
			: 0;
		const int IndexCount =
			NativeIndexCount + SkirtIndexCount;

		if (
			VertexCount <= 0 ||
			VertexCount > 65535 ||
			NativeIndexCount <= 0 ||
			IndexCount <= 0
		)
		{
			return false;
		}

		if (
			!Entry.VertexBuffer ||
			Entry.VertexCapacity < VertexCount
		)
		{
			RenderDX11_SafeRelease(Entry.VertexBuffer);

			D3D11_BUFFER_DESC Desc = {};
			Desc.ByteWidth =
				sizeof(WorldDX11TerrainVertex) * VertexCount;
			Desc.Usage = D3D11_USAGE_DYNAMIC;
			Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			if (FAILED(gDX11Device->CreateBuffer(
				&Desc,
				0,
				&Entry.VertexBuffer
			)))
			{
				return false;
			}

			Entry.VertexCapacity = VertexCount;
		}

		if (
			!Entry.IndexBuffer ||
			Entry.IndexCapacity < IndexCount
		)
		{
			RenderDX11_SafeRelease(Entry.IndexBuffer);

			D3D11_BUFFER_DESC Desc = {};
			Desc.ByteWidth =
				sizeof(unsigned short) * IndexCount;
			Desc.Usage = D3D11_USAGE_DYNAMIC;
			Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			if (FAILED(gDX11Device->CreateBuffer(
				&Desc,
				0,
				&Entry.IndexBuffer
			)))
			{
				return false;
			}

			Entry.IndexCapacity = IndexCount;
		}

		D3D11_MAPPED_SUBRESOURCE VertexMap = {};
		if (FAILED(gDX11Context->Map(
			Entry.VertexBuffer,
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&VertexMap
		)))
		{
			return false;
		}

		WorldDX11TerrainVertex* Vertices =
			static_cast<WorldDX11TerrainVertex*>(VertexMap.pData);

		const int CellStep =
			TileInfo.TileSizeCells / TileInfo.VertexDim;

		if (CellStep <= 0)
		{
			gDX11Context->Unmap(Entry.VertexBuffer, 0);
			return false;
		}

		int VertexIndex = 0;

		for (int z = 0; z <= TileInfo.VertexDim; ++z)
		{
			for (int x = 0; x <= TileInfo.VertexDim; ++x)
			{
				const int CellX =
					TileInfo.X * TileInfo.TileSizeCells +
					x * CellStep;
				const int CellZ =
					TileInfo.Z * TileInfo.TileSizeCells +
					z * CellStep;

				RenderDX11_WriteTerrain2NativeVertex(
					&Vertices[VertexIndex],
					CellX,
					CellZ,
					TileInfo.CellSize
				);
				++VertexIndex;
			}
		}

		if (TileInfo.ConFlags)
		{
			const int HalfStep = CellStep / 2;
			const int StartX =
				TileInfo.X * TileInfo.TileSizeCells;
			const int StartZ =
				TileInfo.Z * TileInfo.TileSizeCells;
			const int EndX = StartX + TileInfo.TileSizeCells;
			const int EndZ = StartZ + TileInfo.TileSizeCells;

			if (HalfStep <= 0)
			{
				gDX11Context->Unmap(Entry.VertexBuffer, 0);
				return false;
			}

			const int EdgeCellX[4] =
			{
				StartX,
				StartX,
				StartX,
				EndX
			};
			const int EdgeCellZ[4] =
			{
				StartZ,
				EndZ,
				StartZ,
				StartZ
			};

			for (int Edge = 0; Edge < 4; ++Edge)
			{
				for (int i = 0; i < TileInfo.VertexDim; ++i)
				{
					const bool Horizontal = Edge < 2;
					const int CellX =
						Horizontal
						? EdgeCellX[Edge] + i * CellStep + HalfStep
						: EdgeCellX[Edge];
					const int CellZ =
						Horizontal
						? EdgeCellZ[Edge]
						: EdgeCellZ[Edge] + i * CellStep + HalfStep;

					RenderDX11_WriteTerrain2NativeVertex(
						&Vertices[VertexIndex],
						CellX,
						CellZ,
						TileInfo.CellSize
					);
					++VertexIndex;
				}
			}
		}

		if (SkirtVertexCount > 0)
		{
			const r3dTerrainDesc& TerrainDesc =
				Terrain2->GetDesc();
			const float SkirtBottomY =
				TerrainDesc.MinHeight -
				R3D_MAX(
					TerrainDesc.MaxHeight -
					TerrainDesc.MinHeight,
					128.0f
				);
			const int StartX =
				TileInfo.X * TileInfo.TileSizeCells;
			const int StartZ =
				TileInfo.Z * TileInfo.TileSizeCells;
			const int EndX =
				StartX + TileInfo.TileSizeCells;
			const int EndZ =
				StartZ + TileInfo.TileSizeCells;

			for (int x = 0; x <= TileInfo.VertexDim; ++x)
			{
				RenderDX11_WriteTerrain2SkirtVertex(
					&Vertices[VertexIndex++],
					StartX + x * CellStep,
					StartZ,
					TileInfo.CellSize,
					SkirtBottomY
				);
			}

			for (int x = 0; x <= TileInfo.VertexDim; ++x)
			{
				RenderDX11_WriteTerrain2SkirtVertex(
					&Vertices[VertexIndex++],
					StartX + x * CellStep,
					EndZ,
					TileInfo.CellSize,
					SkirtBottomY
				);
			}

			for (int z = 0; z <= TileInfo.VertexDim; ++z)
			{
				RenderDX11_WriteTerrain2SkirtVertex(
					&Vertices[VertexIndex++],
					StartX,
					StartZ + z * CellStep,
					TileInfo.CellSize,
					SkirtBottomY
				);
			}

			for (int z = 0; z <= TileInfo.VertexDim; ++z)
			{
				RenderDX11_WriteTerrain2SkirtVertex(
					&Vertices[VertexIndex++],
					EndX,
					StartZ + z * CellStep,
					TileInfo.CellSize,
					SkirtBottomY
				);
			}
		}

		gDX11Context->Unmap(Entry.VertexBuffer, 0);

		if (VertexIndex != VertexCount)
			return false;

		D3D11_MAPPED_SUBRESOURCE IndexMap = {};
		if (FAILED(gDX11Context->Map(
			Entry.IndexBuffer,
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&IndexMap
		)))
		{
			return false;
		}

		const int WrittenIndices =
			Terrain2->BuildDX11TileIndices(
				TileInfo,
				static_cast<unsigned short*>(IndexMap.pData),
				Entry.IndexCapacity
			);

		int TotalWrittenIndices = WrittenIndices;

		if (
			WrittenIndices == NativeIndexCount &&
			SkirtIndexCount > 0
		)
		{
			const int SkirtVertexStart =
				Side * Side +
				(TileInfo.ConFlags ? TileInfo.VertexDim * 4 : 0);

			RenderDX11_AppendTerrain2SkirtIndices(
				static_cast<unsigned short*>(IndexMap.pData),
				TotalWrittenIndices,
				Entry.IndexCapacity,
				TileInfo.VertexDim,
				SkirtVertexStart
			);
		}

		gDX11Context->Unmap(Entry.IndexBuffer, 0);

		if (TotalWrittenIndices != IndexCount)
			return false;

		Entry.TileX = TileInfo.X;
		Entry.TileZ = TileInfo.Z;
		Entry.L = TileInfo.L;
		Entry.ConFlags = TileInfo.ConFlags;
		Entry.VertexDim = TileInfo.VertexDim;
		Entry.PatchSize = TileInfo.WorldDim;
		Entry.IndexCount = IndexCount;
		Entry.Valid = true;
		Entry.LastUsedFrame = gDX11TerrainCacheFrameId;
		++gDX11TerrainPatchUpdateCount;

		(void)VisibleTileIndex;
		return true;
	}

	bool RenderDX11_BindTerrainGeometry(
		ID3D11Buffer* VertexBuffer,
		ID3D11Buffer* IndexBuffer = 0
	)
	{
		if (
			!gDX11Context ||
			!gDX11TerrainInputLayout ||
			!VertexBuffer ||
			(!IndexBuffer && !gDX11TerrainIB)
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
			IndexBuffer ? IndexBuffer : gDX11TerrainIB,
			DXGI_FORMAT_R16_UINT,
			0
		);

		return true;
	}

	int RenderDX11_FindTerrainCacheSlotForAtlasWrite(
		int DrawIndex
	)
	{
		for (int i = 0; i < DX11_TERRAIN_PATCH_CACHE_COUNT; ++i)
		{
			if (!gDX11TerrainPatchCache[i].Valid)
				return i;
		}

		return RenderDX11_ClampInt(
			DrawIndex % DX11_TERRAIN_PATCH_CACHE_COUNT,
			0,
			DX11_TERRAIN_PATCH_CACHE_COUNT - 1
		);
	}

	bool RenderDX11_DrawTerrainV3()
	{
		if (
			!gDX11Context ||
			!gDX11TerrainIB
		)
		{
			return false;
		}

		RenderDX11_EnsureTerrainV3Desc();

		const float ChunkSize =
			gDX11TerrainV3Desc.CellSize *
			static_cast<float>(gDX11TerrainV3Desc.ChunkCells);

		if (ChunkSize <= 1.0f)
			return false;

		const int MaxChunkX =
			RenderDX11_ClampInt(
				static_cast<int>(
					floorf(
						R3D_MAX(
							gDX11TerrainV3Desc.SizeX - ChunkSize,
							0.0f
						) /
						ChunkSize
					)
				),
				0,
				0x7fffffff
			);

		const int MaxChunkZ =
			RenderDX11_ClampInt(
				static_cast<int>(
					floorf(
						R3D_MAX(
							gDX11TerrainV3Desc.SizeZ - ChunkSize,
							0.0f
						) /
						ChunkSize
					)
				),
				0,
				0x7fffffff
			);

		const int PatchSide =
			RenderDX11_GetTerrainPatchSide();

		const int ChunkCountX =
			RenderDX11_ClampInt(
				PatchSide,
				1,
				MaxChunkX + 1
			);

		const int ChunkCountZ =
			RenderDX11_ClampInt(
				PatchSide,
				1,
				MaxChunkZ + 1
			);

		const int CenterChunkX =
			RenderDX11_ClampInt(
				static_cast<int>(floorf(gCam.x / ChunkSize)),
				0,
				MaxChunkX
			);

		const int CenterChunkZ =
			RenderDX11_ClampInt(
				static_cast<int>(floorf(gCam.z / ChunkSize)),
				0,
				MaxChunkZ
			);

		int StartChunkX =
			CenterChunkX -
			ChunkCountX / 2;

		int StartChunkZ =
			CenterChunkZ -
			ChunkCountZ / 2;

		StartChunkX =
			RenderDX11_ClampInt(
				StartChunkX,
				0,
				MaxChunkX - ChunkCountX + 1
			);

		StartChunkZ =
			RenderDX11_ClampInt(
				StartChunkZ,
				0,
				MaxChunkZ - ChunkCountZ + 1
			);

		int RequestChunkXs[DX11_TERRAIN_PATCH_COUNT] = {};
		int RequestChunkZs[DX11_TERRAIN_PATCH_COUNT] = {};
		int RequestChunkLs[DX11_TERRAIN_PATCH_COUNT] = {};
		int RequestChunkCount = 0;

		for (int z = 0; z < ChunkCountZ; ++z)
		{
			for (int x = 0; x < ChunkCountX; ++x)
			{
				if (RequestChunkCount >= DX11_TERRAIN_PATCH_COUNT)
					return false;

				RequestChunkXs[RequestChunkCount] =
					StartChunkX + x;
				RequestChunkZs[RequestChunkCount] =
					StartChunkZ + z;
				RequestChunkLs[RequestChunkCount] =
					DX11_TERRAIN_V3_CACHE_L;

				++RequestChunkCount;
			}
		}

		const bool bTerrainCullingEnabled =
			RenderDX11_WantsTerrainCullEnabled();

		WorldDX11FrustumPlane FrustumPlanes[6] = {};
		const bool bHasFrustum =
			bTerrainCullingEnabled &&
			RenderDX11_BuildFrameFrustum(
				FrustumPlanes
			);

		if (!RenderDX11_WriteTerrainV3CB())
			return false;

		if (!gDX11TerrainV3PathLogged)
		{
			gDX11TerrainV3PathLogged = true;
			char Text[256] = {};
			sprintf_s(
				Text,
				"[DX11][TerrainV3] Active: independent chunk grid "
				"coverage=%dx%d chunkSize=%.1f; TerrainV2 bypassed\n",
				ChunkCountX,
				ChunkCountZ,
				ChunkSize
			);
			OutputDebugStringA(Text);
		}

		gDX11TerrainPatchDrawCount = 0;
		gDX11TerrainPatchCullCount = 0;

		for (int i = 0; i < RequestChunkCount; ++i)
		{
			const int ChunkX =
				RequestChunkXs[i];
			const int ChunkZ =
				RequestChunkZs[i];

			WorldDX11TerrainPatchBounds Bounds = {};
			if (
				RenderDX11_BuildTerrainV3ChunkBounds(
					ChunkX,
					ChunkZ,
					ChunkSize,
					&Bounds
				) &&
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

			int CacheIndex =
				RenderDX11_FindTerrainCacheSlot(
					ChunkX,
					ChunkZ,
					DX11_TERRAIN_V3_CACHE_L
				);

			if (CacheIndex < 0)
			{
				CacheIndex =
					RenderDX11_FindTerrainCacheSlotForWrite(
						RequestChunkXs,
						RequestChunkZs,
						RequestChunkLs,
						RequestChunkCount
					);
			}

			if (CacheIndex < 0)
				return false;

			if (!RenderDX11_EnsureTerrainV3Chunk(
				CacheIndex,
				ChunkX,
				ChunkZ,
				ChunkSize
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

		return gDX11TerrainPatchDrawCount > 0;
	}

	bool RenderDX11_DrawTerrain2AtlasTiles()
	{
		if (
			!gDX11Context ||
			!Terrain2 ||
			!Terrain ||
			!Terrain->IsLoaded()
		)
		{
			return false;
		}

		Terrain2->UpdateDX11VisibleAtlasTiles();

		const int VisibleTileCount =
			Terrain2->GetDX11VisibleAtlasTileCount();

		gDX11Terrain2AtlasVisibleCount =
			VisibleTileCount;
		gDX11Terrain2AtlasDrawCount = 0;
		gDX11Terrain2AtlasSkipInfoCount = 0;
		gDX11Terrain2AtlasSkipSRVCount = 0;
		gDX11Terrain2AtlasRefreshCount = 0;
		gDX11Terrain2AtlasRefreshPendingCount = 0;

		if (VisibleTileCount <= 0)
			return false;

		static bool bNativeTerrain2PathLogged = false;
		if (!bNativeTerrain2PathLogged)
		{
			bNativeTerrain2PathLogged = true;
			char Text[256] = {};
			sprintf_s(
				Text,
				"[DX11][Render] Native Terrain2 tile path active: "
				"visible=%d, density and ConFlags preserved\n",
				VisibleTileCount
			);
			OutputDebugStringA(Text);
		}

		gDX11TerrainPatchDrawCount = 0;
		gDX11TerrainPatchCullCount = 0;
		gDX11Terrain2ActiveAtlasSRVMask = 0;

		for (int i = 0; i < VisibleTileCount; ++i)
		{
			r3dTerrain2::DX11AtlasTileInfo TileInfo = {};

			if (
				!Terrain2->GetDX11VisibleAtlasTileInfo(
					i,
					&TileInfo
				)
			)
			{
				++gDX11Terrain2AtlasSkipInfoCount;
				continue;
			}

			if (!RenderDX11_WriteTerrainCB(&TileInfo))
				return false;

			int CacheIndex =
				RenderDX11_FindTerrainCacheSlot(
					TileInfo.X,
					TileInfo.Z,
					TileInfo.L
				);

			if (CacheIndex < 0)
			{
				CacheIndex =
					RenderDX11_FindTerrainCacheSlotForAtlasWrite(
						i
					);
			}

			if (CacheIndex < 0)
				return false;

			WorldDX11TerrainPatchCacheEntry& Entry =
				gDX11TerrainPatchCache[CacheIndex];

			const bool NeedsMeshUpdate =
				!Entry.Valid ||
				!Entry.VertexBuffer ||
				!Entry.IndexBuffer ||
				Entry.TileX != TileInfo.X ||
				Entry.TileZ != TileInfo.Z ||
				Entry.L != TileInfo.L ||
				Entry.ConFlags != TileInfo.ConFlags ||
				Entry.VertexDim != TileInfo.VertexDim;

			if (
				NeedsMeshUpdate &&
				!RenderDX11_UpdateTerrain2NativeTile(
					CacheIndex,
					i,
					TileInfo
				)
			)
			{
				return false;
			}

			Entry.LastUsedFrame =
				gDX11TerrainCacheFrameId;

			if (!RenderDX11_BindTerrainGeometry(
				Entry.VertexBuffer,
				Entry.IndexBuffer
			))
				return false;

			gDX11Context->DrawIndexed(
				Entry.IndexCount,
				0,
				0
			);

			++gDX11TerrainPatchDrawCount;
			++gDX11Terrain2AtlasDrawCount;
		}

		gDX11Terrain2ActiveAtlasSRVMask = 0;

		return gDX11TerrainPatchDrawCount > 0;
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

		// The Terrain2 quadtree needs its native density-dependent index
		// buffers and ConFlags edge vertices. Until those are ported, use
		// one uniform grid per patch. Mixing the fixed DX11 grid
		// with native Terrain2 LOD tiles creates overlapping cliffs.
		int CellsPerPatch =
			DX11_TERRAIN_GRID_DIM - 1;

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

		const int PatchSide =
			RenderDX11_GetTerrainPatchSide();

		const int TileCountX =
			RenderDX11_ClampInt(
				PatchSide,
				1,
				MaxTileX + 1
			);

		const int TileCountZ =
			RenderDX11_ClampInt(
				PatchSide,
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
		int RequestTileLs[DX11_TERRAIN_PATCH_COUNT] = {};
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

				RequestTileLs[RequestTileCount] = 0;

				++RequestTileCount;
			}
		}

		const bool bTerrainCullingEnabled =
			RenderDX11_WantsTerrainCullEnabled();

		WorldDX11FrustumPlane FrustumPlanes[6] = {};
		const bool bHasFrustum =
			bTerrainCullingEnabled &&
			RenderDX11_BuildFrameFrustum(
				FrustumPlanes
			);

		gDX11TerrainPatchDrawCount = 0;
		gDX11TerrainPatchCullCount = 0;

		static bool bUniformTerrainPathLogged = false;
		if (!bUniformTerrainPathLogged)
		{
			bUniformTerrainPathLogged = true;
			char Text[256] = {};
			sprintf_s(
				Text,
				"[DX11][Render] Terrain uniform-grid path active: "
				"coverage=%dx%d patches patchSize=%.1f; "
				"native Terrain2 LOD atlas geometry bypassed\n",
				TileCountX,
				TileCountZ,
				PatchSize
			);
			OutputDebugStringA(Text);
		}

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
					TileZ,
					0
				);

			if (CacheIndex < 0)
			{
				CacheIndex =
					RenderDX11_FindTerrainCacheSlotForWrite(
						RequestTileXs,
						RequestTileZs,
						RequestTileLs,
						RequestTileCount
					);
			}

			if (CacheIndex < 0)
				return false;

			if (!RenderDX11_EnsureTerrainCacheTile(
				CacheIndex,
				TileX,
				TileZ,
				0,
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

	gDX11Terrain2ActiveAtlasSRVMask = 0;

	return RenderDX11_DrawSelectedTerrain(
		false
	);
}
