#include "r3dPCH.h"
#include "r3d.h"

#include "CommonPostFX.h"

#include "PostFXChief.h"
#include "PFX_SunGlare.h"

#include "rendering/World/WorldRenderer.h"
#include "rendering/DX11/RenderDX11.h"

//------------------------------------------------------------------------

PFX_SunGlare::PFX_SunGlare()
: Parent( this )
, mShadeTexture( NULL )
{
	for( int i = 0, e = MAX_SUNGLARES; i < e; i ++ )
	{
		mPSIDs[ i ] = -1;
	}
}

//------------------------------------------------------------------------

PFX_SunGlare::~PFX_SunGlare()
{

}

//------------------------------------------------------------------------

const PFX_SunGlare::Settings&
PFX_SunGlare::GetSettings() const
{
	return mSettings;
}

//------------------------------------------------------------------------

void
PFX_SunGlare::SetSettings( const Settings& settings )
{
	mSettings = settings;
}

//------------------------------------------------------------------------
/*virtual*/

void PFX_SunGlare::InitImpl()	/*OVERRIDE*/
{

	char name[] = "PS_SUNGLARE00";

	for( int i = 0; i < MAX_SUNGLARES; i ++ )
	{
		name[ sizeof name - 3 ] = '0' + i / 10;
		name[ sizeof name - 2 ] = '0' + i % 10;

		mPSIDs[ i ] = r3dRenderer->GetPixelShaderIdx( name );
	}

	mData.PixelShaderID		= mPSIDs[ 0 ];
	mShadeTexture			= r3dRenderer->LoadTexture( "Data\\Shaders\\Texture\\SunFlareMask01.dds" );
}

//------------------------------------------------------------------------
/*virtual*/

void
PFX_SunGlare::CloseImpl() /*OVERRIDE*/
{
	
}

//------------------------------------------------------------------------
/*virtual*/

void
PFX_SunGlare::PrepareImpl( r3dScreenBuffer* dest, r3dScreenBuffer* /*src*/ ) /*OVERRIDE*/
{
	r3dRenderer->SetTex( mShadeTexture,	PostFXChief::FREE_TEX_STAGE_START );

	r3dRenderer->SetRenderingMode( R3D_BLEND_ADD | R3D_BLEND_NZ | R3D_BLEND_PUSH );

	float vConsts[ 23 ][ 4 ];

	float* thresholds = vConsts[ 0 ];

	// float4 thresholdArr[ 3 ]			: register( c0  );
	for( int i = 0, e = MAX_SUNGLARES; i < e; i ++ )
	{
		thresholds[ i ] = mSettings.Threshold[ i ];
	}

	// float4 tintColorArr[ 10 ]		: register( c3  );
	for( int i = 0, e = MAX_SUNGLARES; i < e; i ++ )
	{
		float opacity = mSettings.Opacity[ i ];

		vConsts[ i + 3 ][ 0 ] = mSettings.Tint[ i ].R / 255.0f * opacity;
		vConsts[ i + 3 ][ 1 ] = mSettings.Tint[ i ].G / 255.0f * opacity;
		vConsts[ i + 3 ][ 2 ] = mSettings.Tint[ i ].B / 255.0f * opacity;
		vConsts[ i + 3 ][ 3 ] = mSettings.Tint[ i ].A / 255.0f * opacity;
	}

	// float4 texcTransformArr[ 10 ]	: register( c13 );
	for( int i = 0, e = MAX_SUNGLARES; i < e; i ++ )
	{
		vConsts[ i + 13 ][ 0 ] = -mSettings.TexScale[ i ] ;
		vConsts[ i + 13 ][ 1 ] = -mSettings.TexScale[ i ] ;
		vConsts[ i + 13 ][ 2 ] = 0.5f * mSettings.TexScale[ i ] + 0.5f ;
		vConsts[ i + 13 ][ 3 ] = 0.5f * mSettings.TexScale[ i ] + 0.5f ;
	}

	int numSamples = R3D_MIN( R3D_MAX( mSettings.NumSunglares, 1 ), 10 );

	mData.PixelShaderID = mPSIDs[ numSamples - 1 ];

	r3dRenderer->pd3ddev->SetPixelShaderConstantF(  0, (float *)vConsts, sizeof vConsts / sizeof vConsts[ 0 ] );

	D3D_V( r3dRenderer->pd3ddev->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER ) );
	D3D_V( r3dRenderer->pd3ddev->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER ) );

	D3D_V( r3dRenderer->pd3ddev->SetSamplerState( 0, D3DSAMP_BORDERCOLOR, 0 ) );

	D3D_V( r3dRenderer->pd3ddev->SetSamplerState( PostFXChief::FREE_TEX_STAGE_START, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER ) );
	D3D_V( r3dRenderer->pd3ddev->SetSamplerState( PostFXChief::FREE_TEX_STAGE_START, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER ) );

	D3D_V( r3dRenderer->pd3ddev->SetSamplerState( PostFXChief::FREE_TEX_STAGE_START, D3DSAMP_BORDERCOLOR, 0 ) );

}
//------------------------------------------------------------------------
/*virtual*/

void
PFX_SunGlare::FinishImpl() /*OVERRIDE*/
{
	g_pPostFXChief->SetDefaultTexAddressMode( 0 );
	g_pPostFXChief->SetDefaultTexAddressMode( PostFXChief::FREE_TEX_STAGE_START );

	r3dRenderer->SetRenderingMode( R3D_BLEND_POP );
}

bool PFX_SunGlare::TryRenderExternalImpl(r3dScreenBuffer* dest, r3dScreenBuffer* src)
{
#if LTS_STUDIO_DX11 && LTS_STUDIO_DX11_WORLD
	if( !WorldRender_IsDX11Active() )
		return false;

	RenderDX11SunGlareSettings dx11Settings;
	memset( &dx11Settings, 0, sizeof dx11Settings );

	dx11Settings.NumSunglares = mSettings.NumSunglares;

	if( dx11Settings.NumSunglares < 1 )
		dx11Settings.NumSunglares = 1;

	if( dx11Settings.NumSunglares > static_cast<int>( MAX_SUNGLARES ) )
		dx11Settings.NumSunglares = static_cast<int>( MAX_SUNGLARES );

	for( int i = 0; i < MAX_SUNGLARES; ++i )
	{
		dx11Settings.Threshold[ i < 4 ? i : 3 ] =
			mSettings.Threshold[ i ];

		const float opacity = mSettings.Opacity[ i ];

		dx11Settings.Tint[ i ][ 0 ] =
			mSettings.Tint[ i ].R / 255.0f * opacity;

		dx11Settings.Tint[ i ][ 1 ] =
			mSettings.Tint[ i ].G / 255.0f * opacity;

		dx11Settings.Tint[ i ][ 2 ] =
			mSettings.Tint[ i ].B / 255.0f * opacity;

		dx11Settings.Tint[ i ][ 3 ] =
			mSettings.Tint[ i ].A / 255.0f * opacity;

		dx11Settings.TexTransform[ i ][ 0 ] =
			-mSettings.TexScale[ i ];

		dx11Settings.TexTransform[ i ][ 1 ] =
			-mSettings.TexScale[ i ];

		dx11Settings.TexTransform[ i ][ 2 ] =
			0.5f * mSettings.TexScale[ i ] + 0.5f;

		dx11Settings.TexTransform[ i ][ 3 ] =
			0.5f * mSettings.TexScale[ i ] + 0.5f;
	}

	dx11Settings.Params[ 0 ] =
		static_cast<float>( dx11Settings.NumSunglares );

	dx11Settings.Params[ 1 ] = 0.0f;
	dx11Settings.Params[ 2 ] = 0.0f;
	dx11Settings.Params[ 3 ] = 0.0f;

	return RenderDX11_ApplySunGlare(
		dx11Settings,
		mShadeTexture
	);
#else
	return false;
#endif
}

//------------------------------------------------------------------------

PFX_SunGlare::Settings::Settings()
: NumSunglares( 1 )
{
	for( int i = 0; i < MAX_SUNGLARES ; i ++ )
	{
		Tint[ i ]		= r3dColor( 178, 51, 230, 255 );
		Opacity[ i ]	= 0.1f; 
		TexScale[ i ]	= 2.0f;
		BlurScale[ i ]	= 0.15f;
		Threshold[ i ]	= 0.1f;
	}
}
