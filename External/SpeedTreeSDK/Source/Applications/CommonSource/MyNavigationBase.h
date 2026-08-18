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

#pragma once
#include "Core/Matrix.h"
#include "Core/FixedArray.h"
#include "Core/Random.h"
#include "Core/Timer.h"
#include "Utilities/Utility.h"

#if (defined(_WIN32) || defined(__GNUC__) || defined(__APPLE__)) && !defined(_XBOX) && !defined(__CELLOS_LV2__) && !defined(_DURANGO) && !defined(__ORBIS__)
    #ifdef _WIN32
        #define ST_WINDOWS // but not Xbox
    #endif
    #define ST_MOUSE_AND_KEYBOARD
#endif


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

    // constants
    const st_float32 c_fDefaultSpeedScalar = -1.0f;
    const st_float32 c_fTranslateFaster = 3.0f;
    const st_float32 c_fTranslateSlower = 0.15f;


    ///////////////////////////////////////////////////////////////////////  
    //  Forward references

    class CMyApplication;


    ///////////////////////////////////////////////////////////////////////  
    //  CMyNavigationBase

    class CMyNavigationBase  
    {
    public: 
            struct SCameraDesc
            {
                SCameraDesc(void) :
                    m_fAzimuth(0.0f),
                    m_fPitch(0.0f),
                    m_vLightDir(0.2f, -0.7f, -0.7f)
                {
                }

                Vec3                        m_vPos;         // in world space
                st_float32                  m_fAzimuth;     // in radians
                st_float32                  m_fPitch;       // in radians
                Vec3                        m_vLightDir;
            };

                                            CMyNavigationBase(CMyApplication* pApp);
    virtual                                 ~CMyNavigationBase( );

            // move the camera
            void                            MoveForward(st_float32 fAmount);
            void                            MoveStraightUp(st_float32 fAmount);
            void                            Strafe(st_float32 fAmount);
            void                            AdjustAzimith(st_float32 fAmount);
            void                            AdjustPitch(st_float32 fAmount);
            void                            SetPosition(const Vec3& vPos);
            void                            Tick(st_float32 fFrameTimeInSec);
            
            // speed control
            void                            SetSpeedScalars(st_float32 fTranslateSpeed = c_fDefaultSpeedScalar, st_float32 fRotateSpeed = c_fDefaultSpeedScalar);

            // preset cameras
            void                            OverwritePresetCamera(st_int32 nIndex);
            void                            GotoPresetCamera(st_int32 nIndex);
            st_bool                         SavePresetCameraFile(const CFixedString& strFilename);
            st_bool                         LoadPresetCameraFile(const CFixedString& strFilename);

            // queries
            const SCameraDesc&              GetCamera(void) const;
            const Mat4x4&                   GetTransform(void);
            CMyApplication*                 GetApp(void);
            st_float32                      GetLastFrameTime(void) const; // in seconds

            // demo mode (move the camera randomly)
            void                            DemoModeToggle(const st_float32 afTime[2], const st_float32 afSpeed[2]);
            st_bool                         IsDemoModeActive(void) const;
            void                            PickNextDemoCamera(void);

    private:
            void                            UpdateTransform(void);
            void                            AdvanceDemoCamera(st_float32 fFrameTimeInSec);

            // camera
            SCameraDesc                     m_sCamera;
            Mat4x4                          m_mTransform;           // matrix built from camera pos, azimuth, and pitch
            CFixedArray<SCameraDesc, 10>    m_aSavedCameras;
            st_int32                        m_nLastPresetCameraIndex;
            st_float32                      m_fLastFrameTimeInSec;

            // external constraints
            CMyApplication*                 m_pApp;

            // config
            st_float32                      m_fTranslateSpeed;
            st_float32                      m_fRotateSpeed;

            // demo mode
            st_bool                         m_bDemoMode;
            st_float32                      m_afDemoRandomTime[2];  // demo camera will stay on in location between [0] and [1] seconds
            st_float32                      m_afDemoRandomSpeed[2]; // demo camera will move with speed between [0] and [1] world units per second
            Vec3                            m_vDemoCameraPosVelocity;
            Vec3                            m_vLightDirVelocity;
            CRandom                         m_cDemoDice;
            st_float32                      m_fDemoCameraTargetDuration;
            CTimer                          m_cDemoTimer;
    };

} // end namespace SpeedTree
