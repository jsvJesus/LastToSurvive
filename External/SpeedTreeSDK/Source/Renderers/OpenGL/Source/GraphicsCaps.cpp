///////////////////////////////////////////////////////////////////////
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

#include "Renderers/OpenGL/OpenGLRenderer.h"
#include "Utilities/glew.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//  Static variables

st_bool CGraphicsCapsOpenGL::m_bGlewInitialized = false;


///////////////////////////////////////////////////////////////////////
//  CGraphicsCapsOpenGL::InitGlew

GLenum CGraphicsCapsOpenGL::InitGlew(void)
{
    GLenum eReturn = GLEW_OK;

    if (!m_bGlewInitialized)
    {
        eReturn = glewInit( );
        m_bGlewInitialized = (eReturn == GLEW_OK);
    }
    
    return eReturn;
}


///////////////////////////////////////////////////////////////////////
//  CGraphicsCapsOpenGL::IsInstancingSupported

st_bool CGraphicsCapsOpenGL::IsInstancingSupported(void)
{
    st_bool bInstancingSupported = false;

    if (InitGlew( ))
        bInstancingSupported = (glDrawElementsInstanced != NULL);

    return bInstancingSupported;
}



///////////////////////////////////////////////////////////////////////
//  CGraphicsCapsOpenGL::IsGlsl150OrHigher

st_bool CGraphicsCapsOpenGL::IsGlsl150OrHigher(void)
{
    st_bool bIsGlsl150OrHigher = false;
    
    // test to see if an OpenGL context has been created and glew has been initialized
    GLenum eReturn = GLEW_OK;
    if (!m_bGlewInitialized)
        eReturn = glewInit( );
    
    if (eReturn == GLEW_OK)
    {
        m_bGlewInitialized = true;
        
        // parse the GLSL version string to determine numeric version
        CFixedString strVersion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
        size_t uiPeriod = strVersion.find('.', 0);
        uiPeriod = strVersion.find('.', uiPeriod + 1);
        if (uiPeriod != CFixedString::npos)
            strVersion[uiPeriod] = ' ';
        st_float32 fVersion = st_float32(atof(strVersion.c_str( )));

        bIsGlsl150OrHigher = (fVersion >= 1.5f);
    }
    
    return bIsGlsl150OrHigher;
}


