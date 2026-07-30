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
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CStateBlockOpenGL::CStateBlockOpenGL

CStateBlockOpenGL::CStateBlockOpenGL( ) :
    m_bAlphaToCoverageSupported(false),
    m_uiDisplayList(0)
{
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockOpenGL::Init

st_bool CStateBlockOpenGL::Init(const SAppState& sAppState, const SRenderState& sRenderState)
{
    m_sAppState = sAppState;
    m_sRenderState = sRenderState;

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockOpenGL::Bind

st_bool CStateBlockOpenGL::Bind(void) const
{
    st_bool bSuccess = true;

    // blending
    if (m_sRenderState.m_bBlending)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
        glDisable(GL_BLEND);
    
    // color mask
    if (m_sRenderState.m_eRenderPass != RENDER_PASS_MAIN)
       glColorMask(false, false, false, true);
    else
        glColorMask(true, true, true, true);
    
    // depth mask & testing function
    if (m_sAppState.m_eOverrideDepthTest == SAppState::OVERRIDE_DEPTH_TEST_DISABLE)
        glDisable(GL_DEPTH_TEST);
    else
        glEnable(GL_DEPTH_TEST);

    if (!m_sAppState.m_bDepthPrepass)
    {
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }
    else
    {
        if (m_sRenderState.m_eRenderPass == RENDER_PASS_DEPTH_PREPASS)
        {
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
        }
        else
        {
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_EQUAL);
        }
    }
    
    // face culling
    if (m_sRenderState.m_eFaceCulling == CULLTYPE_NONE)
        glDisable(GL_CULL_FACE);
    else 
    {
        glEnable(GL_CULL_FACE);
        if (m_sRenderState.m_eFaceCulling == CULLTYPE_BACK)
            glCullFace(GL_BACK);
        else
            glCullFace(GL_FRONT);
    }
    
    // polygon offset
    if (m_sRenderState.m_eRenderPass == RENDER_PASS_SHADOW_CAST)
    {
        // values carefully tuned for SpeedTree reference app, but will likely need to change for
        // other applications
        const st_float32 c_fFactor = (m_sRenderState.m_eFaceCulling == CULLTYPE_NONE) ? 10.0f : 4.0f;
        const st_float32 c_fUnits = (m_sRenderState.m_eFaceCulling == CULLTYPE_NONE) ? 1.0f: 8.0f;
        
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(c_fFactor, c_fUnits);
    }
    else
    {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    
    // multisampling
    if (m_sAppState.m_bMultisampling)
        glEnable(GL_MULTISAMPLE);
    else
        glDisable(GL_MULTISAMPLE);
    
    // alpha-to-coverage
    if (glewIsExtensionSupported("GL_ARB_multisample") == GL_TRUE)
    {
        if (m_sAppState.m_bAlphaToCoverage)
            glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        else
            glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CStateBlockOpenGL::ReleaseGfxResources

void CStateBlockOpenGL::ReleaseGfxResources(void)
{
}
