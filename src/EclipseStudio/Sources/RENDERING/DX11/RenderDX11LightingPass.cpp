#include "r3dPCH.h"
#include "r3d.h"
#include "r3dCam.h"
#include "r3dLight.h"

#include "RENDERING/DX11/RenderDX11LightingPass.h"

#include "RENDERING/DX11/RenderDX11Draw.h"
#include "RENDERING/DX11/RenderDX11GBufferResources.h"
#include "RENDERING/DX11/RenderDX11States.h"
#include "RENDERING/DX11/ShaderDX11.h"
#include "RENDERING/DX11/RenderDX11World.h"

#include <algorithm>
#include <cstring>

extern r3dLightSystem WorldLightSystem;

namespace
{
	static const unsigned int DX11_MAX_DEFERRED_LIGHTS = 64;

	enum DX11LightType
	{
		DX11_LIGHT_POINT = 1,
		DX11_LIGHT_SPOT = 2
	};

	struct FullscreenVertex
	{
		float Position[3];
		float TexCoord[2];
	};

	struct DX11LightingFrameConstants
	{
		float InvViewProj[16];

		float CameraPos[4];
		float CameraForward[4];

		float SunDirection[4];
		float SunColorIntensity[4];

		float AmbientColorIntensity[4];
		float ProbeColorIntensity[4];

		float FogColorDensity[4];
		float FogParams[4];

		float ScreenSize[4];

		// x = opaque shadow enabled
		// y = opaque cascade count
		// z = transparent shadow enabled
		// w = opaque shadow strength
		float ShadowParams[4];

		float ShadowViewProj[R3D_DX11_MAX_SHADOW_SLICES][16];

		// x = split near
		// y = split far
		// z = compare bias
		// w = shadow texel size
		float ShadowCascadeParams[R3D_DX11_MAX_SHADOW_SLICES][4];

		float TransparentShadowViewProj[16];

		// x = compare bias
		// y = texel size
		// z = strength
		// w = enabled
		float TransparentShadowParams[4];

		float Options[4];
	};

	struct DX11ShaderLight
	{
		float PositionRadius[4];
		float ColorIntensity[4];
		float DirectionAngles[4];
		float Params[4];
	};

	struct DX11LightingLightConstants
	{
		float LightCount[4];
		DX11ShaderLight Lights[DX11_MAX_DEFERRED_LIGHTS];
	};

	float NormalizeColorComponent(float value)
	{
		return value > 1.0f ? value / 255.0f : value;
	}

	float NormalizeDX11LightIntensity(float value, float fallback, float maxValue)
	{
		if (value <= 0.0f)
			return fallback;

		// В старом DX9 path intensity часто может быть сильно больше 1.
		// Для DX11 lighting пока нормализуем, иначе HDR улетает в белый экран.
		if (value > 8.0f)
			value *= 0.05f;

		if (value > maxValue)
			value = maxValue;

		if (value < 0.0f)
			value = 0.0f;

		return value;
	}

	void NormalizeVector3(float& x, float& y, float& z)
	{
		const float lenSq = x * x + y * y + z * z;

		if (lenSq <= 0.000001f)
		{
			x = 0.0f;
			y = -1.0f;
			z = 0.0f;
			return;
		}

		const float invLen = 1.0f / sqrtf(lenSq);

		x *= invLen;
		y *= invLen;
		z *= invLen;
	}

	void CopyMatrix(float outMatrix[16], const D3DXMATRIX& matrix)
	{
		memcpy(outMatrix, &matrix, sizeof(float) * 16);
	}

	void SetIdentityMatrix(float outMatrix[16])
	{
		memset(outMatrix, 0, sizeof(float) * 16);

		outMatrix[0] = 1.0f;
		outMatrix[5] = 1.0f;
		outMatrix[10] = 1.0f;
		outMatrix[15] = 1.0f;
	}

	void CopyFloatMatrix(float outMatrix[16], const float inMatrix[16])
	{
		memcpy(
			outMatrix,
			inMatrix,
			sizeof(float) * 16
		);
	}

	void FillShadowConstants(DX11LightingFrameConstants& constants)
	{
		for (unsigned int i = 0; i < R3D_DX11_MAX_SHADOW_SLICES; ++i)
		{
			SetIdentityMatrix(constants.ShadowViewProj[i]);

			constants.ShadowCascadeParams[i][0] = 0.0f;
			constants.ShadowCascadeParams[i][1] = 0.0f;
			constants.ShadowCascadeParams[i][2] = 0.00005f;
			constants.ShadowCascadeParams[i][3] = 1.0f / 2048.0f;
		}

		SetIdentityMatrix(constants.TransparentShadowViewProj);

		constants.ShadowParams[0] = 0.0f;
		constants.ShadowParams[1] = 0.0f;
		constants.ShadowParams[2] = 0.0f;
		constants.ShadowParams[3] = 0.82f;

		constants.TransparentShadowParams[0] = 0.00005f;
		constants.TransparentShadowParams[1] = 1.0f / 512.0f;
		constants.TransparentShadowParams[2] = 0.42f;
		constants.TransparentShadowParams[3] = 0.0f;

		int cascadeCount = 0;

		for (unsigned int i = 0; i < R3D_DX11_MAX_SHADOW_SLICES; ++i)
		{
			r3dDX11ShadowSliceInfo shadowInfo;

			if (!r3dDX11GetShadowSliceInfo(static_cast<int>(i), shadowInfo))
				continue;

			CopyFloatMatrix(
				constants.ShadowViewProj[cascadeCount],
				shadowInfo.ViewProj
			);

			constants.ShadowCascadeParams[cascadeCount][0] =
				shadowInfo.SplitNear;

			constants.ShadowCascadeParams[cascadeCount][1] =
				shadowInfo.SplitFar;

			constants.ShadowCascadeParams[cascadeCount][2] =
				shadowInfo.DepthBias;

			constants.ShadowCascadeParams[cascadeCount][3] =
				shadowInfo.TexelSize;

			constants.ShadowParams[3] =
				shadowInfo.Strength;

			++cascadeCount;
		}

		constants.ShadowParams[0] =
			cascadeCount > 0 ? 1.0f : 0.0f;

		constants.ShadowParams[1] =
			static_cast<float>(cascadeCount);

		r3dDX11TransparentShadowInfo transparentInfo;

		if (r3dDX11GetTransparentShadowInfo(transparentInfo))
		{
			CopyFloatMatrix(
				constants.TransparentShadowViewProj,
				transparentInfo.ViewProj
			);

			constants.TransparentShadowParams[0] =
				transparentInfo.DepthBias;

			constants.TransparentShadowParams[1] =
				transparentInfo.TexelSize;

			constants.TransparentShadowParams[2] =
				transparentInfo.Strength;

			constants.TransparentShadowParams[3] =
				transparentInfo.Enabled;

			constants.ShadowParams[2] = 1.0f;
		}
	}

	void BuildInverseViewProj(float outMatrix[16])
	{
		D3DXMATRIX viewProj;
		D3DXMATRIX invViewProj;

		if (r3dRenderer)
		{
			viewProj = r3dRenderer->ViewProjMatrix;
			D3DXMatrixInverse(&invViewProj, nullptr, &viewProj);
		}
		else
		{
			D3DXMatrixIdentity(&invViewProj);
		}

		CopyMatrix(outMatrix, invViewProj);
	}

	void FillDefaultDirectionalLight(DX11LightingFrameConstants& constants)
	{
		float dirX = -0.35f;
		float dirY = -0.85f;
		float dirZ = 0.25f;

		NormalizeVector3(dirX, dirY, dirZ);

		constants.SunDirection[0] = dirX;
		constants.SunDirection[1] = dirY;
		constants.SunDirection[2] = dirZ;
		constants.SunDirection[3] = 0.0f;

		constants.SunColorIntensity[0] = 1.00f;
		constants.SunColorIntensity[1] = 0.92f;
		constants.SunColorIntensity[2] = 0.78f;
		constants.SunColorIntensity[3] = 0.85f;
	}

	void TryFillDirectionalLightFromWorld(DX11LightingFrameConstants& constants, r3dDX11WorldRenderStats* stats)
	{
		FillDefaultDirectionalLight(constants);

		for (uint32_t i = 0, e = WorldLightSystem.Lights.Count(); i < e; ++i)
		{
			r3dLight* light = WorldLightSystem.Lights[i];

			if (!light || !light->IsOn())
				continue;

			if (light->Type != R3D_DIRECT_LIGHT)
				continue;

			float dirX = light->Direction.x;
			float dirY = light->Direction.y;
			float dirZ = light->Direction.z;

			NormalizeVector3(dirX, dirY, dirZ);

			constants.SunDirection[0] = dirX;
			constants.SunDirection[1] = dirY;
			constants.SunDirection[2] = dirZ;
			constants.SunDirection[3] = 0.0f;

			constants.SunColorIntensity[0] = NormalizeColorComponent(light->R);
			constants.SunColorIntensity[1] = NormalizeColorComponent(light->G);
			constants.SunColorIntensity[2] = NormalizeColorComponent(light->B);
			constants.SunColorIntensity[3] = NormalizeDX11LightIntensity(light->Intensity, 0.85f, 1.35f);

			if (stats)
				++stats->LightingDirectionalLights;

			return;
		}

		if (stats)
			++stats->LightingDirectionalLights;
	}

	void FillFrameConstants(
		const r3dCamera& camera,
		r3dDX11GBufferResources& gbuffer,
		r3dDX11WorldRenderStats* stats,
		DX11LightingFrameConstants& constants
	)
	{
		memset(&constants, 0, sizeof(constants));

		BuildInverseViewProj(constants.InvViewProj);

		constants.CameraPos[0] = camera.X;
		constants.CameraPos[1] = camera.Y;
		constants.CameraPos[2] = camera.Z;
		constants.CameraPos[3] = 1.0f;

		float forwardX = camera.vPointTo.x;
		float forwardY = camera.vPointTo.y;
		float forwardZ = camera.vPointTo.z;

		NormalizeVector3(
			forwardX,
			forwardY,
			forwardZ
		);

		constants.CameraForward[0] = forwardX;
		constants.CameraForward[1] = forwardY;
		constants.CameraForward[2] = forwardZ;
		constants.CameraForward[3] = 0.0f;

		TryFillDirectionalLightFromWorld(constants, stats);

		constants.AmbientColorIntensity[0] = 0.055f;
		constants.AmbientColorIntensity[1] = 0.065f;
		constants.AmbientColorIntensity[2] = 0.075f;
		constants.AmbientColorIntensity[3] = 1.0f;

		constants.ProbeColorIntensity[0] = 0.030f;
		constants.ProbeColorIntensity[1] = 0.035f;
		constants.ProbeColorIntensity[2] = 0.040f;
		constants.ProbeColorIntensity[3] = 1.0f;

		constants.FogColorDensity[0] = 0.42f;
		constants.FogColorDensity[1] = 0.48f;
		constants.FogColorDensity[2] = 0.52f;
		constants.FogColorDensity[3] = 0.015f;

		constants.FogParams[0] = 350.0f;
		constants.FogParams[1] = 4200.0f;
		constants.FogParams[2] = 1.0f;
		constants.FogParams[3] = 0.0f;

		constants.ScreenSize[0] = static_cast<float>(gbuffer.GetWidth());
		constants.ScreenSize[1] = static_cast<float>(gbuffer.GetHeight());
		constants.ScreenSize[2] = gbuffer.GetWidth() > 0 ? 1.0f / static_cast<float>(gbuffer.GetWidth()) : 1.0f;
		constants.ScreenSize[3] = gbuffer.GetHeight() > 0 ? 1.0f / static_cast<float>(gbuffer.GetHeight()) : 1.0f;

		FillShadowConstants(constants);

		int dx11DebugView = 0;

		if (r_dx11_debug_view)
		{
			dx11DebugView = r_dx11_debug_view->GetInt();

			if (dx11DebugView < 0)
				dx11DebugView = 0;

			if (dx11DebugView > 7)
				dx11DebugView = 7;
		}

		// Options contract:
		// x = debug view mode
		// y = ambient/probe enabled
		// z = fog enabled
		// w = spec/gloss enabled
		constants.Options[0] = static_cast<float>(dx11DebugView);
		constants.Options[1] = 1.0f;
		constants.Options[2] = 1.0f;
		constants.Options[3] = 1.0f;

		if (stats)
		{
			stats->LightingGBufferDecoded = 1;
			stats->LightingSpecGlossDecoded = 1;
			stats->LightingFogApplied = 1;
			stats->LightingAmbientApplied = 1;
			stats->LightingProbeApplied = 1;
			stats->LightingShadowed = constants.ShadowParams[1] > 0.0f ? 1 : 0;
		}
	}

	void AddPointLight(
		const r3dLight& light,
		DX11LightingLightConstants& constants,
		unsigned int& count,
		r3dDX11WorldRenderStats* stats
	)
	{
		if (count >= DX11_MAX_DEFERRED_LIGHTS)
			return;

		DX11ShaderLight& outLight = constants.Lights[count++];

		memset(&outLight, 0, sizeof(outLight));

		outLight.PositionRadius[0] = light.X;
		outLight.PositionRadius[1] = light.Y;
		outLight.PositionRadius[2] = light.Z;
		outLight.PositionRadius[3] = std::max(1.0f, light.Radius2);

		outLight.ColorIntensity[0] = NormalizeColorComponent(light.R);
		outLight.ColorIntensity[1] = NormalizeColorComponent(light.G);
		outLight.ColorIntensity[2] = NormalizeColorComponent(light.B);
		outLight.ColorIntensity[3] = std::max(0.0f, light.Intensity);

		outLight.DirectionAngles[0] = 0.0f;
		outLight.DirectionAngles[1] = -1.0f;
		outLight.DirectionAngles[2] = 0.0f;
		outLight.DirectionAngles[3] = 0.0f;

		outLight.Params[0] = static_cast<float>(DX11_LIGHT_POINT);
		outLight.Params[1] = std::max(0.0f, light.Att0);
		outLight.Params[2] = std::max(0.0f, light.Att1);
		outLight.Params[3] = std::max(0.0f, light.Att2);

		if (stats)
			++stats->LightingPointLights;
	}

	void AddSpotLight(
		const r3dLight& light,
		DX11LightingLightConstants& constants,
		unsigned int& count,
		r3dDX11WorldRenderStats* stats
	)
	{
		if (count >= DX11_MAX_DEFERRED_LIGHTS)
			return;

		DX11ShaderLight& outLight = constants.Lights[count++];

		memset(&outLight, 0, sizeof(outLight));

		float dirX = light.Direction.x;
		float dirY = light.Direction.y;
		float dirZ = light.Direction.z;

		NormalizeVector3(dirX, dirY, dirZ);

		float innerAngle = light.SpotAngleInner;
		float outerAngle = light.SpotAngleOuter;

		if (outerAngle <= 0.0f)
			outerAngle = 45.0f;

		if (innerAngle <= 0.0f)
			innerAngle = outerAngle * 0.75f;

		if (innerAngle > outerAngle)
			std::swap(innerAngle, outerAngle);

		innerAngle = std::max(0.1f, std::min(innerAngle, 178.0f));
		outerAngle = std::max(innerAngle + 0.1f, std::min(outerAngle, 179.0f));

		const float innerCos =
			cosf(D3DXToRadian(innerAngle));

		const float outerCos =
			cosf(D3DXToRadian(outerAngle));

		const float spotRange =
			std::max(0.001f, innerCos - outerCos);

		outLight.PositionRadius[0] = light.X;
		outLight.PositionRadius[1] = light.Y;
		outLight.PositionRadius[2] = light.Z;
		outLight.PositionRadius[3] = std::max(1.0f, light.Radius2);

		outLight.ColorIntensity[0] = NormalizeColorComponent(light.R);
		outLight.ColorIntensity[1] = NormalizeColorComponent(light.G);
		outLight.ColorIntensity[2] = NormalizeColorComponent(light.B);
		outLight.ColorIntensity[3] =
			NormalizeDX11LightIntensity(
				light.Intensity,
				1.0f,
				8.0f
			);

		outLight.DirectionAngles[0] = dirX;
		outLight.DirectionAngles[1] = dirY;
		outLight.DirectionAngles[2] = dirZ;
		outLight.DirectionAngles[3] = innerCos;

		outLight.Params[0] = static_cast<float>(DX11_LIGHT_SPOT);
		outLight.Params[1] = outerCos;
		outLight.Params[2] =
			std::max(
				0.01f,
				light.SpotAngleFalloffPow
			) / spotRange;
		outLight.Params[3] = light.bAffectSpecular ? 1.0f : 0.0f;

		if (stats)
			++stats->LightingSpotLights;
	}

	void FillLightConstants(DX11LightingLightConstants& constants, r3dDX11WorldRenderStats* stats)
	{
		memset(&constants, 0, sizeof(constants));

		unsigned int count = 0;

		for (uint32_t i = 0, e = WorldLightSystem.Lights.Count(); i < e; ++i)
		{
			r3dLight* light = WorldLightSystem.Lights[i];

			if (!light || !light->IsOn())
				continue;

			if (light->Type == R3D_OMNI_LIGHT)
			{
				AddPointLight(*light, constants, count, stats);
				continue;
			}

			if (light->Type == R3D_SPOT_LIGHT || light->Type == R3D_PROJECTOR_LIGHT)
			{
				AddSpotLight(*light, constants, count, stats);
				continue;
			}
		}

		constants.LightCount[0] = static_cast<float>(count);
		constants.LightCount[1] = static_cast<float>(DX11_MAX_DEFERRED_LIGHTS);
		constants.LightCount[2] = 0.0f;
		constants.LightCount[3] = 0.0f;
	}
}

r3dDX11LightingPass::r3dDX11LightingPass()
{
}

r3dDX11LightingPass::~r3dDX11LightingPass()
{
	Shutdown();
}

bool r3dDX11LightingPass::Init(
	ID3D11Device* device,
	r3dDX11DrawContext* drawContext,
	r3dDX11ShaderLibrary* shaderLibrary,
	r3dDX11CommonStates* commonStates
)
{
	if (bInitialized)
		return true;

	if (!device || !drawContext || !shaderLibrary || !commonStates)
		return false;

	DrawContext = drawContext;
	ShaderLibrary = shaderLibrary;
	CommonStates = commonStates;

	if (
		!CreateShadersAndLayout(device) ||
		!CreateGeometry(device) ||
		!CreateConstantBuffers(device)
	)
	{
		Shutdown();
		return false;
	}

	bInitialized = true;

	r3dOutToLog("[DX11] Lighting pass initialized\n");

	return true;
}

void r3dDX11LightingPass::Shutdown()
{
	FrameConstants.Shutdown();
	LightConstants.Shutdown();

	delete VertexBuffer;
	VertexBuffer = nullptr;

	delete InputLayout;
	InputLayout = nullptr;

	FullscreenVS = nullptr;
	LightingPS = nullptr;

	CommonStates = nullptr;
	ShaderLibrary = nullptr;
	DrawContext = nullptr;

	bInitialized = false;
}

bool r3dDX11LightingPass::Render(
	const r3dCamera& camera,
	r3dDX11GBufferResources& gbuffer,
	r3dDX11RenderTarget& sceneColor,
	r3dDX11WorldRenderStats* stats
)
{
	if (
		!bInitialized ||
		!DrawContext ||
		!gbuffer.IsInitialized() ||
		!sceneColor.IsValid()
	)
	{
		if (stats)
			++stats->LightingSkippedFailed;

		return false;
	}

	DX11LightingFrameConstants frameConstants;
	DX11LightingLightConstants lightConstants;

	FillFrameConstants(
		camera,
		gbuffer,
		stats,
		frameConstants
	);

	FillLightConstants(
		lightConstants,
		stats
	);

	if (
		!FrameConstants.Update(
			DrawContext->GetContext(),
			&frameConstants,
			sizeof(frameConstants)
		) ||
		!LightConstants.Update(
			DrawContext->GetContext(),
			&lightConstants,
			sizeof(lightConstants)
		)
	)
	{
		if (stats)
			++stats->LightingSkippedFailed;

		return false;
	}

	ID3D11DeviceContext* dxContext =
	DrawContext->GetContext();

	if (!dxContext)
	{
		if (stats)
			++stats->LightingSkippedFailed;

		return false;
	}

	ID3D11RenderTargetView* lightingRTV =
		sceneColor.GetRTV();

	dxContext->OMSetRenderTargets(
		1,
		&lightingRTV,
		nullptr
	);

	D3D11_VIEWPORT lightingViewport{};
	lightingViewport.TopLeftX = 0.0f;
	lightingViewport.TopLeftY = 0.0f;
	lightingViewport.Width = static_cast<float>(sceneColor.GetWidth());
	lightingViewport.Height = static_cast<float>(sceneColor.GetHeight());
	lightingViewport.MinDepth = 0.0f;
	lightingViewport.MaxDepth = 1.0f;

	dxContext->RSSetViewports(
		1,
		&lightingViewport
	);

	DrawContext->SetRasterizerState(
		CommonStates->GetCullNoneRasterizer()
	);

	DrawContext->SetDepthStencilState(
		CommonStates->GetDepthDisabledState()
	);

	DrawContext->SetBlendState(
		CommonStates->GetOpaqueBlendState()
	);

	DrawContext->SetInputLayout(
		InputLayout
	);

	DrawContext->SetTopology(
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);

	DrawContext->SetVertexBuffer(
		VertexBuffer
	);

	DrawContext->SetShaders(
		FullscreenVS,
		LightingPS
	);

	FrameConstants.BindPS(
		DrawContext->GetContext(),
		0
	);

	LightConstants.BindPS(
		DrawContext->GetContext(),
		1
	);

	DrawContext->SetSampler(
		0,
		CommonStates->GetLinearClampSampler()
	);

	DrawContext->SetShaderResource(
		0,
		gbuffer.GetColor().GetSRV()
	);

	DrawContext->SetShaderResource(
		1,
		gbuffer.GetNormal().GetSRV()
	);

	DrawContext->SetShaderResource(
		2,
		gbuffer.GetLinearDepth().GetSRV()
	);

	DrawContext->SetShaderResource(
		3,
		gbuffer.GetAux().GetSRV()
	);

	DrawContext->SetSampler(
		1,
		CommonStates->GetLinearClampSampler()
	);

	for (unsigned int i = 0; i < R3D_DX11_MAX_SHADOW_SLICES; ++i)
	{
		r3dDX11ShadowSliceInfo shadowInfo;

		ID3D11ShaderResourceView* shadowSRV =
			r3dDX11GetShadowSliceInfo(static_cast<int>(i), shadowInfo)
				? shadowInfo.SRV
				: nullptr;

		DrawContext->SetShaderResource(
			4 + i,
			shadowSRV
		);
	}

	r3dDX11TransparentShadowInfo transparentShadowInfo;

	DrawContext->SetShaderResource(
		8,
		r3dDX11GetTransparentShadowInfo(transparentShadowInfo)
			? transparentShadowInfo.SRV
			: nullptr
	);

	DrawContext->Draw(3);

	ID3D11ShaderResourceView* nullSRV = nullptr;

	for (unsigned int i = 0; i < 9; ++i)
		DrawContext->SetShaderResource(i, nullSRV);

	if (stats)
		++stats->LightingPasses;

	return true;
}

bool r3dDX11LightingPass::IsInitialized() const
{
	return bInitialized;
}

bool r3dDX11LightingPass::CreateShadersAndLayout(ID3D11Device* device)
{
	const int vsIndex =
		ShaderLibrary->AddVertexShader(
			"VS_DX11_LIGHTING_FULLSCREEN",
			"Lighting_fullscreen_vs.hls"
		);

	const int psIndex =
		ShaderLibrary->AddPixelShader(
			"PS_DX11_DEFERRED_LIGHTING",
			"Lighting_deferred_ps.hls"
		);

	if (vsIndex < 0 || psIndex < 0)
	{
		r3dOutToLog(
			"[DX11] Lighting shader registration failed: %s\n",
			ShaderLibrary->GetLastError().c_str()
		);

		return false;
	}

	FullscreenVS =
		ShaderLibrary->GetVertexShader(
			vsIndex
		);

	LightingPS =
		ShaderLibrary->GetPixelShader(
			psIndex
		);

	if (
		!FullscreenVS ||
		!LightingPS ||
		!FullscreenVS->IsValid() ||
		!LightingPS->IsValid()
	)
	{
		return false;
	}

	const D3D11_INPUT_ELEMENT_DESC inputElements[] =
	{
		{
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			offsetof(FullscreenVertex, Position),
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		},
		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			offsetof(FullscreenVertex, TexCoord),
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		}
	};

	InputLayout = new r3dDX11InputLayout();

	if (!InputLayout->Create(
			device,
			inputElements,
			static_cast<unsigned int>(_countof(inputElements)),
			FullscreenVS->GetBytecode(),
			FullscreenVS->GetBytecodeSize(),
			"DX11.Lighting.InputLayout"))
	{
		delete InputLayout;
		InputLayout = nullptr;
		return false;
	}

	return true;
}

bool r3dDX11LightingPass::CreateGeometry(ID3D11Device* device)
{
	const FullscreenVertex vertices[] =
	{
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
		{ { -1.0f,  3.0f, 0.0f }, { 0.0f, -1.0f } },
		{ {  3.0f, -1.0f, 0.0f }, { 2.0f, 1.0f } }
	};

	VertexBuffer = new r3dDX11VertexBuffer();

	if (!VertexBuffer->Create(
			device,
			sizeof(vertices),
			sizeof(FullscreenVertex),
			vertices,
			R3D_DX11_BUFFER_IMMUTABLE,
			"DX11.Lighting.FullscreenVB"))
	{
		delete VertexBuffer;
		VertexBuffer = nullptr;
		return false;
	}

	return true;
}

bool r3dDX11LightingPass::CreateConstantBuffers(ID3D11Device* device)
{
	if (!FrameConstants.Create(
			device,
			sizeof(DX11LightingFrameConstants),
			"DX11.Lighting.FrameCB"))
	{
		return false;
	}

	if (!LightConstants.Create(
			device,
			sizeof(DX11LightingLightConstants),
			"DX11.Lighting.LightCB"))
	{
		return false;
	}

	return true;
}