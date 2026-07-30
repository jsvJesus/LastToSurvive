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
//  CRenderTargetOpenGL::CRenderTargetOpenGL

CRenderTargetOpenGL::CRenderTargetOpenGL( ) :
    m_eType(RENDER_TARGET_TYPE_COLOR),
    m_nWidth(-1),
    m_nHeight(-1),
    m_nNumSamples(1),
    m_uiFbo(0),
    m_uiRenderBuffer(0),
    m_uiRenderTexture(0)
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOpenGL::~CRenderTargetOpenGL

CRenderTargetOpenGL::~CRenderTargetOpenGL( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOpenGL::InitGfx

st_bool CRenderTargetOpenGL::InitGfx(ERenderTargetType eType, st_int32 nWidth, st_int32 nHeight, st_int32 nNumSamples)
{
    st_bool bSuccess = false;

    if (nWidth > 0 && nHeight > 0)
    {
        // copy parameters
        m_eType = eType;
        m_nWidth = nWidth;
        m_nHeight = nHeight;
        m_nNumSamples = nNumSamples;

        // local vars
        const st_bool c_bMSAA = (m_nNumSamples > 1);

        // create the texture object
        glGenTextures(1, &m_uiRenderTexture);

        // different types of textures
        if (eType == RENDER_TARGET_TYPE_COLOR)
        {
            glBindTexture(c_bMSAA ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, m_uiRenderTexture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            if (c_bMSAA)
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, nNumSamples, GL_RGBA, nWidth, nHeight, GL_FALSE);
            else
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, GL_FALSE);
        }
        else if (eType == RENDER_TARGET_TYPE_DEPTH)
        {
            glBindTexture(c_bMSAA ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, m_uiRenderTexture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            #ifndef ST_OPENGL_USE_CORE_3_2
                glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
            #endif

            if (c_bMSAA)
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, nNumSamples, GL_DEPTH_COMPONENT24, nWidth, nHeight, GL_FALSE);
            else
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_nWidth, m_nHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
        }
        else // assume RENDER_TARGET_TYPE_SHADOW_MAP
        {
            glBindTexture(GL_TEXTURE_2D, m_uiRenderTexture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            #ifndef ST_OPENGL_USE_CORE_3_2
                glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_RED);
            #endif

            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, nWidth, nHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, GL_FALSE);
        }

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOpenGL::ReleaseGfxResources

void CRenderTargetOpenGL::ReleaseGfxResources(void)
{
    if (m_uiFbo != 0)
    {
        glDeleteFramebuffersEXT(1, &m_uiFbo);
        m_uiFbo = 0;
    }
    if (m_uiRenderBuffer != 0)
    {
        glDeleteRenderbuffersEXT(1, &m_uiRenderBuffer);
        m_uiRenderBuffer = 0;
    }
    if (m_uiRenderTexture != 0)
    {
        glDeleteTextures(1, &m_uiRenderTexture);
        m_uiRenderTexture = 0;
    }
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOpenGL::Clear

void CRenderTargetOpenGL::Clear(const Vec4& vColor)
{
    ST_UNREF_PARAM(vColor);

    glColorMask(true, true, true, true);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOpenGL::SetAsTarget

st_bool CRenderTargetOpenGL::SetAsTarget(void)
{
    const CRenderTargetOpenGL* pTarget = this;

    return SetGroupAsTarget(&pTarget, 1);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOpenGL::ReleaseAsTarget

void CRenderTargetOpenGL::ReleaseAsTarget(void)
{
    const CRenderTargetOpenGL* pTarget = this;

    ReleaseGroupAsTarget(&pTarget, 1);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOpenGL::BindAsTexture

st_bool CRenderTargetOpenGL::BindAsTexture(st_int32 nRegister, st_bool bPointFilter) const
{
    ST_UNREF_PARAM(bPointFilter);

    glActiveTextureARB(GL_TEXTURE0_ARB + nRegister);

    if (m_eType != RENDER_TARGET_TYPE_SHADOW_MAP)
        glBindTexture(m_nNumSamples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, m_uiRenderTexture);
    else
        glBindTexture(GL_TEXTURE_2D, m_uiRenderTexture);

    return (m_uiRenderTexture != 0);
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOpenGL::UnBindAsTexture

void CRenderTargetOpenGL::UnBindAsTexture(st_int32 nRegister) const
{
    ST_UNREF_PARAM(nRegister);

    // intentionally blank
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOpenGL::GetFbo

GLuint CRenderTargetOpenGL::GetFbo(void) const
{
    return m_uiFbo;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOpenGL::OnResetDevice

void CRenderTargetOpenGL::OnResetDevice(void)
{
    // intentionally blank
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOpenGL::OnLostDevice

void CRenderTargetOpenGL::OnLostDevice(void)
{
    // intentionally blank
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOpenGL::SetGroupAsTarget

st_bool CRenderTargetOpenGL::SetGroupAsTarget(const CRenderTargetOpenGL** pTargets, st_int32 nNumTargets)
{
    st_bool bSuccess = false;

    assert(pTargets);
    assert(nNumTargets <= c_nMaxNumRenderTargets);

    if (nNumTargets > 0)
    {
        bSuccess = true;

        // determine if this is a shadow map
        st_bool c_bShadowMap = (nNumTargets == 1 && pTargets[0]->m_eType == RENDER_TARGET_TYPE_SHADOW_MAP);

        // arbitrarily, the first target will house the shared FBO
        CRenderTargetOpenGL* pPrimaryTarget = const_cast<CRenderTargetOpenGL*>(pTargets[0]);

        // if the framebuffer hasn't been created yet, create it and bind the renderbuffers to it (once)
        if (pPrimaryTarget->m_uiFbo == 0)
        {
            // generate & bind the FBO
            glGenFramebuffersEXT(1, &pPrimaryTarget->m_uiFbo);
            assert(pPrimaryTarget->m_uiFbo);
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pPrimaryTarget->m_uiFbo);

            // run through each render target, attaching to FBO
            GLenum eColorAttachment = GL_COLOR_ATTACHMENT0_EXT;
            for (st_int32 i = 0; i < nNumTargets; ++i)
            {
                const CRenderTargetOpenGL* pTarget = pTargets[i];
                assert(pTarget);

                if (pTarget->m_eType == RENDER_TARGET_TYPE_COLOR)
                {
                    // attach texture to framebuffer
                    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, eColorAttachment, (pPrimaryTarget->m_nNumSamples > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, pTarget->m_uiRenderTexture, 0);

                    ++eColorAttachment;
                }
                else // RENDER_TARGET_TYPE_DEPTH or RENDER_TARGET_TYPE_SHADOWMAP
                {
                    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, (pPrimaryTarget->m_nNumSamples > 1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, pTarget->m_uiRenderTexture, 0);
                }
            }

            // verify that framebuffers are supported and correctly initialized
            GLenum eStatus = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
            if (eStatus == GL_FRAMEBUFFER_UNSUPPORTED_EXT)
            {
                CCore::SetError("CRenderTargetOpenGL::SetGroupAsTarget(): Framebuffer configuration not supported");
                bSuccess = false;
            }
            else if (eStatus != GL_FRAMEBUFFER_COMPLETE_EXT)
            {
                CCore::SetError("CRenderTargetOpenGL::SetGroupAsTarget(): Failed to create framebuffer [error code: %d]", eStatus);
                bSuccess = false;
            }
        }
        else
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, pPrimaryTarget->m_uiFbo);

        if (bSuccess)
        {
            if (!c_bShadowMap)
            {
                // build buffer enum array
                GLenum aeBuffers[c_nMaxNumRenderTargets];
                GLenum eColorAttachment = GL_COLOR_ATTACHMENT0_EXT;
                for (st_int32 i = 0; i < nNumTargets; ++i)
                {
                    const CRenderTargetOpenGL* pTarget = pTargets[i];
                    assert(pTarget);

                    if (pTarget->m_eType == RENDER_TARGET_TYPE_COLOR)
                        aeBuffers[i] = eColorAttachment++;
                    else
                        aeBuffers[i] = GL_DEPTH_ATTACHMENT_EXT;
                }

                const st_int32 c_nNumColorAttachments = (eColorAttachment - GL_COLOR_ATTACHMENT0_EXT);
                glDrawBuffers(c_nNumColorAttachments, aeBuffers);
            }
            else
            {
                // without these calls, glCheckFramebufferStatusEXT() returns GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
            }

            glGetIntegerv(GL_VIEWPORT, pPrimaryTarget->m_aBackupViewport);
            glViewport(0, 0, pPrimaryTarget->m_nWidth, pPrimaryTarget->m_nHeight);
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderTargetOpenGL::ReleaseGroupAsTarget

void CRenderTargetOpenGL::ReleaseGroupAsTarget(const CRenderTargetOpenGL** pTargets, st_int32 nNumTargets)
{
    ST_UNREF_PARAM(pTargets);
    ST_UNREF_PARAM(nNumTargets);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    
    CRenderTargetOpenGL* pPrimaryTarget = const_cast<CRenderTargetOpenGL*>(pTargets[0]);
    glViewport(pPrimaryTarget->m_aBackupViewport[0], pPrimaryTarget->m_aBackupViewport[1], pPrimaryTarget->m_aBackupViewport[2], pPrimaryTarget->m_aBackupViewport[3]);
}
