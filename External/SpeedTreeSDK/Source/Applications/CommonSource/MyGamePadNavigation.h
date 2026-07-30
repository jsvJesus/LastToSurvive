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
#include "MyNavigationBase.h"
#include "MyParamAdjuster.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

    ///////////////////////////////////////////////////////////////////////  
    //  Forward references

    class CMyApplication;
    class CMyConfigFile;


    ///////////////////////////////////////////////////////////////////////  
    //  Class CMyGamePadNavigation
    //
    //  Geared mostly for terrain-style navigation, not object inspection

    class CMyGamePadNavigation : public CMyNavigationBase  
    {
    public:
            enum EButton
            {
                BUTTON_A_OR_X,
                BUTTON_B_OR_CIRCLE,
                BUTTON_X_OR_SQUARE,
                BUTTON_Y_OR_TRIANGLE,
                BUTTON_DIR_UP,
                BUTTON_DIR_DOWN,
                BUTTON_DIR_RIGHT,
                BUTTON_DIR_LEFT,
                BUTTON_LEFT_SHOULDER,
                BUTTON_RIGHT_SHOULDER,
                BUTTON_LEFT_STICK,
                BUTTON_RIGHT_STICK
            };

                                    CMyGamePadNavigation(CMyApplication* pApp);
                                    ~CMyGamePadNavigation( );

            void                    Tick(st_float32 fFrameTimeInSec);

            // input
            void                    Sticks(const Vec2& vLeft, const Vec2& vRight);
            void                    ButtonPressed(EButton eButton);
            void                    Triggers(st_bool bLeft, st_bool bRight);

    private:
            // parameter adjusters
            CMyParamAdjuster        m_sLightDirAdjuster;

            // pad state
            st_bool                 m_bLeftTrigger;
            st_bool                 m_bRightTrigger;

            // other
            CMyApplication*         m_pApp;
            st_int32                m_nPresetCamera;
    };

} // end namespace SpeedTree
