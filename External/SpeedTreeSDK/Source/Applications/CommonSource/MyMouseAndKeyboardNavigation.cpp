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

#include "MyMouseAndKeyboardNavigation.h"
#include "MyApplication.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  Constants

// WASD-style navigation
const st_uchar c_chControlW = 23;
const st_uchar c_chControlA = 1;
const st_uchar c_chControlS = 19;
const st_uchar c_chControlD = 4;


///////////////////////////////////////////////////////////////////////  
//  CMyMouseAndKeyboardNavigation::CMyMouseAndKeyboardNavigation

CMyMouseAndKeyboardNavigation::CMyMouseAndKeyboardNavigation(CMyApplication* pApp) :
    CMyNavigationBase(pApp),
    m_nLastX(pApp->GetCmdLineOptions( ).m_nWindowWidth / 2),
    m_nLastY(pApp->GetCmdLineOptions( ).m_nWindowHeight / 2),
    m_bIgnoreNextMouseMotion(true),
    m_sLightDirAdjuster(CMyParamAdjuster::DIRECTION),
    m_sWindDirAdjuster(CMyParamAdjuster::DIRECTION),
    m_bHandTuningActive(false),
    m_bWkeyIsDown(false),
    m_bAkeyIsDown(false),
    m_bSkeyIsDown(false),
    m_bDkeyIsDown(false),
    m_bConstrainMouse(true)
{
    memset(m_abMouseButtonDown, 0, sizeof(m_abMouseButtonDown));

    // cameras
    const CMyConfigFile& sConfig = CMyNavigationBase::GetApp( )->GetConfig( );
    (void) CMyNavigationBase::LoadPresetCameraFile(sConfig.m_sNavigation.m_strCameraFilename);

    // speed scalars
    CMyNavigationBase::SetSpeedScalars(sConfig.m_sNavigation.m_fTranslateScalar, sConfig.m_sNavigation.m_fRotateScalar);

    // parameter adjusters
    m_sLightDirAdjuster.SetInitDir(CCoordSys::ConvertFromStd(sConfig.m_sLighting.m_vDirection));
    m_sWindDirAdjuster.SetInitDir(CCoordSys::ConvertFromStd(sConfig.m_sWind.m_vDirection));
    m_asHandTuneAdjusters[0].SetInitFloat(sConfig.m_sDemo.m_vHandTunedValues0.x, sConfig.m_sDemo.m_vHandTunedValues0.y, sConfig.m_sDemo.m_vHandTunedValues0.z, sConfig.m_sDemo.m_vHandTunedValues0.w);
    m_asHandTuneAdjusters[1].SetInitFloat(sConfig.m_sDemo.m_vHandTunedValues1.x, sConfig.m_sDemo.m_vHandTunedValues1.y, sConfig.m_sDemo.m_vHandTunedValues1.z, sConfig.m_sDemo.m_vHandTunedValues1.w);
    m_asHandTuneAdjusters[2].SetInitFloat(sConfig.m_sDemo.m_vHandTunedValues2.x, sConfig.m_sDemo.m_vHandTunedValues2.y, sConfig.m_sDemo.m_vHandTunedValues2.z, sConfig.m_sDemo.m_vHandTunedValues2.w);
    m_asHandTuneAdjusters[3].SetInitFloat(sConfig.m_sDemo.m_vHandTunedValues3.x, sConfig.m_sDemo.m_vHandTunedValues3.y, sConfig.m_sDemo.m_vHandTunedValues3.z, sConfig.m_sDemo.m_vHandTunedValues3.w);

    const Vec4 vHandTunedValues(m_asHandTuneAdjusters[0].m_fFloatValue, m_asHandTuneAdjusters[1].m_fFloatValue, m_asHandTuneAdjusters[2].m_fFloatValue, m_asHandTuneAdjusters[3].m_fFloatValue);
    GetApp( )->UpdateHandTunedValues(vHandTunedValues);
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseAndKeyboardNavigation::~CMyMouseAndKeyboardNavigation

CMyMouseAndKeyboardNavigation::~CMyMouseAndKeyboardNavigation( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseAndKeyboardNavigation::Tick

void CMyMouseAndKeyboardNavigation::Tick(st_float32 fFrameTimeInSec)
{
    CMyNavigationBase::Tick(fFrameTimeInSec);

    if (CMyNavigationBase::IsDemoModeActive( ))
    {
        // if demo mode is on, the light is likely being moved by CMyNavigationBase::Tick(), so 
        // copy it into the light adjuster and the main application
        m_sLightDirAdjuster.SetInitDir(CMyNavigationBase::GetCamera( ).m_vLightDir);
        GetApp( )->UpdateLightDir(CMyNavigationBase::GetCamera( ).m_vLightDir);
    }
    else
    {
        const st_bool bShiftKeyDown = GetKeyState(VK_SHIFT) < 0;
        const st_bool bControlKeyDown = GetKeyState(VK_CONTROL) < 0;
        const st_float32 c_fMoveDistance = fFrameTimeInSec * (bShiftKeyDown ? c_fTranslateFaster : 1.0f) * (bControlKeyDown ? c_fTranslateSlower : 1.0f);

        if (m_bWkeyIsDown)
            CMyNavigationBase::MoveForward(c_fMoveDistance);
        if (m_bAkeyIsDown)
            CMyNavigationBase::Strafe(c_fMoveDistance);
        if (m_bSkeyIsDown)
            CMyNavigationBase::MoveForward(-c_fMoveDistance);
        if (m_bDkeyIsDown)
            CMyNavigationBase::Strafe(-c_fMoveDistance);
    }
}

    
///////////////////////////////////////////////////////////////////////  
//  CMyMouseAndKeyboardNavigation::MouseClick

void CMyMouseAndKeyboardNavigation::MouseClick(EMouseButton eButton, st_bool bPressed, st_int32 x, st_int32 y)
{
    m_abMouseButtonDown[eButton] = bPressed;

    // toggle WASD navigation mouse constraint?
    if (m_abMouseButtonDown[MOUSE_BUTTON_LEFT])
        m_bConstrainMouse = !m_bConstrainMouse;

    // skip to next camera in demo mode?
    if (m_abMouseButtonDown[MOUSE_BUTTON_LEFT] && CMyNavigationBase::IsDemoModeActive( ))
        CMyNavigationBase::PickNextDemoCamera( );

    m_nLastX = x;
    m_nLastY = y;
}


///////////////////////////////////////////////////////////////////////  
//  ScreenToWindowPos

POINT ScreenToWindowPos(const POINT& sScreenPos)
{
    POINT sWindowPos;
    sWindowPos.x = sScreenPos.x;
    sWindowPos.y = sScreenPos.y;

    (void) ScreenToClient(GetActiveWindow( ), &sWindowPos);

    return sWindowPos;
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseAndKeyboardNavigation::MouseMotion

void CMyMouseAndKeyboardNavigation::MouseMotion(st_int32 x, st_int32 y)
{
    // query/computer center of window
    RECT sWindowGeometry;
    GetWindowRect(GetActiveWindow( ), &sWindowGeometry);
    POINT sCenterInScreenCoords;
    sCenterInScreenCoords.x = (sWindowGeometry.left + sWindowGeometry.right) / 2;
    sCenterInScreenCoords.y = (sWindowGeometry.bottom + sWindowGeometry.top) / 2;

    // convert to center in window coordinates
    POINT sCenterInWindowCoords = ScreenToWindowPos(sCenterInScreenCoords);

    // determine if the SetCursorPos() event at the bottom of this function caused this MouseMotion() event
    m_bIgnoreNextMouseMotion = (x == sCenterInWindowCoords.x && y == sCenterInWindowCoords.y);

    st_bool bKeepCursorCentered = false;
    if (!m_bIgnoreNextMouseMotion && !CMyNavigationBase::IsDemoModeActive( ))
    {
        // there are a number of things that might be going on when the
        // mouse moves -- check for each condition here

        // adjusting hand-tuned values
        if (m_bHandTuningActive)
        {
            const st_int32 c_nNumHandTuners = 4;
            for (st_int32 i = 0; i < c_nNumHandTuners; ++i)
                if (m_asHandTuneAdjusters[i].IsActive( ))
                    m_asHandTuneAdjusters[i].Move(x, y);

            const Vec4 vHandTunedValues(m_asHandTuneAdjusters[0].m_fFloatValue, m_asHandTuneAdjusters[1].m_fFloatValue, m_asHandTuneAdjusters[2].m_fFloatValue, m_asHandTuneAdjusters[3].m_fFloatValue);
            GetApp( )->UpdateHandTunedValues(vHandTunedValues);
        }
        // adjusting the light dir?
        else if (m_sLightDirAdjuster.IsActive( ))
        {
            m_sLightDirAdjuster.Move(x, y);
            GetApp( )->UpdateLightDir(m_sLightDirAdjuster.GetDir( ));
        }
        // adjusting the wind dir?
        else if (m_sWindDirAdjuster.IsActive( ))
        {
            m_sWindDirAdjuster.Move(x, y);
            GetApp( )->UpdateWindDir(m_sWindDirAdjuster.GetDir( ));
        }
        // translate straight up and down?
        else if (m_abMouseButtonDown[MOUSE_BUTTON_RIGHT])
        {
            const st_bool bShiftKeyDown = GetKeyState(VK_SHIFT) < 0;
            const st_bool bControlKeyDown = GetKeyState(VK_CONTROL) < 0;
            const st_float32 c_fMoveDistance = CMyNavigationBase::GetLastFrameTime( ) * (bShiftKeyDown ? c_fTranslateFaster : 1.0f) * (bControlKeyDown ? c_fTranslateSlower : 1.0f);

            CMyNavigationBase::MoveStraightUp(c_fMoveDistance * st_float32(m_nLastY - y));
            bKeepCursorCentered = true;
        }
        // or regular WASD navigation?
        else
        {
            if (m_bConstrainMouse)
            {
                CMyNavigationBase::AdjustAzimith(st_float32(m_nLastX - x));
                CMyNavigationBase::AdjustPitch(st_float32(m_nLastY - y));

                bKeepCursorCentered = true;
            }
        }
    }
    else
        m_bIgnoreNextMouseMotion = false;

    #if defined(_WIN32)
        if (bKeepCursorCentered)
        {
            POINT sCursorPos;
            GetCursorPos(&sCursorPos);

            if (sCursorPos.x != sCenterInScreenCoords.x || sCursorPos.y != sCenterInScreenCoords.y)
            {
                SetCursorPos(sCenterInScreenCoords.x, sCenterInScreenCoords.y);
                m_bIgnoreNextMouseMotion = true;
            }
        }
    #endif

    m_nLastX = x;
    m_nLastY = y;
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseAndKeyboardNavigation::KeyDown

void CMyMouseAndKeyboardNavigation::KeyDown(st_uchar chKey)
{
    const CMyConfigFile& sConfig = CMyNavigationBase::GetApp( )->GetConfig( );

    if (chKey >= '0' && chKey <= '9')
    {
        st_bool bControlKeyDown = GetKeyState(VK_CONTROL) < 0;
        st_bool bShiftKeyDown = GetKeyState(VK_SHIFT) < 0;

        // save preset camera position?
        if (bShiftKeyDown)
        {
            CMyNavigationBase::OverwritePresetCamera(chKey - '0');
        }
        // hand tune adjustment?
        else if (bControlKeyDown && chKey >= '1' && chKey <= '4')
        {
            m_asHandTuneAdjusters[chKey - '1'].Start(m_nLastX, m_nLastY);
            m_bHandTuningActive = true;
        }
        // jump to preset camera?
        else
        {
            CMyNavigationBase::GotoPresetCamera(chKey - '0');
            m_sLightDirAdjuster.SetInitDir(CMyNavigationBase::GetCamera( ).m_vLightDir);
            GetApp( )->UpdateLightDir(CMyNavigationBase::GetCamera( ).m_vLightDir);
        }
    }
    else
    {
        switch (chKey)
        {

        // toggle terrain following
        case ' ': // spacebar
            GetApp( )->ToggleFeature(CMyApplication::TOGGLE_TERRAIN_FOLLOWING);
            break;

        // toggle billboard tree visibility (3D trees stay on)
        case 'b': case 'B':
            GetApp( )->ToggleFeature(CMyApplication::TOGGLE_BILLBOARD_VISIBILITY);
            break;

        // toggle collision
        case 'c': case 'C':
            GetApp( )->ToggleFeature(CMyApplication::TOGGLE_COLLISION);
            break;

        // toggle overlay visibility
        case 'e': case 'E': 
            GetApp( )->ToggleFeature(CMyApplication::TOGGLE_OVERLAY_VISIBILITY);
            break;

        // wind direction adjustment
        case 'f': case 'F':
            m_sWindDirAdjuster.Start(m_nLastX, m_nLastY);
            break;

        // toggle grass visibility
        case 'g': case 'G':
            GetApp( )->ToggleFeature(CMyApplication::TOGGLE_GRASS_VISIBILITY);
            break;

        // toggle depth blur
        case 'j': case 'J':
            GetApp( )->ToggleFeature(CMyApplication::TOGGLE_DEPTH_BLUR);
            break;

        // toggle terrain visibility ('L'and)
        case 'l': case 'L':
            GetApp( )->ToggleFeature(CMyApplication::TOGGLE_TERRAIN_VISIBILITY);
            break;

        // rotate bloom display mode
        case 'o': case 'O':
            GetApp( )->ToggleFeature(CMyApplication::TOGGLE_BLOOM_DISPLAY_MODE);
            break;

        // print resource stats to console if active
        case 'q': case 'Q': 
            GetApp( )->ReportResourceUsage( );
            break;

        // toggle demo navigation mode
        case 'r': case 'R': 
            CMyNavigationBase::DemoModeToggle(sConfig.m_sDemo.m_afCameraTime, sConfig.m_sDemo.m_afCameraSpeed);
            break;

        // toggle 3D tree visibility (billboards stay on)
        case 't': case 'T':
            GetApp( )->ToggleFeature(CMyApplication::TOGGLE_3D_TREE_VISIBILITY);
            break;

        // toggle texturing
        case 'u': case 'U':
            GetApp( )->ToggleFeature(CMyApplication::TOGGLE_TEXTURING);
            break;

        // light direction adjustment
        case 'v': case 'V':
            m_sLightDirAdjuster.Start(m_nLastX, m_nLastY);
            break;

        // print frame CPU profile to console if active
        case 'x': case 'X':
            {
            #if !defined(ST_FALLBACK_TIMING) && !defined(ST_INTEL_GPA)
                Report("#define ST_FALLBACK_TIMING or ST_INTEL_GPA in ScopeTrace.h to activate built-in profiling");
            #else
                CString strReport;
                CScopeTrace::Report(CScopeTrace::FORMAT_PRINT, strReport);
                printf("%s\n", strReport.c_str( ));
            #endif
            }
            break;

        // toggle sky visibility
        case 'y': case 'Y':
            GetApp( )->ToggleFeature(CMyApplication::TOGGLE_SKY_VISIBILITY);
            break;

        }

        // WASD-style navigation
        switch (chKey)
        {
        case 'w': case 'W': 
            m_bWkeyIsDown = true;
            break;
        case 'a': case 'A': 
            m_bAkeyIsDown = true;
            break;
        case 's': case 'S': 
            m_bSkeyIsDown = true;
            break;
        case 'd': case 'D': 
            m_bDkeyIsDown = true;
            break;
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  CMyMouseAndKeyboardNavigation::KeyUp

void CMyMouseAndKeyboardNavigation::KeyUp(st_uchar chKey)
{
    st_bool bControlKeyDown = GetKeyState(VK_CONTROL) < 0;

    // parameter adjusters
    switch (chKey)
    {
    case '1': case '2': case '3': case '4':
        if (bControlKeyDown)
        {
            m_asHandTuneAdjusters[chKey - '1'].End( );
            m_bHandTuningActive = false;
        }
        break;
    case 'f': case 'F':
        m_sWindDirAdjuster.End( );
        break;
    case 'v': case 'V':
        m_sLightDirAdjuster.End( );
        break;
    }

    // WASD-style navigation
    switch (chKey)
    {
    case 'w': case 'W': 
        m_bWkeyIsDown = false;
        break;
    case 'a': case 'A': 
        m_bAkeyIsDown = false;
        break;
    case 's': case 'S': 
        m_bSkeyIsDown = false;
        break;
    case 'd': case 'D': 
        m_bDkeyIsDown = false;
        break;
    }
}

