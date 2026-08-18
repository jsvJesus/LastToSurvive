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
    //  Enumeration EMouseButton

    enum EMouseButton
    {
        MOUSE_BUTTON_LEFT, MOUSE_BUTTON_MIDDLE, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_COUNT
    };


    ///////////////////////////////////////////////////////////////////////  
    //  Forward references

    class CMyApplication;
    class CMyConfigFile;


    ///////////////////////////////////////////////////////////////////////  
    //  Class CMyMouseAndKeyboardNavigation
    //
    //  Geared mostly for terrain-style navigation, not object inspection

    class CMyMouseAndKeyboardNavigation : public CMyNavigationBase  
    {
    public: 
                                    CMyMouseAndKeyboardNavigation(CMyApplication* pApp);
                                    ~CMyMouseAndKeyboardNavigation( );

            void                    Tick(st_float32 fFrameTimeInSec);

            // mouse input
            void                    MouseClick(EMouseButton eButton, st_bool bPressed, st_int32 x, st_int32 y);
            void                    MouseMotion(st_int32 x, st_int32 y);

            // keyboard input
            void                    KeyDown(st_uchar chKey);
            void                    KeyUp(st_uchar chKey);

    private:
            // mouse mechanics
            st_int32                m_nLastX;
            st_int32                m_nLastY;
            st_bool                 m_abMouseButtonDown[MOUSE_BUTTON_COUNT];
            st_bool                 m_bIgnoreNextMouseMotion;

            // parameter adjusters
            CMyParamAdjuster        m_sLightDirAdjuster;
            CMyParamAdjuster        m_sWindDirAdjuster;
            CMyParamAdjuster        m_asHandTuneAdjusters[4];
            st_bool                 m_bHandTuningActive;

            // WASD-style navigation
            st_bool                 m_bWkeyIsDown;
            st_bool                 m_bAkeyIsDown;
            st_bool                 m_bSkeyIsDown;
            st_bool                 m_bDkeyIsDown;
            st_bool                 m_bConstrainMouse;
    };

} // end namespace SpeedTree
