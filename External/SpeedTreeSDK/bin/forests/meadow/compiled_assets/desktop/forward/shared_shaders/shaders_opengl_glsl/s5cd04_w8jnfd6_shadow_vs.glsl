///////////////////////////////////////////////////////////////////////  
//  s5cd04_w8jnfd6_shadow_vs.glsl
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
#define ST_VERTEX_SHADER
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
#define ST_BRANCHES_PRESENT false
#define ST_ONLY_BRANCHES_PRESENT false
#define ST_FRONDS_PRESENT false
#define ST_ONLY_FRONDS_PRESENT false
#define ST_LEAVES_PRESENT true
#define ST_ONLY_LEAVES_PRESENT true
#define ST_FACING_LEAVES_PRESENT false
#define ST_ONLY_FACING_LEAVES_PRESENT false
#define ST_RIGID_MESHES_PRESENT false
#define ST_ONLY_RIGID_MESHES_PRESENT false
#define VERTEX_PROPERTY_POSITION_PRESENT true
#define VERTEX_PROPERTY_DIFFUSETEXCOORDS_PRESENT true
#define VERTEX_PROPERTY_NORMAL_PRESENT true
#define VERTEX_PROPERTY_LODPOSITION_PRESENT false
#define VERTEX_PROPERTY_GEOMETRYTYPEHINT_PRESENT true
#define VERTEX_PROPERTY_LEAFCARDCORNER_PRESENT false
#define VERTEX_PROPERTY_LEAFCARDLODSCALAR_PRESENT false
#define VERTEX_PROPERTY_LEAFCARDSELFSHADOWOFFSET_PRESENT false
#define VERTEX_PROPERTY_WINDBRANCHDATA_PRESENT true
#define VERTEX_PROPERTY_WINDEXTRADATA_PRESENT true
#define VERTEX_PROPERTY_WINDFLAGS_PRESENT true
#define VERTEX_PROPERTY_LEAFANCHORPOINT_PRESENT true
#define VERTEX_PROPERTY_BONEID_PRESENT false
#define VERTEX_PROPERTY_BRANCHSEAMDIFFUSE_PRESENT false
#define VERTEX_PROPERTY_BRANCHSEAMDETAIL_PRESENT false
#define VERTEX_PROPERTY_DETAILTEXCOORDS_PRESENT false
#define VERTEX_PROPERTY_TANGENT_PRESENT false
#define VERTEX_PROPERTY_LIGHTMAPTEXCOORDS_PRESENT false
#define VERTEX_PROPERTY_AMBIENTOCCLUSION_PRESENT true
#define ST_USED_AS_GRASS true
#define ST_MULTIPASS_ACTIVE false

// effect LOD macros
#define ST_EFFECT_FORWARD_LIGHTING_PER_VERTEX         true
#define ST_EFFECT_FORWARD_LIGHTING_PER_PIXEL          false
#define ST_EFFECT_FORWARD_LIGHTING_TRANSITION         false
#define ST_EFFECT_AMBIENT_OCCLUSION                   true
#define ST_EFFECT_AMBIENT_CONTRAST                    true
#define ST_EFFECT_AMBIENT_CONTRAST_FADE               false
#define ST_EFFECT_DETAIL_LAYER                        false
#define ST_EFFECT_DETAIL_LAYER_FADE                   false
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
#define ST_EFFECT_BACKFACE_CULLING                    false
#define ST_EFFECT_AMBIENT_IMAGE_LIGHTING              true
#define ST_EFFECT_AMBIENT_IMAGE_LIGHTING_FADE         false
#define ST_EFFECT_HUE_VARIATION                       false
#define ST_EFFECT_HUE_VARIATION_FADE                  false
#define ST_EFFECT_SHADOW_SMOOTHING                    false
#define ST_EFFECT_DIFFUSE_MAP_OPAQUE                  false

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
#define ST_WIND_EFFECT_GLOBAL_WIND false
#define ST_WIND_EFFECT_GLOBAL_PRESERVE_SHAPE true
#define ST_WIND_EFFECT_BRANCH_SIMPLE_1 false
#define ST_WIND_EFFECT_BRANCH_DIRECTIONAL_1 true
#define ST_WIND_EFFECT_BRANCH_DIRECTIONAL_FROND_1 false
#define ST_WIND_EFFECT_BRANCH_TURBULENCE_1 false
#define ST_WIND_EFFECT_BRANCH_WHIP_1 false
#define ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_1 true
#define ST_WIND_EFFECT_BRANCH_SIMPLE_2 false
#define ST_WIND_EFFECT_BRANCH_DIRECTIONAL_2 false
#define ST_WIND_EFFECT_BRANCH_DIRECTIONAL_FROND_2 false
#define ST_WIND_EFFECT_BRANCH_TURBULENCE_2 false
#define ST_WIND_EFFECT_BRANCH_WHIP_2 false
#define ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_2 true
#define ST_WIND_EFFECT_LEAF_RIPPLE_VERTEX_NORMAL_1 false
#define ST_WIND_EFFECT_LEAF_RIPPLE_COMPUTED_1 true
#define ST_WIND_EFFECT_LEAF_TUMBLE_1 true
#define ST_WIND_EFFECT_LEAF_TWITCH_1 true
#define ST_WIND_EFFECT_LEAF_OCCLUSION_1 true
#define ST_WIND_EFFECT_LEAF_RIPPLE_VERTEX_NORMAL_2 false
#define ST_WIND_EFFECT_LEAF_RIPPLE_COMPUTED_2 true
#define ST_WIND_EFFECT_LEAF_TUMBLE_2 true
#define ST_WIND_EFFECT_LEAF_TWITCH_2 true
#define ST_WIND_EFFECT_LEAF_OCCLUSION_2 true
#define ST_WIND_EFFECT_FROND_RIPPLE_ONE_SIDED false
#define ST_WIND_EFFECT_FROND_RIPPLE_TWO_SIDED true
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

#ifndef ST_INCLUDE_ROLLING_WIND_NOISE
#define ST_INCLUDE_ROLLING_WIND_NOISE

#ifndef ST_INCLUDE_SAMPLERS_AND_TEXTURE_MACROS
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

#endif
ST_TEXTURE_DECLARE(PerlinNoiseKernel, 7);


///////////////////////////////////////////////////////////////////////
//  WindNoiseTurbulence

float WindNoiseTurbulence(float2 vPos, float fStartingZoom)
{
	float fReturn = 0.0;

	if (ST_DIRECTX9 == 1 || ST_PS3 == 1)
	{
		float2 vWave = cos(vPos);
		fReturn = vWave.x + vWave.y;
	}
	else
	{
		float fZoom = fStartingZoom;
		float fValue = 0.0;

		while (fZoom >= 1.0)
		{
			float4 vTexSample = ST_TEXTURE_SAMPLE_LINEAR_WRAP_LOD_0(PerlinNoiseKernel, frac(vPos / fZoom));

			// option A) use a single texture sample for speed (disabled), or...
			{
				//fValue += vTexSample.r * fZoom;
			}

			// option B) use a composite of the rgba lookups for a smoother noise pattern
			{
				fValue += dot(vTexSample, float4(0.25, 0.25, 0.25, 0.25)) * fZoom;
			}

			fZoom /= 2.0;
		}
		
		fReturn = (0.5 * fValue / fStartingZoom);
	}

	return fReturn;
}


///////////////////////////////////////////////////////////////////////
//  WindNoiseMarble

float WindNoiseMarble(float2 vPos, float fTwist, float fPeriod, float fTurbulence)
{
	// c_xPeriod and c_yPeriod together define the angle of the lines
	// c_xPeriod and c_yPeriod both 0 ==> it becomes a normal clouds or turbulence pattern

	const float c_fPeriodX = 5.0 * fPeriod;	// defines repetition of marble lines in x direction
	const float c_fPeriodY = 5.0 * fPeriod;	// defines repetition of marble lines in y direction

	float xyValue = WindNoiseTurbulence(vPos, fTurbulence);

	return abs(sin(ST_PI * vPos.x * c_fPeriodX + vPos.y * c_fPeriodY + fTwist * xyValue));
}


#define ST_MIN_ROLLING_WIND_STRENGTH 0.2

///////////////////////////////////////////////////////////////////////
//  WindFieldStrengthAt

float WindFieldStrengthAt(float2 vPos)
{
	float fStrength = WindNoiseMarble(u_sRolling.m_fNoiseSize * (vPos - float2(u_sRolling.m_fOffsetX, u_sRolling.m_fOffsetY)),
									  u_sRolling.m_fNoiseTwist,
									  u_sRolling.m_fNoisePeriod,
									  u_sRolling.m_fNoiseTurbulence);
	
	// shape the noise function for our needs
	fStrength *= fStrength;
	fStrength *= fStrength;

	return max(fStrength, ST_MIN_ROLLING_WIND_STRENGTH);
}

#endif // ST_INCLUDE_ROLLING_WIND_NOISE

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

#define ST_WIND_LEAF_TRIG_OFFSET_SCALAR float3(10.0, 10.0, 10.0)

#if (ST_WIND_IS_ACTIVE)

#ifndef ST_INCLUDE_WIND
#define ST_INCLUDE_WIND

#ifndef ST_WIND_LOD_ROLLING_FADE
	#define ST_WIND_LOD_ROLLING_FADE false
#endif


///////////////////////////////////////////////////////////////////////
//  Structure SWindBranchVertex

struct SWindBranchVertex
{
	float				m_fWeight;
	float3				m_vWindNormal;
	float				m_vWindNormalPacked;
};


///////////////////////////////////////////////////////////////////////
//  Structure SWindVertex

struct SWindVertex
{
	// data provided per-vertex
	float3				m_vPos;
	float3				m_vNormal;
	float3				m_vTangent;
	float2				m_vDiffuseTexCoords;
	float3				m_vLeafAnchorPoint;
	float				m_fGeometryTypeHint;
	float				m_fWindFlag;

	// branch data
	SWindBranchVertex	m_sBranch1;
	SWindBranchVertex	m_sBranch2;

	// in_vWindExtraData

	// if fronds
	float3				m_vFrondRippleDir;
	float				m_fFrondRippleScalar;
	float				m_fFrondLengthPercent;

	// if leaf geometry
	float				m_fLeafWindScalar;
	float3				m_vLeafGrowthDir;
	float3				m_vLeafRippleDir;
};


///////////////////////////////////////////////////////////////////////
//  Structure SWindTreeInstance

struct SWindTreeInstance
{
	float3				m_vPos;
	float				m_fScalar;
	float				m_fLeafTrigOffset;	// unique per leaf to keep wind out of sync
	float				m_fLodTransition;
};


///////////////////////////////////////////////////////////////////////
//  Structure SWindBranchOptions

struct SWindBranchOptions
{
	bool				m_bSimple;
	bool				m_bDirectional;
	bool				m_bDirectionalFrond;
	bool				m_bTurbulence;
	bool				m_bWhip;
	bool				m_bOscillatingComplex;
};


///////////////////////////////////////////////////////////////////////
//  Structure SWindLeafOptions

struct SWindLeafOptions
{
	bool				m_bRippleVertexNormal;
	bool				m_bRippleComputed;
	bool				m_bTumble;
	bool				m_bTwitch;
	bool				m_bOcclusion;
};


///////////////////////////////////////////////////////////////////////
//  Structure SWindOptions

struct SWindOptions
{
	bool				m_bGlobalWind;
	bool				m_bGlobalPreserveShape;

	SWindBranchOptions	m_sBranch1;
	SWindBranchOptions	m_sBranch2;
	bool				m_bBranchWindActive;
	SWindLeafOptions	m_sLeaf1;
	SWindLeafOptions	m_sLeaf2;

	bool				m_bFrondRippleOneSided;
	bool				m_bFrondRippleTwoSided;
	bool				m_bFrondRippleAdjustLighting;

	bool				m_bRollingOnThisLod;	// normally just the highest LOD has this active
	bool				m_bModelUsesRolling;	// lower LODs with no rolling wind need this so they can match the faded-out
												// rolling effect of the higher LOD

	bool				m_bCoordSysZUp;
	bool				m_bCoordSysLeftHanded;

	// LOD
	bool				m_bLodGlobal;
	bool				m_bLodBranch;
	bool				m_bLodFull;
	bool				m_bLodNone_x_Global;
	bool				m_bLodNone_x_Branch;
	bool				m_bLodNone_x_Full;
	bool				m_bLodGlobal_x_Branch;
	bool				m_bLodGlobal_x_Full;
	bool				m_bLodBranch_x_Full;
	bool				m_bLodNone;
	bool				m_bLodRollingFade;
	bool				m_bLodBillboardGlobalWind;

	// geometry
	bool				m_bBranchesPresent;
	bool				m_bOnlyBranchesPresent;
	bool				m_bFrondsPresent;
	bool				m_bOnlyFrondsPresent;
	bool				m_bLeavesPresent;
	bool				m_bOnlyLeavesPresent;
	bool				m_bFacingLeavesPresent;
	bool				m_bOnlyFacingLeavesPresent;
	bool				m_bRigidMeshesPresent;
	bool				m_bOnlyRigidMeshesPresent;

	// vertex properties
	bool				m_bVertexPropertyWindExtraDataPresent;
};


///////////////////////////////////////////////////////////////////////
//  Structure SWindConstants

struct SWindConstants
{
	float				m_fWallTime;					// time, in seconds, from the beginning on the app
	float3				m_vDirection;					// direction of the wind
	float				m_fStrength;                    // wind strength (0.0 = none, 1.0 = max)
	float3				m_vPalmStyleBranchAnchor;		// position of the wind anchor used by 'palm style' branch motion
	SWindGlobal			m_sGlobal;						// global wind parameters
	bool				m_bPreserveGlobalWindShape;		// flag for global wind
	SWindBranchLevel	m_sBranch1;						// branch wind level 1 parameters
	SWindBranchLevel	m_sBranch2;						// branch wind level 2 parameters
	SWindLeaf			m_sLeaf1;                       // leaf wind group 1 parameters
	SWindLeaf			m_sLeaf2;                       // leaf wind group 2 parameters
	SWindFrond			m_sFrondRipple;                 // frond wind parameters
	SWindRolling		m_sRolling;                     // rolling wind parameters
};


///////////////////////////////////////////////////////////////////////
//  Structure SWindInput

struct SWindInput
{
	SWindVertex			m_sVertex;						// data comes from the vertex buffer; per-vertex wind data
	SWindTreeInstance	m_sInstance;					// comes from the per-instance stream (e.g. tree pos, orientation, LOD state)
	SWindConstants		m_sConstants;					// comes from the shader constant buffer; changes per base tree being rendered
	SWindOptions		m_sOptions;						// wind options set either during compilation or at run-time per base tree

	// derived values
	float				m_fUniqueVertexValue; // unique per vertex, used as trig offset
};


///////////////////////////////////////////////////////////////////////
//  Structure SWindOutput

struct SWindOutput
{
	float3				m_vPos;
	float3				m_vNormal;
	float3				m_vTangent;
};


///////////////////////////////////////////////////////////////////////
//  WindTwitch

float WindTwitch(SWindInput sIn, SWindLeaf sLeafType, float fOffset)
{
	const float c_fTwitchFudge = 0.87;
	const float fTime = sLeafType.m_fTwitchTime + fOffset;

	float4 vOscillations = SineWaves4(float4(fTime + sIn.m_sInstance.m_fLeafTrigOffset * sIn.m_sInstance.m_fLeafTrigOffset,
											 c_fTwitchFudge * fTime + sIn.m_sInstance.m_fLeafTrigOffset,
											 0.0, 0.0));

	float fTwitch = vOscillations.x * vOscillations.y * vOscillations.y;
	fTwitch = (fTwitch + 1.0) * 0.5;

	return sLeafType.m_fTwitchThrow * pow(saturate(fTwitch), sLeafType.m_fTwitchSharpness);
}


///////////////////////////////////////////////////////////////////////
//  WindOscillateBranch
//
//  This function computes an oscillation value and whip value if necessary.
//  Whip and oscillation are combined like this to minimize calls to
//  SineWaves4( ) when possible.

float WindOscillateBranch(SWindBranchLevel	sBranchLevel,
						  SWindBranchVertex	sBranchVertex,
						  float				fWindStrength,
						  float				fTime,
						  bool				bWhip,
						  bool				bComplex,
						  inout float4		vOscillations)
{
	float fOscillation;
	if (bComplex)
	{
		vOscillations = SineWaves4(float4(fTime,
										  fTime * sBranchLevel.m_fTwitchFreqScale,
										  sBranchLevel.m_fTwitchFreqScale * 0.5 * fTime,
										  fTime + (1.0 - sBranchVertex.m_fWeight))); // w is used only by whip

		float fFineDetail = vOscillations.x;
		float fBroadDetail = vOscillations.y * vOscillations.z;

		float fTarget = 1.0;
		float fAmount = fBroadDetail;
		if (fBroadDetail < 0.0)
		{
			fTarget = -fTarget;
			fAmount = -fAmount;
		}

		fBroadDetail = lerp(fBroadDetail, fTarget, fAmount);
		fBroadDetail = lerp(fBroadDetail, fTarget, fAmount);

		fOscillation = fBroadDetail * sBranchLevel.m_fTwitch * (1.0 - fWindStrength) + fFineDetail * (1.0 - sBranchLevel.m_fTwitch);

		if (bWhip)
			fOscillation *= 1.0 + (vOscillations.w * sBranchLevel.m_fWhip) * sBranchVertex.m_fWeight;
	}
	else
	{
		vOscillations = SineWaves4(float4(fTime,
										  fTime * 0.689,
										  0.0,
										  fTime + (1.0 - sBranchVertex.m_fWeight))); // w is used only by whip

		fOscillation = vOscillations.x + vOscillations.y * vOscillations.x;

		if (bWhip)
			fOscillation *= 1.0 + (vOscillations.w * sBranchLevel.m_fWhip) * sBranchVertex.m_fWeight;
	}

	return fOscillation;
}


///////////////////////////////////////////////////////////////////////
//  WindTurbulence

float WindTurbulence(float fTime, float fOffset, float fGlobalTime, float fTurbulence)
{
	const float c_fTurbulenceFactor = 0.1;

	float4 vOscillations = SineWaves4(float4(fTime * c_fTurbulenceFactor + fOffset,
											 fGlobalTime * fTurbulence * c_fTurbulenceFactor + fOffset,
											 0.0, 0.0));

	return 1.0 - (vOscillations.x * vOscillations.y * vOscillations.x * vOscillations.y * fTurbulence);
}


///////////////////////////////////////////////////////////////////////
//  WindGlobalBehavior

//	There are a few things we can do to reduce the shader instructions
//	generated by this shader.
//
//  Set all to true for maximum opt, lowest quality
#define ST_WIND_GLOBAL_OPT_HEIGHT_EXPONENT_ALWAYS_ONE	false
#define ST_WIND_GLOBAL_OPT_SKIP_DIRECTION_ADHERENCE		false
#define ST_WIND_GLOBAL_OPT_NO_ROLLING					false

#define ST_WIND_GLOBAL_BEND_POINT						0.25 // about a quarter of the way up
#define ST_WIND_GLOBAL_FREQUENCY_ADJUST					0.8

float3 WindGlobalBehavior(SWindInput sIn, float fFieldStrength)
{
    // to preserve the length of the tree after it's been bent over, we need the distance of
	// this point to the origin of the tree
	float fOriginalDistanceFromTreeBase = 1.0;
	if (sIn.m_sConstants.m_bPreserveGlobalWindShape)
		fOriginalDistanceFromTreeBase = length(sIn.m_sVertex.m_vPos.xyz);

	// compute how much the height contributes; the higher the point, the more the bend
	float fPosHeight = sIn.m_sOptions.m_bCoordSysZUp ? sIn.m_sVertex.m_vPos.z : sIn.m_sVertex.m_vPos.y;
	float fBendWeight = max(fPosHeight - sIn.m_sConstants.m_sGlobal.m_fHeight * ST_WIND_GLOBAL_BEND_POINT, 0.0) / sIn.m_sConstants.m_sGlobal.m_fHeight;

	// use exponent for curve for a more natural look; trees don't normally become more flexible on the way up linearly
	if (!ST_WIND_GLOBAL_OPT_HEIGHT_EXPONENT_ALWAYS_ONE)
		fBendWeight = pow(saturate(fBendWeight), sIn.m_sConstants.m_sGlobal.m_fHeightExponent);

	// create two separate sine waves (out of phase, different frequencies)
    float2 vSineWaves = SineWaves2(float2(sIn.m_sInstance.m_vPos.x + sIn.m_sConstants.m_sGlobal.m_fTime,
                                          sIn.m_sInstance.m_vPos.y + sIn.m_sConstants.m_sGlobal.m_fTime * ST_WIND_GLOBAL_FREQUENCY_ADJUST));

	// build a wave from two sine waves that's not repetitive/predictable
	float fComplexOscillation = vSineWaves.x + vSineWaves.y * vSineWaves.y;

	// sIn.m_sConstants.m_sGlobal.m_fDistance holds the Modeler-tuned parameter governing the max distance the tree will bend
    float fRollingScalar = (!ST_WIND_GLOBAL_OPT_NO_ROLLING && sIn.m_sOptions.m_bRollingOnThisLod) ? fFieldStrength : 0.0;
    float fMotionMagnitude = sIn.m_sConstants.m_sGlobal.m_fDistance * fComplexOscillation;

	// sIn.m_sConstants.m_sGlobal.m_fAdherence holds the Modeler-tuned parameter governing the minimum distance
	// the tree will bend in the wind direction
    if (!ST_WIND_GLOBAL_OPT_SKIP_DIRECTION_ADHERENCE)
        fMotionMagnitude += sIn.m_sConstants.m_sGlobal.m_fAdherence * sIn.m_sConstants.m_sGlobal.m_fHeight * fRollingScalar;

	// magnitude is adjusted by the bend weight
    fMotionMagnitude *= fBendWeight;

	// compute the wind-blown position by using the magnitude in conjunction with wind direction
	float3 vWindBlownPos = sIn.m_sVertex.m_vPos;
    if (sIn.m_sOptions.m_bCoordSysZUp)
        vWindBlownPos.xy += sIn.m_sConstants.m_vDirection.xy * fMotionMagnitude;
    else
        vWindBlownPos.xz += sIn.m_sConstants.m_vDirection.xz * fMotionMagnitude;

	// left alone, the tree will look very skewed after the above operations; we use the original length to
    // bring it back to its original size
    if (sIn.m_sConstants.m_bPreserveGlobalWindShape)
        vWindBlownPos.xyz = normalize(vWindBlownPos.xyz) * fOriginalDistanceFromTreeBase;

	// return net effect of global function, not new position
    return (vWindBlownPos - sIn.m_sVertex.m_vPos);
}


///////////////////////////////////////////////////////////////////////
//  WindLeafRipple

float3 WindLeafRipple(SWindInput sIn, SWindLeaf sLeafType, float fScalar, bool bDirectional)
{
	// compute how much to move
	float2 vInput = float2(sLeafType.m_fRippleTime + sIn.m_sInstance.m_fLeafTrigOffset, 0.0);
	float fMoveAmount = sLeafType.m_fRippleDistance * SineWaves2(vInput).x;

	return (bDirectional ? sIn.m_sVertex.m_vNormal : sIn.m_sVertex.m_vLeafRippleDir) * fMoveAmount * fScalar;
}


///////////////////////////////////////////////////////////////////////
//  WindLeafTumble
//
//	Will modify the vertex's normal and tangent.

float3 WindLeafTumble(SWindInput		sIn,
					  out float3		vOutNormal,
					  out float3		vOutTangent,
					  SWindLeaf			sLeafType,
					  SWindLeafOptions	sLeafOptions,
					  float				fScalar,
					  float3			vAnchor)
{
	// compute all oscillations up front
	float4 vOscillations = SineWaves4(float4(sLeafType.m_fTumbleTime + sIn.m_sInstance.m_fLeafTrigOffset,
											 sLeafType.m_fTumbleTime * 0.75 - sIn.m_sInstance.m_fLeafTrigOffset,
											 sLeafType.m_fTumbleTime * 0.01 + sIn.m_sInstance.m_fLeafTrigOffset,
											 sLeafType.m_fTumbleTime + sIn.m_sInstance.m_fLeafTrigOffset));

	float3 vOriginPos = sIn.m_sVertex.m_vPos - vAnchor;
	float fLength = length(vOriginPos);

	// twist
	float fOsc = vOscillations.x + vOscillations.y * vOscillations.y;

	float3x3 matTumble = ArbitraryAxisRotationMatrix(sIn.m_sVertex.m_vLeafGrowthDir, fScalar * sLeafType.m_fTumbleTwist * fOsc);

	// with wind
	float3 vAxis = wind_cross(sIn.m_sVertex.m_vLeafGrowthDir, sIn.m_sConstants.m_vDirection);
	float fDot = clamp(dot(sIn.m_sConstants.m_vDirection, sIn.m_sVertex.m_vLeafGrowthDir), -1.0, 1.0);
	if (sIn.m_sOptions.m_bCoordSysZUp)
		vAxis.z += fDot;
	else
		vAxis.y += fDot;
	vAxis = normalize(vAxis);

    float fAngle = ApproxAcos(fDot);

	fOsc = vOscillations.y - vOscillations.x * vOscillations.x;

	float fTwitch = 0.0;
	if (sLeafOptions.m_bTwitch)
		fTwitch = WindTwitch(sIn, sLeafType, sIn.m_sInstance.m_fLeafTrigOffset);

	float3x3 matTmp = ArbitraryAxisRotationMatrix(vAxis, fScalar * (fAngle * sLeafType.m_fTumbleDirectionAdherence + fOsc * sLeafType.m_fTumbleFlip + fTwitch));
	matTumble = mul_float3x3_float3x3(matTumble, matTmp);

	vOutNormal = lerp(sIn.m_sVertex.m_vNormal, mul_float3x3_float3(matTumble, sIn.m_sVertex.m_vNormal), sIn.m_sOptions.m_bLodRollingFade ? sIn.m_sInstance.m_fLodTransition : 1.0);
	vOutTangent = lerp(sIn.m_sVertex.m_vTangent, mul_float3x3_float3(matTumble, sIn.m_sVertex.m_vTangent), sIn.m_sOptions.m_bLodRollingFade ? sIn.m_sInstance.m_fLodTransition : 1.0);

	vOriginPos = mul_float3x3_float3(matTumble, vOriginPos);

	return normalize(vOriginPos) * fLength + vAnchor - sIn.m_sVertex.m_vPos;
}


///////////////////////////////////////////////////////////////////////
//  WindLeaf
//
//	Returns net effect of leaf wind on position.

float3 WindLeaf(SWindInput  		sIn,
				out float3			vOutNormal,
				out float3			vOutTangent,
				SWindLeaf			sLeafType,
				SWindLeafOptions	sLeafOptions,
				float3				vAnchor)
{
	if (sLeafOptions.m_bOcclusion)
	{
		float2 vDir = -normalize(sIn.m_sOptions.m_bCoordSysZUp ? vAnchor.xy : vAnchor.xz);

		float fDot = dot(vDir.xy, sIn.m_sConstants.m_vDirection.xy);
		float fDirContribution = (fDot + 1.0) * 0.5;
		fDirContribution = lerp(sLeafType.m_fTumbleTwist, 1.0, fDirContribution);
		sIn.m_sVertex.m_fLeafWindScalar *= fDirContribution;
	}

	float fRippleScalar = 1.0;
	float fTumbleScalar = 1.0;
	if (sIn.m_sOptions.m_bRollingOnThisLod)
	{
		float fNoise = WindFieldStrengthAt(ConvertToStdCoordSys(vAnchor + sIn.m_sInstance.m_vPos).xy);

		fRippleScalar = lerp(1.0, lerp(sIn.m_sConstants.m_sRolling.m_fLeafRippleMin, 1.0, fNoise), sIn.m_sOptions.m_bLodRollingFade ? sIn.m_sInstance.m_fLodTransition : 1.0);
		fTumbleScalar = lerp(1.0, lerp(sIn.m_sConstants.m_sRolling.m_fLeafTumbleMin, 1.0, fNoise), sIn.m_sOptions.m_bLodRollingFade ? sIn.m_sInstance.m_fLodTransition : 1.0);
	}

	float3 vRippleComponent = float3(0.0, 0.0, 0.0);
	if (sLeafOptions.m_bRippleVertexNormal || sLeafOptions.m_bRippleComputed)
	{
		vRippleComponent = WindLeafRipple(sIn,
										  sLeafType,
										  sIn.m_sVertex.m_fLeafWindScalar * fRippleScalar,
										  sLeafOptions.m_bRippleVertexNormal);
	}

	float3 vTumbleComponent = float3(0.0, 0.0, 0.0);
	if (sLeafOptions.m_bTumble)
	{
		vTumbleComponent = WindLeafTumble(sIn,
										  vOutNormal,
										  vOutTangent,
										  sLeafType,
										  sLeafOptions,
										  sIn.m_sVertex.m_fLeafWindScalar * fTumbleScalar,
										  vAnchor);
	}
	else
	{
		vOutNormal = sIn.m_sVertex.m_vNormal;
		vOutTangent = sIn.m_sVertex.m_vTangent;
	}

	return vRippleComponent + vTumbleComponent;
}


///////////////////////////////////////////////////////////////////////
//  WindFrondRippleOneSided
//
//	Returns net effect of one-sided frond ripple.

float3 WindFrondRippleOneSided(SWindInput sIn)
{
	float fOffset = (sIn.m_sVertex.m_vDiffuseTexCoords.x < 0.5 ? 0.75 : 0.0);

	float4 vOscillations = SineWaves4(float4(sIn.m_sConstants.m_sFrondRipple.m_fFrondTime * sIn.m_sVertex.m_vDiffuseTexCoords.y * sIn.m_sConstants.m_sFrondRipple.m_fTile + fOffset, 0.0, 0.0, 0.0));
	float fAmount = sIn.m_sVertex.m_fFrondRippleScalar * vOscillations.x * sIn.m_sConstants.m_sFrondRipple.m_fFrondDistance;

	return fAmount * sIn.m_sVertex.m_vNormal;
}


///////////////////////////////////////////////////////////////////////
//  WindFrondRippleTwoSided
//
//	Returns net effect of two-sided frond ripple.

float3 WindFrondRippleTwoSided(SWindInput sIn)
{
	float4 vOscillations = SineWaves4(float4(sIn.m_sConstants.m_sFrondRipple.m_fFrondTime * sIn.m_sVertex.m_fFrondLengthPercent * sIn.m_sConstants.m_sFrondRipple.m_fTile, 0.0, 0.0, 0.0));
	float fAmount = sIn.m_sVertex.m_fFrondRippleScalar * vOscillations.x * sIn.m_sConstants.m_sFrondRipple.m_fFrondDistance;

	return fAmount * sIn.m_sVertex.m_vFrondRippleDir;
}



///////////////////////////////////////////////////////////////////////
//  WindSimpleBranch
//
//	Returns net effect, not including original position.

float3 WindSimpleBranch(SWindInput  		sIn,
						SWindBranchLevel	sBranchLevel,
						SWindBranchOptions	sBranchOptions,
						SWindBranchVertex	sBranchVertex)
{
	float3 vWindNormal = sBranchVertex.m_vWindNormal * sBranchVertex.m_fWeight;

	// oscillate
	float4 vOscillations;
	float fOsc = WindOscillateBranch(sBranchLevel,
									 sBranchVertex,
									 sIn.m_sConstants.m_fStrength,
									 sBranchLevel.m_fBranchTime + sIn.m_fUniqueVertexValue,
									 sBranchOptions.m_bWhip,
									 sBranchOptions.m_bOscillatingComplex,
									 vOscillations);

	return vWindNormal * fOsc * sBranchLevel.m_fBranchDistance;
}


///////////////////////////////////////////////////////////////////////
//  WindDirectionalBranch

float3 WindDirectionalBranch(SWindInput 		sIn,
							 SWindBranchLevel	sBranchLevel,
							 SWindBranchOptions	sBranchOptions,
							 SWindBranchVertex	sBranchVertex,
							 float				fFieldStrength)
{
	float3 vLocalWindVector = sBranchVertex.m_vWindNormal * sBranchVertex.m_fWeight;
	float fTime = sBranchLevel.m_fBranchTime + sIn.m_sInstance.m_vPos.x + sIn.m_sInstance.m_vPos.y;

	// oscillate
	float4 vOscillations;
	float fOsc = WindOscillateBranch(sBranchLevel,
									 sBranchVertex,
									 sIn.m_sConstants.m_fStrength,
									 fTime,
									 sBranchOptions.m_bWhip,
									 sBranchOptions.m_bOscillatingComplex,
									 vOscillations);

	float3 vPos = vLocalWindVector * fOsc * sBranchLevel.m_fBranchDistance;

	// add in the direction, accounting for turbulence
	float fAdherenceScale = 1.0;
	if (sBranchOptions.m_bTurbulence)
	{
		float fUniqueVertexValue = sBranchVertex.m_vWindNormalPacked;
		fAdherenceScale = WindTurbulence(fTime, fUniqueVertexValue, sIn.m_sConstants.m_fWallTime, sBranchLevel.m_fTurbulence);
	}

	if (sBranchOptions.m_bWhip)
		fAdherenceScale += vOscillations.w * sIn.m_sConstants.m_fStrength * sBranchLevel.m_fWhip;

	float3 vAdjustedWindDir = sIn.m_sConstants.m_vDirection;
	if (sIn.m_sOptions.m_bRollingOnThisLod)
	{
		if (sIn.m_sOptions.m_bCoordSysZUp)
			vAdjustedWindDir.z += sIn.m_sConstants.m_sRolling.m_fBranchVerticalOffset * fFieldStrength;
		else
			vAdjustedWindDir.y += sIn.m_sConstants.m_sRolling.m_fBranchVerticalOffset * fFieldStrength;
	}

	return vPos + vAdjustedWindDir * sBranchLevel.m_fDirectionAdherence * fAdherenceScale * sBranchVertex.m_fWeight;
}


///////////////////////////////////////////////////////////////////////
//  WindDirectionalBranch

float3 WindDirectionalBranchFrondStyle(SWindInput			sIn,
									   SWindBranchLevel		sBranchLevel,
									   SWindBranchOptions	sBranchOptions,
									   SWindBranchVertex	sBranchVertex,
									   float				fFieldStrength)
{
	float3 vLocalWindVector = sBranchVertex.m_vWindNormal * sBranchVertex.m_fWeight;
	float fTime = sBranchLevel.m_fBranchTime + sIn.m_fUniqueVertexValue;

	// oscillate
	float4 vOscillations;
	float fOsc = WindOscillateBranch(sBranchLevel,
									 sBranchVertex,
									 sIn.m_sConstants.m_fStrength,
									 fTime,
									 sBranchOptions.m_bWhip,
									 sBranchOptions.m_bOscillatingComplex,
									 vOscillations);

	float3 vPos = sIn.m_sVertex.m_vPos + vLocalWindVector * fOsc * sBranchLevel.m_fBranchDistance;

	// add in the direction, accounting for turbulence
	float fAdherenceScale = 1.0;
	if (sBranchOptions.m_bTurbulence)
	{
		float fUniqueVertexValue = sBranchVertex.m_vWindNormalPacked;
		fAdherenceScale = WindTurbulence(fTime, fUniqueVertexValue, sIn.m_sConstants.m_fWallTime, sBranchLevel.m_fTurbulence);
	}

	if (sBranchOptions.m_bWhip)
		fAdherenceScale += vOscillations.w * sIn.m_sConstants.m_fStrength * sBranchLevel.m_fWhip;

	float3 vWindAdherenceVector = sIn.m_sConstants.m_vPalmStyleBranchAnchor - vPos.xyz;

	return (vPos + vWindAdherenceVector * sBranchLevel.m_fDirectionAdherence * fAdherenceScale * sBranchVertex.m_fWeight * (sIn.m_sOptions.m_bRollingOnThisLod ? fFieldStrength : 1.0)) - sIn.m_sVertex.m_vPos;
}


///////////////////////////////////////////////////////////////////////
//  WindBranchBehavior
//
//	Returns net effect of branch behavior, not updated position.

float3 WindBranchBehavior(SWindInput sIn, float fFieldStrength, float fEffectFade)
{
	// level 1
	float3 vBranchDelta = float3(0.0, 0.0, 0.0);
	if (sIn.m_sOptions.m_sBranch1.m_bSimple)
	{
		vBranchDelta = WindSimpleBranch(sIn, sIn.m_sConstants.m_sBranch1, sIn.m_sOptions.m_sBranch1, sIn.m_sVertex.m_sBranch1);
	}
	else if (sIn.m_sOptions.m_sBranch1.m_bDirectional)
	{
		vBranchDelta = WindDirectionalBranch(sIn, sIn.m_sConstants.m_sBranch1, sIn.m_sOptions.m_sBranch1, sIn.m_sVertex.m_sBranch1, fFieldStrength);
	}
	else if (sIn.m_sOptions.m_sBranch1.m_bDirectionalFrond)
	{
		vBranchDelta = WindDirectionalBranchFrondStyle(sIn, sIn.m_sConstants.m_sBranch1, sIn.m_sOptions.m_sBranch1, sIn.m_sVertex.m_sBranch1, fFieldStrength);
	}

	// copy output position to input position so that level 2 effect builds on level 1 output
	sIn.m_sVertex.m_vPos += vBranchDelta;

	// level 2
	if (sIn.m_sOptions.m_sBranch2.m_bSimple)
	{
		vBranchDelta += WindSimpleBranch(sIn, sIn.m_sConstants.m_sBranch2, sIn.m_sOptions.m_sBranch2, sIn.m_sVertex.m_sBranch2);
	}
	else if (sIn.m_sOptions.m_sBranch2.m_bDirectional)
	{
		vBranchDelta += WindDirectionalBranch(sIn, sIn.m_sConstants.m_sBranch2, sIn.m_sOptions.m_sBranch2, sIn.m_sVertex.m_sBranch2, fFieldStrength);
	}
	else if (sIn.m_sOptions.m_sBranch2.m_bDirectionalFrond)
	{
		vBranchDelta += WindDirectionalBranchFrondStyle(sIn, sIn.m_sConstants.m_sBranch2, sIn.m_sOptions.m_sBranch2, sIn.m_sVertex.m_sBranch2, fFieldStrength);
	}

	return vBranchDelta;
}


///////////////////////////////////////////////////////////////////////
//  SpeedTreeWindModel
//
//	This is the main wind function that handles all wind effects, including
//	global, branch, frond, and leave wind. It also handles rolling wind effects
//	and gradual or popping LOD scaling.

void SpeedTreeWindModel(SWindInput sIn, out SWindOutput sOut)
{
	// init output
	sOut.m_vPos = sIn.m_sVertex.m_vPos;
	sOut.m_vNormal = sIn.m_sVertex.m_vNormal;
	sOut.m_vTangent = sIn.m_sVertex.m_vTangent;

	// need this position later for smooth LOD
	float3 vOriginalVertexPosition = sIn.m_sVertex.m_vPos;

	// rolling wind, defaults to 0.0 (rolling wind effect is off)
	float fRollingWindStrength = sIn.m_sOptions.m_bModelUsesRolling ? ST_MIN_ROLLING_WIND_STRENGTH : 0.0;

	// if active look up rolling wind strength from noise field
	if (sIn.m_sOptions.m_bRollingOnThisLod)
	{
		float fNoiseField = WindFieldStrengthAt(sIn.m_sConstants.m_sRolling.m_fBranchRipple * sIn.m_sVertex.m_vPos.xy * sIn.m_sInstance.m_fScalar + sIn.m_sInstance.m_vPos.xy);
		fRollingWindStrength = lerp(sIn.m_sConstants.m_sRolling.m_fBranchFieldMin, 1.0, fNoiseField);

		// reduce noise if fade effect is active
		if (sIn.m_sOptions.m_bLodRollingFade)
		{
			fRollingWindStrength *= sIn.m_sInstance.m_fLodTransition;
		}
	}

	// global component
	float3 vGlobalWindEffect = float3(0.0, 0.0, 0.0);
	if (sIn.m_sOptions.m_bGlobalWind)
		vGlobalWindEffect = WindGlobalBehavior(sIn, fRollingWindStrength);

	// branch component
	float3 vBranchWindEffect = float3(0.0, 0.0, 0.0);
	if (sIn.m_sOptions.m_bBranchWindActive)
		vBranchWindEffect = WindBranchBehavior(sIn, fRollingWindStrength, sIn.m_sInstance.m_fLodTransition);

	// frond component
	float3 vFrondWindEffect = float3(0.0, 0.0, 0.0);
	if (sIn.m_sOptions.m_bLodFull && sIn.m_sOptions.m_bFrondsPresent && sIn.m_sOptions.m_bVertexPropertyWindExtraDataPresent)
	{
		if (sIn.m_sOptions.m_bOnlyFrondsPresent || sIn.m_sVertex.m_fGeometryTypeHint == ST_GEOMETRY_TYPE_HINT_FRONDS)
		{
			if (sIn.m_sOptions.m_bFrondRippleOneSided)
			{
				vFrondWindEffect = WindFrondRippleOneSided(sIn);
			}
			else if (sIn.m_sOptions.m_bFrondRippleTwoSided)
			{
				vFrondWindEffect = WindFrondRippleTwoSided(sIn);
			}
		}
	}

	// leaf component
	float3 vLeafWindEffect = float3(0.0, 0.0, 0.0);
	if (sIn.m_sOptions.m_bLodFull && (sIn.m_sOptions.m_bLeavesPresent || sIn.m_sOptions.m_bFacingLeavesPresent))
	{
		// this condition is only necessary if a non-leaf geometry type is mixed into the draw call
		if (sIn.m_sOptions.m_bOnlyLeavesPresent || sIn.m_sVertex.m_fGeometryTypeHint > ST_GEOMETRY_TYPE_HINT_FRONDS)
		{
			if (sIn.m_sVertex.m_fWindFlag > 0.0) // is leaf wind type 1 or 2 in effect?
			{
				vLeafWindEffect	= WindLeaf(sIn, sOut.m_vNormal, sOut.m_vTangent, sIn.m_sConstants.m_sLeaf2, sIn.m_sOptions.m_sLeaf2, sIn.m_sVertex.m_vLeafAnchorPoint);
			}
			else
			{
				vLeafWindEffect	= WindLeaf(sIn, sOut.m_vNormal, sOut.m_vTangent, sIn.m_sConstants.m_sLeaf1, sIn.m_sOptions.m_sLeaf1, sIn.m_sVertex.m_vLeafAnchorPoint);
			}
		}
	}

	// handle smooth LOD transition; there are four distinct levels of details for wind:
	//
	//	1. No wind effect [LOWEST/FASTEST]
	//	2. Global wind effect (gentle sway in trees and/or billboards)
	//  3. Branch wind effect (branches move independently, often includes global if set in Modeler's wind settings)
	//  4. Frond and leaf wind effects [HIGHEST/SLOWEST] (detailed wind, often includes branch & global when set in Modeler's wind settings)
	//
	//	these states (none, global, branch, full) allow these particular effects to pass through only if they're enabled in the Modeler
	//
	//	possible wind LOD states:
	//	- none, no LOD transition (ST_WIND_LOD_NONE)
	//	- global, no LOD tranition (ST_WIND_LOD_GLOBAL)
	//	- up-to-branch wind, no LOD transition (ST_WIND_LOD_BRANCH)
	//	- all effects enabled, no LOD transition (ST_WIND_LOD_FULL)
	//	- LOD transition between no wind and global wind (ST_WIND_LOD_NONE_X_GLOBAL)
	//	- LOD transition between no wind and branch wind (ST_WIND_LOD_NONE_X_BRANCH)
	//	- LOD transition between no wind and full wind (ST_WIND_LOD_NONE_X_FULL)
	//	- LOD transition between global wind and branch wind (ST_WIND_LOD_GLOBAL_X_BRANCH)
	//	- LOD transition between global wind and full wind (ST_WIND_LOD_GLOBAL_X_FULL)
	//	- LOD transition between branch wind and full wind (ST_WIND_LOD_BRANCH_X_FULL)

	if (sIn.m_sOptions.m_bLodNone_x_Global)
		sOut.m_vPos += vGlobalWindEffect * sIn.m_sInstance.m_fLodTransition;
	else if (sIn.m_sOptions.m_bLodNone_x_Branch)
		sOut.m_vPos += (vGlobalWindEffect + vBranchWindEffect) * sIn.m_sInstance.m_fLodTransition;
	else if (sIn.m_sOptions.m_bLodNone_x_Full)
		sOut.m_vPos += (vGlobalWindEffect + vBranchWindEffect + vFrondWindEffect + vLeafWindEffect) * sIn.m_sInstance.m_fLodTransition;
	else if (sIn.m_sOptions.m_bLodGlobal_x_Branch)
		sOut.m_vPos += vGlobalWindEffect + vBranchWindEffect * sIn.m_sInstance.m_fLodTransition;
	else if (sIn.m_sOptions.m_bLodGlobal_x_Full)
		sOut.m_vPos += vGlobalWindEffect + (vBranchWindEffect + vFrondWindEffect + vLeafWindEffect) * sIn.m_sInstance.m_fLodTransition;
	else if (sIn.m_sOptions.m_bLodBranch_x_Full)
		sOut.m_vPos += vGlobalWindEffect + vBranchWindEffect + (vFrondWindEffect + vLeafWindEffect) * sIn.m_sInstance.m_fLodTransition;
	else if (sIn.m_sOptions.m_bLodGlobal)
		sOut.m_vPos += vGlobalWindEffect;
	else if (sIn.m_sOptions.m_bLodBranch)
		sOut.m_vPos += vGlobalWindEffect + vBranchWindEffect;
	else if (sIn.m_sOptions.m_bLodFull)
		sOut.m_vPos += vGlobalWindEffect + vBranchWindEffect + vFrondWindEffect + vLeafWindEffect;
	else if (sIn.m_sOptions.m_bLodNone)
	{
		// do nothing
	}

	// adjust normal if rolling wind is active
	if (sIn.m_sOptions.m_bRollingOnThisLod)
	{
		float3 vWindEffectNetDir = sOut.m_vPos - vOriginalVertexPosition;
		sOut.m_vNormal = normalize(vWindEffectNetDir * u_sRolling.m_fBranchLightingAdjust * (sIn.m_sOptions.m_bLodRollingFade ? sIn.m_sInstance.m_fLodTransition : 1.0) + sOut.m_vNormal);
	}
}

#endif // ST_INCLUDE_WIND

#endif // ST_WIND_IS_ACTIVE


///////////////////////////////////////////////////////////////////////
//	Vertex shader entry point
//
//	Main depth-only vertex shader for 3D geometry

#if (ST_OPENGL)

	// attribute data coming from the app to the vertex shader
	layout(location = 0) attribute float4 in_vAttrib0;
	layout(location = 1) attribute float4 in_vAttrib1;
	layout(location = 2) attribute float4 in_vAttrib2;
	layout(location = 3) attribute float4 in_vAttrib3;
	layout(location = 4) attribute float4 in_vAttrib4;
	layout(location = 5) attribute float4 in_vAttrib5;

	// instancing input attributes
	layout(location = 13) attribute float4 in_vInstancePosAndScalar;
	layout(location = 14) attribute float4 in_vInstanceUpVectorAndLod1;
	layout(location = 15) attribute float4 in_vInstanceRightVectorAndLod2;

	// output parameters (will pass to pixel shader)
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

	void main(void)

#else

	void main(// input attributes
			  $HLSL_VERTEX_SHADER_INPUTS$

			  // output parameters (will pass to pixel shader)
			  , out float4 v2p_vInterpolant0 : ST_VS_OUT_POS
			  , out float2 v2p_vInterpolant1 : ST_VS_OUT_ATTR1

			  // user interpolants
			  #if (ST_USER_INTERPOLANT0)
				, out float4 v2p_vUserInterpolant0 : ST_VS_OUT_ATTR2
			  #endif
			  #if (ST_USER_INTERPOLANT1)
				, out float4 v2p_vUserInterpolant1 : ST_VS_OUT_ATTR3
			  #endif
			  #if (ST_USER_INTERPOLANT2)
				, out float4 v2p_vUserInterpolant2 : ST_VS_OUT_ATTR4
			  #endif
			  #if (ST_USER_INTERPOLANT3)
				, out float4 v2p_vUserInterpolant3 : ST_VS_OUT_ATTR5
			  #endif
			 )
#endif
{
	#if (ST_XBOX_360)
		// compute the instance index
		int nNumIndicesPerInstance = u_fInstancingInfo.x;
		int nInstanceIndex = (in_nIndex + 0.5f) / nNumIndicesPerInstance;

		// compute the mesh index
		int nMeshIndex = in_nIndex - nInstanceIndex * nNumIndicesPerInstance;

		// fetch the actual mesh vertex data
		float4 in_vInstancePosAndScalar;
		float4 in_vInstanceUpVectorAndLod1;
		float4 in_vInstanceRightVectorAndLod2;
        float4 in_vAttrib0;
        float4 in_vAttrib1;
        float4 in_vAttrib2;
        float4 in_vAttrib3;
        float4 in_vAttrib4;
        float4 in_vAttrib5;
        asm
        {
            vfetch in_vAttrib0, nMeshIndex, texcoord2; // ST_VS_IN_ATTR3
            vfetch in_vAttrib1, nMeshIndex, texcoord3; // ST_VS_IN_ATTR4
            vfetch in_vAttrib2, nMeshIndex, texcoord4; // ST_VS_IN_ATTR5
            vfetch in_vAttrib3, nMeshIndex, texcoord5; // ST_VS_IN_ATTR6
            vfetch in_vAttrib4, nMeshIndex, texcoord6; // ST_VS_IN_ATTR7
            vfetch in_vAttrib5, nMeshIndex, texcoord7; // ST_VS_IN_ATTR8
            vfetch in_vInstancePosAndScalar, nInstanceIndex, position0; // ST_VS_IN_ATTR0
            vfetch in_vInstanceUpVectorAndLod1, nInstanceIndex, texcoord0; // ST_VS_IN_ATTR1
            vfetch in_vInstanceRightVectorAndLod2, nInstanceIndex, texcoord1; // ST_VS_IN_ATTR2
        };
	#endif

	// set up all possible input vertex properties; give initial values even to those properties that will not be used
	// to avoid compilation warnings on some platforms; many will go unused in depth-only and will be optimized out
	// by the shader compiler
	float3 in_vPosition = float3(0.0, 0.0, 0.0);
	float3 in_vLodPosition = float3(0.0, 0.0, 0.0);
	float  in_fGeometryTypeHint = 0.0;
	float2 in_vDiffuseTexCoords = float2(0.0, 0.0);
	float3 in_vLeafCardCorner = float3(0.0, 0.0, 0.0);
	float  in_fLeafCardLodScalar = 0.0;
	float4 in_vWindBranchData = float4(0.0, 0.0, 0.0, 0.0);
	float3 in_vWindExtraData = float3(0.0, 0.0, 0.0);
	float  in_fWindFlags = 0.0;
	float3 in_vLeafAnchorPoint = float3(0.0, 0.0, 0.0);
	float3 in_vBranchSeamDiffuse = float3(0.0, 0.0, 0.0);
	float2 in_vBranchSeamDetail = float2(0.0, 0.0);
	float2 in_vDetailTexCoords = float2(0.0, 0.0);
	float3 in_vNormal = float3(0.0, 0.0, 0.0);
	float3 in_vTangent = float3(0.0, 0.0, 0.0);
	float  in_fAmbientOcclusion = 0.0;

	// unpack incoming vertex properties, if necessary
    float4 vDecodedAttrib0 = in_vAttrib0;
    float4 vDecodedAttrib1 = in_vAttrib1;
    float4 vDecodedAttrib2 = in_vAttrib2;
    float4 vDecodedAttrib3 = in_vAttrib3;
    float4 vDecodedAttrib4 = in_vAttrib4;
    float4 vDecodedAttrib5 = DecodeFloat4FromUBytes(in_vAttrib5);

	// let those vertex properties that were passed in set the correct initial values, all others will be unused
    in_vPosition = vDecodedAttrib0.xyz;
    in_vDiffuseTexCoords = vDecodedAttrib1.xy;
    in_vNormal = vDecodedAttrib5.xyz;
    in_fGeometryTypeHint = vDecodedAttrib0.w;
    in_vWindBranchData = vDecodedAttrib2.xyzw;
    in_vWindExtraData = vDecodedAttrib3.xyz;
    in_fWindFlags = vDecodedAttrib1.z;
    in_vLeafAnchorPoint = vDecodedAttrib4.xyz;
    in_fAmbientOcclusion = vDecodedAttrib5.w;

	// pull in instancing data if available and supported in rendering code
	#define c_vInstancePos				in_vInstancePosAndScalar.xyz
	#define c_fInstanceScalar			in_vInstancePosAndScalar.w
	#define c_vInstanceUpVector			in_vInstanceUpVectorAndLod1.xyz
	#define c_vInstanceRightVector		in_vInstanceRightVectorAndLod2.xyz
	#define c_fInstanceLodTransition	in_vInstanceUpVectorAndLod1.w
	#define c_fInstanceLodValue			in_vInstanceRightVectorAndLod2.w

	// all possible values that can be output from the vertex shader;
	float3 out_vNormal = float3(0.0, 0.0, 0.0);
	float  out_fBillboardTo3dFade = 0.0;
	float4 out_vProjection = float4(0.0, 0.0, 0.0, 0.0);
	float2 out_vDiffuseTexCoords = float2(0.0, 0.0);

	// branches, fronds, and leaf meshes use smooth LOD
	if (!ST_USED_AS_GRASS && ST_EFFECT_SMOOTH_LOD && (ST_BRANCHES_PRESENT || ST_FRONDS_PRESENT || ST_LEAVES_PRESENT))
		in_vPosition = lerp(in_vLodPosition, in_vPosition, c_fInstanceLodTransition);

    // build an orientation matrix using the incoming up and right vector; will rotate and tilt the instance
    float3 vInstanceOutVector = normalize(cross(c_vInstanceUpVector, c_vInstanceRightVector));
    float3x3 mOrientation = BuildOrientationMatrix(c_vInstanceRightVector, vInstanceOutVector, c_vInstanceUpVector);

    ST_MULTIPASS_STABILIZE
    {
        in_vPosition = mul_float3x3_float3(mOrientation, in_vPosition);
        in_vNormal = mul_float3x3_float3(mOrientation, in_vNormal);
        in_vTangent = mul_float3x3_float3(mOrientation, in_vTangent);
        in_vLeafAnchorPoint = mul_float3x3_float3(mOrientation, in_vLeafAnchorPoint);
    }

    // because this section of code is only for facing leaves, we can skip it if no facing leaves are present.
    if (ST_FACING_LEAVES_PRESENT)
    {
        // this is a bit tricky -- at this stage, ST_FACING_LEAVES_PRESENT is true, but this one draw call may also have regular,
        // non-facing leaves in it (i.e. ST_LEAVES_PRESENT is true). the conditional below only checks in_fGeometryTypeHint if
        // this shader was compiled with ST_LEAVES_PRESENT set to true. otherwise the compiler will optimize this
        // condition to always true. if both are present, the in_fGeometryTypeHint check cannot be avoided.
        if (!ST_LEAVES_PRESENT || in_fGeometryTypeHint == ST_GEOMETRY_TYPE_HINT_FACING_LEAVES)
		{
			in_vLeafAnchorPoint = in_vPosition;

			// LOD interpolation; cards are shrunk & grown, depending on the LOD setting
			if (ST_EFFECT_SMOOTH_LOD)
				in_vLeafCardCorner.xy *= lerp(in_fLeafCardLodScalar, 1.0, c_fInstanceLodTransition);

			// build the corner coordinate and apply camera-facing transform
			float4 vLeafCardCorner = float4(ConvertFromStdCoordSys(float3(0.0, -in_vLeafCardCorner.x, in_vLeafCardCorner.y)), 1.0);
			vLeafCardCorner = mul_float4x4_float4(u_mCameraFacingMatrix, vLeafCardCorner);
			in_vPosition += vLeafCardCorner.xyz;

			// in_vLeafCardCorner.z contains an offset value to prevent z-fighting from a bunch of flat cards that may be lying in the
			// same plane -- z-fighting can be worse with wind as the cards move in and out of the same plane
			in_vPosition += in_vLeafCardCorner.z * u_vCameraDirection;
		}
	}

	// the trig offset is a big part of having the wind behave distinctly among instances; this sets the
	// base value and may be modified later in this shader
    float fLeafWindTrigOffset = dot(c_vInstancePos + in_vPosition, ST_WIND_LEAF_TRIG_OFFSET_SCALAR);

	// handle all of the wind simulation here
///////////////////////////////////////////////////////////////////////
//
//	This file is designed to take various per-vertex data and macros defined
//	by the SRT exporter and fill out all the structures needed by the
//	all-enocompassing SpeedTree wind call.

#if (ST_WIND_IS_ACTIVE)

	// setup wind input structure
    SWindInput sWindIn;

	// core per-vertex data
	sWindIn.m_sVertex.m_vPos = in_vPosition;
	sWindIn.m_sVertex.m_vNormal = in_vNormal;
	sWindIn.m_sVertex.m_vTangent = in_vTangent;
	sWindIn.m_sVertex.m_vDiffuseTexCoords = in_vDiffuseTexCoords;
	sWindIn.m_sVertex.m_vLeafAnchorPoint = in_vLeafAnchorPoint;
	sWindIn.m_sVertex.m_fGeometryTypeHint = in_fGeometryTypeHint;
	sWindIn.m_sVertex.m_fWindFlag = in_fWindFlags;

	// branch-specific per-vertex data
	sWindIn.m_sVertex.m_sBranch1.m_fWeight = in_vWindBranchData.x;
	sWindIn.m_sVertex.m_sBranch1.m_vWindNormal = mul_float3x3_float3(mOrientation, UnpackNormalFromFloat(in_vWindBranchData.y));
	sWindIn.m_sVertex.m_sBranch1.m_vWindNormalPacked = in_vWindBranchData.y;
	sWindIn.m_sVertex.m_sBranch2.m_fWeight = in_vWindBranchData.z;
	sWindIn.m_sVertex.m_sBranch2.m_vWindNormal = mul_float3x3_float3(mOrientation, UnpackNormalFromFloat(in_vWindBranchData.w));
	sWindIn.m_sVertex.m_sBranch2.m_vWindNormalPacked = in_vWindBranchData.w;

	// frond-specific per-vertex data
	sWindIn.m_sVertex.m_vFrondRippleDir = mul_float3x3_float3(mOrientation, UnpackNormalFromFloat(in_vWindExtraData.x));
	sWindIn.m_sVertex.m_fFrondRippleScalar = in_vWindExtraData.y;
	sWindIn.m_sVertex.m_fFrondLengthPercent = in_vWindExtraData.z;

	// leaf-specific per-vertex data
	sWindIn.m_sVertex.m_fLeafWindScalar = in_vWindExtraData.x;
	sWindIn.m_sVertex.m_vLeafGrowthDir = mul_float3x3_float3(mOrientation, UnpackNormalFromFloat(in_vWindExtraData.y));
	sWindIn.m_sVertex.m_vLeafRippleDir = mul_float3x3_float3(mOrientation, UnpackNormalFromFloat(in_vWindExtraData.z));

	// instance data
	sWindIn.m_sInstance.m_vPos = c_vInstancePos;
	sWindIn.m_sInstance.m_fScalar = c_fInstanceScalar;
	sWindIn.m_sInstance.m_fLeafTrigOffset = fLeafWindTrigOffset;
	sWindIn.m_sInstance.m_fLodTransition = c_fInstanceLodTransition;

	// copy constant buffer data over; constant data only changes per base tree type
	sWindIn.m_sConstants.m_fWallTime = u_fWallTime;
	sWindIn.m_sConstants.m_vDirection = u_vDirection.xyz;

	sWindIn.m_sConstants.m_vPalmStyleBranchAnchor = u_vAnchor;
	sWindIn.m_sConstants.m_sGlobal = u_sGlobal;
	sWindIn.m_sConstants.m_bPreserveGlobalWindShape = ST_WIND_EFFECT_GLOBAL_PRESERVE_SHAPE;
	sWindIn.m_sConstants.m_sBranch1 = u_sBranch1;
	sWindIn.m_sConstants.m_sBranch2 = u_sBranch2;

	sWindIn.m_sConstants.m_sFrondRipple = u_sFrondRipple;
	sWindIn.m_sConstants.m_fStrength = u_fStrength;
	sWindIn.m_sConstants.m_sRolling = u_sRolling;

	// leaves
	sWindIn.m_sConstants.m_sLeaf1 = u_sLeaf1;
	sWindIn.m_sConstants.m_sLeaf2 = u_sLeaf2;

	// options
	sWindIn.m_sOptions.m_bGlobalWind = ST_WIND_EFFECT_GLOBAL_WIND;
	sWindIn.m_sOptions.m_bGlobalPreserveShape = ST_WIND_EFFECT_GLOBAL_PRESERVE_SHAPE;

	sWindIn.m_sOptions.m_sBranch1.m_bSimple = (ST_WIND_EFFECT_BRANCH_SIMPLE_1 && ST_BRANCH_LEVEL_1_ACTIVE);
	sWindIn.m_sOptions.m_sBranch1.m_bDirectional = (ST_WIND_EFFECT_BRANCH_DIRECTIONAL_1 && ST_BRANCH_LEVEL_1_ACTIVE);
	sWindIn.m_sOptions.m_sBranch1.m_bDirectionalFrond = (ST_WIND_EFFECT_BRANCH_DIRECTIONAL_FROND_1 && ST_BRANCH_LEVEL_1_ACTIVE);
	sWindIn.m_sOptions.m_sBranch1.m_bTurbulence = ST_WIND_EFFECT_BRANCH_TURBULENCE_1;
	sWindIn.m_sOptions.m_sBranch1.m_bWhip = ST_WIND_EFFECT_BRANCH_WHIP_1;
	sWindIn.m_sOptions.m_sBranch1.m_bOscillatingComplex = ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_1;

	sWindIn.m_sOptions.m_sBranch2.m_bSimple = (ST_WIND_EFFECT_BRANCH_SIMPLE_2 && ST_BRANCH_LEVEL_2_ACTIVE);
	sWindIn.m_sOptions.m_sBranch2.m_bDirectional = (ST_WIND_EFFECT_BRANCH_DIRECTIONAL_2 && ST_BRANCH_LEVEL_2_ACTIVE);
	sWindIn.m_sOptions.m_sBranch2.m_bDirectionalFrond = (ST_WIND_EFFECT_BRANCH_DIRECTIONAL_FROND_2 && ST_BRANCH_LEVEL_2_ACTIVE);
	sWindIn.m_sOptions.m_sBranch2.m_bTurbulence = ST_WIND_EFFECT_BRANCH_TURBULENCE_2;
	sWindIn.m_sOptions.m_sBranch2.m_bWhip = ST_WIND_EFFECT_BRANCH_WHIP_2;
	sWindIn.m_sOptions.m_sBranch2.m_bOscillatingComplex = ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_2;

	sWindIn.m_sOptions.m_bBranchWindActive = ST_WIND_BRANCH_WIND_ACTIVE;
	sWindIn.m_sOptions.m_sLeaf1.m_bRippleVertexNormal = ST_WIND_EFFECT_LEAF_RIPPLE_VERTEX_NORMAL_1;
	sWindIn.m_sOptions.m_sLeaf1.m_bRippleComputed = ST_WIND_EFFECT_LEAF_RIPPLE_COMPUTED_1;
	sWindIn.m_sOptions.m_sLeaf1.m_bTumble = ST_WIND_EFFECT_LEAF_TUMBLE_1;
	sWindIn.m_sOptions.m_sLeaf1.m_bTwitch = ST_WIND_EFFECT_LEAF_TWITCH_1;
	sWindIn.m_sOptions.m_sLeaf1.m_bOcclusion = ST_WIND_EFFECT_LEAF_OCCLUSION_1;
	sWindIn.m_sOptions.m_sLeaf2.m_bRippleVertexNormal = ST_WIND_EFFECT_LEAF_RIPPLE_VERTEX_NORMAL_2;
	sWindIn.m_sOptions.m_sLeaf2.m_bRippleComputed = ST_WIND_EFFECT_LEAF_RIPPLE_COMPUTED_2;
	sWindIn.m_sOptions.m_sLeaf2.m_bTumble = ST_WIND_EFFECT_LEAF_TUMBLE_2;
	sWindIn.m_sOptions.m_sLeaf2.m_bTwitch = ST_WIND_EFFECT_LEAF_TWITCH_2;
	sWindIn.m_sOptions.m_sLeaf2.m_bOcclusion = ST_WIND_EFFECT_LEAF_OCCLUSION_2;
	sWindIn.m_sOptions.m_bFrondRippleOneSided = ST_WIND_EFFECT_FROND_RIPPLE_ONE_SIDED;
	sWindIn.m_sOptions.m_bFrondRippleTwoSided = ST_WIND_EFFECT_FROND_RIPPLE_TWO_SIDED;
	sWindIn.m_sOptions.m_bFrondRippleAdjustLighting = ST_WIND_EFFECT_FROND_RIPPLE_ADJUST_LIGHTING;
	sWindIn.m_sOptions.m_bRollingOnThisLod = ST_WIND_EFFECT_ROLLING_ON_THIS_LOD;
	sWindIn.m_sOptions.m_bModelUsesRolling = ST_MODEL_USES_ROLLING_WIND;
	sWindIn.m_sOptions.m_bCoordSysZUp = ST_COORDSYS_Z_UP;
	sWindIn.m_sOptions.m_bLodGlobal = ST_WIND_LOD_GLOBAL;
	sWindIn.m_sOptions.m_bLodBranch = ST_WIND_LOD_BRANCH;
	sWindIn.m_sOptions.m_bLodFull = ST_WIND_LOD_FULL;
	sWindIn.m_sOptions.m_bLodNone_x_Global = ST_WIND_LOD_NONE_X_GLOBAL;
	sWindIn.m_sOptions.m_bLodNone_x_Branch = ST_WIND_LOD_NONE_X_BRANCH;
	sWindIn.m_sOptions.m_bLodNone_x_Full = ST_WIND_LOD_NONE_X_FULL;
	sWindIn.m_sOptions.m_bLodGlobal_x_Branch = ST_WIND_LOD_GLOBAL_X_BRANCH;
	sWindIn.m_sOptions.m_bLodGlobal_x_Full = ST_WIND_LOD_GLOBAL_X_FULL;
	sWindIn.m_sOptions.m_bLodBranch_x_Full = ST_WIND_LOD_BRANCH_X_FULL;
	sWindIn.m_sOptions.m_bLodNone = ST_WIND_LOD_NONE;
	sWindIn.m_sOptions.m_bLodRollingFade = ST_WIND_EFFECT_ROLLING_ON_THIS_LOD && ST_WIND_LOD_ROLLING_FADE;
	sWindIn.m_sOptions.m_bLodBillboardGlobalWind = ST_WIND_LOD_BILLBOARD_GLOBAL;
	sWindIn.m_sOptions.m_bBranchesPresent = ST_BRANCHES_PRESENT;
	sWindIn.m_sOptions.m_bOnlyBranchesPresent = ST_ONLY_BRANCHES_PRESENT;
	sWindIn.m_sOptions.m_bFrondsPresent = ST_FRONDS_PRESENT;
	sWindIn.m_sOptions.m_bOnlyFrondsPresent = ST_ONLY_FRONDS_PRESENT;
	sWindIn.m_sOptions.m_bLeavesPresent = ST_LEAVES_PRESENT;
	sWindIn.m_sOptions.m_bOnlyLeavesPresent = ST_ONLY_LEAVES_PRESENT;
	sWindIn.m_sOptions.m_bFacingLeavesPresent = ST_FACING_LEAVES_PRESENT;
	sWindIn.m_sOptions.m_bOnlyFacingLeavesPresent = ST_ONLY_FACING_LEAVES_PRESENT;
	sWindIn.m_sOptions.m_bRigidMeshesPresent = ST_RIGID_MESHES_PRESENT;
	sWindIn.m_sOptions.m_bOnlyRigidMeshesPresent = ST_ONLY_RIGID_MESHES_PRESENT;
	sWindIn.m_sOptions.m_bVertexPropertyWindExtraDataPresent = VERTEX_PROPERTY_WINDEXTRADATA_PRESENT;

	// derived per-vertex data
    #define ST_UNIQUE_VERTEX_SCALAR 0.38
    sWindIn.m_fUniqueVertexValue = dot(ST_UNIQUE_VERTEX_SCALAR * in_vPosition + c_vInstancePos, float3(1.0, 1.0, 1.0));
    #undef ST_UNIQUE_VERTEX_SCALAR

	// SpeedTreeWindModel will modify a few parameters
    SWindOutput sWindOut;

	// call to master wind function that will handle everything; shader compiler optimizer will remove functions that aren't used
	SpeedTreeWindModel(sWindIn, sWindOut);

    // setup output
    in_vPosition = sWindOut.m_vPos;
	in_vNormal = sWindOut.m_vNormal;
	in_vTangent = sWindOut.m_vTangent;

#endif // ST_WIND_IS_ACTIVE



	// scale the whole tree
	in_vPosition *= c_fInstanceScalar;

	// move instance into position
	in_vPosition += c_vInstancePos;

	// distance (may be used a few times or not at all and optimized out)
	float fDistanceFromCamera = distance(u_vLodRefPosition, in_vPosition); // in_vPosition has been scaled and translated by this point

	// pass hints to pixel shader for fading by alpha value (fade to billboard & grass rendering)
	if (ST_USED_AS_GRASS)
		out_fBillboardTo3dFade = FadeGrass(fDistanceFromCamera) * u_fAlphaScalar; // grass just fades out, not to a billboard
	else if (ST_EFFECT_FADE_TO_BILLBOARD)
		out_fBillboardTo3dFade = Fade3dTree(c_fInstanceLodValue) * u_fAlphaScalar;

	// simple parameter copying from input attribs to output interpolants
	out_vDiffuseTexCoords = in_vDiffuseTexCoords;

	// final screen projection
	out_vProjection = ProjectToScreen(float4(in_vPosition, 1.0));

	// pack each outgoing value into output interpolants; the "v2p" prefix indicates that these
	// variables are values that go from [v]ertex-[2]-[p]ixel shader
	v2p_vInterpolant0.xyzw = out_vProjection;
	v2p_vInterpolant1.xy = out_vDiffuseTexCoords;

	// assign user interpolant values here, e.g.:
	//	#if (ST_USER_INTERPOLANT0)
	//	    v2p_vUserInterpolant0 = float4(1, 1, 1, 1);
	//	#endif
	//
	//	if defined as non-zero, ST_USER_INTERPOLANT0 through ST_USER_INTERPOLANT3 should be set in
	//	Template_UserInterpolants.fx, which will carry through both the vertex
	//	and pixel shader templates
}
