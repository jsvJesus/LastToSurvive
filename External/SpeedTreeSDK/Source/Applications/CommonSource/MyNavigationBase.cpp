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

#include "MyNavigationBase.h"
#include "MyApplication.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::CMyNavBase

CMyNavigationBase::CMyNavigationBase(CMyApplication* pApp) :
    m_nLastPresetCameraIndex(0),
    m_fLastFrameTimeInSec(0.1f),
    m_pApp(pApp),
    m_fTranslateSpeed(0.2f),
    m_fRotateSpeed(0.002f),
    m_bDemoMode(false),
    m_vDemoCameraPosVelocity(1.0f, 0.0f, 0.0),
    m_cDemoDice(rand( )),
    m_fDemoCameraTargetDuration(1.0f)
{
    m_aSavedCameras.resize(10);

    m_afDemoRandomTime[0] = 8.0f;
    m_afDemoRandomTime[1] = 15.0f;
    m_afDemoRandomSpeed[0] = 1.0f;
    m_afDemoRandomSpeed[1] = 7.0f;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::~CMyNavBase

CMyNavigationBase::~CMyNavigationBase( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::MoveForward

void CMyNavigationBase::MoveForward(st_float32 fAmount)
{
    fAmount *= m_fTranslateSpeed;

    m_sCamera.m_vPos += CCoordSys::ConvertFromStd(fAmount * cosf(m_sCamera.m_fAzimuth) * cosf(m_sCamera.m_fPitch),
                                                  fAmount * sinf(m_sCamera.m_fAzimuth) * cosf(m_sCamera.m_fPitch),
                                                  fAmount * sinf(m_sCamera.m_fPitch));
    UpdateTransform( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::MoveStraightUp

void CMyNavigationBase::MoveStraightUp(st_float32 fAmount)
{
    fAmount *= m_fTranslateSpeed * 0.5f; // cut speed in half -- slower works better for this translation direction
    
    m_sCamera.m_vPos += CCoordSys::UpAxis( ) * fAmount;

    UpdateTransform( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::Strafe

void CMyNavigationBase::Strafe(st_float32 fAmount)
{
    fAmount *= m_fTranslateSpeed;

    Vec3 vDelta(CCoordSys::ConvertFromStd(fAmount * cosf(m_sCamera.m_fAzimuth + c_fHalfPi), 
                                          fAmount * sinf(m_sCamera.m_fAzimuth + c_fHalfPi), 
                                          0.0f));
    if (CCoordSys::IsLeftHanded( ))
        vDelta = -vDelta;

    m_sCamera.m_vPos += vDelta;

    UpdateTransform( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::AdjustAzimith

void CMyNavigationBase::AdjustAzimith(st_float32 fAmount)
{
    fAmount *= m_fRotateSpeed;

    const st_float32 c_fAzimuthDir = CCoordSys::IsLeftHanded( ) ? -1.0f : 1.0f;
    m_sCamera.m_fAzimuth += fAmount * c_fAzimuthDir;

    UpdateTransform( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::AdjustPitch

void CMyNavigationBase::AdjustPitch(st_float32 fAmount)
{
    fAmount *= m_fRotateSpeed;

    m_sCamera.m_fPitch += fAmount;

    UpdateTransform( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::SetPosition

void CMyNavigationBase::SetPosition(const Vec3& vPos)
{
    m_sCamera.m_vPos = vPos;

    UpdateTransform( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::SetSpeedScalars

void CMyNavigationBase::SetSpeedScalars(st_float32 fTranslateSpeed, st_float32 fRotateSpeed)
{
    if (fTranslateSpeed != -1.0f)
        m_fTranslateSpeed = fTranslateSpeed;

    if (fRotateSpeed != -1.0f)
        m_fRotateSpeed = fRotateSpeed;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::Tick

void CMyNavigationBase::Tick(st_float32 fFrameTimeInSec)
{
    if (IsDemoModeActive( ))
        AdvanceDemoCamera(fFrameTimeInSec);

    m_fLastFrameTimeInSec = fFrameTimeInSec;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::OverwritePresetCamera

void CMyNavigationBase::OverwritePresetCamera(st_int32 nIndex)
{
    if (nIndex > -1 && nIndex < st_int32(m_aSavedCameras.size( )))
    {
        m_aSavedCameras[nIndex] = m_sCamera;
        m_nLastPresetCameraIndex = nIndex;

        Report("Saved camera %d\n", nIndex);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::GotoPresetCamera

void CMyNavigationBase::GotoPresetCamera(st_int32 nIndex)
{
    if (nIndex > -1 && nIndex < st_int32(m_aSavedCameras.size( )))
    {
        m_sCamera = m_aSavedCameras[nIndex];
        m_nLastPresetCameraIndex = nIndex;

        UpdateTransform( );
    }
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::SavePresetCameraFile

st_bool CMyNavigationBase::SavePresetCameraFile(const CFixedString& strFilename)
{
    st_bool bSuccess = false;

    FILE* pFile = fopen(strFilename.c_str( ), "w");
    if (pFile)
    {
        for (CFixedArray<SCameraDesc>::const_iterator iter = m_aSavedCameras.begin( ); iter != m_aSavedCameras.end( ); ++iter)
        {
            Vec3 vPos = CCoordSys::ConvertToStd(iter->m_vPos);
            Vec3 vLightDir = CCoordSys::ConvertToStd(iter->m_vLightDir);
            fprintf(pFile, "%g %g %g %g %g %g %g %g\n", vPos[0], vPos[1], vPos[2], iter->m_fAzimuth, iter->m_fPitch, vLightDir.x, vLightDir.y, vLightDir.z);
        }

        fclose(pFile);

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::LoadPresetCameraFile

st_bool CMyNavigationBase::LoadPresetCameraFile(const CFixedString& strFilename)
{
    st_bool bSuccess = false;

    FILE* pFile = fopen(strFilename.c_str( ), "r");
    if (pFile)
    {
        m_aSavedCameras.clear( );
        while (!feof(pFile))
        {
            SCameraDesc sNew;
            if (fscanf(pFile, "%g %g %g %g %g %g %g %g", &sNew.m_vPos[0], &sNew.m_vPos[1], &sNew.m_vPos[2], &sNew.m_fAzimuth, &sNew.m_fPitch, &sNew.m_vLightDir.x, &sNew.m_vLightDir.y, &sNew.m_vLightDir.z) == 8)
            {
                sNew.m_vPos = CCoordSys::ConvertFromStd(sNew.m_vPos);
                sNew.m_vLightDir = CCoordSys::ConvertFromStd(sNew.m_vLightDir);
                m_aSavedCameras.push_back(sNew);
            }
        }
        fclose(pFile);
        
        // make sure we still have 10
        m_aSavedCameras.resize(10);

        bSuccess = true;
    }
    
    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::GetCamera

const CMyNavigationBase::SCameraDesc& CMyNavigationBase::GetCamera(void) const
{
    return m_sCamera;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::GetTransform

const Mat4x4& CMyNavigationBase::GetTransform(void)
{
    return m_mTransform;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::GetApp

CMyApplication* CMyNavigationBase::GetApp(void)
{
    return m_pApp;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::GetLastFrameTime

st_float32 CMyNavigationBase::GetLastFrameTime(void) const
{
    return m_fLastFrameTimeInSec;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::DemoModeToggle

void CMyNavigationBase::DemoModeToggle(const st_float32 afTime[2], const st_float32 afSpeed[2])
{
    m_bDemoMode = !m_bDemoMode;

    if (m_bDemoMode)
    {
        m_afDemoRandomTime[0] = afTime[0];
        m_afDemoRandomTime[1] = afTime[1];
        m_afDemoRandomSpeed[0] = afSpeed[0];
        m_afDemoRandomSpeed[1] = afSpeed[1];

        PickNextDemoCamera( );
    }
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::IsDemoMode

st_bool CMyNavigationBase::IsDemoModeActive(void) const
{
    return m_bDemoMode;
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::PickNextDemoCamera

void CMyNavigationBase::PickNextDemoCamera(void)
{
    // pick next camera
    st_int32 nNewCamera = m_nLastPresetCameraIndex;
    while (nNewCamera == m_nLastPresetCameraIndex)
        nNewCamera = m_cDemoDice.GetInteger(0, 9);

    GotoPresetCamera(nNewCamera);
    m_nLastPresetCameraIndex = nNewCamera;

    // pick random values
    m_vDemoCameraPosVelocity.Set(m_cDemoDice.GetFloat(-1.0f, 1.0f), m_cDemoDice.GetFloat(-1.0f, 1.0f), m_cDemoDice.GetFloat(-0.5f, 0.5f));
    m_vDemoCameraPosVelocity.Normalize( );
    m_vDemoCameraPosVelocity *= m_cDemoDice.GetFloat(m_afDemoRandomSpeed[0], m_afDemoRandomSpeed[1]);
    m_fDemoCameraTargetDuration = m_cDemoDice.GetFloat(m_afDemoRandomTime[0], m_afDemoRandomTime[1]);

    // pick light vector & compute velocity
    Vec3 vRandomLightDir(m_cDemoDice.GetFloat(-1.0f, 1.0f), m_cDemoDice.GetFloat(-1.0f, 1.0f), m_cDemoDice.GetFloat(-1.0f, -0.2f));
    vRandomLightDir.Normalize( );
    vRandomLightDir = CCoordSys::ConvertFromStd(vRandomLightDir);

    // cap the delta magnitude so we don't have big sweeping light changes each time
    Vec3 vDelta = vRandomLightDir - m_sCamera.m_vLightDir;

    const st_float32 c_fMaxMagnitude = 0.4f;
    st_float32 fDeltaMag = vDelta.Magnitude( );
    if (fDeltaMag > c_fMaxMagnitude)
        vDelta /= vDelta.Magnitude( ) * (1.0f / c_fMaxMagnitude);
    m_vLightDirVelocity = vDelta / m_fDemoCameraTargetDuration;

    // set up timer
    m_cDemoTimer.Start( );
    m_cDemoTimer.Mark(0);
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::UpdateTransform

void CMyNavigationBase::UpdateTransform(void)
{
    // adjust camera position
    if (m_pApp)
        m_sCamera.m_vPos = m_pApp->ConstrainCameraPos(m_sCamera.m_vPos);

    // adjust transform
    m_mTransform.SetIdentity( );

    // basically doing a polar-to-Cartesian conversion by getting an (x,y,z) target value from azimuth & pitch
    Vec3 vTarget = CCoordSys::ConvertFromStd(cosf(m_sCamera.m_fAzimuth) * cosf(m_sCamera.m_fPitch), sinf(m_sCamera.m_fAzimuth) * cosf(m_sCamera.m_fPitch), sinf(m_sCamera.m_fPitch));

    // add 90.0 degrees to the pitch angle to define the up vector and recalculate
    Vec3 vUp = CCoordSys::ConvertFromStd(cosf(m_sCamera.m_fAzimuth) * cosf(m_sCamera.m_fPitch + c_fHalfPi), sinf(m_sCamera.m_fAzimuth) * cosf(m_sCamera.m_fPitch + c_fHalfPi), sinf(m_sCamera.m_fPitch + c_fHalfPi));

    const Vec3 vOrigin;
    m_mTransform.LookAt(vOrigin, vTarget, vUp);

    m_mTransform.Translate(-m_sCamera.m_vPos.x, -m_sCamera.m_vPos.y, -m_sCamera.m_vPos.z);

    if (CCoordSys::IsLeftHanded( ))
    {
        m_mTransform.m_afRowCol[0][2] = -m_mTransform.m_afRowCol[0][2];
        m_mTransform.m_afRowCol[1][2] = -m_mTransform.m_afRowCol[1][2];
        m_mTransform.m_afRowCol[2][2] = -m_mTransform.m_afRowCol[2][2];
        m_mTransform.m_afRowCol[3][2] = -m_mTransform.m_afRowCol[3][2];
    }
}


///////////////////////////////////////////////////////////////////////  
//  CMyNavBase::AdvanceDemoCamera

void CMyNavigationBase::AdvanceDemoCamera(st_float32 fFrameTimeInSec)
{
    // jump to the next camera if time
    const st_float32 fCurrentTimeInSec = m_cDemoTimer.Stop( ) / 1000.0f;
    if (fCurrentTimeInSec >= m_fDemoCameraTargetDuration)
        PickNextDemoCamera( );

    // adjust camera
    m_sCamera.m_vPos += CCoordSys::ConvertFromStd(m_vDemoCameraPosVelocity * fFrameTimeInSec);
    UpdateTransform( );

    // adjust light
    m_sCamera.m_vLightDir += m_vLightDirVelocity * fFrameTimeInSec;
    m_sCamera.m_vLightDir = m_sCamera.m_vLightDir.Normalize( );
}
