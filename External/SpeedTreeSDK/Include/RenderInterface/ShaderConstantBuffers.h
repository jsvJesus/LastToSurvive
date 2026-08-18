///////////////////////////////////////////////////////////////////////
//  Preprocessor

#pragma once
#include "Core/ExportBegin.h"
#include "Core/Types.h"
#include "Core/Vector.h"
#include "Core/Matrix.h"

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
#define ST_CONST_BUF_REGISTER_COUNT              8

///////////////////////////////////////////////////////////////////////
//  Packing

#ifdef ST_SETS_PACKING_INTERNALLY
    #pragma pack(push, 4)
#endif

///////////////////////////////////////////////////////////////////////
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

    // constant buffer register table for use with DX9 and 360
    struct SRegisterGroupInfo
    {
        st_int32        m_nRegisterStart;
        st_int32        m_nNumRegisters;
    };

    static const SRegisterGroupInfo c_asBufferRegisters[ST_CONST_BUF_REGISTER_COUNT] =
    {
        {   0,  49 },   // ST_CONST_BUF_REGISTER_FRAME
        {  49,  28 },   // ST_CONST_BUF_REGISTER_BASE_TREE
        {  77,   1 },   // ST_CONST_BUF_REGISTER_INSTANCING
        {  78,  21 },   // ST_CONST_BUF_REGISTER_MATERIAL
        {  99,  19 },   // ST_CONST_BUF_REGISTER_WIND_DYNAMICS
        { 118,   4 },   // ST_CONST_BUF_REGISTER_FOG_AND_SKY
        { 122,   1 },   // ST_CONST_BUF_REGISTER_TERRAIN
        { 123,   3 }    // ST_CONST_BUF_REGISTER_BLOOM
    };

    struct SDirLight
    {
        Vec3                m_vAmbient;                       // current light's ambient color
        float               m__Pad0;
        Vec3                m_vDiffuse;                       // current light's diffuse color
        float               m__Pad1;
        Vec3                m_vSpecular;                      // current light's specular color
        float               m__Pad2;
        Vec3                m_vTransmission;                  // current light's transmission color
        float               m__Pad3;
        Vec3                m_vDir;                           // normalized light direction vector
        float               m__Pad4;
    };

    struct SShadows
    {
        Vec4                m_vMapRanges;                     // ending distance in world units for each of the cascaded shadow maps
        Vec4                m_avSmoothingTable[3];            // texel offsets to use for shadow smoothing
        Mat4x4              m_amLightModelViewProjs[4];       // light map projections for up to four shadow map projections
        Vec2                m_vTexelOffset;                   // xy value is { 0.5 / shadow_map_res.x, 0.5 / shadow_map_res.y }
        st_float32          m_fFadeStartPercent;              // percent (0.0 to 1.0) from the end of the last cascade where shadow begins to fade out
        st_float32          m_fFadeInverseDistance;           // upstream computation to help optimize shadow fade computation in shader
        st_float32          m_fTerrainAmbientOcclusion;       // [0.0, 1.0] value controlling per-vertex terrain AO darkness
        Vec3                m__Pad0;
    };

    struct SWindGlobal
    {
        st_float32          m_fTime;                          // time, in seconds, as needed by global wind algorithm
        st_float32          m_fDistance;                      // distance, in model units, that the model will move
        st_float32          m_fHeight;                        // height of the model
        st_float32          m_fHeightExponent;                // exponent that controls the curvature of the model (1 = linear, >1 bends more near the top
        st_float32          m_fAdherence;                     // how much wind direction affects the motion
        Vec3                m__Pad0;
    };

    struct SWindBranchLevel
    {
        st_float32          m_fBranchTime;                    // time, in seconds, as needed by branch wind algorithm
        st_float32          m_fBranchDistance;                // distance, in model units, that the model will move
        st_float32          m_fTwitch;                        // Sway bias, controls the uniformity of the branch oscillation
        st_float32          m_fTwitchFreqScale;               // Sway freq, controls how frequently branch sway varies during oscillation
        st_float32          m_fWhip;                          // controls how much the branch is allowed to deform during oscillations
        st_float32          m_fDirectionAdherence;            // how much wind direction affects the motion
        st_float32          m_fTurbulence;                    // controls how much chaotic motion is added due branch motion as wind strength increases
        float               m__Pad0;
    };

    struct SWindLeaf
    {
        st_float32          m_fRippleTime;                    // time, in seconds, as needed by leaf ripple algorithm
        st_float32          m_fRippleDistance;                // how much, in model units, leaf vertices can move
        st_float32          m_fLeewardScalar;                 // how much leaf wind is scaled on the side of the model farthest away from the wind source
        st_float32          m_fTumbleTime;                    // time, in seconds, as needed by leaf tumble algorithm
        st_float32          m_fTumbleFlip;                    // how far, in degrees, a leaf can flip
        st_float32          m_fTumbleTwist;                   // how far, in degrees, a leaf can twist
        st_float32          m_fTumbleDirectionAdherence;      // how much a leaf will rotate 'into' the wind
        st_float32          m_fTwitchThrow;                   // how far, in degrees, a leaf will twitch when applicable
        st_float32          m_fTwitchSharpness;               // controls how rapidly leaves move when the twitch
        st_float32          m_fTwitchTime;                    // time, in seconds, as needed by the twitch algorithm
        Vec2                m__Pad0;
    };

    struct SWindFrond
    {
        st_float32          m_fFrondTime;                     // time, in seconds, as needed by frond wind algorithm
        st_float32          m_fFrondDistance;                 // how far, in model units, frond vertices can move
        st_float32          m_fTile;                          // how tightly the oscillation pattern is applied along the length of the frond
        st_float32          m_fLightingScalar;                // controls how much the surface normal is modified by frond motion
    };

    struct SWindRolling
    {
        st_float32          m_fBranchFieldMin;                // branch motion scalar applied when the noise field is low
        st_float32          m_fBranchLightingAdjust;          // controls how much surface normals are modified on branches due to rolling motion
        st_float32          m_fBranchVerticalOffset;          // controls how much vertical motion is added when the noise field is high
        st_float32          m_fLeafRippleMin;                 // ripple scalar applied when the noise field is low
        st_float32          m_fLeafTumbleMin;                 // tumble scalar applied when the noise field is low
        st_float32          m_fNoisePeriod;                   // period value used by the noise field computation
        st_float32          m_fNoiseSize;                     // size value used by the noise field computation
        st_float32          m_fNoiseTurbulence;               // turbulence value used by the noise field computation
        st_float32          m_fNoiseTwist;                    // twist value used by the noise field computation
        st_float32          m_fOffsetX;                       // x offset of pattern to move noise pattern across the field
        st_float32          m_fOffsetY;                       // y offset of pattern to move noise pattern across the field
        st_float32          m_fBranchRipple;                  // controls how much a branch can deform due to rolling motion
    };


    CShaderConstantBufferRI_TemplateList
    struct ST_DLL_LINK SFrameConstBuf : CShaderConstantBufferRI_t
    {
        Vec4                u_vHandTunedParams;               // tied to user input for art tuning of various parameters
        Mat4x4              u_mModelViewProj3d;               // 3D modelview_matrix * projection_matrix; modelview has no translation component
        Mat4x4              u_mProjectionInverse3d;           // inverse of projection_matrix; used in example deferred lighting shader
        Mat4x4              u_mModelViewProj2d;               // 2D modelview_matrix * projection_matrix; used by fullscreen shaders
        Mat4x4              u_mCameraFacingMatrix;            // used to turn facing leaves toward the camera
        Vec3                u_vCameraPosition;                // 3D position of camera in world space
        float               m__Pad0;
        Vec3                u_vCameraDirection;               // normalized camera direction vector
        float               m__Pad1;
        Vec3                u_vLodRefPosition;                // 3D position used for LOD computations in world space (when rendering into shadow maps)
        float               m__Pad2;
        Vec2                u_vViewport;                      // x = viewport width, y = viewport height
        Vec2                u_vViewportInverse;               // 1.0 / Viewport
        st_float32          u_fWallTime;                      // real time, in seconds, from the beginning of the app
        st_float32          u_fFarClip;                       // far clipping plane distance in world units
        Vec2                m__Pad3;
        SDirLight           u_sDirLight;                      // all directional light data
        SShadows            u_sShadows;                       // all shadow data
    };
    #define SFrameConstBuf_t SFrameConstBuf<TShaderConstantBufferClass, TTextureClass>


    CShaderConstantBufferRI_TemplateList
    struct ST_DLL_LINK SBaseTreeConstBuf : CShaderConstantBufferRI_t
    {
        st_float32          u_f3dGrassStartDist;              // distance from camera in world units when grass begins to fade away
        st_float32          u_f3dGrassRange;                  // distance in world units over which grass fades from opaque to fully transparent
        st_float32          u_fBillboardHorzFade;             // [0.0, 1.0] value to control fade overlap from 3D to billboard; 1.0 is fully overlapping
        st_float32          u_fOneMinusBillboardHorzFade;     // 1.0 - u_fBillboardHorzFade
        st_float32          u_fBillboardStartDist;            // distance in world units where billboard begins to fade in, minus u_fBillboardRange
        st_float32          u_fBillboardRange;                // distance in world units over which billboard fades in
        st_float32          u_fBillboardCullDist;             // distance from camera in world units where billboards are no longer drawn, minus u_fBillboardRange
        st_float32          u_fHueVariationByPos;             // [0.0, 1.0] value to control per-inst-pos hue variation; 0.0 is none
        st_float32          u_fHueVariationByVertex;          // [0.0, 1.0] value to control per-vertex hue variation; 0.0 is none
        st_float32          u_fAmbientImageScalar;            // scalar value for tuning ambient image contribution; 0.0 is none
        st_float32          u_fNumBillboards;                 // number of 360-degree billboard images used for active base tree
        st_float32          u_fRadiansPerImage;               // numerical helper for shader computation of which billboard image is visible
        Vec4                u_avBillboardTexCoords[24];       // packed billboard texcoords, one float4 per 360-degree billboard image
        Vec3                u_vHueVariationColor;             // rgb color to be used with hue variation
        float               m__Pad0;
    };
    #define SBaseTreeConstBuf_t SBaseTreeConstBuf<TShaderConstantBufferClass, TTextureClass>


    CShaderConstantBufferRI_TemplateList
    struct ST_DLL_LINK SInstancingConstBuf : CShaderConstantBufferRI_t
    {
        st_float32          u_fInstancingInfo;                // used by Xbox 360 only; x = number of indices per instance
        Vec3                m__Pad0;
    };
    #define SInstancingConstBuf_t SInstancingConstBuf<TShaderConstantBufferClass, TTextureClass>


    CShaderConstantBufferRI_TemplateList
    struct ST_DLL_LINK SMaterialConstBuf : CShaderConstantBufferRI_t
    {
        st_float32          u_fShininess;                     // current material shininess
        st_float32          u_fBranchSeamWeight;              // controls curve used in branch seam blending; set in the Modeler
        st_float32          u_fOneMinusAmbientContrastFactor; // used to control level of "ambient contrast", a SpeedTree-specific lighting component
        st_float32          u_fTransmissionShadowBrightness;  // controls shadow brightness in materials where transmission is active
        st_float32          u_fTransmissionViewDependency;    // controls the relationship between the light angle and amount of transmission effect
        st_float32          u_fAlphaScalar;                   // multiplied directly with the texture's raw alpha value; >1 often needed for A2C
        Vec2                m__Pad0;
        Vec3                u_vAmbientColor;                  // current ambient material color
        float               m__Pad1;
        Vec3                u_vDiffuseColor;                  // current diffuse material color
        float               m__Pad2;
        Vec3                u_vSpecularColor;                 // current specular material color
        float               m__Pad3;
        Vec3                u_vTransmissionColor;             // current transmission material color
        float               m__Pad4;
        Vec4                u_avEffectConfigFlags[5];         // rendering effect flags used under 'unified shader' compilation mode
        Vec4                u_avWindConfigFlags[7];           // wind state flags used under 'unified shader' compilation mode
        Vec4                u_avWindLodFlags[3];              // wind LOD flags used under 'unified shader' compilation mode
    };
    #define SMaterialConstBuf_t SMaterialConstBuf<TShaderConstantBufferClass, TTextureClass>


    CShaderConstantBufferRI_TemplateList
    struct ST_DLL_LINK SWindDynamicsConstBuf : CShaderConstantBufferRI_t
    {
        Vec3                u_vDirection;                     // direction of the wind
        float               m__Pad0;
        Vec3                u_vAnchor;                        // position of the wind anchor used by 'palm style' branch motion
        float               m__Pad1;
        SWindGlobal         u_sGlobal;                        // global wind parameters
        SWindBranchLevel    u_sBranch1;                       // branch wind level 1 parameters
        SWindBranchLevel    u_sBranch2;                       // branch wind level 2 parameters
        SWindLeaf           u_sLeaf1;                         // leaf wind group 1 parameters
        SWindLeaf           u_sLeaf2;                         // leaf wind group 2 parameters
        SWindFrond          u_sFrondRipple;                   // frond wind parameters
        st_float32          u_fStrength;                      // wind strength (0.0 = none, 1.0 = max)
        Vec3                m__Pad2;
        SWindRolling        u_sRolling;                       // rolling wind parameters
    };
    #define SWindDynamicsConstBuf_t SWindDynamicsConstBuf<TShaderConstantBufferClass, TTextureClass>


    CShaderConstantBufferRI_TemplateList
    struct ST_DLL_LINK SFogAndSkyConstBuf : CShaderConstantBufferRI_t
    {
        st_float32          u_fFogEndDist;                    // for linear fog effect, distance in world units where fog coverage is absolute
        st_float32          u_fFogSpan;                       // for linear fog effect, distance in world units from start to end fog values
        st_float32          u_fSunSize;                       // sky pixel shader uses a procedural sun disk; this controls the size
        st_float32          u_fSunSpreadExponent;             // sky pixel shader uses a procedural sun disk; this controls the sun/sky blend
        Vec3                u_vFogColor;                      // rgb color of the fog effect
        float               m__Pad0;
        Vec3                u_vSkyColor;                      // base color for the sky
        float               m__Pad1;
        Vec3                u_vSunColor;                      // basic color of the procedural sun disk
        float               m__Pad2;
    };
    #define SFogAndSkyConstBuf_t SFogAndSkyConstBuf<TShaderConstantBufferClass, TTextureClass>


    CShaderConstantBufferRI_TemplateList
    struct ST_DLL_LINK STerrainConstBuf : CShaderConstantBufferRI_t
    {
        st_float32          u_fSplatTile0;                    // terrain system uses three textures for splatting; texture repeat values for map 0 here
        st_float32          u_fSplatTile1;                    // terrain system uses three textures for splatting; texture repeat values for map 1 here
        st_float32          u_fSplatTile2;                    // terrain system uses three textures for splatting; texture repeat values for map 2 here
        st_float32          u_fTerrainAmbientImageScalar;     // ambient image scalar for terrain (forward rendering only); 0.0 means to contribution
    };
    #define STerrainConstBuf_t STerrainConstBuf<TShaderConstantBufferClass, TTextureClass>


    CShaderConstantBufferRI_TemplateList
    struct ST_DLL_LINK SBloomConstBuf : CShaderConstantBufferRI_t
    {
        st_float32          u_fBrightPass;                    // [0.0, 1.0] value, determines hi-pass threshold for bloom effect
        st_float32          u_fDownsample;                    // controls how many times hi-pass frame will be downsampled before applied
        st_float32          u_fDownsampleLoopStart;           // loop helper for bloom hi-pass pixel shader
        st_float32          u_fDownsampleLoopEnd;             // loop helper for bloom hi-pass pixel shader
        st_float32          u_fBlurKernelSize;                // controls how blurred the bloom pass will be
        st_float32          u_fBlurKernelStep;                // loop helper for bloom Blur pass
        st_float32          u_fBlurPixelOffset;               // shader helper for bloom Blur pass
        st_float32          u_fBloomEffectScalar;             // scales how much the bloom Effect is added to the final bloom composite
        st_float32          u_fHighPassFloor;                 // lowest value for high pass filter
        st_float32          u_fFinalMainScalar;               // controls how much the main render contributes to the final bloom composite
        Vec2                m__Pad0;
    };
    #define SBloomConstBuf_t SBloomConstBuf<TShaderConstantBufferClass, TTextureClass>

    #ifdef __CELLOS_LV2__

        const st_int32 c_nNumPs3ShaderConstants = 126;
        static const char* c_aszShaderConstantIds[c_nNumPs3ShaderConstants] = 
        {
            "u_vHandTunedParams",
            "u_mModelViewProj3d[0]",
            "u_mModelViewProj3d[1]",
            "u_mModelViewProj3d[2]",
            "u_mModelViewProj3d[3]",
            "u_mProjectionInverse3d[0]",
            "u_mProjectionInverse3d[1]",
            "u_mProjectionInverse3d[2]",
            "u_mProjectionInverse3d[3]",
            "u_mModelViewProj2d[0]",
            "u_mModelViewProj2d[1]",
            "u_mModelViewProj2d[2]",
            "u_mModelViewProj2d[3]",
            "u_mCameraFacingMatrix[0]",
            "u_mCameraFacingMatrix[1]",
            "u_mCameraFacingMatrix[2]",
            "u_mCameraFacingMatrix[3]",
            "u_vCameraPosition",
            "u_vCameraDirection",
            "u_vLodRefPosition",
            "__placeholder0",
            "__placeholder1",
            "u_sDirLight.m_vAmbient",
            "u_sDirLight.m_vDiffuse",
            "u_sDirLight.m_vSpecular",
            "u_sDirLight.m_vTransmission",
            "u_sDirLight.m_vDir",
            "u_sShadows.m_vMapRanges",
            "u_sShadows.m_avSmoothingTable[0]",
            "u_sShadows.m_avSmoothingTable[1]",
            "u_sShadows.m_avSmoothingTable[2]",
            "u_sShadows.m_amLightModelViewProjs[0][0]",
            "u_sShadows.m_amLightModelViewProjs[0][1]",
            "u_sShadows.m_amLightModelViewProjs[0][2]",
            "u_sShadows.m_amLightModelViewProjs[0][3]",
            "u_sShadows.m_amLightModelViewProjs[1][0]",
            "u_sShadows.m_amLightModelViewProjs[1][1]",
            "u_sShadows.m_amLightModelViewProjs[1][2]",
            "u_sShadows.m_amLightModelViewProjs[1][3]",
            "u_sShadows.m_amLightModelViewProjs[2][0]",
            "u_sShadows.m_amLightModelViewProjs[2][1]",
            "u_sShadows.m_amLightModelViewProjs[2][2]",
            "u_sShadows.m_amLightModelViewProjs[2][3]",
            "u_sShadows.m_amLightModelViewProjs[3][0]",
            "u_sShadows.m_amLightModelViewProjs[3][1]",
            "u_sShadows.m_amLightModelViewProjs[3][2]",
            "u_sShadows.m_amLightModelViewProjs[3][3]",
            "u_sShadows.__placeholder0",
            "u_sShadows.m_fTerrainAmbientOcclusion",
            "__placeholder2",
            "__placeholder3",
            "__placeholder4",
            "u_avBillboardTexCoords[0]",
            "u_avBillboardTexCoords[1]",
            "u_avBillboardTexCoords[2]",
            "u_avBillboardTexCoords[3]",
            "u_avBillboardTexCoords[4]",
            "u_avBillboardTexCoords[5]",
            "u_avBillboardTexCoords[6]",
            "u_avBillboardTexCoords[7]",
            "u_avBillboardTexCoords[8]",
            "u_avBillboardTexCoords[9]",
            "u_avBillboardTexCoords[10]",
            "u_avBillboardTexCoords[11]",
            "u_avBillboardTexCoords[12]",
            "u_avBillboardTexCoords[13]",
            "u_avBillboardTexCoords[14]",
            "u_avBillboardTexCoords[15]",
            "u_avBillboardTexCoords[16]",
            "u_avBillboardTexCoords[17]",
            "u_avBillboardTexCoords[18]",
            "u_avBillboardTexCoords[19]",
            "u_avBillboardTexCoords[20]",
            "u_avBillboardTexCoords[21]",
            "u_avBillboardTexCoords[22]",
            "u_avBillboardTexCoords[23]",
            "u_vHueVariationColor",
            "u_fInstancingInfo",
            "__placeholder5",
            "__placeholder6",
            "u_vAmbientColor",
            "u_vDiffuseColor",
            "u_vSpecularColor",
            "u_vTransmissionColor",
            "u_avEffectConfigFlags[0]",
            "u_avEffectConfigFlags[1]",
            "u_avEffectConfigFlags[2]",
            "u_avEffectConfigFlags[3]",
            "u_avEffectConfigFlags[4]",
            "u_avWindConfigFlags[0]",
            "u_avWindConfigFlags[1]",
            "u_avWindConfigFlags[2]",
            "u_avWindConfigFlags[3]",
            "u_avWindConfigFlags[4]",
            "u_avWindConfigFlags[5]",
            "u_avWindConfigFlags[6]",
            "u_avWindLodFlags[0]",
            "u_avWindLodFlags[1]",
            "u_avWindLodFlags[2]",
            "u_vDirection",
            "u_vAnchor",
            "u_sGlobal.__placeholder0",
            "u_sGlobal.m_fAdherence",
            "u_sBranch1.__placeholder0",
            "u_sBranch1.__placeholder1",
            "u_sBranch2.__placeholder0",
            "u_sBranch2.__placeholder1",
            "u_sLeaf1.__placeholder0",
            "u_sLeaf1.__placeholder1",
            "u_sLeaf1.__placeholder2",
            "u_sLeaf2.__placeholder0",
            "u_sLeaf2.__placeholder1",
            "u_sLeaf2.__placeholder2",
            "u_sFrondRipple.__placeholder0",
            "u_fStrength",
            "u_sRolling.__placeholder0",
            "u_sRolling.__placeholder1",
            "u_sRolling.__placeholder2",
            "__placeholder7",
            "u_vFogColor",
            "u_vSkyColor",
            "u_vSunColor",
            "__placeholder8",
            "__placeholder9",
            "__placeholder10",
            "__placeholder11"
        };

    #endif // __CELLOS_LV2__

} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
    #pragma pack(pop)
#endif

#include "Core/ExportEnd.h"

