#include "r3dPCH.h"
#include "r3d.h"
#include "r3dCam.h"
#include "r3dLight.h"

#include "RENDERING/DX11/RenderDX11SkyPass.h"

#include "RENDERING/DX11/RenderDX11Draw.h"
#include "RENDERING/DX11/RenderDX11GBufferResources.h"
#include "RENDERING/DX11/RenderDX11States.h"
#include "RENDERING/DX11/ShaderDX11.h"

#include <cstring>

extern r3dLightSystem WorldLightSystem;

namespace
{
	struct SkyVertex
	{
		float Position[3];
		float TexCoord[2];
	};

	struct DX11SkyConstants
	{
		float InvViewProj[16];

		float CameraPos[4];

		float SunDirection[4];
		float SunColorIntensity[4];

		float HorizonColor[4];
		float ZenithColor[4];

		float FogColorDensity[4];
		float CloudParams[4];

		float MoonParams[4];
		float Options[4];
	};

	float NormalizeColorComponent(float value)
	{
		return value > 1.0f ? value / 255.0f : value;
	}

	void NormalizeVector3(float& x, float& y, float& z)
	{
		const float lenSq = x * x + y * y + z * z;

		if (lenSq <= 0.000001f)
		{
			x = -0.35f;
			y = -0.85f;
			z = 0.25f;
		}

		const float invLen =
			1.0f / sqrtf(x * x + y * y + z * z);

		x *= invLen;
		y *= invLen;
		z *= invLen;
	}

	void CopyMatrix(float outMatrix[16], const D3DXMATRIX& matrix)
	{
		memcpy(outMatrix, &matrix, sizeof(float) * 16);
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

	void FillDefaultSun(DX11SkyConstants& constants)
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
		constants.SunColorIntensity[3] = 0.65f;
	}

	void TryFillSunFromWorld(DX11SkyConstants& constants)
	{
		FillDefaultSun(constants);

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

			float intensity = light->Intensity;

			if (intensity <= 0.0f)
				intensity = 0.65f;

			if (intensity > 8.0f)
				intensity *= 0.05f;

			if (intensity > 1.0f)
				intensity = 1.0f;

			constants.SunColorIntensity[3] = intensity;

			return;
		}
	}

	void FillSkyConstants(const r3dCamera& camera, DX11SkyConstants& constants)
	{
		memset(&constants, 0, sizeof(constants));

		BuildInverseViewProj(constants.InvViewProj);

		constants.CameraPos[0] = camera.X;
		constants.CameraPos[1] = camera.Y;
		constants.CameraPos[2] = camera.Z;
		constants.CameraPos[3] = 1.0f;

		TryFillSunFromWorld(constants);

		// Clear color / horizon parity.
		// Эти цвета пока процедурные, потом можно взять из старого time-of-day / environment.
		constants.HorizonColor[0] = 0.42f;
		constants.HorizonColor[1] = 0.47f;
		constants.HorizonColor[2] = 0.50f;
		constants.HorizonColor[3] = 1.0f;

		constants.ZenithColor[0] = 0.055f;
		constants.ZenithColor[1] = 0.075f;
		constants.ZenithColor[2] = 0.105f;
		constants.ZenithColor[3] = 1.0f;

		constants.FogColorDensity[0] = 0.42f;
		constants.FogColorDensity[1] = 0.48f;
		constants.FogColorDensity[2] = 0.52f;
		constants.FogColorDensity[3] = 0.85f;

		// x = cloud amount
		// y = cloud scale
		// z = cloud speed/time
		// w = cloud softness
		constants.CloudParams[0] = 0.45f;
		constants.CloudParams[1] = 1.35f;
		constants.CloudParams[2] = r3dGetTime() * 0.006f;
		constants.CloudParams[3] = 0.18f;

		// x = moon enabled
		// y = moon disk power
		// z = moon glow power
		// w = moon intensity
		constants.MoonParams[0] = 1.0f;
		constants.MoonParams[1] = 900.0f;
		constants.MoonParams[2] = 20.0f;
		constants.MoonParams[3] = 0.20f;

		// x = sky enabled
		// y = sun/moon enabled
		// z = clouds enabled
		// w = horizon fog enabled
		constants.Options[0] = 1.0f;
		constants.Options[1] = 1.0f;
		constants.Options[2] = 1.0f;
		constants.Options[3] = 1.0f;
	}
}

r3dDX11SkyPass::r3dDX11SkyPass()
{
}

r3dDX11SkyPass::~r3dDX11SkyPass()
{
	Shutdown();
}

bool r3dDX11SkyPass::Init(
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

	r3dOutToLog("[DX11] Sky pass initialized\n");

	return true;
}

void r3dDX11SkyPass::Shutdown()
{
	SkyConstants.Shutdown();

	delete VertexBuffer;
	VertexBuffer = nullptr;

	delete InputLayout;
	InputLayout = nullptr;

	SkyVS = nullptr;
	SkyPS = nullptr;

	CommonStates = nullptr;
	ShaderLibrary = nullptr;
	DrawContext = nullptr;

	bInitialized = false;
}

bool r3dDX11SkyPass::Render(
	const r3dCamera& camera,
	r3dDX11GBufferResources& gbuffer,
	r3dDX11RenderTarget& sceneColor
)
{
	if (
		!bInitialized ||
		!DrawContext ||
		!gbuffer.IsInitialized() ||
		!sceneColor.IsValid() ||
		!gbuffer.GetDepthStencilView()
	)
	{
		return false;
	}

	// Sky можно рисовать и в debug modes:
	// depth state не даст ему залезть поверх terrain/vegetation.

	DX11SkyConstants constants;
	FillSkyConstants(camera, constants);

	if (!SkyConstants.Update(
			DrawContext->GetContext(),
			&constants,
			sizeof(constants)))
	{
		return false;
	}

	ID3D11DeviceContext* dxContext =
		DrawContext->GetContext();

	if (!dxContext)
		return false;

	ID3D11RenderTargetView* skyRTV =
		sceneColor.GetRTV();

	dxContext->OMSetRenderTargets(
		1,
		&skyRTV,
		gbuffer.GetDepthStencilView()
	);

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(sceneColor.GetWidth());
	viewport.Height = static_cast<float>(sceneColor.GetHeight());
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	dxContext->RSSetViewports(
		1,
		&viewport
	);

	DrawContext->SetRasterizerState(
		CommonStates->GetCullNoneRasterizer()
	);

	// Depth state for sky:
	// fullscreen triangle writes z=1.0.
	// It passes only where GBuffer depth is still clear == 1.0.
	DrawContext->SetDepthStencilState(
		CommonStates->GetDepthReadWriteState()
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
		SkyVS,
		SkyPS
	);

	SkyConstants.BindPS(
		DrawContext->GetContext(),
		0
	);

	DrawContext->Draw(3);

	return true;
}

bool r3dDX11SkyPass::IsInitialized() const
{
	return bInitialized;
}

bool r3dDX11SkyPass::CreateShadersAndLayout(ID3D11Device* device)
{
	const int vsIndex =
		ShaderLibrary->AddVertexShader(
			"VS_DX11_SKY_FULLSCREEN",
			"Sky_fullscreen_vs.hls"
		);

	const int psIndex =
		ShaderLibrary->AddPixelShader(
			"PS_DX11_SKY_BACKGROUND",
			"Sky_background_ps.hls"
		);

	if (vsIndex < 0 || psIndex < 0)
	{
		r3dOutToLog(
			"[DX11] Sky shader registration failed: %s\n",
			ShaderLibrary->GetLastError().c_str()
		);

		return false;
	}

	SkyVS =
		ShaderLibrary->GetVertexShader(
			vsIndex
		);

	SkyPS =
		ShaderLibrary->GetPixelShader(
			psIndex
		);

	if (
		!SkyVS ||
		!SkyPS ||
		!SkyVS->IsValid() ||
		!SkyPS->IsValid()
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
			offsetof(SkyVertex, Position),
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		},
		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			offsetof(SkyVertex, TexCoord),
			D3D11_INPUT_PER_VERTEX_DATA,
			0
		}
	};

	InputLayout = new r3dDX11InputLayout();

	if (!InputLayout->Create(
			device,
			inputElements,
			static_cast<unsigned int>(_countof(inputElements)),
			SkyVS->GetBytecode(),
			SkyVS->GetBytecodeSize(),
			"DX11.Sky.InputLayout"))
	{
		delete InputLayout;
		InputLayout = nullptr;
		return false;
	}

	return true;
}

bool r3dDX11SkyPass::CreateGeometry(ID3D11Device* device)
{
	const SkyVertex vertices[] =
	{
		{ { -1.0f, -1.0f, 1.0f }, { 0.0f, 1.0f } },
		{ { -1.0f,  3.0f, 1.0f }, { 0.0f, -1.0f } },
		{ {  3.0f, -1.0f, 1.0f }, { 2.0f, 1.0f } }
	};

	VertexBuffer = new r3dDX11VertexBuffer();

	if (!VertexBuffer->Create(
			device,
			sizeof(vertices),
			sizeof(SkyVertex),
			vertices,
			R3D_DX11_BUFFER_IMMUTABLE,
			"DX11.Sky.FullscreenVB"))
	{
		delete VertexBuffer;
		VertexBuffer = nullptr;
		return false;
	}

	return true;
}

bool r3dDX11SkyPass::CreateConstantBuffers(ID3D11Device* device)
{
	if (!SkyConstants.Create(
			device,
			sizeof(DX11SkyConstants),
			"DX11.Sky.ConstantsCB"))
	{
		return false;
	}

	return true;
}