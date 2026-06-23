#pragma once

#include "RENDERING/DX11/RenderDX11Resources.h"

class r3dCamera;
class r3dDX11CommonStates;
class r3dDX11DrawContext;
class r3dDX11InputLayout;
class r3dDX11PixelShader;
class r3dDX11ShaderLibrary;
class r3dDX11TextureLibrary;
class r3dDX11VertexShader;
struct r3dDX11WorldRenderStats;
struct ID3D11Device;
struct ID3D11RasterizerState;
struct ID3D11ShaderResourceView;
struct GrassTextureCell;
struct GrassMaskTextureEntry;
struct ID3D11Texture2D;

struct r3dDX11GrassInstance
{
	float PositionAlphaRef[4];
	float ScaleTint[4];
	float AnimParams[4];
	float CellParams[4];
};

struct r3dDX11GrassConstants
{
	float ViewProj[16];
	float CameraPos_Time[4];
	float Params[4];
};

class r3dDX11GrassPass final
{
public:
	r3dDX11GrassPass();
	~r3dDX11GrassPass();

	bool Init(
		ID3D11Device* device,
		r3dDX11DrawContext* drawContext,
		r3dDX11ShaderLibrary* shaderLibrary,
		r3dDX11CommonStates* commonStates,
		r3dDX11TextureLibrary* textureLibrary
	);

	void Shutdown();

	bool RenderGBuffer(
		const r3dCamera& camera,
		const D3DXMATRIX& viewProj,
		r3dDX11WorldRenderStats* stats
	);

	bool RenderDepth(
		const r3dCamera& camera,
		const D3DXMATRIX& viewProj,
		r3dDX11WorldRenderStats* stats
	);

	bool RenderShadow(
		const r3dCamera& camera,
		const D3DXMATRIX& viewProj,
		r3dDX11WorldRenderStats* stats,
		float depthBias
	);

	bool IsInitialized() const;

	struct ChunkGpu
	{
		r3dDX11VertexBuffer VertexBuffers[5];
		r3dDX11IndexBuffer IndexBuffer;
		unsigned int VertexCounts[5];
		unsigned int IndexCount;

		ChunkGpu()
		{
			for( int i = 0; i < 5; ++i )
				VertexCounts[i] = 0;

			IndexCount = 0;
		}
	};

	struct TextureGpu
	{
		ID3D11Texture2D* Texture;
		ID3D11ShaderResourceView* SRV;
		int Width;
		int Height;

		TextureGpu();
	};

private:
	bool CreateShadersAndLayout(ID3D11Device* device);
	bool CreateRasterizers(ID3D11Device* device);
	bool EnsureInstanceCapacity(unsigned int count);
	bool EnsureChunkGpu(int typeIdx, unsigned int chunkIdx, ChunkGpu** outGpu);
	bool DrawInternal(
		const r3dCamera& camera,
		const D3DXMATRIX& viewProj,
		bool depthOnly,
		r3dDX11WorldRenderStats* stats
	);

	bool EnsureHeightTextureGpu(const GrassTextureCell& texCell, TextureGpu** outGpu);
	bool EnsureMaskTextureGpu(const GrassMaskTextureEntry* maskEntry, TextureGpu** outGpu);

	void SetCommonStates(bool depthOnly);
	void ReleaseChunkGpu();

private:
	ID3D11Device* Device = nullptr;
	r3dDX11DrawContext* DrawContext = nullptr;
	r3dDX11ShaderLibrary* ShaderLibrary = nullptr;
	r3dDX11CommonStates* CommonStates = nullptr;
	r3dDX11TextureLibrary* TextureLibrary = nullptr;

	r3dDX11VertexShader* GrassVS = nullptr;
	r3dDX11PixelShader* GrassGBufferPS = nullptr;
	r3dDX11PixelShader* GrassDepthPS = nullptr;
	r3dDX11InputLayout* GrassLayout = nullptr;

	r3dDX11ConstantBuffer GrassConstants;
	r3dDX11VertexBuffer InstanceBuffer;

	ID3D11RasterizerState* CullNoneRasterizer = nullptr;
	ID3D11RasterizerState* ShadowRasterizer = nullptr;

	unsigned int InstanceCapacity = 0;
	bool bInitialized = false;
};