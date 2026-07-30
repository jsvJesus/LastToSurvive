///////////////////////////////////////////////////////////////////////  
//
//  All source files prefixed with "My" indicate an example implementation, 
//  meant to detail what an application might (and, in most cases, should)
//  do when interfacing with the SpeedTree SDK.  These files constitute 
//  the bulk of the SpeedTree Reference Application and are not part
//  of the official SDK, but merely example or "reference" implementations.
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2016 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#include "MyGamePadNavigation.h"
#include "MyApplication.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CMyGamePadNavigation::CMy360GamePadNavigation

CMyGamePadNavigation::CMyGamePadNavigation(CMyApplication* pApp) :
    CMyNavigationBase(pApp),
    m_sLightDirAdjuster(CMyParamAdjuster::DIRECTION),
    m_bLeftTrigger(false),
    m_bRightTrigger(false),
    m_pApp(NULL),
    m_nPresetCamera(0)
{
    // copy parameter(s)
    assert(pApp);
    m_pApp = pApp;

    // cameras
    const CMyConfigFile& sConfig = CMyNavigationBase::GetApp( )->GetConfig( );
    (void) CMyNavigationBase::LoadPresetCameraFile(sConfig.m_sNavigation.m_strCameraFilename);

    // speed scalars
    CMyNavigationBase::SetSpeedScalars(sConfig.m_sNavigation.m_fTranslateScalar, sConfig.m_sNavigation.m_fRotateScalar);

    // parameter adjusters
    m_sLightDirAdjuster.SetInitDir(CCoordSys::ConvertFromStd(sConfig.m_sLighting.m_vDirection));
}


///////////////////////////////////////////////////////////////////////  
//  CMyGamePadNavigation::~CMyGamePadNavigation

CMyGamePadNavigation::~CMyGamePadNavigation( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyGamePadNavigation::Tick

void CMyGamePadNavigation::Tick(st_float32 fFrameTimeInSec)
{
    CMyNavigationBase::Tick(fFrameTimeInSec);

    if (CMyNavigationBase::IsDemoModeActive( ))
    {
        // if demo mode is on, the light is likely being moved by CMyNavigationBase::Tick(), so 
        // copy it into the light adjuster and the main application
        m_sLightDirAdjuster.SetInitDir(CMyNavigationBase::GetCamera( ).m_vLightDir);
        GetApp( )->UpdateLightDir(CMyNavigationBase::GetCamera( ).m_vLightDir);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CMyGamePadNavigation::Sticks

void CMyGamePadNavigation::Sticks(const Vec2& vLeft, const Vec2& vRight)
{
    // platform-specific speed adjustments
    #ifdef _XBOX
        const st_float32 c_fPlatformTranslateScalar = 2.0f;
        const st_float32 c_fPlatformRotateScalar = 400.0f;
        const st_float32 c_fPlatformLightScalarX = 2.5f;
        const st_float32 c_fPlatformLightScalarY = 1.25f;
    #elif defined(_DURANGO)
        const st_float32 c_fPlatformTranslateScalar = 2.0f;
        const st_float32 c_fPlatformRotateScalar = 400.0f;
        const st_float32 c_fPlatformLightScalarX = 2.5f;
        const st_float32 c_fPlatformLightScalarY = 1.25f;
    #elif defined(__CELLOS_LV2__)
        const st_float32 c_fPlatformTranslateScalar = 2.0f;
        const st_float32 c_fPlatformRotateScalar = 400.0f;
        const st_float32 c_fPlatformLightScalarX = 2.5f;
        const st_float32 c_fPlatformLightScalarY = 1.25f;
    #elif defined(__ORBIS__)
        const st_float32 c_fPlatformTranslateScalar = 2.0f;
        const st_float32 c_fPlatformRotateScalar = 400.0f;
        const st_float32 c_fPlatformLightScalarX = 2.5f;
        const st_float32 c_fPlatformLightScalarY = 1.25f;
    #endif

    // adjust light direction
    if (m_bLeftTrigger)
    {
        if (fabs(vRight.x) > fabs(vRight.y))
            m_sLightDirAdjuster.m_fHorzAngle += vRight.x * c_fPlatformLightScalarX;
        else
            m_sLightDirAdjuster.m_fVertAngle += vRight.y * c_fPlatformLightScalarY;

        m_sLightDirAdjuster.ComputeDirection( );
        m_pApp->UpdateLightDir(m_sLightDirAdjuster.GetDir( ));
    }
    // main navigation
    else
    {
        // right-trigger speed up
        const st_float32 c_fSpeedScalar = m_bRightTrigger ? c_fTranslateFaster : 1.0f;

        CMyNavigationBase::MoveForward(vLeft.y * c_fPlatformTranslateScalar * c_fSpeedScalar);
        CMyNavigationBase::Strafe(vLeft.x * c_fPlatformTranslateScalar * c_fSpeedScalar);
        CMyNavigationBase::AdjustAzimith(vRight.x * c_fPlatformRotateScalar);
        CMyNavigationBase::AdjustPitch(vRight.y * c_fPlatformRotateScalar);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CMyGamePadNavigation::ButtonPressed

void CMyGamePadNavigation::ButtonPressed(EButton eButton)
{
    switch (eButton)
    {
    case BUTTON_A_OR_X:
        m_pApp->ToggleFeature(CMyApplication::TOGGLE_3D_TREE_VISIBILITY);
        break;
    case BUTTON_X_OR_SQUARE:
        m_pApp->ToggleFeature(CMyApplication::TOGGLE_GRASS_VISIBILITY);
        break;
    case BUTTON_Y_OR_TRIANGLE:
        m_pApp->ToggleFeature(CMyApplication::TOGGLE_BILLBOARD_VISIBILITY);
        break;
    case BUTTON_DIR_UP:
        break;
    case BUTTON_DIR_DOWN:
        break;
    case BUTTON_DIR_RIGHT:
        if (++m_nPresetCamera == 10)
            m_nPresetCamera = 0;
        CMyNavigationBase::GotoPresetCamera(m_nPresetCamera);
        break;
    case BUTTON_DIR_LEFT:
        if (--m_nPresetCamera == -1)
            m_nPresetCamera = 9;
        CMyNavigationBase::GotoPresetCamera(m_nPresetCamera);
        break;
    case BUTTON_RIGHT_SHOULDER:
        break;
    case BUTTON_LEFT_STICK:
        break;
    case BUTTON_RIGHT_STICK:
        break;
    #ifdef MY_TERRAIN_ACTIVE
        case BUTTON_B_OR_CIRCLE:
            m_pApp->ToggleFeature(CMyApplication::TOGGLE_TERRAIN_VISIBILITY);
            break;
        case BUTTON_LEFT_SHOULDER:
            m_pApp->ToggleFeature(CMyApplication::TOGGLE_TERRAIN_FOLLOWING);
            break;
    #endif
    }
}


///////////////////////////////////////////////////////////////////////  
//  CMyGamePadNavigation::Triggers

void CMyGamePadNavigation::Triggers(st_bool bLeft, st_bool bRight)
{
    m_bLeftTrigger = bLeft;
    m_bRightTrigger = bRight;
}

