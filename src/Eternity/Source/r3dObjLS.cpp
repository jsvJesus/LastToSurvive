#include "r3dPCH.h"
#include "r3d.h"
#include "r3dBinMesh.h"

#include "r3dBackgroundTaskDispatcher.h"

#if defined(_WIN64) && !defined(WO_SERVER)
#define R3D_SPEEDTREE_SRT_ENABLED 1
#define ST_EVALUATION_BUILD
#include "../../../External/SpeedTreeSDK/Include/Core/Core.h"
#pragma comment(lib, "..\\..\\External\\SpeedTreeSDK\\Lib\\Windows\\VS2015.x64\\SpeedTreeCore_Windows_v7.1_VS2015_MT64_Static.lib")
#else
#define R3D_SPEEDTREE_SRT_ENABLED 0
#endif

static int _r3dFinishObjectLoading(r3dMesh *obj)
{
	if(!obj->NumVertices || !obj->NumIndices) {
		return 0;
	}
	
	obj->RecalcBoundBox();

	r3d_assert(obj->VertexTangents);
	r3d_assert(obj->VertexTangentWs);
	r3d_assert(obj->VertexUVs);
	r3d_assert(obj->VertexNormals);
	r3d_assert(obj->VertexPositions);

	return 1;
}

/*virtual*/ r3dPoint3D* 
r3dMesh :: GetVertexPositions() const /*OVERRIDE*/
{
	return VertexPositions ;
}

/*virtual*/ uint32_t*
r3dMesh :: GetIndices() const /*OVERRIDE*/
{
	return Indices ;
}

void r3dMesh :: InitVertsList(int NumVerts)
{
	NumVertices     = NumVerts;

	r3d_assert(VertexPositions == NULL);
	r3d_assert(VertexUVs == NULL);
	r3d_assert(VertexNormals == NULL);
	r3d_assert(VertexTangents == NULL);
	r3d_assert(VertexTangentWs == NULL);

	VertexPositions = new r3dPoint3D[NumVertices];
	VertexUVs	= new r3dPoint2D[NumVertices];
	VertexNormals = new r3dVector[NumVertices];
	VertexTangents = new r3dVector[NumVertices];
	VertexTangentWs = new char[NumVertices];

	return;
}

void r3dMesh::InitIndexList(int numIndexes)
{
	NumIndices = numIndexes;
	Indices = new uint32_t[numIndexes];

	return;
}

static bool r3dIsSrtMeshFile( const char* fname )
{
	const char* dot = strrchr( fname, '.' );
	return dot && !stricmp( dot, ".srt" );
}

#if R3D_SPEEDTREE_SRT_ENABLED
static void r3dGetFileDir( char* outDir, int outDirSize, const char* fname )
{
	r3dscpy_s( outDir, outDirSize, fname );

	char* slash1 = strrchr( outDir, '\\' );
	char* slash2 = strrchr( outDir, '/' );
	char* slash = slash1 > slash2 ? slash1 : slash2;

	if( slash )
		*( slash + 1 ) = 0;
	else
		outDir[ 0 ] = 0;
}

static const char* r3dGetBaseName( const char* fname )
{
	const char* slash1 = strrchr( fname, '\\' );
	const char* slash2 = strrchr( fname, '/' );
	const char* slash = slash1 > slash2 ? slash1 : slash2;
	return slash ? slash + 1 : fname;
}

static void r3dReplaceExtensionWithDds( char (&path)[ MAX_PATH ] )
{
	char* dot = strrchr( path, '.' );
	if( dot )
		r3dscpy( dot, ".dds" );
}

static bool r3dResolveSrtTexturePath( char (&outPath)[ MAX_PATH ], const char* srtPath, const char* textureName )
{
	outPath[ 0 ] = 0;

	if( !textureName || !*textureName )
		return false;

	char fixedTexture[ MAX_PATH ];
	r3dscpy_s( fixedTexture, _countof( fixedTexture ), textureName );
	for( char* p = fixedTexture; *p; ++p )
	{
		if( *p == '/' )
			*p = '\\';
	}

	char srtDir[ MAX_PATH ];
	r3dGetFileDir( srtDir, _countof( srtDir ), srtPath );

	const char* candidates[ 3 ] =
	{
		fixedTexture,
		0,
		0
	};

	char dirRelative[ MAX_PATH ];
	sprintf( dirRelative, "%s%s", srtDir, fixedTexture );
	candidates[ 1 ] = dirRelative;

	char dirBaseName[ MAX_PATH ];
	sprintf( dirBaseName, "%s%s", srtDir, r3dGetBaseName( fixedTexture ) );
	candidates[ 2 ] = dirBaseName;

	for( int i = 0; i < _countof( candidates ); ++i )
	{
		if( !candidates[ i ] || !*candidates[ i ] )
			continue;

		r3dscpy_s( outPath, _countof( outPath ), candidates[ i ] );
		if( r3d_access( outPath, 0 ) == 0 )
			return true;

		r3dReplaceExtensionWithDds( outPath );
		if( r3d_access( outPath, 0 ) == 0 )
			return true;
	}

	outPath[ 0 ] = 0;
	return false;
}

static r3dTexture* r3dLoadSrtTexture( const char* resolvedPath )
{
	if( !resolvedPath || !*resolvedPath )
		return 0;

	D3DPOOL pool = r_no_managed_textures->GetInt() ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
	return r3dRenderer->LoadTexture( resolvedPath, D3DFMT_UNKNOWN, false, 1, 1, pool, TexMem );
}

static r3dMaterial* r3dGetSrtMaterial( const SpeedTree::SRenderState* rs, const char* srtPath, int drawCallIndex, bool useDefaultMaterial )
{
	if( useDefaultMaterial || !rs )
		return r3dMaterialLibrary::GetDefaultMaterial();

	char diffusePath[ MAX_PATH ];
	char normalPath[ MAX_PATH ];
	char specPath[ MAX_PATH ];
	const bool hasDiffuse = r3dResolveSrtTexturePath( diffusePath, srtPath, static_cast< const char* >( rs->m_apTextures[ SpeedTree::TL_DIFFUSE ] ) );
	const bool hasNormal = r3dResolveSrtTexturePath( normalPath, srtPath, static_cast< const char* >( rs->m_apTextures[ SpeedTree::TL_NORMAL ] ) );
	const bool hasSpec = r3dResolveSrtTexturePath( specPath, srtPath, static_cast< const char* >( rs->m_apTextures[ SpeedTree::TL_SPECULAR_MASK ] ) );

	if( !hasDiffuse )
	{
		r3dOutToLog( "SpeedTreeSRT: missing diffuse texture for '%s' draw %d, using default material\n", srtPath, drawCallIndex );
		return r3dMaterialLibrary::GetDefaultMaterial();
	}

	std::string cacheKey = diffusePath;
	cacheKey += "|";
	cacheKey += hasNormal ? normalPath : "";
	cacheKey += "|";
	cacheKey += hasSpec ? specPath : "";
	cacheKey += "|";
	cacheKey += rs->m_bBlending ? "blend" : "opaque";
	cacheKey += "|";
	cacheKey += rs->m_eFaceCulling == SpeedTree::CULLTYPE_NONE ? "2sided" : "1sided";

	static std::map< std::string, r3dMaterial* > s_SrtMaterials;
	std::map< std::string, r3dMaterial* >::iterator it = s_SrtMaterials.find( cacheKey );
	if( it != s_SrtMaterials.end() )
		return it->second;

	r3dMaterial* mat = new r3dMaterial();

	char matName[ R3D_MAX_MATERIAL_NAME ];
	sprintf( matName, "SpeedTreeSRT_%08x", r3dHash::MakeHash( cacheKey.c_str() ) );
	r3dscpy_s( mat->Name, _countof( mat->Name ), matName );
	r3dscpy_s( mat->DepotName, _countof( mat->DepotName ), srtPath );
	r3dGetFileDir( mat->OriginalDir, _countof( mat->OriginalDir ), srtPath );

	mat->DiffuseColor = r3dColor(
		R3D_CLAMP( int( rs->m_vDiffuseColor.x * rs->m_fDiffuseScalar * 255.0f ), 0, 255 ),
		R3D_CLAMP( int( rs->m_vDiffuseColor.y * rs->m_fDiffuseScalar * 255.0f ), 0, 255 ),
		R3D_CLAMP( int( rs->m_vDiffuseColor.z * rs->m_fDiffuseScalar * 255.0f ), 0, 255 ) );
	mat->SpecularColor = r3dColor(
		R3D_CLAMP( int( rs->m_vSpecularColor.x * 255.0f ), 0, 255 ),
		R3D_CLAMP( int( rs->m_vSpecularColor.y * 255.0f ), 0, 255 ),
		R3D_CLAMP( int( rs->m_vSpecularColor.z * 255.0f ), 0, 255 ) );
	mat->SpecularPower = rs->m_fShininess;

	if( rs->m_eFaceCulling == SpeedTree::CULLTYPE_NONE )
		mat->Flags |= R3D_MAT_DOUBLESIDED;

	if( !rs->m_bDiffuseAlphaMaskIsOpaque )
		mat->Flags |= R3D_MAT_HASALPHA | R3D_MAT_FORCEHASALPHA;

	if( rs->m_bBlending )
		mat->Flags |= R3D_MAT_TRANSPARENT | R3D_MAT_HASALPHA | R3D_MAT_FORCEHASALPHA;

	mat->Texture = r3dLoadSrtTexture( diffusePath );
	if( hasNormal )
		mat->BumpTexture = r3dLoadSrtTexture( normalPath );
	if( hasSpec )
		mat->GlossTexture = r3dLoadSrtTexture( specPath );

	if( !mat->Texture )
	{
		r3dOutToLog( "SpeedTreeSRT: failed to load diffuse '%s' for '%s', using default material\n", diffusePath, srtPath );
		delete mat;
		return r3dMaterialLibrary::GetDefaultMaterial();
	}

	if( hasNormal && !mat->BumpTexture )
		r3dOutToLog( "SpeedTreeSRT: failed to load normal '%s' for '%s'\n", normalPath, srtPath );
	if( hasSpec && !mat->GlossTexture )
		r3dOutToLog( "SpeedTreeSRT: failed to load specular '%s' for '%s'\n", specPath, srtPath );

	s_SrtMaterials[ cacheKey ] = mat;
	return mat;
}

static bool r3dLoadSpeedTreeSrtIntoMesh( r3dMesh& mesh, const char* srtPath, bool useDefaultMaterial )
{
	SpeedTree::CCore tree;
	if( !tree.LoadTree( srtPath ) )
	{
		r3dOutToLog( "SpeedTreeSRT: LoadTree failed for '%s': %s\n", srtPath, SpeedTree::CCore::GetError() );
		return false;
	}

	const SpeedTree::SGeometry* geom = tree.GetGeometry();
	if( !geom || geom->m_nNumLods <= 0 || !geom->m_pLods )
	{
		r3dOutToLog( "SpeedTreeSRT: no geometry in '%s'\n", srtPath );
		return false;
	}

	const SpeedTree::SLod* lod = 0;
	for( int lodIdx = 0; lodIdx < geom->m_nNumLods; ++lodIdx )
	{
		const SpeedTree::SLod& candidate = geom->m_pLods[ lodIdx ];
		if( candidate.m_nNumDrawCalls > 0 && candidate.m_pDrawCalls )
		{
			lod = &candidate;
			break;
		}
	}

	if( !lod )
	{
		r3dOutToLog( "SpeedTreeSRT: no LOD draw calls in '%s'\n", srtPath );
		return false;
	}

	int numVertices = 0;
	int numIndices = 0;
	int numChunks = 0;
	for( int drawIdx = 0; drawIdx < lod->m_nNumDrawCalls; ++drawIdx )
	{
		const SpeedTree::SDrawCall& dc = lod->m_pDrawCalls[ drawIdx ];
		if( dc.m_nNumVertices <= 0 || dc.m_nNumIndices <= 0 || !dc.m_pVertexData || !dc.m_pIndexData )
			continue;

		numVertices += dc.m_nNumVertices;
		numIndices += dc.m_nNumIndices;
		++numChunks;
	}

	if( numVertices <= 0 || numIndices <= 0 || numChunks <= 0 )
	{
		r3dOutToLog( "SpeedTreeSRT: empty LOD0 geometry in '%s'\n", srtPath );
		return false;
	}

	if( numChunks > r3dMesh::ConstNumMatChunks )
	{
		r3dOutToLog( "SpeedTreeSRT: too many draw calls (%d) in '%s'\n", numChunks, srtPath );
		return false;
	}

	memset( mesh.Name, 0, sizeof( mesh.Name ) );
	r3dscpy_s( mesh.Name, _countof( mesh.Name ), r3dGetBaseName( srtPath ) );
	mesh.vPivot = r3dPoint3D( 0, 0, 0 );

	mesh.InitVertsList( numVertices );
	mesh.InitIndexList( numIndices );

	r3d_assert( mesh.MatChunksNames == 0 );
	mesh.MatChunksNames = new char*[ 256 ];
	mesh.NumMatChunks = 0;

	bool hasMissingTangents = false;
	int vertexBase = 0;
	int indexBase = 0;

	for( int drawIdx = 0; drawIdx < lod->m_nNumDrawCalls; ++drawIdx )
	{
		const SpeedTree::SDrawCall& dc = lod->m_pDrawCalls[ drawIdx ];
		if( dc.m_nNumVertices <= 0 || dc.m_nNumIndices <= 0 || !dc.m_pVertexData || !dc.m_pIndexData )
			continue;

		for( int vertIdx = 0; vertIdx < dc.m_nNumVertices; ++vertIdx )
		{
			float values[ 4 ] = { 0, 0, 0, 0 };
			const int dstIdx = vertexBase + vertIdx;

			if( dc.GetProperty( SpeedTree::VERTEX_PROPERTY_POSITION, vertIdx, values ) )
				mesh.VertexPositions[ dstIdx ] = r3dPoint3D( values[ 0 ], values[ 1 ], values[ 2 ] );
			else
				mesh.VertexPositions[ dstIdx ] = r3dPoint3D( 0, 0, 0 );

			if( dc.GetProperty( SpeedTree::VERTEX_PROPERTY_NORMAL, vertIdx, values ) )
				mesh.VertexNormals[ dstIdx ] = r3dPoint3D( values[ 0 ], values[ 1 ], values[ 2 ] );
			else
				mesh.VertexNormals[ dstIdx ] = r3dPoint3D( 0, 1, 0 );

			if( dc.GetProperty( SpeedTree::VERTEX_PROPERTY_DIFFUSE_TEXCOORDS, vertIdx, values ) )
				mesh.VertexUVs[ dstIdx ] = r3dPoint2D( values[ 0 ], values[ 1 ] );
			else
				mesh.VertexUVs[ dstIdx ] = r3dPoint2D( 0, 0 );

			if( dc.GetProperty( SpeedTree::VERTEX_PROPERTY_TANGENT, vertIdx, values ) )
			{
				mesh.VertexTangents[ dstIdx ] = r3dPoint3D( values[ 0 ], values[ 1 ], values[ 2 ] );
				mesh.VertexTangentWs[ dstIdx ] = char( 255 );
			}
			else
			{
				mesh.VertexTangents[ dstIdx ] = r3dPoint3D( 1, 0, 0 );
				mesh.VertexTangentWs[ dstIdx ] = char( 255 );
				hasMissingTangents = true;
			}
		}

		if( dc.m_b32BitIndices )
		{
			const SpeedTree::st_uint32* indices = reinterpret_cast< const SpeedTree::st_uint32* >( static_cast< const SpeedTree::st_byte* >( dc.m_pIndexData ) );
			for( int i = 0; i < dc.m_nNumIndices; ++i )
				mesh.Indices[ indexBase + i ] = vertexBase + indices[ i ];
		}
		else
		{
			const SpeedTree::st_uint16* indices = reinterpret_cast< const SpeedTree::st_uint16* >( static_cast< const SpeedTree::st_byte* >( dc.m_pIndexData ) );
			for( int i = 0; i < dc.m_nNumIndices; ++i )
				mesh.Indices[ indexBase + i ] = vertexBase + indices[ i ];
		}

		const int chunkIdx = mesh.NumMatChunks;
		mesh.MatChunks[ chunkIdx ].StartIndex = indexBase;
		mesh.MatChunks[ chunkIdx ].EndIndex = indexBase + dc.m_nNumIndices;
		mesh.MatChunks[ chunkIdx ].Mat = r3dGetSrtMaterial( static_cast< const SpeedTree::SRenderState* >( dc.m_pRenderState ), srtPath, drawIdx, useDefaultMaterial );
		mesh.MatChunksNames[ chunkIdx ] = new char[ 128 ];
		sprintf( mesh.MatChunksNames[ chunkIdx ], "SpeedTreeSRT_%02d", drawIdx );
		++mesh.NumMatChunks;

		vertexBase += dc.m_nNumVertices;
		indexBase += dc.m_nNumIndices;
	}

	mesh.SetLoaded();

	if( hasMissingTangents )
		mesh.RecalcBasisVectors();

	_r3dFinishObjectLoading( &mesh );
	r3dOutToLog( "SpeedTreeSRT: loaded '%s' as r3dMesh (%d verts, %d indices, %d chunks)\n", srtPath, numVertices, numIndices, mesh.NumMatChunks );
	return true;
}
#endif

bool getFileTimestamp(const char* fname, FILETIME& writeTime)
{
	HANDLE hFile = CreateFile(fname, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	if(hFile ==  INVALID_HANDLE_VALUE)
	{
		// we are reading from acrhive
		return false;
	}
	GetFileTime(hFile, 0, 0, &writeTime);
	CloseHandle(hFile);

	return true;
}

void setFileTimestamp(const char* fname, const FILETIME& time)
{
	HANDLE hFile = CreateFile(fname, GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_WRITE_ATTRIBUTES, 0);
	if(hFile ==  INVALID_HANDLE_VALUE)
	{
		r3dOutToLog("Error in setFileTimestamp! Error: %d", GetLastError()); // shouldn't be possible, as we are checking that file exist before calling this function
		return;
	}
	SetFileTime(hFile, &time, &time, &time);
	CloseHandle(hFile);
}

void ToSkinFileName( char (&skinFile)[256], const char* baseFile )
{
	r3dscpy(skinFile, baseFile);
	r3dscpy(&skinFile[strlen(skinFile)-3], "wgt");
}

struct MeshLoadParams : r3dTaskParams
{
	r3dMesh* Loadee ;
	bool UseDefaultMaterial ;
};

/*static*/
void r3dMesh::DoLoadMesh( r3dTaskParams* params )
{
	MeshLoadParams* mlp = static_cast< MeshLoadParams* >( params ) ;

	mlp->Loadee->DoLoad( mlp->UseDefaultMaterial ) ;
}

static r3dTaskParramsArray< MeshLoadParams > g_MeshLoadParamsArray ;

bool r3dMesh::Load(const char* fname, bool use_default_material /*= false*/, bool force_sync /*= true*/ )
{
	char szFixedName[MAX_PATH];	
	FixFileName(fname, szFixedName);
	FileName = szFixedName;

	MeshLoaded ( FileName.c_str () );

#if R3D_ALLOW_ASYNC_MESH_LOADING
	if( g_async_loading->GetInt() && R3D_IS_MAIN_THREAD() && !force_sync && g_pBackgroundTaskDispatcher )
	{
		r3dBackgroundTaskDispatcher::TaskDescriptor td ;

		MeshLoadParams* params = g_MeshLoadParamsArray.Alloc() ;

		params->Loadee = this ;
		params->UseDefaultMaterial = use_default_material ;

		td.Fn				= DoLoadMesh ;
		td.Params			= params ;
		td.CompletionFlag	= NULL ;

		g_pBackgroundTaskDispatcher->AddTask( td ) ;

		return TRUE ;
	}
	else
#endif
	{
		return DoLoad( use_default_material ) ;
	}
}

#define SCB_EXT ".scb"

template< int N > 
static void ToBin( char (&outinFName)[ N ], const char* inFName )
{
	
	r3dscpy(outinFName, inFName);

	if( char* dot_start = strchr( outinFName, '.' ) )
	{
		r3dscpy( dot_start, SCB_EXT );
	}
	else
	{
		strcat( outinFName, SCB_EXT );
	}	
}

/*static*/ bool r3dMesh::CanLoad( const char* fname )
{
	if( r3dIsSrtMeshFile( fname ) )
		return r3d_access( fname, 0 ) == 0;

	char bin_file[512];
	r3dscpy(bin_file, fname);

	ToBin( bin_file, fname ) ;

	bool bin_exist = r3d_access(bin_file, 0) == 0;
	bool txt_exist = r3d_access(fname, 0) == 0;

	if(!(txt_exist || bin_exist)) // if sco and scb both missing - then bail out
	{
		return false ;
	}

	return true ;
}

bool r3dMesh::DoLoad( bool use_default_material )
{
	r3dCSHolderWithDeviceQueue csholder( g_ResourceCritSection ) ; (void)csholder ;

	const char* fname = FileName.c_str() ;

	struct ArtBugComment
	{
		ArtBugComment( const char* fname )
		{
			r3dArtBugComment( ( r3dString( "Loading mesh " ) + fname ).c_str() ) ;
		}

		~ArtBugComment()
		{
			r3dArtBugComment( 0 ) ;
		}		
	} artBugComment( fname ) ;

	if( r3dIsSrtMeshFile( fname ) )
	{
#if R3D_SPEEDTREE_SRT_ENABLED
		const bool loaded = r3dLoadSpeedTreeSrtIntoMesh( *this, fname, use_default_material );
		if( loaded )
		{
			ResetXForm();
			FindAlphaTextures();
		}
		return loaded;
#else
		r3dArtBug( "r3dMesh::Load(): SpeedTree SRT loader is enabled only for x64 client/editor builds: '%s'\n", fname );
		return false;
#endif
	}

	char bin_file[512];
	ToBin( bin_file, fname ) ;

	bool bin_exist = r3d_access(bin_file, 0) == 0;
	bool txt_exist = r3d_access(fname, 0) == 0;

	if(!(txt_exist || bin_exist)) // if sco and scb both missing - then bail out
	{
		// ptumik: show art bug, but do not crash editor
//#ifndef FINAL_BUILD
//		r3dError("Erorr: Missing mesh file: %s\n", fname);
//#endif
		r3dArtBug("r3dMesh::Load(): Can't load '%s' or '%s', file doesn't exist\n", fname, bin_file);
		return false;
	}
#ifndef FINAL_BUILD
	if(bin_exist && !txt_exist) // if bin exists but text sco is no longer there, show error and bail out
	{
		Flags |= obfMissingSource ;
		r3dArtBug("r3dMesh::Load(): Missing sco file for %s\n", fname);
	}
#endif

	bool res = false;

	bool save_bin = false;
	bool load_text = false;
	bool loading_archive = false;
	FILETIME creationTime;
	if(txt_exist && !getFileTimestamp(fname, creationTime))
		loading_archive = true;
	
	if(bin_exist)
	{
		FILETIME binCreationTime;
		if(!getFileTimestamp(bin_file, binCreationTime))
			loading_archive = true;
		
		if(!loading_archive && txt_exist &&
			(creationTime.dwLowDateTime != binCreationTime.dwLowDateTime ||
			creationTime.dwHighDateTime != binCreationTime.dwHighDateTime))
			load_text = true;
		else
		{
			r3dFile* f = r3d_open(bin_file, "rb");
			if(!LoadBin(f, use_default_material ))
				load_text = true;
			else
				res = true;
			fclose(f);
		}
	}
	else
		load_text = true;

	MatChunksNames = 0;
	if(load_text)
	{
		r3dFile* f = r3d_open(fname, "rt");
		int result = LoadAscii(f, use_default_material );
		if(result)
			res = true;
		else
			r3dArtBug("r3dMesh::Load(): Can't load '%s'\n", fname);
		fclose(f);

		if(result == 1) // save bin only if we didn't encounter any problems while loading text file
			save_bin = true;

		// to remain consistent
		if( !pWeights )
		{
			TryLoadWeights( fname );
		}
	}

	if(save_bin && !loading_archive)
	{
		this->OptimizeVCache() ;

		if(!SaveBin(bin_file) )
			r3dOutToLog("r3dMesh::Load(): Failed to save binary file mesh '%s'\n", bin_file);
		else
		{
			setFileTimestamp(bin_file, creationTime);
			// create ps3 version of mesh file
			// ptumik: disabled.
			/*char ps3_file[512];
			r3dscpy(ps3_file, fname);
			int len = strlen(fname);
			r3dscpy(&ps3_file[len-4], ".sc3");
			SaveBinPS3(ps3_file);
			setFileTimestamp(ps3_file, creationTime);*/
		}
	}
	if(MatChunksNames)
	{
		for(int i=0; i<NumMatChunks; ++i)
			delete [] MatChunksNames[i];
		delete [] MatChunksNames;
		MatChunksNames = 0;
	}

	// reset objects position to pivot. basically, in object's local coord system (0,0,0) = Pivot
	ResetXForm();

	FindAlphaTextures();

	return res;
}

int r3dMesh::LoadAscii( r3dFile *f, bool use_default_material )
{
	char inbuf[256], buf1[128];

	if(!f) 
		return 0;
	if(!f->IsValid()) 
		return 0;

	while(!feof(f)) {
		*inbuf = 0;
		if(fgets(inbuf, sizeof(inbuf), f) == NULL)
			break;
		if(strnicmp(inbuf,"[ObjectBegin]", 13) == 0)
			goto found;
	};

	return 0;

found:

	// Name=
	fgets(inbuf, sizeof(inbuf), f);
	sscanf(inbuf, "%s %s", buf1, Name);	
	if(stricmp("Name=", buf1))  {
		r3dArtBug("Invalid SCO\n");
		return 0;
	}

	int res = 1;

	r3dPoint3D CPoint;
	fgets(inbuf, sizeof(inbuf), f);
	sscanf(inbuf, "%s %f %f %f", buf1, &CPoint.X, &CPoint.Y, &CPoint.Z);

	// Verts=
	fgets(inbuf, sizeof(inbuf), f);
	sscanf(inbuf, "%s %d", buf1, &NumVertices);

	InitVertsList(NumVertices);
	vPivot        = CPoint;

	static bool showed_error = false;
	bool output_to_log_done = false;

	for(int i=0; i<NumVertices; i++)
	{
		fgets(inbuf, sizeof(inbuf), f);

		r3dPoint3D nrm;
		r3dPoint3D tangent;
		float      wtangent;
		int        clrr, clrg, clrb;
		int scanned = sscanf(inbuf, "%f %f %f %f %f %f %f %f %f %f %d %d %d", 
			&VertexPositions[i].x, 
			&VertexPositions[i].y, 
			&VertexPositions[i].z, 
			&nrm.x,
			&nrm.y,
			&nrm.z,
			&tangent.x,
			&tangent.y,
			&tangent.z,
			&wtangent,
			&clrr,
			&clrg,
			&clrb
			);

		// see if we have supplied normal
		if(scanned < 10)
		{
			res = 2; // make sure not to convert that file into binary, as there is something wrong with it
			if(!showed_error)
			{
				// DO NOT REMOVE THIS MESSAGE BOX!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				// FUCK ARTISTS TO CORRECT MESHES, BUT DO NOT REMOVE IT!!!!
				MessageBox(0, "You are loading mesh of old format!\nPlease check log file (r3dlog.txt) for 'MESH_LOAD_BUG' and re export those meshes.\nThey might have visual artifacts!\nAnd that mesh will not be exported to PS3!\n\nThis message will appear only once.", "Error!", MB_OK);
				showed_error = true;
			}
			if(!output_to_log_done) // output to log only once per mesh
			{
				r3dArtBug("[MESH_LOAD_BUG] mesh file '%s' of old format. Please re-export it!!!\n", f->GetFileName());
				output_to_log_done = true;
			}
			tangent = r3dVector(1,0,0);
			wtangent = 1;
			if(scanned < 4)
				nrm = r3dVector(0,1,0);
		}
		
		if(scanned == 13)
		{
			if(VertexColors == NULL)
			{
				// precise & bending are not supported at the same time
				r3d_assert( !( VertexFlags & vfPrecise ) ) ;

				VertexColors = new r3dColor[NumVertices];
				VertexFlags  |= vfBending ;
			}
			
			VertexColors[i] = r3dColor(clrr, clrg, clrb);
		}

		VertexNormals[i] = nrm;
		VertexTangents[i] = tangent;
		VertexTangentWs[i] = char( wtangent > 0 ? 255 : 0 );
	}

	// Faces=
	fgets(inbuf, sizeof(inbuf), f);
	int numFaces = 0;
	sscanf(inbuf, "%s %d", buf1, &numFaces);

	InitIndexList(numFaces*3);

	char current_material[128];
	memset(current_material, 0, 128);
	NumMatChunks = 0;
	r3d_assert(MatChunksNames == 0);
	MatChunksNames = new char*[256];
	for(int i=0; i<numFaces; i++)
	{
		int	tmp1, VIdx[3];
		float	TX[3], TY[3];
		char	matname[128];

		fgets(inbuf, sizeof(inbuf), f);
		sscanf(inbuf, "%d %d %d %d %s %f %f %f %f %f %f",
			&tmp1,
			&VIdx[0], &VIdx[1], &VIdx[2],
			matname,
			&TX[0], &TY[0], &TX[1], &TY[1], &TX[2], &TY[2]
		);
		if(i==0)
		{
			r3dscpy(current_material, matname);
			MatChunksNames[NumMatChunks]  = new char[128];
			r3dscpy_s(MatChunksNames[NumMatChunks], 128, matname);
			MatChunks[NumMatChunks].StartIndex = 0;
			if( use_default_material )
			{
				MatChunks[NumMatChunks].Mat = r3dMaterialLibrary::GetDefaultMaterial() ;
			}
			else
			{
				MatChunks[NumMatChunks].Mat = r3dMaterialLibrary::RequestMaterialByMesh(matname, f->GetFileName(), Flags & obfPlayerMesh ? true : false );
			}
			extern int _r3d_MatLib_SkipAllMaterials;
			if(!_r3d_MatLib_SkipAllMaterials)
				r3d_assert(MatChunks[NumMatChunks].Mat);
		}

		Indices[i*3+0] = VIdx[0];
		Indices[i*3+1] = VIdx[1];
		Indices[i*3+2] = VIdx[2];

		VertexUVs[VIdx[0]].x = TX[0];
		VertexUVs[VIdx[0]].y = TY[0];
		VertexUVs[VIdx[1]].x = TX[1];
		VertexUVs[VIdx[1]].y = TY[1];
		VertexUVs[VIdx[2]].x = TX[2];
		VertexUVs[VIdx[2]].y = TY[2];

		if(strcmp(current_material, matname)!=0)
		{
			// new material
			r3d_assert(i!=0);
			MatChunks[NumMatChunks].EndIndex = i*3;
			NumMatChunks++;
			r3d_assert(NumMatChunks < ConstNumMatChunks);

			MatChunks[NumMatChunks].StartIndex = i*3;

			if( use_default_material )
			{
				MatChunks[NumMatChunks].Mat = r3dMaterialLibrary::GetDefaultMaterial() ;
			}
			else
			{
				MatChunks[NumMatChunks].Mat = r3dMaterialLibrary::RequestMaterialByMesh(matname, f->GetFileName(), Flags & obfPlayerMesh ? true : false );
			}
			r3dscpy(current_material, matname);		
			MatChunksNames[NumMatChunks]  = new char[128];
			r3dscpy_s(MatChunksNames[NumMatChunks], 128, matname);
		}
	}
	MatChunks[NumMatChunks].EndIndex = numFaces*3;
	++NumMatChunks;

	InterlockedExchange( &m_Loaded, 1 ) ;

	_r3dFinishObjectLoading(this);

	return res;
}

//-----------------------------------------------------------------------
// this version us also used on PS3! Update PS3 too if you change it
static const uint32_t R3DMESH_BINARY_VERSION = 0xFADC0038;
uint32_t R3DMESH_BINARY_VERSION_get() { return R3DMESH_BINARY_VERSION; }

// return false to notify to load text version of mesh
// return true - success

bool r3dMesh::LoadBin(r3dFile *f, bool use_default_material )
{
	if(!f->IsValid()) 
		return false;

	uint32_t version;
	fread(&version, sizeof(uint32_t), 1, f);
	if( version != R3DMESH_BINARY_VERSION )
		return false;

	DWORD flags = 0;
	fread(&flags, sizeof flags, 1, f);

	int len = 0;
	fread(&len, sizeof(len), 1, f);
	memset(Name, 0, sizeof(Name));
	fread(Name, len, 1, f);
	
	fread(&vPivot, sizeof(r3dPoint3D), 1, f);

	fread(&NumVertices, sizeof(NumVertices), 1, f);
	InitVertsList(NumVertices);

	fread(VertexPositions, sizeof(r3dPoint3D)*NumVertices, 1, f);
	fread(VertexUVs, sizeof(r3dPoint2D)*NumVertices, 1, f);
	fread(VertexNormals, sizeof(r3dVector)*NumVertices, 1, f);
	fread(VertexTangents, sizeof(r3dVector)*NumVertices, 1, f);

	fread( VertexTangentWs, sizeof VertexTangentWs[0] * NumVertices, 1, f );

	fread(&NumIndices, sizeof(NumIndices), 1, f);
	InitIndexList(NumIndices);
	fread(Indices, sizeof(uint32_t)*NumIndices, 1, f);

	fread(&NumMatChunks, sizeof(NumMatChunks), 1, f);
	for(int i=0; i<NumMatChunks; ++i)
	{
		fread(&MatChunks[i].StartIndex, sizeof(int), 1, f);
		fread(&MatChunks[i].EndIndex, sizeof(int), 1, f);
		int len = 0;
		fread(&len, sizeof(int), 1, f);
		char* mat_name = new char[len+1];
		memset(mat_name, 0, len+1);
		fread(mat_name, len, 1, f);
		if( use_default_material )
		{
			MatChunks[i].Mat = r3dMaterialLibrary::GetDefaultMaterial() ;
		}
		else
		{
			MatChunks[i].Mat = r3dMaterialLibrary::RequestMaterialByMesh(mat_name, f->GetFileName(), Flags & obfPlayerMesh ? true : false );
		}
		delete [] mat_name;
	}

	if( flags & 1 )
	{
		AllocateWeights();

		LoadWeights_BinaryV1(f, true);
	}
	
	if( flags & 2 )
	{
		r3d_assert(VertexColors == NULL);
		VertexColors = new r3dColor[NumVertices];
		VertexFlags |= vfBending ;

		r3d_assert( !( VertexFlags & vfPrecise ) ) ;
		fread(VertexColors, sizeof(VertexColors[0])*NumVertices, 1, f);
	}

	InterlockedExchange( &m_Loaded, 1 ) ;

	_r3dFinishObjectLoading(this);

	return true;
}

//-----------------------------------------------------------------------
bool r3dMesh::SaveBin(const char* fname)
{
	FILE* f = fopen(fname, "wb");
	if(!f) 
		return false;

	fwrite(&R3DMESH_BINARY_VERSION, sizeof(uint32_t), 1, f);

	DWORD flags = 0;
	if(pWeights)
		flags |= 1;
	if(VertexColors)
		flags |= 2;
	fwrite(&flags, sizeof(DWORD), 1, f);

	int len = strlen(Name);
	fwrite(&len, sizeof(len), 1, f);
	fwrite(Name, len, 1, f);

	fwrite(&vPivot, sizeof(r3dPoint3D), 1, f);

	fwrite(&NumVertices, sizeof(NumVertices), 1, f);

	r3d_assert(VertexPositions);
	r3d_assert(VertexUVs);
	r3d_assert(VertexNormals);
	r3d_assert(VertexTangents);
	r3d_assert(VertexTangentWs);

	fwrite(VertexPositions, sizeof(r3dPoint3D)*NumVertices, 1, f);
	fwrite(VertexUVs, sizeof(r3dPoint2D)*NumVertices, 1, f);
	fwrite(VertexNormals, sizeof(r3dVector)*NumVertices, 1, f);
	fwrite(VertexTangents, sizeof(r3dVector)*NumVertices, 1, f);
	fwrite(VertexTangentWs, sizeof(VertexTangentWs[0]) * NumVertices, 1, f);

	fwrite(&NumIndices, sizeof(NumIndices), 1, f);
	fwrite(Indices, sizeof(uint32_t)*NumIndices, 1, f);

	fwrite(&NumMatChunks, sizeof(NumMatChunks), 1, f);
	for(int i=0; i<NumMatChunks; ++i)
	{
		fwrite(&MatChunks[i].StartIndex, sizeof(int), 1, f);
		fwrite(&MatChunks[i].EndIndex, sizeof(int), 1, f);
		int len = strlen(MatChunksNames[i]);//strlen(MatChunks[i].Mat->Name);
		fwrite(&len, sizeof(int), 1, f);
		//fwrite(MatChunks[i].Mat->Name, len, 1, f);
		fwrite(MatChunksNames[i], len, 1, f);
	}

	if( pWeights )
	{
		SaveWeights_BinaryV1(f);
	}
	
	if( VertexColors )
	{
		fwrite(VertexColors, sizeof(VertexColors[0])*NumVertices, 1, f);
	}

	fclose(f);
	return true;
}

using namespace r3dTL;
bool r3dMesh::SaveBinPS3(const char* fname)
{
	FILE* f = fopen(fname, "wb");
	if(!f) 
		return false;

	fwrite_be(R3DMESH_BINARY_VERSION, f);

	int len = strlen(Name);
	fwrite_be(len, f);
	fwrite_be(Name, len, f);

	fwrite_be(vPivot, f);

	fwrite_be(NumVertices, f);

	r3d_assert(VertexPositions);
	r3d_assert(VertexUVs);
	r3d_assert(VertexNormals);
	r3d_assert(VertexTangents);
	r3d_assert(VertexTangentWs);

	fwrite_be(VertexPositions, NumVertices, f);
	fwrite_be(VertexUVs, NumVertices, f);
	fwrite_be(VertexNormals, NumVertices, f);
	fwrite_be(VertexTangents, NumVertices, f);
	fwrite_be(VertexTangentWs, sizeof(VertexTangentWs[ 0 ]) * NumVertices, f);

	fwrite_be(NumIndices, f);
	fwrite_be(Indices, NumIndices, f);

	fwrite_be(NumMatChunks, f);
	for(int i=0; i<NumMatChunks; ++i)
	{
		fwrite_be(MatChunks[i].StartIndex, f);
		fwrite_be(MatChunks[i].EndIndex, f);
		int len = strlen(MatChunksNames[i]);
		fwrite_be(len, f);
		fwrite_be(MatChunksNames[i], len, f);
	}

	fclose(f);
	return true;
}

//------------------------------------------------------------------------

void
r3dMesh::FindAlphaTextures()
{
	r3d_assert( m_Loaded ) ;

	HasAlphaTextures = false;
	extern int _r3d_MatLib_SkipAllMaterials;
	if(_r3d_MatLib_SkipAllMaterials)
		return;

	for (int i=0;i<NumMatChunks;i++)
	{
		r3dMaterial* Mat = MatChunks[i].Mat;

		if( Mat->Flags & R3D_MAT_HASALPHA )
		{
			HasAlphaTextures = true;
			break;
		}
	}	

}

//------------------------------------------------------------------------

/*static*/
void
r3dMesh::Init()
{
	g_MeshLoadParamsArray.Init( 64 ) ;
}

//------------------------------------------------------------------------

/*static*/
void
r3dMesh::Close()
{

}
