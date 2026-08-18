///////////////////////////////////////////////////////////////////////  
//  sg41y_shadow_ps.glsl
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2016 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com


///////////////////////////////////////////////////////////////////////
//	Lots of #defines controlled by the user settings during SRT compilation

#ifndef ST_OPENGL
	#define ST_OPENGL 1
#endif

// 'ST_OPENGL_NO_INSTANCING' possibly added at run-time by the OpenGL version of the SDK
#ifndef ST_OPENGL_NO_INSTANCING
#extension GL_EXT_gpu_shader4 : enable // needed for gl_InstanceID
#endif
#define ST_PIXEL_SHADER
#define ST_COORDSYS_Z_UP true
#define ST_COORDSYS_Y_UP false
#define ST_COORDSYS_LEFT_HANDED false
#define ST_COORDSYS_RIGHT_HANDED true

// user preference defines (render settings)
#define ST_ALPHA_TEST_NOISE false
#define ST_DEFERRED_A2C_ENABLED 0
#define ST_DEPTH_PREPASS_USED false
#define ST_FOG_CURVE ST_FOG_CURVE_LINEAR
#define ST_FOG_COLOR ST_FOG_COLOR_DYNAMIC
#define ST_SHADOWS_ENABLED true
#define ST_SHADOWS_SMOOTH false
#define ST_SHADOWS_NUM_MAPS 3
#define ST_BRANCHES_PRESENT true
#define ST_ONLY_BRANCHES_PRESENT true
#define ST_FRONDS_PRESENT false
#define ST_ONLY_FRONDS_PRESENT false
#define ST_LEAVES_PRESENT false
#define ST_ONLY_LEAVES_PRESENT false
#define ST_FACING_LEAVES_PRESENT false
#define ST_ONLY_FACING_LEAVES_PRESENT false
#define ST_RIGID_MESHES_PRESENT false
#define ST_ONLY_RIGID_MESHES_PRESENT false
#define PIXEL_PROPERTY_PROJECTION_PRESENT true
#define PIXEL_PROPERTY_FOGSCALAR_PRESENT false
#define PIXEL_PROPERTY_FOGCOLOR_PRESENT false
#define PIXEL_PROPERTY_DIFFUSETEXCOORDS_PRESENT true
#define PIXEL_PROPERTY_DETAILTEXCOORDS_PRESENT false
#define PIXEL_PROPERTY_PERVERTEXLIGHTINGCOLOR_PRESENT false
#define PIXEL_PROPERTY_LIGHTDIRINTANGENTSPACE_PRESENT false
#define PIXEL_PROPERTY_NORMAL_PRESENT false
#define PIXEL_PROPERTY_BINORMAL_PRESENT false
#define PIXEL_PROPERTY_TANGENT_PRESENT false
#define PIXEL_PROPERTY_SPECULARHALFVECTOR_PRESENT false
#define PIXEL_PROPERTY_PERVERTEXSPECULARDOT_PRESENT false
#define PIXEL_PROPERTY_PERVERTEXAMBIENTCONTRAST_PRESENT false
#define PIXEL_PROPERTY_BILLBOARDTO3DFADE_PRESENT false
#define PIXEL_PROPERTY_TRANSMISSIONFACTOR_PRESENT false
#define PIXEL_PROPERTY_RENDEREFFECTSFADE_PRESENT false
#define PIXEL_PROPERTY_AMBIENTOCCLUSION_PRESENT false
#define PIXEL_PROPERTY_BRANCHSEAMDIFFUSE_PRESENT false
#define PIXEL_PROPERTY_BRANCHSEAMDETAIL_PRESENT false
#define PIXEL_PROPERTY_SHADOWDEPTH_PRESENT false
#define PIXEL_PROPERTY_SHADOWMAPPROJECTION0_PRESENT false
#define PIXEL_PROPERTY_SHADOWMAPPROJECTION1_PRESENT false
#define PIXEL_PROPERTY_SHADOWMAPPROJECTION2_PRESENT false
#define PIXEL_PROPERTY_SHADOWMAPPROJECTION3_PRESENT false
#define PIXEL_PROPERTY_HUEVARIATION_PRESENT false
#define ST_USED_AS_GRASS false
#define ST_MULTIPASS_ACTIVE false

// effect LOD macros
#define ST_EFFECT_FORWARD_LIGHTING_PER_VERTEX         true
#define ST_EFFECT_FORWARD_LIGHTING_PER_PIXEL          true
#define ST_EFFECT_FORWARD_LIGHTING_TRANSITION         true
#define ST_EFFECT_AMBIENT_OCCLUSION                   true
#define ST_EFFECT_AMBIENT_CONTRAST                    true
#define ST_EFFECT_AMBIENT_CONTRAST_FADE               true
#define ST_EFFECT_DETAIL_LAYER                        true
#define ST_EFFECT_DETAIL_LAYER_FADE                   true
#define ST_EFFECT_DETAIL_NORMAL_LAYER                 false
#define ST_EFFECT_SPECULAR                            false
#define ST_EFFECT_SPECULAR_FADE                       false
#define ST_EFFECT_TRANSMISSION                        false
#define ST_EFFECT_TRANSMISSION_FADE                   false
#define ST_EFFECT_BRANCH_SEAM_SMOOTHING               false
#define ST_EFFECT_BRANCH_SEAM_SMOOTHING_FADE          false
#define ST_EFFECT_SMOOTH_LOD                          true
#define ST_EFFECT_FADE_TO_BILLBOARD                   false
#define ST_EFFECT_HAS_HORZ_BB                         false
#define ST_EFFECT_BACKFACE_CULLING                    true
#define ST_EFFECT_AMBIENT_IMAGE_LIGHTING              true
#define ST_EFFECT_AMBIENT_IMAGE_LIGHTING_FADE         false
#define ST_EFFECT_HUE_VARIATION                       false
#define ST_EFFECT_HUE_VARIATION_FADE                  false
#define ST_EFFECT_SHADOW_SMOOTHING                    false
#define ST_EFFECT_DIFFUSE_MAP_OPAQUE                  true

// wind LOD macros
#define ST_WIND_LOD_GLOBAL false
#define ST_WIND_LOD_BRANCH false
#define ST_WIND_LOD_FULL true
#define ST_WIND_LOD_NONE_X_GLOBAL false
#define ST_WIND_LOD_NONE_X_BRANCH false
#define ST_WIND_LOD_NONE_X_FULL false
#define ST_WIND_LOD_GLOBAL_X_BRANCH false
#define ST_WIND_LOD_GLOBAL_X_FULL true
#define ST_WIND_LOD_BRANCH_X_FULL false
#define ST_WIND_LOD_NONE false
#define ST_WIND_LOD_ROLLING_FADE true
#define ST_WIND_BRANCH_WIND_ACTIVE true
#define ST_WIND_LOD_BILLBOARD_GLOBAL true

// wind config macros
#define ST_WIND_IS_ACTIVE 1
#define ST_WIND_EFFECT_GLOBAL_WIND true
#define ST_WIND_EFFECT_GLOBAL_PRESERVE_SHAPE true
#define ST_WIND_EFFECT_BRANCH_SIMPLE_1 true
#define ST_WIND_EFFECT_BRANCH_DIRECTIONAL_1 false
#define ST_WIND_EFFECT_BRANCH_DIRECTIONAL_FROND_1 false
#define ST_WIND_EFFECT_BRANCH_TURBULENCE_1 false
#define ST_WIND_EFFECT_BRANCH_WHIP_1 false
#define ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_1 true
#define ST_WIND_EFFECT_BRANCH_SIMPLE_2 false
#define ST_WIND_EFFECT_BRANCH_DIRECTIONAL_2 true
#define ST_WIND_EFFECT_BRANCH_DIRECTIONAL_FROND_2 false
#define ST_WIND_EFFECT_BRANCH_TURBULENCE_2 true
#define ST_WIND_EFFECT_BRANCH_WHIP_2 false
#define ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_2 true
#define ST_WIND_EFFECT_LEAF_RIPPLE_VERTEX_NORMAL_1 false
#define ST_WIND_EFFECT_LEAF_RIPPLE_COMPUTED_1 false
#define ST_WIND_EFFECT_LEAF_TUMBLE_1 true
#define ST_WIND_EFFECT_LEAF_TWITCH_1 false
#define ST_WIND_EFFECT_LEAF_OCCLUSION_1 false
#define ST_WIND_EFFECT_LEAF_RIPPLE_VERTEX_NORMAL_2 false
#define ST_WIND_EFFECT_LEAF_RIPPLE_COMPUTED_2 false
#define ST_WIND_EFFECT_LEAF_TUMBLE_2 true
#define ST_WIND_EFFECT_LEAF_TWITCH_2 true
#define ST_WIND_EFFECT_LEAF_OCCLUSION_2 false
#define ST_WIND_EFFECT_FROND_RIPPLE_ONE_SIDED false
#define ST_WIND_EFFECT_FROND_RIPPLE_TWO_SIDED false
#define ST_WIND_EFFECT_FROND_RIPPLE_ADJUST_LIGHTING true
#define ST_WIND_EFFECT_ROLLING_ON_THIS_LOD true
#define ST_MODEL_USES_ROLLING_WIND true
#define ST_BRANCH_LEVEL_1_ACTIVE true
#define ST_BRANCH_LEVEL_2_ACTIVE false


///////////////////////////////////////////////////////////////////////
//	Include files

///////////////////////////////////////////////////////////////////////
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2016 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#ifndef ST_INCLUDE_SETUP
#define ST_INCLUDE_SETUP


///////////////////////////////////////////////////////////////////////
//	Include files

///////////////////////////////////////////////////////////////////////
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2016 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#ifndef ST_INCLUDE_SYMBOLS
#define ST_INCLUDE_SYMBOLS

// should match EFogCurve in Core.h
#define ST_FOG_CURVE_DISABLED	0
#define ST_FOG_CURVE_LINEAR		1
#define ST_FOG_CURVE_EXP		2
#define ST_FOG_CURVE_EXP2		3
#define ST_FOG_CURVE_USER		4

// should match EFogColorType in Core.h
#define ST_FOG_COLOR_CONSTANT	0
#define ST_FOG_COLOR_DYNAMIC	1

#endif // ST_INCLUDE_SYMBOLS


///////////////////////////////////////////////////////////////////////
//	Set up default values for configuration/per-platform macros if not previously set

// coordinate system
#if !defined(ST_COORDSYS_Z_UP) && !defined(ST_COORDSYS_Y_UP)
	#define ST_COORDSYS_Z_UP true
	#define ST_COORDSYS_Y_UP false
#endif
#if !defined(ST_COORDSYS_RIGHT_HANDED) && !defined(ST_COORDSYS_LEFT_HANDED)
	#define ST_COORDSYS_RIGHT_HANDED true
	#define ST_COORDSYS_LEFT_HANDED false
#endif

// platforms
#ifndef ST_OPENGL
	#define ST_OPENGL 0
#endif
#ifndef ST_DIRECTX9
	#define ST_DIRECTX9 0
#endif
#ifndef ST_XBOX_360
	#define ST_XBOX_360 0
#endif
#ifndef ST_DIRECTX11
	#define ST_DIRECTX11 0
#endif
#ifndef ST_XBOX_ONE
	#define ST_XBOX_ONE 0
#endif
#ifndef ST_PS3
	#define ST_PS3 0
#endif
#ifndef ST_PS4
	#define ST_PS4 0
#endif


///////////////////////////////////////////////////////////////////////  
//	Platform-adapting macros

// specifying outputs for both vertex and pixel shaders
#if (ST_DIRECTX9) || (ST_PS3)
	#define ST_VS_OUT_POS		POSITION
	#define ST_PS_OUTPUT		COLOR
	#define ST_RENDER_TARGET0	: COLOR0
	#define ST_RENDER_TARGET1	: COLOR1
	#define ST_RENDER_TARGET2	: COLOR2
	#define ST_RENDER_TARGET3	: COLOR3
#elif (ST_PS4)
	#define ST_VS_OUT_POS		S_POSITION
	#define ST_PS_OUTPUT		S_TARGET_OUTPUT
	#define ST_RENDER_TARGET0	: S_TARGET_OUTPUT0
	#define ST_RENDER_TARGET1	: S_TARGET_OUTPUT1
	#define ST_RENDER_TARGET2	: S_TARGET_OUTPUT2
	#define ST_RENDER_TARGET3	: S_TARGET_OUTPUT3
#elif (ST_DIRECTX11)
	#define ST_VS_OUT_POS		SV_POSITION
	#define ST_PS_OUTPUT		SV_TARGET
	#define ST_RENDER_TARGET0	: SV_Target0
	#define ST_RENDER_TARGET1	: SV_Target1
	#define ST_RENDER_TARGET2	: SV_Target2
	#define ST_RENDER_TARGET3	: SV_Target3
#elif (ST_OPENGL)
	#define ST_VS_OUT_POS
	#define ST_PS_OUTPUT
	#define ST_RENDER_TARGET0
	#define ST_RENDER_TARGET1
	#define ST_RENDER_TARGET2
	#define ST_RENDER_TARGET3
#endif

// specifying simple pixel float4 color return
#if (ST_OPENGL)
	#if (__VERSION__ >= 150)
		out vec4 vOutFragColor;
		#define ST_PIXEL_COLOR_RETURN(color) vOutFragColor = color; return
	#else
		#define ST_PIXEL_COLOR_RETURN(color) gl_FragColor = color; return
	#endif
#else
	#define ST_PIXEL_COLOR_RETURN(color) return color
#endif

#define ST_UNREF_PARAM(x)		(x) = (x)
#define ST_TRANSPARENCY_ACTIVE	(!ST_EFFECT_DIFFUSE_MAP_OPAQUE || ST_EFFECT_FADE_TO_BILLBOARD || ST_USED_AS_GRASS)
#define ST_ALPHA_KILL_THRESHOLD	0.1


///////////////////////////////////////////////////////////////////////  
//  Semantic Bindings
//
//	Differences between NVIDIA & ATI hardware, as well as Cg and HLSL compilers
//	call for this abstract of vertex attribute semantics

// vertex input attribute names
#if (ST_DIRECTX9) || (ST_DIRECTX11) || (ST_XBOX_360)
	#define ST_VS_IN_ATTR0	POSITION
	#define ST_VS_IN_ATTR1	TEXCOORD0
	#define ST_VS_IN_ATTR2	TEXCOORD1
	#define ST_VS_IN_ATTR3	TEXCOORD2
	#define ST_VS_IN_ATTR4	TEXCOORD3
	#define ST_VS_IN_ATTR5	TEXCOORD4
	#define ST_VS_IN_ATTR6	TEXCOORD5
	#define ST_VS_IN_ATTR7	TEXCOORD6
	#define ST_VS_IN_ATTR8	TEXCOORD7
	#define ST_VS_IN_ATTR9	TEXCOORD8
	#define ST_VS_IN_ATTR10	TEXCOORD9
	#define ST_VS_IN_ATTR11	TEXCOORD10
	#define ST_VS_IN_ATTR12	TEXCOORD11
	#define ST_VS_IN_ATTR13	TEXCOORD12
	#define ST_VS_IN_ATTR14	TEXCOORD13
	#define ST_VS_IN_ATTR15	TEXCOORD14
#elif (ST_PS3)
	#define ST_VS_IN_ATTR0	ATTR0
	#define ST_VS_IN_ATTR1	ATTR1
	#define ST_VS_IN_ATTR2	ATTR2
	#define ST_VS_IN_ATTR3	ATTR3
	#define ST_VS_IN_ATTR4	ATTR4
	#define ST_VS_IN_ATTR5	ATTR5
	#define ST_VS_IN_ATTR6	ATTR6
	#define ST_VS_IN_ATTR7	ATTR7
	#define ST_VS_IN_ATTR8	ATTR8
	#define ST_VS_IN_ATTR9	ATTR9
	#define ST_VS_IN_ATTR10	ATTR10
	#define ST_VS_IN_ATTR11	ATTR11
	#define ST_VS_IN_ATTR12	ATTR12
	#define ST_VS_IN_ATTR13	ATTR13
	#define ST_VS_IN_ATTR14	ATTR14
	#define ST_VS_IN_ATTR15	ATTR15
#endif

// vertex output / pixel input attribute names
#define ST_VS_OUT_ATTR0		POSITION
#define ST_VS_OUT_ATTR1		TEXCOORD0
#define ST_VS_OUT_ATTR2		TEXCOORD1
#define ST_VS_OUT_ATTR3		TEXCOORD2
#define ST_VS_OUT_ATTR4		TEXCOORD3
#define ST_VS_OUT_ATTR5		TEXCOORD4
#define ST_VS_OUT_ATTR6		TEXCOORD5
#define ST_VS_OUT_ATTR7		TEXCOORD6
#define ST_VS_OUT_ATTR8		TEXCOORD7
#define ST_VS_OUT_ATTR9		TEXCOORD8
#define ST_VS_OUT_ATTR10	TEXCOORD9
#define ST_VS_OUT_ATTR11	TEXCOORD10
#define ST_VS_OUT_ATTR12	TEXCOORD11
#define ST_VS_OUT_ATTR13	TEXCOORD12
#define ST_VS_OUT_ATTR14	TEXCOORD13
#define ST_VS_OUT_ATTR15	TEXCOORD14



///////////////////////////////////////////////////////////////////////  
//  Xbox-specific command
//
//	With the way the 360 shader compiler geneates code & optimizes, wind 
//	compuations on 360 don't quite match between depth-only and lighting
//	passes.

#if (ST_XBOX_360) && (ST_MULTIPASS_ACTIVE)
	// ST_MULTIPASS_ACTIVE is #defined by the SRT Exporter for all shaders when
	// depth-only prepass is selected in the Compiler app
	#define ST_MULTIPASS_STABILIZE [isolate]
#else
	#define ST_MULTIPASS_STABILIZE
#endif


///////////////////////////////////////////////////////////////////////  
//  Getting GLSL syntax more in line with HLSL/Cg the rest of our platforms use

#if (ST_OPENGL)

	///////////////////////////////////////////////////////////////////////  
	//  Synchronize vector types
	
	#define float2		vec2
	#define float3		vec3
	#define float4		vec4
	#define float3x3	mat3
	#define float4x4	mat4
    #define uint2       uvec2

	
	///////////////////////////////////////////////////////////////////////  
	//  saturate (clamp in GLSL)

	float saturate(float fValue)
	{
		return clamp(fValue, 0.0, 1.0);
	}

	
	///////////////////////////////////////////////////////////////////////  
	//  saturate (clamp in GLSL)

	float2 saturate(float2 vValue)
	{
		return clamp(vValue, float2(0.0, 0.0), float2(1.0, 1.0));
	}

	
	///////////////////////////////////////////////////////////////////////  
	//  saturate (clamp in GLSL)

	float3 saturate(float3 vValue)
	{
		return clamp(vValue, float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0));
	}

	
	///////////////////////////////////////////////////////////////////////  
	//  saturate (clamp in GLSL)

	float4 saturate(float4 vValue)
	{
		return clamp(vValue, float4(0.0, 0.0, 0.0, 0.0), float4(1.0, 1.0, 1.0, 1.0));
	}

	
	///////////////////////////////////////////////////////////////////////  
	//  lerp() is mix() in GLSL

	#define lerp mix
	#define clip discard
	#define frac fract
	#define fmod mod
	#define wind_cross(a, b) cross((b), (a))
	#define v2p_vInterpolant0 gl_Position

	
	///////////////////////////////////////////////////////////////////////  
	//  mul_float4x4_float4

	float4 mul_float4x4_float4(float4x4 mMatrix, float4 vVector)
	{
		return mMatrix * vVector;
	}
	
	
	///////////////////////////////////////////////////////////////////////  
	//  mul_float4_float4x4

	float4 mul_float4_float4x4(float4 vVector, float4x4 mMatrix)
	{
		return vVector * mMatrix;
	}
	
	
	///////////////////////////////////////////////////////////////////////  
	//  mul_float3x3_float3x3

	float3x3 mul_float3x3_float3x3(float3x3 mMatrixA, float3x3 mMatrixB)
	{
		return mMatrixA * mMatrixB;
	}


	///////////////////////////////////////////////////////////////////////  
	//  mul_float3x3_float3

	float3 mul_float3x3_float3(float3x3 mMatrix, float3 vVector)
	{
		return mMatrix * vVector;
	}
	

	///////////////////////////////////////////////////////////////////////  
	//  const doesn't mean the same thing in GLSL

	#define const
	#define static
	
#else

	///////////////////////////////////////////////////////////////////////  
	//  mul_float4x4_float4

	float4 mul_float4x4_float4(float4x4 mMatrix, float4 vVector)
	{
		if (ST_PS3)
		{
			return mul(vVector, mMatrix);
		}
		else
		{
			return mul(mMatrix, vVector);
		}
	}
	
	
	///////////////////////////////////////////////////////////////////////  
	//  mul_float4_float4x4

	float4 mul_float4_float4x4(float4 vVector, float4x4 mMatrix)
	{
		if (ST_PS3)
		{
			return mul(mMatrix, vVector);
		}
		else
		{		
			return mul(vVector, mMatrix);
		}
	}


	///////////////////////////////////////////////////////////////////////  
	//  mul_float3x3_float3x3

	float3x3 mul_float3x3_float3x3(float3x3 mMatrixA, float3x3 mMatrixB)
	{
		if (ST_PS3)
		{
			return mul(mMatrixB, mMatrixA);
		}
		else
		{
			return mul(mMatrixA, mMatrixB);
		}
	}


	///////////////////////////////////////////////////////////////////////  
	//  mul_float3x3_float3

	float3 mul_float3x3_float3(float3x3 mMatrix, float3 vVector)
	{
		if (ST_PS3)
		{
			return mul(vVector, mMatrix);
		}
		else
		{
			return mul(mMatrix, vVector);
		}
	}

	
	///////////////////////////////////////////////////////////////////////  
	//  cross()'s parameters are backwards in GLSL

	#define wind_cross(a, b) cross((a), (b))

#endif


///////////////////////////////////////////////////////////////////////  
//  Global constants
//
//	These correspond to the EGeometryTypeHint enumeration defined in Core.h

#define ST_GEOMETRY_TYPE_HINT_BRANCHES			0.0
#define ST_GEOMETRY_TYPE_HINT_FRONDS			1.0
#define ST_GEOMETRY_TYPE_HINT_LEAVES			2.0
#define ST_GEOMETRY_TYPE_HINT_FACING_LEAVES		3.0
#define ST_GEOMETRY_TYPE_HINT_RIGID_MESHES		4.0

#endif // ST_INCLUDE_SETUP
// utility macro
#define ST_MACRO_CONCAT(a, b) a##b

// limits and sizes
#define ST_MAX_NUM_BILLBOARDS_PER_BASE_TREE      24
#define ST_NUM_EFFECT_CONFIG_FLOAT4S             5
#define ST_NUM_WIND_CONFIG_FLOAT4S               7
#define ST_NUM_WIND_LOD_FLOAT4S                  3

// constant buffer register indices
#define ST_CONST_BUF_REGISTER_FRAME              0
#define ST_CONST_BUF_REGISTER_BASE_TREE          1
#define ST_CONST_BUF_REGISTER_INSTANCING         2
#define ST_CONST_BUF_REGISTER_MATERIAL           3
#define ST_CONST_BUF_REGISTER_WIND_DYNAMICS      4
#define ST_CONST_BUF_REGISTER_FOG_AND_SKY        5
#define ST_CONST_BUF_REGISTER_TERRAIN            6
#define ST_CONST_BUF_REGISTER_BLOOM              7

#if (ST_OPENGL || ST_DIRECTX11 || ST_XBOX_ONE || ST_PS4)

	// platform-specific constant buffer syntax
	#if (ST_PS4)
		#define ST_CBUFFER(name, reg) ConstantBuffer name : register(b##reg)
	#elif (ST_OPENGL)
		#define ST_CBUFFER(name, reg) layout(std140) uniform name
	#else
		#define ST_CBUFFER(name, reg) cbuffer name : register(b##reg)
	#endif

	struct SDirLight
	{
		float3              m_vAmbient;                         // current light's ambient color
		float3              m_vDiffuse;                         // current light's diffuse color
		float3              m_vSpecular;                        // current light's specular color
		float3              m_vTransmission;                    // current light's transmission color
		float3              m_vDir;                             // normalized light direction vector
	};

	struct SShadows
	{
		float4              m_vMapRanges;                       // ending distance in world units for each of the cascaded shadow maps
		float4              m_avSmoothingTable[3];              // texel offsets to use for shadow smoothing
		float4x4            m_amLightModelViewProjs[4];         // light map projections for up to four shadow map projections
		float2              m_vTexelOffset;                     // xy value is { 0.5 / shadow_map_res.x, 0.5 / shadow_map_res.y }
		float               m_fFadeStartPercent;                // percent (0.0 to 1.0) from the end of the last cascade where shadow begins to fade out
		float               m_fFadeInverseDistance;             // upstream computation to help optimize shadow fade computation in shader
		float               m_fTerrainAmbientOcclusion;         // [0.0, 1.0] value controlling per-vertex terrain AO darkness
	};

	struct SWindGlobal
	{
		float               m_fTime;                            // time, in seconds, as needed by global wind algorithm
		float               m_fDistance;                        // distance, in model units, that the model will move
		float               m_fHeight;                          // height of the model
		float               m_fHeightExponent;                  // exponent that controls the curvature of the model (1 = linear, >1 bends more near the top
		float               m_fAdherence;                       // how much wind direction affects the motion
	};

	struct SWindBranchLevel
	{
		float               m_fBranchTime;                      // time, in seconds, as needed by branch wind algorithm
		float               m_fBranchDistance;                  // distance, in model units, that the model will move
		float               m_fTwitch;                          // Sway bias, controls the uniformity of the branch oscillation
		float               m_fTwitchFreqScale;                 // Sway freq, controls how frequently branch sway varies during oscillation
		float               m_fWhip;                            // controls how much the branch is allowed to deform during oscillations
		float               m_fDirectionAdherence;              // how much wind direction affects the motion
		float               m_fTurbulence;                      // controls how much chaotic motion is added due branch motion as wind strength increases
	};

	struct SWindLeaf
	{
		float               m_fRippleTime;                      // time, in seconds, as needed by leaf ripple algorithm
		float               m_fRippleDistance;                  // how much, in model units, leaf vertices can move
		float               m_fLeewardScalar;                   // how much leaf wind is scaled on the side of the model farthest away from the wind source
		float               m_fTumbleTime;                      // time, in seconds, as needed by leaf tumble algorithm
		float               m_fTumbleFlip;                      // how far, in degrees, a leaf can flip
		float               m_fTumbleTwist;                     // how far, in degrees, a leaf can twist
		float               m_fTumbleDirectionAdherence;        // how much a leaf will rotate 'into' the wind
		float               m_fTwitchThrow;                     // how far, in degrees, a leaf will twitch when applicable
		float               m_fTwitchSharpness;                 // controls how rapidly leaves move when the twitch
		float               m_fTwitchTime;                      // time, in seconds, as needed by the twitch algorithm
	};

	struct SWindFrond
	{
		float               m_fFrondTime;                       // time, in seconds, as needed by frond wind algorithm
		float               m_fFrondDistance;                   // how far, in model units, frond vertices can move
		float               m_fTile;                            // how tightly the oscillation pattern is applied along the length of the frond
		float               m_fLightingScalar;                  // controls how much the surface normal is modified by frond motion
	};

	struct SWindRolling
	{
		float               m_fBranchFieldMin;                  // branch motion scalar applied when the noise field is low
		float               m_fBranchLightingAdjust;            // controls how much surface normals are modified on branches due to rolling motion
		float               m_fBranchVerticalOffset;            // controls how much vertical motion is added when the noise field is high
		float               m_fLeafRippleMin;                   // ripple scalar applied when the noise field is low
		float               m_fLeafTumbleMin;                   // tumble scalar applied when the noise field is low
		float               m_fNoisePeriod;                     // period value used by the noise field computation
		float               m_fNoiseSize;                       // size value used by the noise field computation
		float               m_fNoiseTurbulence;                 // turbulence value used by the noise field computation
		float               m_fNoiseTwist;                      // twist value used by the noise field computation
		float               m_fOffsetX;                         // x offset of pattern to move noise pattern across the field
		float               m_fOffsetY;                         // y offset of pattern to move noise pattern across the field
		float               m_fBranchRipple;                    // controls how much a branch can deform due to rolling motion
	};

	ST_CBUFFER(SFrameCBLayout, ST_CONST_BUF_REGISTER_FRAME)
	{
		float4              u_vHandTunedParams;                 // tied to user input for art tuning of various parameters
		float4x4            u_mModelViewProj3d;                 // 3D modelview_matrix * projection_matrix; modelview has no translation component
		float4x4            u_mProjectionInverse3d;             // inverse of projection_matrix; used in example deferred lighting shader
		float4x4            u_mModelViewProj2d;                 // 2D modelview_matrix * projection_matrix; used by fullscreen shaders
		float4x4            u_mCameraFacingMatrix;              // used to turn facing leaves toward the camera
		float3              u_vCameraPosition;                  // 3D position of camera in world space
		float3              u_vCameraDirection;                 // normalized camera direction vector
		float3              u_vLodRefPosition;                  // 3D position used for LOD computations in world space (when rendering into shadow maps)
		float2              u_vViewport;                        // x = viewport width, y = viewport height
		float2              u_vViewportInverse;                 // 1.0 / Viewport
		float               u_fWallTime;                        // real time, in seconds, from the beginning of the app
		float               u_fFarClip;                         // far clipping plane distance in world units
		SDirLight           u_sDirLight;                        // all directional light data
		SShadows            u_sShadows;                         // all shadow data
	};

	ST_CBUFFER(SBaseTreeCBLayout, ST_CONST_BUF_REGISTER_BASE_TREE)
	{
		float               u_f3dGrassStartDist;                // distance from camera in world units when grass begins to fade away
		float               u_f3dGrassRange;                    // distance in world units over which grass fades from opaque to fully transparent
		float               u_fBillboardHorzFade;               // [0.0, 1.0] value to control fade overlap from 3D to billboard; 1.0 is fully overlapping
		float               u_fOneMinusBillboardHorzFade;       // 1.0 - u_fBillboardHorzFade
		float               u_fBillboardStartDist;              // distance in world units where billboard begins to fade in, minus u_fBillboardRange
		float               u_fBillboardRange;                  // distance in world units over which billboard fades in
		float               u_fBillboardCullDist;               // distance from camera in world units where billboards are no longer drawn, minus u_fBillboardRange
		float               u_fHueVariationByPos;               // [0.0, 1.0] value to control per-inst-pos hue variation; 0.0 is none
		float               u_fHueVariationByVertex;            // [0.0, 1.0] value to control per-vertex hue variation; 0.0 is none
		float               u_fAmbientImageScalar;              // scalar value for tuning ambient image contribution; 0.0 is none
		float               u_fNumBillboards;                   // number of 360-degree billboard images used for active base tree
		float               u_fRadiansPerImage;                 // numerical helper for shader computation of which billboard image is visible
		float4              u_avBillboardTexCoords[24];         // packed billboard texcoords, one float4 per 360-degree billboard image
		float3              u_vHueVariationColor;               // rgb color to be used with hue variation
	};

	ST_CBUFFER(SInstancingCBLayout, ST_CONST_BUF_REGISTER_INSTANCING)
	{
		float               u_fInstancingInfo;                  // used by Xbox 360 only; x = number of indices per instance
	};

	ST_CBUFFER(SMaterialCBLayout, ST_CONST_BUF_REGISTER_MATERIAL)
	{
		float               u_fShininess;                       // current material shininess
		float               u_fBranchSeamWeight;                // controls curve used in branch seam blending; set in the Modeler
		float               u_fOneMinusAmbientContrastFactor;   // used to control level of "ambient contrast", a SpeedTree-specific lighting component
		float               u_fTransmissionShadowBrightness;    // controls shadow brightness in materials where transmission is active
		float               u_fTransmissionViewDependency;      // controls the relationship between the light angle and amount of transmission effect
		float               u_fAlphaScalar;                     // multiplied directly with the texture's raw alpha value; >1 often needed for A2C
		float3              u_vAmbientColor;                    // current ambient material color
		float3              u_vDiffuseColor;                    // current diffuse material color
		float3              u_vSpecularColor;                   // current specular material color
		float3              u_vTransmissionColor;               // current transmission material color
		float4              u_avEffectConfigFlags[5];           // rendering effect flags used under 'unified shader' compilation mode
		float4              u_avWindConfigFlags[7];             // wind state flags used under 'unified shader' compilation mode
		float4              u_avWindLodFlags[3];                // wind LOD flags used under 'unified shader' compilation mode
	};

	ST_CBUFFER(SWindDynamicsCBLayout, ST_CONST_BUF_REGISTER_WIND_DYNAMICS)
	{
		float3              u_vDirection;                       // direction of the wind
		float3              u_vAnchor;                          // position of the wind anchor used by 'palm style' branch motion
		SWindGlobal         u_sGlobal;                          // global wind parameters
		SWindBranchLevel    u_sBranch1;                         // branch wind level 1 parameters
		SWindBranchLevel    u_sBranch2;                         // branch wind level 2 parameters
		SWindLeaf           u_sLeaf1;                           // leaf wind group 1 parameters
		SWindLeaf           u_sLeaf2;                           // leaf wind group 2 parameters
		SWindFrond          u_sFrondRipple;                     // frond wind parameters
		float               u_fStrength;                        // wind strength (0.0 = none, 1.0 = max)
		SWindRolling        u_sRolling;                         // rolling wind parameters
	};

	ST_CBUFFER(SFogAndSkyCBLayout, ST_CONST_BUF_REGISTER_FOG_AND_SKY)
	{
		float               u_fFogEndDist;                      // for linear fog effect, distance in world units where fog coverage is absolute
		float               u_fFogSpan;                         // for linear fog effect, distance in world units from start to end fog values
		float               u_fSunSize;                         // sky pixel shader uses a procedural sun disk; this controls the size
		float               u_fSunSpreadExponent;               // sky pixel shader uses a procedural sun disk; this controls the sun/sky blend
		float3              u_vFogColor;                        // rgb color of the fog effect
		float3              u_vSkyColor;                        // base color for the sky
		float3              u_vSunColor;                        // basic color of the procedural sun disk
	};

	ST_CBUFFER(STerrainCBLayout, ST_CONST_BUF_REGISTER_TERRAIN)
	{
		float               u_fSplatTile0;                      // terrain system uses three textures for splatting; texture repeat values for map 0 here
		float               u_fSplatTile1;                      // terrain system uses three textures for splatting; texture repeat values for map 1 here
		float               u_fSplatTile2;                      // terrain system uses three textures for splatting; texture repeat values for map 2 here
		float               u_fTerrainAmbientImageScalar;       // ambient image scalar for terrain (forward rendering only); 0.0 means to contribution
	};

	ST_CBUFFER(SBloomCBLayout, ST_CONST_BUF_REGISTER_BLOOM)
	{
		float               u_fBrightPass;                      // [0.0, 1.0] value, determines hi-pass threshold for bloom effect
		float               u_fDownsample;                      // controls how many times hi-pass frame will be downsampled before applied
		float               u_fDownsampleLoopStart;             // loop helper for bloom hi-pass pixel shader
		float               u_fDownsampleLoopEnd;               // loop helper for bloom hi-pass pixel shader
		float               u_fBlurKernelSize;                  // controls how blurred the bloom pass will be
		float               u_fBlurKernelStep;                  // loop helper for bloom Blur pass
		float               u_fBlurPixelOffset;                 // shader helper for bloom Blur pass
		float               u_fBloomEffectScalar;               // scales how much the bloom Effect is added to the final bloom composite
		float               u_fHighPassFloor;                   // lowest value for high pass filter
		float               u_fFinalMainScalar;                 // controls how much the main render contributes to the final bloom composite
	};

#endif

#if (ST_DIRECTX9 || ST_XBOX_360 || ST_PS3)

	struct SDirLight
	{
		float3      m_vAmbient;                                           // current light's ambient color
		float3      m_vDiffuse;                                           // current light's diffuse color
		float3      m_vSpecular;                                          // current light's specular color
		float3      m_vTransmission;                                      // current light's transmission color
		float3      m_vDir;                                               // normalized light direction vector
	};

	struct SShadows
	{
		float4      m_vMapRanges;                                         // ending distance in world units for each of the cascaded shadow maps
		float4      m_avSmoothingTable[3];                                // texel offsets to use for shadow smoothing
		float4x4    m_amLightModelViewProjs[4];                           // light map projections for up to four shadow map projections
		float4      __placeholder0;                  
		#define     m_vTexelOffset                   __placeholder0.xy    // xy value is { 0.5 / shadow_map_res.x, 0.5 / shadow_map_res.y }
		#define     m_fFadeStartPercent              __placeholder0.z     // percent (0.0 to 1.0) from the end of the last cascade where shadow begins to fade out
		#define     m_fFadeInverseDistance           __placeholder0.w     // upstream computation to help optimize shadow fade computation in shader
		float       m_fTerrainAmbientOcclusion;                           // [0.0, 1.0] value controlling per-vertex terrain AO darkness
	};

	struct SWindGlobal
	{
		float4      __placeholder0;                  
		#define     m_fTime                          __placeholder0.x     // time, in seconds, as needed by global wind algorithm
		#define     m_fDistance                      __placeholder0.y     // distance, in model units, that the model will move
		#define     m_fHeight                        __placeholder0.z     // height of the model
		#define     m_fHeightExponent                __placeholder0.w     // exponent that controls the curvature of the model (1 = linear, >1 bends more near the top
		float       m_fAdherence;                                         // how much wind direction affects the motion
	};

	struct SWindBranchLevel
	{
		float4      __placeholder0;                  
		#define     m_fBranchTime                    __placeholder0.x     // time, in seconds, as needed by branch wind algorithm
		#define     m_fBranchDistance                __placeholder0.y     // distance, in model units, that the model will move
		#define     m_fTwitch                        __placeholder0.z     // Sway bias, controls the uniformity of the branch oscillation
		#define     m_fTwitchFreqScale               __placeholder0.w     // Sway freq, controls how frequently branch sway varies during oscillation
		float4      __placeholder1;                  
		#define     m_fWhip                          __placeholder1.x     // controls how much the branch is allowed to deform during oscillations
		#define     m_fDirectionAdherence            __placeholder1.y     // how much wind direction affects the motion
		#define     m_fTurbulence                    __placeholder1.z     // controls how much chaotic motion is added due branch motion as wind strength increases
	};

	struct SWindLeaf
	{
		float4      __placeholder0;                  
		#define     m_fRippleTime                    __placeholder0.x     // time, in seconds, as needed by leaf ripple algorithm
		#define     m_fRippleDistance                __placeholder0.y     // how much, in model units, leaf vertices can move
		#define     m_fLeewardScalar                 __placeholder0.z     // how much leaf wind is scaled on the side of the model farthest away from the wind source
		#define     m_fTumbleTime                    __placeholder0.w     // time, in seconds, as needed by leaf tumble algorithm
		float4      __placeholder1;                  
		#define     m_fTumbleFlip                    __placeholder1.x     // how far, in degrees, a leaf can flip
		#define     m_fTumbleTwist                   __placeholder1.y     // how far, in degrees, a leaf can twist
		#define     m_fTumbleDirectionAdherence      __placeholder1.z     // how much a leaf will rotate 'into' the wind
		#define     m_fTwitchThrow                   __placeholder1.w     // how far, in degrees, a leaf will twitch when applicable
		float4      __placeholder2;                  
		#define     m_fTwitchSharpness               __placeholder2.x     // controls how rapidly leaves move when the twitch
		#define     m_fTwitchTime                    __placeholder2.y     // time, in seconds, as needed by the twitch algorithm
	};

	struct SWindFrond
	{
		float4      __placeholder0;                  
		#define     m_fFrondTime                     __placeholder0.x     // time, in seconds, as needed by frond wind algorithm
		#define     m_fFrondDistance                 __placeholder0.y     // how far, in model units, frond vertices can move
		#define     m_fTile                          __placeholder0.z     // how tightly the oscillation pattern is applied along the length of the frond
		#define     m_fLightingScalar                __placeholder0.w     // controls how much the surface normal is modified by frond motion
	};

	struct SWindRolling
	{
		float4      __placeholder0;                  
		#define     m_fBranchFieldMin                __placeholder0.x     // branch motion scalar applied when the noise field is low
		#define     m_fBranchLightingAdjust          __placeholder0.y     // controls how much surface normals are modified on branches due to rolling motion
		#define     m_fBranchVerticalOffset          __placeholder0.z     // controls how much vertical motion is added when the noise field is high
		#define     m_fLeafRippleMin                 __placeholder0.w     // ripple scalar applied when the noise field is low
		float4      __placeholder1;                  
		#define     m_fLeafTumbleMin                 __placeholder1.x     // tumble scalar applied when the noise field is low
		#define     m_fNoisePeriod                   __placeholder1.y     // period value used by the noise field computation
		#define     m_fNoiseSize                     __placeholder1.z     // size value used by the noise field computation
		#define     m_fNoiseTurbulence               __placeholder1.w     // turbulence value used by the noise field computation
		float4      __placeholder2;                  
		#define     m_fNoiseTwist                    __placeholder2.x     // twist value used by the noise field computation
		#define     m_fOffsetX                       __placeholder2.y     // x offset of pattern to move noise pattern across the field
		#define     m_fOffsetY                       __placeholder2.z     // y offset of pattern to move noise pattern across the field
		#define     m_fBranchRipple                  __placeholder2.w     // controls how much a branch can deform due to rolling motion
	};

	// frame group
	float4              u_vHandTunedParams               : register(c0);      // tied to user input for art tuning of various parameters
	float4x4            u_mModelViewProj3d               : register(c1);      // 3D modelview_matrix * projection_matrix; modelview has no translation component
	float4x4            u_mProjectionInverse3d           : register(c5);      // inverse of projection_matrix; used in example deferred lighting shader
	float4x4            u_mModelViewProj2d               : register(c9);      // 2D modelview_matrix * projection_matrix; used by fullscreen shaders
	float4x4            u_mCameraFacingMatrix            : register(c13);     // used to turn facing leaves toward the camera
	float3              u_vCameraPosition                : register(c17);     // 3D position of camera in world space
	float3              u_vCameraDirection               : register(c18);     // normalized camera direction vector
	float3              u_vLodRefPosition                : register(c19);     // 3D position used for LOD computations in world space (when rendering into shadow maps)
	float4              __placeholder0                   : register(c20);
	#define             u_vViewport                      __placeholder0.xy    // x = viewport width, y = viewport height
	#define             u_vViewportInverse               __placeholder0.zw    // 1.0 / Viewport
	float4              __placeholder1                   : register(c21);
	#define             u_fWallTime                      __placeholder1.x     // real time, in seconds, from the beginning of the app
	#define             u_fFarClip                       __placeholder1.y     // far clipping plane distance in world units
	SDirLight           u_sDirLight                      : register(c22);     // all directional light data
	SShadows            u_sShadows                       : register(c27);     // all shadow data

	// basetree group
	float4              __placeholder2                   : register(c49);
	#define             u_f3dGrassStartDist              __placeholder2.x     // distance from camera in world units when grass begins to fade away
	#define             u_f3dGrassRange                  __placeholder2.y     // distance in world units over which grass fades from opaque to fully transparent
	#define             u_fBillboardHorzFade             __placeholder2.z     // [0.0, 1.0] value to control fade overlap from 3D to billboard; 1.0 is fully overlapping
	#define             u_fOneMinusBillboardHorzFade     __placeholder2.w     // 1.0 - u_fBillboardHorzFade
	float4              __placeholder3                   : register(c50);
	#define             u_fBillboardStartDist            __placeholder3.x     // distance in world units where billboard begins to fade in, minus u_fBillboardRange
	#define             u_fBillboardRange                __placeholder3.y     // distance in world units over which billboard fades in
	#define             u_fBillboardCullDist             __placeholder3.z     // distance from camera in world units where billboards are no longer drawn, minus u_fBillboardRange
	#define             u_fHueVariationByPos             __placeholder3.w     // [0.0, 1.0] value to control per-inst-pos hue variation; 0.0 is none
	float4              __placeholder4                   : register(c51);
	#define             u_fHueVariationByVertex          __placeholder4.x     // [0.0, 1.0] value to control per-vertex hue variation; 0.0 is none
	#define             u_fAmbientImageScalar            __placeholder4.y     // scalar value for tuning ambient image contribution; 0.0 is none
	#define             u_fNumBillboards                 __placeholder4.z     // number of 360-degree billboard images used for active base tree
	#define             u_fRadiansPerImage               __placeholder4.w     // numerical helper for shader computation of which billboard image is visible
	float4              u_avBillboardTexCoords[24]       : register(c52);     // packed billboard texcoords, one float4 per 360-degree billboard image
	float3              u_vHueVariationColor             : register(c76);     // rgb color to be used with hue variation

	// instancing group
	float               u_fInstancingInfo                : register(c77);     // used by Xbox 360 only; x = number of indices per instance

	// material group
	float4              __placeholder5                   : register(c78);
	#define             u_fShininess                     __placeholder5.x     // current material shininess
	#define             u_fBranchSeamWeight              __placeholder5.y     // controls curve used in branch seam blending; set in the Modeler
	#define             u_fOneMinusAmbientContrastFactor __placeholder5.z     // used to control level of "ambient contrast", a SpeedTree-specific lighting component
	#define             u_fTransmissionShadowBrightness  __placeholder5.w     // controls shadow brightness in materials where transmission is active
	float4              __placeholder6                   : register(c79);
	#define             u_fTransmissionViewDependency    __placeholder6.x     // controls the relationship between the light angle and amount of transmission effect
	#define             u_fAlphaScalar                   __placeholder6.y     // multiplied directly with the texture's raw alpha value; >1 often needed for A2C
	float3              u_vAmbientColor                  : register(c80);     // current ambient material color
	float3              u_vDiffuseColor                  : register(c81);     // current diffuse material color
	float3              u_vSpecularColor                 : register(c82);     // current specular material color
	float3              u_vTransmissionColor             : register(c83);     // current transmission material color
	float4              u_avEffectConfigFlags[5]         : register(c84);     // rendering effect flags used under 'unified shader' compilation mode
	float4              u_avWindConfigFlags[7]           : register(c89);     // wind state flags used under 'unified shader' compilation mode
	float4              u_avWindLodFlags[3]              : register(c96);     // wind LOD flags used under 'unified shader' compilation mode

	// winddynamics group
	float3              u_vDirection                     : register(c99);     // direction of the wind
	float3              u_vAnchor                        : register(c100);    // position of the wind anchor used by 'palm style' branch motion
	SWindGlobal         u_sGlobal                        : register(c101);    // global wind parameters
	SWindBranchLevel    u_sBranch1                       : register(c103);    // branch wind level 1 parameters
	SWindBranchLevel    u_sBranch2                       : register(c105);    // branch wind level 2 parameters
	SWindLeaf           u_sLeaf1                         : register(c107);    // leaf wind group 1 parameters
	SWindLeaf           u_sLeaf2                         : register(c110);    // leaf wind group 2 parameters
	SWindFrond          u_sFrondRipple                   : register(c113);    // frond wind parameters
	float               u_fStrength                      : register(c114);    // wind strength (0.0 = none, 1.0 = max)
	SWindRolling        u_sRolling                       : register(c115);    // rolling wind parameters

	// fogandsky group
	float4              __placeholder7                   : register(c118);
	#define             u_fFogEndDist                    __placeholder7.x     // for linear fog effect, distance in world units where fog coverage is absolute
	#define             u_fFogSpan                       __placeholder7.y     // for linear fog effect, distance in world units from start to end fog values
	#define             u_fSunSize                       __placeholder7.z     // sky pixel shader uses a procedural sun disk; this controls the size
	#define             u_fSunSpreadExponent             __placeholder7.w     // sky pixel shader uses a procedural sun disk; this controls the sun/sky blend
	float3              u_vFogColor                      : register(c119);    // rgb color of the fog effect
	float3              u_vSkyColor                      : register(c120);    // base color for the sky
	float3              u_vSunColor                      : register(c121);    // basic color of the procedural sun disk

	// terrain group
	float4              __placeholder8                   : register(c122);
	#define             u_fSplatTile0                    __placeholder8.x     // terrain system uses three textures for splatting; texture repeat values for map 0 here
	#define             u_fSplatTile1                    __placeholder8.y     // terrain system uses three textures for splatting; texture repeat values for map 1 here
	#define             u_fSplatTile2                    __placeholder8.z     // terrain system uses three textures for splatting; texture repeat values for map 2 here
	#define             u_fTerrainAmbientImageScalar     __placeholder8.w     // ambient image scalar for terrain (forward rendering only); 0.0 means to contribution

	// bloom group
	float4              __placeholder9                   : register(c123);
	#define             u_fBrightPass                    __placeholder9.x     // [0.0, 1.0] value, determines hi-pass threshold for bloom effect
	#define             u_fDownsample                    __placeholder9.y     // controls how many times hi-pass frame will be downsampled before applied
	#define             u_fDownsampleLoopStart           __placeholder9.z     // loop helper for bloom hi-pass pixel shader
	#define             u_fDownsampleLoopEnd             __placeholder9.w     // loop helper for bloom hi-pass pixel shader
	float4              __placeholder10                  : register(c124);
	#define             u_fBlurKernelSize                __placeholder10.x    // controls how blurred the bloom pass will be
	#define             u_fBlurKernelStep                __placeholder10.y    // loop helper for bloom Blur pass
	#define             u_fBlurPixelOffset               __placeholder10.z    // shader helper for bloom Blur pass
	#define             u_fBloomEffectScalar             __placeholder10.w    // scales how much the bloom Effect is added to the final bloom composite
	float4              __placeholder11                  : register(c125);
	#define             u_fHighPassFloor                 __placeholder11.x    // lowest value for high pass filter
	#define             u_fFinalMainScalar               __placeholder11.y    // controls how much the main render contributes to the final bloom composite


#endif // #if (ST_DIRECTX9 || ST_XBOX_360)

///////////////////////////////////////////////////////////////////////
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2016 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#ifndef ST_INCLUDE_SAMPLERS_AND_TEXTURE_MACROS
#define ST_INCLUDE_SAMPLERS_AND_TEXTURE_MACROS


///////////////////////////////////////////////////////////////////////
//  Texture and sampler setup defines

#define ST_SAMPLER_LINEAR_WRAP					0
#define ST_SAMPLER_LINEAR_CLAMPED				1
#define ST_SAMPLER_POINT_CLAMPED_NO_MIPMAP		2
#define ST_SAMPLER_SHADOW_COMPARE				3

#define ST_LOD_LEVEL_0							0


#if (ST_DIRECTX11) // PC DX11 & Xbox One

	// samplers shared by the texture macros
	#define ST_SAMPLER_DECLARE(name, type, reg)								type name : register(s##reg)

	ST_SAMPLER_DECLARE(samLinearWrap, SamplerState, ST_SAMPLER_LINEAR_WRAP);
	ST_SAMPLER_DECLARE(samLinearClamped, SamplerState, ST_SAMPLER_LINEAR_CLAMPED);
	ST_SAMPLER_DECLARE(samPointClampedNoMipmap, SamplerState, ST_SAMPLER_POINT_CLAMPED_NO_MIPMAP);
	ST_SAMPLER_DECLARE(samShadowMapCompare, SamplerComparisonState, ST_SAMPLER_SHADOW_COMPARE);

	// texture declarations
	#define ST_TEXTURE_DECLARE(name, reg)									Texture2D name : register(t##reg)
	#define ST_TEXTURE_DECLARE_MS(name, reg, num_samples)					Texture2DMS<float4, num_samples> name : register(t##reg)
	#define ST_TEXTURE_DECLARE_SHADOW(name, reg)							Texture2D name : register(t##reg)

	// texture sampling commands
	#define ST_TEXTURE_SAMPLE_LINEAR_WRAP(name, texcoord)					name.Sample(samLinearWrap, texcoord)
	#define ST_TEXTURE_SAMPLE_LINEAR_WRAP_LOD_0(name, texcoord)				name.SampleLevel(samLinearWrap, texcoord, ST_LOD_LEVEL_0)
	#define ST_TEXTURE_SAMPLE_LINEAR_CLAMPED(name, texcoord)				name.SampleLevel(samLinearClamped, texcoord, ST_LOD_LEVEL_0)
	#define ST_TEXTURE_SAMPLE_POINT_CLAMP_NO_MIPMAP(name, texcoord)			name.SampleLevel(samPointClampedNoMipmap, texcoord, ST_LOD_LEVEL_0)
	#define ST_TEXTURE_SAMPLE_MS_POINT_CLAMP(name, texcoord, sample)		name.Load(texcoord, sample)
	#define ST_TEXTURE_SAMPLE_SHADOW_COMPARE(name, sampler, texcoord)		((name).SampleCmpLevelZero(samShadowMapCompare, (texcoord).xy, (texcoord).z))

#elif (ST_PS4) // PS4

	// samplers shared by the texture macros
	#define ST_SAMPLER_DECLARE(name, type, reg)								type name : register(s##reg)

	ST_SAMPLER_DECLARE(samLinearWrap, SamplerState, ST_SAMPLER_LINEAR_WRAP);
	ST_SAMPLER_DECLARE(samLinearClamped, SamplerState, ST_SAMPLER_LINEAR_CLAMPED);
	ST_SAMPLER_DECLARE(samPointClampedNoMipmap, SamplerState, ST_SAMPLER_POINT_CLAMPED_NO_MIPMAP);
	ST_SAMPLER_DECLARE(samShadowMapCompare, SamplerComparisonState, ST_SAMPLER_SHADOW_COMPARE);

	// texture declarations
	#define ST_TEXTURE_DECLARE(name, reg)									Texture2D name : register(t##reg)
	#define ST_TEXTURE_DECLARE_MS(name, reg, num_samples)					Texture2D name : register(t##reg)
	#define ST_TEXTURE_DECLARE_SHADOW(name, reg)							Texture2D name : register(t##reg)

	// texture sampling commands
	#define ST_TEXTURE_SAMPLE_LINEAR_WRAP(name, texcoord)					name.Sample(samLinearWrap, texcoord)
	#define ST_TEXTURE_SAMPLE_LINEAR_WRAP_LOD_0(name, texcoord)				name.SampleLOD(samLinearWrap, texcoord, ST_LOD_LEVEL_0)
	#define ST_TEXTURE_SAMPLE_LINEAR_CLAMPED(name, texcoord)				name.SampleLOD(samLinearClamped, texcoord, ST_LOD_LEVEL_0)
	#define ST_TEXTURE_SAMPLE_POINT_CLAMP_NO_MIPMAP(name, texcoord)			name.SampleLOD(samPointClampedNoMipmap, texcoord, ST_LOD_LEVEL_0)
	#define ST_TEXTURE_SAMPLE_MS_POINT_CLAMP(name, texcoord, sample)		name.Sample(samPointClampedNoMipmap, texcoord)
	#define ST_TEXTURE_SAMPLE_SHADOW_COMPARE(name, sampler, texcoord)		((name).SampleCmpLOD0(samShadowMapCompare, (texcoord).xy, (texcoord).z))

#elif (ST_OPENGL) // PC, Mac, and Linux GLSL

	// opengl texture declarations
	//
	// instead of using 'layout(binding = X)' syntax, which requires #version 420, we bind the samplers using opengl API
	// calls in the function BindTextureSamplers() declared in [SDK]\Include\Renderers\OpenGL\Shaders_inl.h

	#define ST_TEXTURE_DECLARE(name, reg)									uniform sampler2D name
	#define ST_TEXTURE_DECLARE_MS(name, reg, num_samples)					uniform sampler2DMS name
	#define ST_TEXTURE_DECLARE_SHADOW(name, reg)							uniform sampler2DShadow name

	// texture sampling commands
	#if (__VERSION__ >= 150)
		#define ST_TEXTURE_SAMPLE_LINEAR_WRAP(name, texcoord)				texture(name, texcoord)
		#define ST_TEXTURE_SAMPLE_LINEAR_WRAP_LOD_0(name, texcoord)			texture(name, texcoord)
		#define ST_TEXTURE_SAMPLE_LINEAR_CLAMPED(name, texcoord)			texture(name, texcoord)
		#define ST_TEXTURE_SAMPLE_POINT_CLAMP_NO_MIPMAP(name, texcoord)		texture(name, texcoord)
		#define ST_TEXTURE_SAMPLE_MS_POINT_CLAMP(name, texcoord, sample)	texelFetch(name, ivec2(texcoord), sample)
		#define ST_TEXTURE_SAMPLE_SHADOW_COMPARE(name, sampler, texcoord)	textureProj(sampler, texcoord)
	#else
		#define ST_TEXTURE_SAMPLE_LINEAR_WRAP(name, texcoord)				texture2D(name, texcoord)
		#define ST_TEXTURE_SAMPLE_LINEAR_WRAP_LOD_0(name, texcoord)			texture2D(name, texcoord)
		#define ST_TEXTURE_SAMPLE_LINEAR_CLAMPED(name, texcoord)			texture2D(name, texcoord)
		#define ST_TEXTURE_SAMPLE_POINT_CLAMP_NO_MIPMAP(name, texcoord)		texture2D(name, texcoord)
		#define ST_TEXTURE_SAMPLE_MS_POINT_CLAMP(name, texcoord, sample)	texture2D(name, texcoord)
		#define ST_TEXTURE_SAMPLE_SHADOW_COMPARE(name, sampler, texcoord)	shadow2DProj(sampler, texcoord).r

	#endif

#else // DX9, Xbox 360, and PS3

	// texture declarations
	#define ST_TEXTURE_DECLARE(name, reg)									texture name; sampler2D sam##name : register(s##reg) = sampler_state { Texture = <name>; }
	#define ST_TEXTURE_DECLARE_MS(name, reg, num_samples)					texture name; sampler2D sam##name : register(s##reg) = sampler_state { Texture = <name>; }
	#define ST_TEXTURE_DECLARE_SHADOW(name, reg)							texture name; sampler2D sam##name : register(s##reg) = sampler_state { Texture = <name>; }

	// texture sampling commands
	#define ST_TEXTURE_SAMPLE_LINEAR_WRAP(name, texcoord)					tex2D(sam##name, texcoord)
	#define ST_TEXTURE_SAMPLE_LINEAR_WRAP_LOD_0(name, texcoord)				tex2Dlod(sam##name, float4((texcoord).xy, 0.0, 0))
	#define ST_TEXTURE_SAMPLE_LINEAR_CLAMPED(name, texcoord)				tex2D(sam##name, texcoord)
	#define ST_TEXTURE_SAMPLE_POINT_CLAMP_NO_MIPMAP(name, texcoord)			tex2Dlod(sam##name, float4((texcoord).xy, 0.0, 0))
	#define ST_TEXTURE_SAMPLE_MS_POINT_CLAMP(name, texcoord, sample)		tex2Dlod(sam##name, float4((texcoord).xy, 0.0, 0))
	#define ST_TEXTURE_SAMPLE_SHADOW_COMPARE(name, sampler, texcoord)		tex2Dproj(sampler, texcoord).r

#endif

#endif // ST_INCLUDE_SAMPLERS_AND_TEXTURE_MACROS

///////////////////////////////////////////////////////////////////////
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2016 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#ifndef ST_INCLUDE_UTILITY
#define ST_INCLUDE_UTILITY


///////////////////////////////////////////////////////////////////////
//  Constants

#define ST_PI		3.14159265358979323846
#define ST_TWO_PI	6.283185307179586476925


///////////////////////////////////////////////////////////////////////
//  ConvertToStdCoordSys
//
//	By default, SpeedTree operates in a right-handed, Z-up coordinate system. This
//	function will convert from an alternate coordinate system to SpeedTree's right-
//	handed Z-up system if an alternate has been selected. The supported alternate
//	coordinate systems are right-handed Y-up, left-handed Y-up, and left-handed Z-up.

float3 ConvertToStdCoordSys(float3 vCoord)
{
	if (ST_COORDSYS_Z_UP)
		return ST_COORDSYS_LEFT_HANDED ? float3(vCoord.x, -vCoord.y, vCoord.z) : vCoord;
	else
		return ST_COORDSYS_LEFT_HANDED ? float3(vCoord.x, vCoord.z, vCoord.y) : float3(vCoord.x, -vCoord.z, vCoord.y);
}


///////////////////////////////////////////////////////////////////////
//  ConvertFromStdCoordSys
//
//	By default, SpeedTree operates in a right-handed, Z-up coordinate system. This
//	function will convert from SpeedTree's standard system to an alternate coord system
//	if one has been selected. The supported alternate coordinate systems are right-
//	handed Y-up, left-handed Y-up, and left-handed Z-up.

float3 ConvertFromStdCoordSys(float3 vCoord)
{
	if (ST_COORDSYS_Z_UP)
		return ST_COORDSYS_LEFT_HANDED ? float3(vCoord.x, -vCoord.y, vCoord.z) : vCoord;
	else
		return ST_COORDSYS_LEFT_HANDED ? float3(vCoord.x, vCoord.z, vCoord.y) : float3(vCoord.x, vCoord.z, -vCoord.y);
}



///////////////////////////////////////////////////////////////////////
//  Helper Function: BuildOrientationMatrix
//
//	Given three vectors to define an orientation, a matrix is built based on
//	the rendering platform and chosen coordinate system.

float3x3 BuildOrientationMatrix(float3 vRight, float3 vOut, float3 vUp)
{
	if (ST_COORDSYS_Z_UP)
		return (ST_OPENGL != 0) ? float3x3(vRight, vOut, vUp) : float3x3(vRight.x, vOut.x, vUp.x,
																		 vRight.y, vOut.y, vUp.y,
																		 vRight.z, vOut.z, vUp.z);
	else
		return (ST_OPENGL != 0) ? float3x3(vRight, vUp, vOut) : float3x3(vRight.x, vUp.x, vOut.x,
																		 vRight.y, vUp.y, vOut.y,
																		 vRight.z, vUp.z, vOut.z);
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: BuildBillboardOrientationMatrix
//
//	Given three vectors to define an orientation, a matrix is built based on
//	the rendering platform and chosen coordinate system to turn the facing
//	leaf card geometry; this function is pretty basic, the algorithm for
//	determining the vector values is in the calling code.

float3x3 BuildBillboardOrientationMatrix(float3 vRight, float3 vOut, float3 vUp)
{
	if (ST_OPENGL != 0 || ST_PS3 != 0)
	{
		if (ST_COORDSYS_LEFT_HANDED)
			return float3x3(-vRight, vOut, vUp);
		else
		{
			if (ST_COORDSYS_Y_UP)
				vOut = -vOut;

			return float3x3(vRight, vOut, vUp);
		}
	}
	else
	{
		if (ST_COORDSYS_LEFT_HANDED)
			return float3x3(-vRight.x, vOut.x, vUp.x,
							-vRight.y, vOut.y, vUp.y,
							-vRight.z, vOut.z, vUp.z);
		else
		{
			if (ST_COORDSYS_Y_UP)
				vOut = -vOut;

			return float3x3(vRight.x, vOut.x, vUp.x,
							vRight.y, vOut.y, vUp.y,
							vRight.z, vOut.z, vUp.z);
		}
	}

}


///////////////////////////////////////////////////////////////////////
//  Helper Function: DotProductLighting
//
//	Simple lighting equation SpeedTree uses in a few shaders; SpeedTree lerps
//	from the ambient color to diffuse by the dot product of the normal and
//	light direction vectors.

float3 DotProductLighting(float3 vAmbientColor, float3 vDiffuseColor, float fDot)
{
	return lerp(vAmbientColor, vDiffuseColor, saturate(fDot));
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: AmbientContrast
//
//	This is one technique SpeedTree employs to help keep the dark-side lighting
//	of vegetation interesting (non-flat). This is simpler than our image-based
//	ambient lighting but not mutually exclusive.

float AmbientContrast(float fContrastFactor, float fDot)
{
	return lerp(fContrastFactor, 1.0, abs(fDot));
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: Fade3dTree
//
//	The SDK's Core library will compute a 3D instance's LOD value such that:
//		- [1.0 -> 0.0] means the 3D tree is moving from highest 3D LOD to
//                     lowest 3D LOD
//		- [0.0 -> -1.0] means the 3D tree is fading gradually from lowest
//                      3D LOD to full billboard
//
//	Return value for Fade3dTree: 1.0 is no fade, 0.0 is full fade

float Fade3dTree(float fLodValue)
{
	// C code:
	//if (fLodValue < 0.0f)
	//{
	//	// fade is active
	//	return 1.0f - -fLodValue;
	//}
	//else
	//	// fade is inactive
	//	return 1.0f;

	return 1.0 - saturate(-fLodValue);
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: FadeBillboard
//
//	As the camera moves away from a tree instance, the lowest 3D LOD model
//	will begin to alpha fade as a billboard representation begins to fade
//	in (the opposite happens when the camera moves closer). After billboards
//	get far enough away, an optional behavior is for the billboards to either
//	pop or fade out.
//
//	Several shader constants govern this behavior:
//
//	  - u_fBillboardRange: distance in world units over which billboard fades in
//
//	  - u_fBillboardStartDist: distance in world units where billboard begins to
//                             fade in (minus u_fBillboardRange for shader optimization)
//
//	  - u_fBillboardCullDist: distance from camera in world units where billboards
//                            are no longer drawn (minus u_fBillboardRange for
//                            shader optimization)
//
//	Two macros, #defined just above the function definition below, also control
//	this function's behavior:
//
//	  - ST_CULL_DISTANT_BILLBOARDS: set to true to cull billboards after a certain
//								    distance; false means they are culled by far clip.
//
//	  - ST_FADE_CULLED_BILLBOARDS: if/when culled, set to true to fade the
//								   billboards, false to have them pop out.
//
//	This function takes the instance's position from the camera (not each
//	vertex's position, but the base pos of the instance) in world units.
//
//	It returns a [0.0, 1.0] value. 0.0 means the billboard is invisible, 1.0
//	means it is fully visible. Values in between indicate a fade state.

#define ST_CULL_DISTANT_BILLBOARDS false
#define ST_FADE_CULLED_BILLBOARDS  true

float FadeBillboard(float fDistanceFromCamera)
{
	if (ST_CULL_DISTANT_BILLBOARDS && fDistanceFromCamera > u_fBillboardCullDist)
		return (ST_FADE_CULLED_BILLBOARDS ? saturate(1.0 - (fDistanceFromCamera - u_fBillboardCullDist) / u_fBillboardRange) : 0.0);
	else
		return saturate((fDistanceFromCamera - u_fBillboardStartDist) / u_fBillboardRange);
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: FadeGrass
//
//	Given how far a grass instance is from the camera (or LOD ref point) in world
//	units, this function will generate a [0.0, 1.0] scalar value that begins
//	at distance u_f3dGrassStartDist and spans a distance of u_fBillboardRange.
//
//	Since grass instances fade out as the camera moves further away, the scale value
//	will be 1.0 when (dist <= u_f3dGrassStartDist), making the grass fully visible.
//	As the distance moves toward (u_f3dGrassRange + u_fBillboardRange), it will
//	move linearly to 0.0, making the grass invisible and able to be culled.

float FadeGrass(float fDistanceFromCamera)
{
	return saturate(1.0 - (fDistanceFromCamera - u_f3dGrassStartDist) / u_f3dGrassRange);
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: ComputeTransmissionFactor
//
//	Transmission (SpeedTree's approximation of subsurface scattering), is
//	computed using the surface normal together with a "view dependency" value
//	set by the artist in the Modeler. View dependency controls how much the
//	viewing angle limits the effect of translucency. A value of 1.0 (the
//	default) fades light transmission in as the camera looks towards the
//	light source. A value of 0.0 allows light transmission on the front-lit
//	side.

float ComputeTransmissionFactor(float3 vNormal, float3 vLightDir, float fViewDependency, bool bBackfaceCulling)
{
	// adjust normal to compensate for backfacing geometry
	if (bBackfaceCulling)
		vNormal = -vNormal;

	// how much is the camera looking into the light source?
    float fBackContribution = ST_COORDSYS_LEFT_HANDED ? (dot(-u_vCameraDirection, vLightDir) + 1.0) * 0.5 :
                                                        (dot(u_vCameraDirection, vLightDir) + 1.0) * 0.5;

	// compute the result to get the right falloff
	fBackContribution *= fBackContribution;
	fBackContribution *= fBackContribution;

	// use the reverse normal to compute scatter
    float fScatterDueToNormal = (dot(-vLightDir, -vNormal) + 1.0) * 0.5;

	// choose between scatter and back contribution based on artist-controlled parameter
	float fTransmissionFactor = lerp(fScatterDueToNormal, fBackContribution, fViewDependency);

	// back it off based on how much leaf is in the way
	float fReductionDueToNormal = ST_COORDSYS_LEFT_HANDED ? (dot(-u_vCameraDirection, vNormal) + 1.0) * 0.5 :
															(dot(u_vCameraDirection, vNormal) + 1.0) * 0.5;
	fTransmissionFactor *= fReductionDueToNormal;

	return fTransmissionFactor;
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: ProjectToScreen
//
//	Simple screen projection using a composite modelview / projection matrix
//	uploaded to u_mModelViewProj3d. SpeedTree subtracts the camera position
//	from the incoming position to help curb floating-point resolution issues
//	with large position values.

float4 ProjectToScreen(float4 vPos)
{
	// the camera stay at the origin, so everything else is translated to it
	vPos.xyz -= u_vCameraPosition;

	return mul_float4x4_float4(u_mModelViewProj3d, vPos);
}


///////////////////////////////////////////////////////////////////////
//	Helper Function: DecodeVectorFromColor
//
//	Converts a color of range [0, 1] to a vector of range [-1, 1].
//
//	float4 version.

float4 DecodeVectorFromColor(float4 vColor)
{
	return vColor * 2.0 - 1.0;
}


///////////////////////////////////////////////////////////////////////
//	Helper Function: DecodeVectorFromColor
//
//	Converts a color of range [0, 1] to a vector of range [-1, 1].
//
//	float3 version.

float3 DecodeVectorFromColor(float3 vColor)
{
	return vColor * 2.0 - 1.0;
}


///////////////////////////////////////////////////////////////////////
//	Helper Function: EncodeVectorToColor
//
//	Converts a vector of range [-1, 1] to a color of range [0, 1].

float3 EncodeVectorToColor(float3 vVector)
{
	return vVector * 0.5 + 0.5;
}


///////////////////////////////////////////////////////////////////////
//	Helper Function: DecodeFloat4FromUBytes
//
//	This function helps to align SpeedTree's supported platforms behaviors when
//	storing normal values as unsigned bytes.

float4 DecodeFloat4FromUBytes(float4 vCompressedBytes)
{
	if (ST_DIRECTX11 != 0 || ST_PS4 != 0)
		return vCompressedBytes * 2.0 - 1.0;
	else if (ST_XBOX_360 != 0)
		// 360 automatically normalizes signed byte values when uploaded as D3DDECLTYPE_BYTE4N, but
		// free swizzle is still used to counter big endian
		return vCompressedBytes.wzyx / 127.5 - 1.0;
	else
		return vCompressedBytes / 127.5 - 1.0;
}


///////////////////////////////////////////////////////////////////////
//	Helper Function: CheckForEarlyExit
//
//	Basically this is a function to encapsulate alpha testing. It depends
//	on whether the app is using alpha testing or alpha-to-coverage to
//	handle transparency. Also note OpenGL's use of discard versus HLSL's
//	use of clip. In OpenGL, discard is not a valid call from a vertex shader
//	so it is #ifdef'd out.

void CheckForEarlyExit(float fAlphaValue, bool bTransparencyActive)
{
	if (bTransparencyActive)
	{
		#if (ST_OPENGL)
			// at least one GLSL compiler will not permit discard to be present in a vertex
			// shader's source, even if it isn't used, so we put this #ifdef around it
			#ifdef ST_PIXEL_SHADER
				if (fAlphaValue < ST_ALPHA_KILL_THRESHOLD)
					discard;
			#endif
		#else
			clip(fAlphaValue - ST_ALPHA_KILL_THRESHOLD);
		#endif
	}
}


///////////////////////////////////////////////////////////////////////
//  Sinusoidal
//
//	Faster/approximate sin wave.

float4 SineWaves4(float4 vData)
{
	return sin(vData);
}

float2 SineWaves2(float2 vData)
{
	return sin(vData);
}

#undef ST_SIN_TO_APPROX_RATIO



///////////////////////////////////////////////////////////////////////
//  UnpackNormalFromFloat
//
//	Designed to work with 16-bit float values, this function pulls a float3
//	of range [-1, 1] from a single 16-bit float value. Also works with 32-bit
//	floats, but spreads the error better for 16.

float3 UnpackNormalFromFloat(float fValue)
{
	#define ST_DECODE_KEY float3(16.0, 1.0, 0.0625)

	// decode into [0,1] range
	float3 vDecodedValue = frac(fValue / ST_DECODE_KEY);

	// move back into [-1,1] range & normalize
	return (vDecodedValue * 2.0 - 1.0);

	#undef ST_DECODE_KEY
}


////////////////////////////////////////////////////////////
//	PackNormalIntoFloat_Stereographic
//
//	http://aras-p.info/texts/CompactNormalStorage.html

float2 PackNormalIntoFloat2_Stereographic(float3 n)
{
	float scale = 1.7777;

	float2 enc = n.xy / (n.z + 1.0);
	enc /= scale;
	enc = enc * 0.5 + 0.5;

	return enc;
}


////////////////////////////////////////////////////////////
//	UnpackNormalFromFloat_Stereographic
//
//	http://aras-p.info/texts/CompactNormalStorage.html

float3 UnpackNormalFromFloat2_Stereographic(float2 enc2)
{
	float4 enc = float4(enc2, 0, 0);

	float scale = 1.7777;
	float3 nn = enc.xyz * float3(2.0 * scale, 2.0 * scale,0) + float3(-scale, -scale, 1.0);
	float g = 2.0 / dot(nn.xyz, nn.xyz);

	float3 n;
	n.xy = g * nn.xy;
	n.z = g - 1.0;

	return n;
}


////////////////////////////////////////////////////////////
//	PackNormalIntoFloat_Spheremap
//
//	http://aras-p.info/texts/CompactNormalStorage.html

float2 PackNormalIntoFloat2_Spheremap(float3 n)
{
	float f = sqrt(8.0 * n.z + 8.0);

	return n.xy / f + 0.5;
}


////////////////////////////////////////////////////////////
//	UnpackNormalFromFloat_Spheremap

float3 UnpackNormalFromFloat2_Spheremap(float2 enc2)
{
	float2 fenc = enc2 * 4.0 - 2.0;
	float f = dot(fenc, fenc);
	float g = sqrt(1.0 - f / 4.0);
	float3 n;
	n.xy = fenc * g;
	n.z = 1.0 - f / 2.0;

	return normalize(n);
}


////////////////////////////////////////////////////////////
//	PackTwoIntoOne
//
//  Used when packing values in our example g-buffer: this function
//  packs two 8-bit values into one 8-bit value by dropping the 4
//  least significant bits for each value and packing each
//  side-by-side. The incoming values must be in [0.0, 1.0] range.

float PackTwoIntoOne(float2 vValues)
{
    // PS3 does not support uints; uints are recommended for these operations
    // by the fx compiler (warnings issued otherwise)

    #if (ST_PS3 == 1 || ST_DIRECTX9 == 1)
        // convert to integer values
        int2 vByteValues = int2(vValues * 255.0);

        // remove least significant bits (converting two 8-bit values to two 4-bit)
        vByteValues = int2((vByteValues.x / 16) * 16, vByteValues.y / 16);

        return float(vByteValues.x + vByteValues.y) / 255.0;
    #else
        // convert to integer values
        uint2 vByteValues = uint2(vValues * 255.0);

        // remove least significant bits (converting two 8-bit values to two 4-bit)
        vByteValues = uint2((vByteValues.x / 16u) * 16u, vByteValues.y / 16u);

        return float(vByteValues.x + vByteValues.y) / 255.0;
    #endif
}


////////////////////////////////////////////////////////////
//	UnpackTwoFromOne
//
//  Used when un packing values in our example g-buffer: this
//  function unpacks the values stored by PackTwoIntoOne() above.

float2 UnpackTwoFromOne(float fPackedValue)
{
    // PS3 does not support uints; uints are recommended for these operations
    // by the fx compiler (warnings issued otherwise)

    #if (ST_PS3 == 1)
        // use of abs() to remove ps_3_0 compilation warnings
        int iCombinedMsbs = int(abs(fPackedValue) * 255.0);

        int x = (iCombinedMsbs / 16u) * 16u;
        float y = (float(iCombinedMsbs) - x) * 16u;

        // mix of uint and float types also due to ps_3_0 compilation warnings
        return float2(float(x), y) / 255.0;
    #else
        // use of abs() to remove ps_3_0 compilation warnings
        uint iCombinedMsbs = uint(abs(fPackedValue) * 255.0);

        uint x = (iCombinedMsbs / 16u) * 16u;
        float y = (float(iCombinedMsbs) - x) * 16u;

        // mix of uint and float types also due to ps_3_0 compilation warnings
        return float2(float(x), y) / 255.0;
    #endif
}


///////////////////////////////////////////////////////////////////////
//  ApproxAcos

float ApproxAcos(float x)
{
    return (-0.69813170079773212 * x * x - 0.87266462599716477) * x + 1.5707963267948966;
}


///////////////////////////////////////////////////////////////////////
//  ArbitraryAxisRotationMatrix
//
//  Constructs an arbitrary axis rotation matrix. Resulting matrix will
//	rotate a point fAngle radians around vAxis.

float3x3 ArbitraryAxisRotationMatrix(float3 vAxis, float fAngle)
{
	// compute sin/cos of fAngle
	float2 vSinCos;
	#if (ST_OPENGL)
		vSinCos.x = sin(fAngle);
		vSinCos.y = cos(fAngle);
	#else
		sincos(fAngle, vSinCos.x, vSinCos.y);
	#endif

	#define c_var vSinCos.y
	#define s_var vSinCos.x
	#define x_var vAxis.x
	#define y_var vAxis.y
	#define z_var vAxis.z
	float t_var = 1.0 - c_var;

	return float3x3(t_var * x_var * x_var + c_var,			t_var * x_var * y_var - s_var * z_var,	t_var * x_var * z_var + s_var * y_var,
					t_var * x_var * y_var + s_var * z_var,	t_var * y_var * y_var + c_var,			t_var * y_var * z_var - s_var * x_var,
					t_var * x_var * z_var - s_var * y_var,	t_var * y_var * z_var + s_var * x_var,	t_var * z_var * z_var + c_var);

	#undef c_var
	#undef s_var
	#undef x_var
	#undef y_var
	#undef z_var
}


///////////////////////////////////////////////////////////////////////
//  LimitToCameraPlane
//
//	Used in conjunction with billboard wind -- it confines the wind motion
//	to the plane defined by the screen

float3 LimitToCameraPlane(float3 vDir)
{
	const float3 c_vCameraRight = (ST_OPENGL != 0) ? u_mCameraFacingMatrix[1].xyz : float3(u_mCameraFacingMatrix[0].y, u_mCameraFacingMatrix[1].y, u_mCameraFacingMatrix[2].y);
	const float3 c_vCameraUp = (ST_OPENGL != 0) ? u_mCameraFacingMatrix[2].xyz : float3(u_mCameraFacingMatrix[0].z, u_mCameraFacingMatrix[1].z, u_mCameraFacingMatrix[2].z);

	return dot(vDir, c_vCameraRight) * c_vCameraRight + dot(vDir, c_vCameraUp) * c_vCameraUp;
}


///////////////////////////////////////////////////////////////////////
//  ProjectToLightSpace
//
//	Used to project SpeedTree geometry into a shadow map.

float4 ProjectToLightSpace(float3 vPos, float4x4 mLightViewProj)
{
	float4 vProjection = mul_float4x4_float4(mLightViewProj, float4(vPos, 1.0));

	if (ST_DIRECTX9 != 0 || ST_DIRECTX11 != 0)
	{
		vProjection.xy = 0.5 * vProjection.xy / vProjection.w + float2(0.5, 0.5) + u_sShadows.m_vTexelOffset;
		vProjection.y = 1.0 - vProjection.y;
	}
	else
	{
		vProjection.xyz += vProjection.www;
		vProjection.xy += u_sShadows.m_vTexelOffset;
		vProjection.xyz *= 0.5;

		if (ST_PS4 != 0)
			vProjection.y = 1.0 - vProjection.y;
	}

	return vProjection;
}


///////////////////////////////////////////////////////////////////////
//  GenerateHueVariationByPos
//
//	Determines a unique [0, 1] float3 based on attributes unique to an instance;
//	in this case we're using a combination of the instance's 3D position
//	and orientation vector.

float3 GenerateHueVariationByPos(float fVariationScalar, float3 vInstanceRightVector)
{
	return fVariationScalar * fmod(vInstanceRightVector, float3(1.0, 1.0, 1.0)) * u_vHueVariationColor;
}


///////////////////////////////////////////////////////////////////////
//  GenerateHueVariationByVertex
//
//	Determine a unique [0, 1] float3 value based on attributes unique to a vertex
//	that *do not* change with LOD. We're using the normal in combination
//	with the instance's orientation vector.
//
//	This function can be modified as needed, but care should be take not to
//	use vertex attributes that might change as LOD changes, else the hue
//	variation will change along with it.

float3 GenerateHueVariationByVertex(float fVariationScalar, float3 vNormal)
{
	return fVariationScalar * sin(vNormal * 20.0f) * u_vHueVariationColor.rgb;
}


///////////////////////////////////////////////////////////////////////
//  FadeShadowByDepth
//
//	In the SpeedTree example shadow system, the shadow fades out after a
//	certain distance away from the camera. Given a shadow map look up value,
//	this function will push it to 1.0 (white / no shadow) based on a couple
//	of parameters. Function paramters:
//
//		fShadowMapValue: the raw value from the shadowmap look up (0.0 is
//						 fully shadowed, 1.0 is fully lit)
//
//		fDepthAtShadowedPoint: distance from the camera to the pixel
//
//		fStartingFadeDepth: [0.0, 1.0] value designating where the shadow
//							should begin to fade. 0.0 is at the camera pos,
//							1.0 is u_fFarClip.x
//
//		fFadeFactor: value computed on upload to save shader instructions; it's
//					 1.0f / (end_of_last_cascade - fStartingFadeDepth), where
//					 end_of_last_cascade is in same units as fStartingFadeDepth

float FadeShadowByDepth(float fShadowMapValue, float fDepthAtShadowedPoint, float fStartingFadeDepth, float fFadeFactor)
{
	float fFade = saturate((fDepthAtShadowedPoint - fStartingFadeDepth) * fFadeFactor);

	return lerp(fShadowMapValue, 1.0, fFade);
}


///////////////////////////////////////////////////////////////////////
//  Helper function: BillboardSelectMapFromAtlas
//
//	Billboard instances are rendered in batches of the same base tree (e.g. all
//	palm trees are rendered in one call), but they may all have arbitrary
//	orientations. Each base tree also probably has several billboard renderings
//	of the tree from different angles (360-degree billboards), all in the same
//	billboard atlas that's current bound. This function will figure out which
//	atlas entry is the correct one.
//
//	Given a camera, an instance orientation (right, out, up), and the number of
//	360-degree billboard images uploaded for the current base tree, this function
//	will determine the correct texcoords per vertex from the billboard atlas.
//
//	The coordinate system figures heavily into the computation, so you'll see use
//	of ST_COORDSYS_RIGHT_HANDED, ST_COORDSYS_Z_UP, etc.

float2 BillboardSelectMapFromAtlas(float2 vUnitTexCoords,		// texcoords of cutout billboard, defined in Compiler, in [0.0,1.0] range;
																// will be converted to correct smaller texcoords to access sub-image in atlas
								   float3 vRightVector,			// right orientation of current instance
								   float3 vOutVector,			// out orientation of current instance
								   float3 vUpVector,			// up orientation of current instance
								   float3 vCameraDirection)		// which way the main camera is facing
{
	// algorithmic depends on opposite direction
	float3 vDir = -vCameraDirection.xyz;

	// given a particular coordinate system some adjustments are necessary for algorithm that expects right-handed/Z-up
	if (ST_COORDSYS_LEFT_HANDED)
	{
		if (ST_COORDSYS_Y_UP)
			vDir = float3(-vDir.x, vDir.y, -vDir.z);
		else
			vRightVector = float3(-vRightVector.x, -vRightVector.y, vRightVector.z);
	}

	// project camera direction onto plane that holds vRightVector and vOutVector vectors
	float fUpDot = dot(vDir, vUpVector);
	float3 vProjectedCamera = normalize(vDir - fUpDot * vUpVector);

	// determine which angle the projection lands on; corresponds to the correct bb sub-image
	float fRightDot = dot(vRightVector, vProjectedCamera);
	if (ST_PS3 != 0)
		fRightDot = clamp(fRightDot, -1.0, 1.0);
    float fRightAngle = ApproxAcos(fRightDot);

	// need a value in [0,2pi] range, not [-pi,pi]
	float fOutDot = dot(vOutVector, vProjectedCamera);
	if (fOutDot < 0.0)
		fRightAngle = ST_TWO_PI - fRightAngle;

	// bump angle to be in the middle of the pie slice
	fRightAngle += ST_PI / u_fNumBillboards;
	fRightAngle = fmod(fRightAngle, ST_TWO_PI);

	// convert angle to billboard image index
	int nImageIndex	= int(fRightAngle / u_fRadiansPerImage);

	// u_avBillboardTexCoords holds the texcoords for each 360-degree image for a given base tree; each single
	// entry xyzw defines a sub-rectangle in the billboard atlas and does it using four values:
	//
	//		(left u texcoord, bottom v texcoord, width of u span, height of v span)
	//
	// for example, if there are four evenly-spaced billboard images in a given atlas and no other textures
	// reside there, the four entries in u_avBillboardTexCoords might be:
	//
	//		u_avBillboardTexCoords[0] = (0.0, 0.5, 0.5, 0.5) // top left bb image
	//		u_avBillboardTexCoords[1] = (0.5, 0.5, 0.5, 0.5) // top right bb image
	//		u_avBillboardTexCoords[1] = (0.0, 1.0, 0.5, 0.5) // bottom left bb image
	//		u_avBillboardTexCoords[1] = (0.5, 1.0, 0.5, 0.5) // bottom right bb image
	//
	// note that because billboard images may be oriented vertically or horizontally, the .x coordinate in
	// each entry may be negative, indicating a horizontal orientation; the example above assumes all
	// vertical orientations

	// nImage index holds the correct offset into this table
	#ifndef g_avBillboardTexCoords
		float4 vTableEntry = u_avBillboardTexCoords[nImageIndex];
	#else
		// in opengl, g_avBillboardTexCoords is defined a bit differently
		float4 vTableEntry = A_avSingleUniformBlock[BILLBOARD_TEXCOORDS_LOCATION + nImageIndex];
	#endif

	// some further adjustment is still needed for alternate coordinate systems
	if (ST_COORDSYS_RIGHT_HANDED && ST_COORDSYS_Y_UP)
		vUnitTexCoords.x = 1.0 - vUnitTexCoords.x;
	else if (ST_COORDSYS_LEFT_HANDED && ST_COORDSYS_Z_UP)
		vUnitTexCoords.x = 1.0 - vUnitTexCoords.x;

	// if .x coordinate is negative, the billboard is oriented horizontally; this code also applies to the unit
	// texcoords to the sub-image of the billboard atlas
	float2 vAtlasTexCoords = (vTableEntry.x < 0.0) ? float2(-vTableEntry.x + vTableEntry.z * vUnitTexCoords.y,
															vTableEntry.y + vTableEntry.w * vUnitTexCoords.x) :
													 float2(vTableEntry.x + vTableEntry.z * vUnitTexCoords.x,
															vTableEntry.y + vTableEntry.w * vUnitTexCoords.y);

	return vAtlasTexCoords;
}


///////////////////////////////////////////////////////////////////////
//  RgbToLuminance
//
//	Centralized function so that users can choose between an accurate conversion
//	or quick and dirty.

float RgbToLuminance(float3 vRGB)
{
	return dot(vRGB, float3(0.299, 0.587, 0.114)); // CCIR 601 luminance coeffs
}


#endif // ST_INCLUDE_UTILITY
///////////////////////////////////////////////////////////////////////
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2016 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#ifndef ST_INCLUDE_TREE_TEXTURES
#define ST_INCLUDE_TREE_TEXTURES


///////////////////////////////////////////////////////////////////////
//	Texture/material bank used for 3d geometry shaders
//
//	In OpenGL/GLSL, changes in these sampler names (or additions) should be followed by 
//	a change to BindTextureSamplers() in Shaders_inl.h in the OpenGL render library.

ST_TEXTURE_DECLARE(DiffuseMap, 0);
ST_TEXTURE_DECLARE(NormalMap, 1);
ST_TEXTURE_DECLARE(DetailDiffuseMap, 2);
ST_TEXTURE_DECLARE(DetailNormalMap, 3);
ST_TEXTURE_DECLARE(SpecularMaskMap, 4);
ST_TEXTURE_DECLARE(TransmissionMaskMap, 5);
ST_TEXTURE_DECLARE(NoiseMap, 6);

#endif // ST_INCLUDE_TREE_TEXTURES
///////////////////////////////////////////////////////////////////////
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2016 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#ifndef ST_INCLUDE_TEXTURE_UTILITY
#define ST_INCLUDE_TEXTURE_UTILITY


///////////////////////////////////////////////////////////////////////
//  LookupColorWithDetail
//
//	This function encapsulates the diffuse and detail layer texture lookups.
//
//	It will always look up the diffuse texture. If a detail layer is present,
//	it will look it up and lerp between the diffuse and detail layer by the
//	detail map's alpha channel value. If the detail layer should fade away,
//	it will handle that was well.

float3 LookupColorWithDetail(float2 vDiffuseTexCoords, float2 vDetailTexCoords, float fRenderEffectsFade)
{
	float3 vFinal = ST_TEXTURE_SAMPLE_LINEAR_WRAP(DiffuseMap, vDiffuseTexCoords).rgb;

	if (ST_EFFECT_DETAIL_LAYER)
	{
		float4 texDetailColor = ST_TEXTURE_SAMPLE_LINEAR_WRAP(DetailDiffuseMap, vDetailTexCoords);
		if (ST_EFFECT_DETAIL_LAYER_FADE)
			texDetailColor.a *= fRenderEffectsFade;

		vFinal = lerp(vFinal, texDetailColor.rgb, texDetailColor.a);
	}

	return vFinal;
}


///////////////////////////////////////////////////////////////////////
//  LookupNormalWithDetail
//
//	This function encapsulates the diffuse and detail layer normal map
//	lookups.
//
//	It will always look up the diffuse normal texture. If a detail normal
//	layer is present, it will look it up and lerp between the diffuse and
//	detail layer normals by the detail normal's alpha channel value
//	by the detail's *color* alpha channel value (the same one used in
//	LookupColorWithDetail). If the detail layer should fade away, it will
//	handle that was well.
//
//	The normal map color values [0,1] are converted to a vector [-1,1] before
//	returning.

float3 LookupNormalWithDetail(float2 vDiffuseTexCoords, float2 vDetailTexCoords, float fRenderEffectsFade)
{
	float3 vFinal = ST_TEXTURE_SAMPLE_LINEAR_WRAP(NormalMap, vDiffuseTexCoords).rgb;

	if (ST_EFFECT_DETAIL_NORMAL_LAYER)
	{
		float4 texDetailColor = ST_TEXTURE_SAMPLE_LINEAR_WRAP(DetailDiffuseMap, vDetailTexCoords);
		if (ST_EFFECT_DETAIL_LAYER_FADE)
			texDetailColor.a *= fRenderEffectsFade;

		float3 vDetailNormal = ST_TEXTURE_SAMPLE_LINEAR_WRAP(DetailNormalMap, vDetailTexCoords).rgb;
		vFinal = lerp(vFinal, vDetailNormal, texDetailColor.a);
	}

	return DecodeVectorFromColor(vFinal);
}


///////////////////////////////////////////////////////////////////////  
//	AlphaTestNoise_Billboard
//
//	The SpeedTree SDK generates a small noise texture to facilitate a
//	"fizzle" effect when alpha testing is used over alpha-to-coverage.
//	Alpha fizzle is the term applied to the "fizzling" from one LOD to
//	another based upon changing alpha testing values in unison with
//	returning alpha noise from the pixel shader.
//
//	This function uses the billboard's texcoords to lookup the noise
//	texture to get a smooth fizzle effect.

float AlphaTestNoise_Billboard(float2 vTexCoords)
{
	#define ST_BILLBOARD_ALPHA_NOISE_SCALAR 30.0

    return (ST_ALPHA_TEST_NOISE ? ST_TEXTURE_SAMPLE_LINEAR_WRAP(NoiseMap, vTexCoords * ST_BILLBOARD_ALPHA_NOISE_SCALAR).r : 1.0);
}


///////////////////////////////////////////////////////////////////////  
//	AlphaTestNoise_3dTree
//
//	The SpeedTree SDK generates a small noise texture to facilitate a
//	"fizzle" effect when alpha testing is used over alpha-to-coverage.
//	Alpha fizzle is the term applied to the "fizzling" from one LOD to
//	another based upon changing alpha testing values in unison with
//	returning alpha noise from the pixel shader.
//
//	This function uses the 3d tree's texcoords to lookup the noise
//	texture to get a smooth fizzle effect.

float AlphaTestNoise_3dTree(float2 vTexCoords)
{
	#define ST_TREE_ALPHA_NOISE_SCALAR	0.5

    return (ST_ALPHA_TEST_NOISE ? ST_TEXTURE_SAMPLE_LINEAR_WRAP(NoiseMap, vTexCoords * ST_TREE_ALPHA_NOISE_SCALAR).r : 1.0);
}

#endif // ST_INCLUDE_TEXTURE_UTILITY

///////////////////////////////////////////////////////////////////////
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2016 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#ifndef ST_INCLUDE_USER_INTERPOLANTS
#define ST_INCLUDE_USER_INTERPOLANTS


//	Due to the way shaders are generated from templates in the SpeedTree 6.x and 7.x systems, it's
//	more or less impossible to modify the templates to add extra interpolants (those values
//	passed from the vertex to the pixel shaders). We added this system to make it manageable.
//
//	Users have the option of adding up to four extra float4 interpolants by modifying the
//	source contained in this file. By adding "#define ST_USER_INTERPOLANT0" to this file, an
//	extra float4 will be passed from the vertex shader to the pixel shader. If #define'd, 
//	float4 v2p_vUserInterpolant ("v2p" = vertex-to-pixel) will be created. It must be
//	fully assigned in the vertex shader, else the compilation is likely to fail. Search 
//  for "ST_USER_INTERPOLANT0" in Template_3dGeometry_Vertex.fx or
//  Template_3dGeometry_Vertex_Deferred.fx) to find where this should be done.
//
//	Keep in mind that users may put conditions on when the user interpolants are active.
//	That is, interpolants don't have to be active for every generated shader. If, for example,
//	you're passing a new value to be used with specular lighting, you can check for the
//	presence of specular lighting by using code like:
//	
//	#ifdef SPECULAR // defined by the Compiler when generating
//					// each shader, based on effect LOD dialog
//		#define ST_USER_INTERPOLANT0
//	#endif
//	
//	Then, within the vertex shader, only assign the interpolant when present:
//
//	#ifdef ST_USER_INTERPOLANT0
//		v2p_vUserInterpolant0 = float4(0, 0, 0, 0);
//	#endif
//
//	Be sure to only access the v2p_vUserInterpolantN variables in the pixel shader when they're
//	defined.
//
//	Note that these changes apply to the forward- and deferred-render templates used for the
//	3D tree geometry only. Billboard templates are not affected since they do not use dynamic
//	interpolants. Users are free to modify the less complex billboard templates as needed.


//	Define all or none of the four user interpolants as 0 or 1; use #if (ST_USER_INTERPOLANT*)
//  conditions to restrict use if viable (see top of this file for details)

#define ST_USER_INTERPOLANT0 0
#define ST_USER_INTERPOLANT1 0
#define ST_USER_INTERPOLANT2 0
#define ST_USER_INTERPOLANT3 0

#endif // ST_INCLUDE_USER_INTERPOLANTS


///////////////////////////////////////////////////////////////////////
//	Pixel shader entry point
//
//	Main depth-only pixel shader for 3D geometry

#if (ST_OPENGL)

	// parameters coming from vertex shader to pixel/fragment shader
	// v2p_vInterpolant0 is #defined as gl_Position
	varying float2 v2p_vInterpolant1;

	// user interpolants
	#if (ST_USER_INTERPOLANT0)
		varying float4 v2p_vUserInterpolant0;
	#endif
	#if (ST_USER_INTERPOLANT1)
		varying float4 v2p_vUserInterpolant1;
	#endif
	#if (ST_USER_INTERPOLANT2)
		varying float4 v2p_vUserInterpolant2;
	#endif
	#if (ST_USER_INTERPOLANT3)
		varying float4 v2p_vUserInterpolant3;
	#endif

	// main function definition
	void main(void)

#else

	// main return value
	#if (ST_DIRECTX9) || (ST_XBOX_360)
		float4
	#else
		void
	#endif

	main(// input interpolants
		   float4 v2p_vInterpolant0 : ST_VS_OUT_POS
		 , float2 v2p_vInterpolant1 : ST_VS_OUT_ATTR1

		 // user interpolants
		 #if (ST_USER_INTERPOLANT0)
			, float4 v2p_vUserInterpolant0 : ST_VS_OUT_ATTR2
		 #endif
		 #if (ST_USER_INTERPOLANT1)
			, float4 v2p_vUserInterpolant1 : ST_VS_OUT_ATTR3
		 #endif
		 #if (ST_USER_INTERPOLANT2)
			, float4 v2p_vUserInterpolant2 : ST_VS_OUT_ATTR4
		 #endif
		 #if (ST_USER_INTERPOLANT3)
			, float4 v2p_vUserInterpolant3 : ST_VS_OUT_ATTR5
		 #endif
		)

	#if (ST_DIRECTX9) || (ST_XBOX_360)
		: COLOR
	#endif

#endif
{
	// branch geometry almost always has opaque textures if which case we use an
	// empty/null pixel shader for shadow casting
	if (!ST_ONLY_BRANCHES_PRESENT)
	{
		// all possible variables that may come from the vertex shader; the "v2p" prefix indicates that these
		// variables are values that go from [v]ertex-[2]-[p]ixel shader
		float2 v2p_vDiffuseTexCoords = float2(0.0, 0.0);
		float  v2p_fBillboardTo3dFade = 1.0;

		// set initial values for those v2p parameters that are in use
    v2p_vDiffuseTexCoords = v2p_vInterpolant1.xy;

		// diffuse texture lookup
		float fAlpha = ST_TEXTURE_SAMPLE_LINEAR_WRAP(DiffuseMap, v2p_vDiffuseTexCoords).a;

		// scale alpha by material alpha scalar set in Modeler
		fAlpha *= u_fAlphaScalar;

		// attempt to get earliest possible exit if pixel is transparent
		CheckForEarlyExit(fAlpha, true);
	}

	#if (ST_DIRECTX9 || ST_XBOX_360)
		return float4(0.0, 0.0, 0.0, 1.0);
	#endif
}

