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
#include "Utilities/Utility.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//  Function: GetOpenGLDataType

GLenum GetOpenGLDataType(EVertexFormat eFormat)
{
    GLenum aeOpenGLFormats[VERTEX_FORMAT_COUNT] =
    {
        GL_FLOAT,           // VERTEX_FORMAT_FULL_FLOAT
        GL_HALF_FLOAT,      // VERTEX_FORMAT_HALF_FLOAT
        GL_UNSIGNED_BYTE    // VERTEX_FORMAT_BYTE
    };

    return aeOpenGLFormats[eFormat];
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::SetVertexDecl

st_bool CGeometryBufferOpenGL::SetVertexDecl(const SVertexDecl& sVertexDecl, const CShaderTechnique* pTechnique, const SVertexDecl& sInstanceVertexDecl)
{
    st_bool bSuccess = true;

    ST_UNREF_PARAM(pTechnique);

    // is the instance vertex decl empty?
    const st_bool c_bInstanceVertexDeclEmpty = (sInstanceVertexDecl.m_uiVertexSize == 0);

    // which vertex decl matches the shader vertex input?
    const SVertexDecl& sShaderVertexDecl = (c_bInstanceVertexDeclEmpty ? sVertexDecl : sInstanceVertexDecl);

    st_int32 nOffset = 0;
    for (st_int32 i = 0; i < VERTEX_ATTRIB_COUNT; ++i)
    {
        const SVertexDecl::SAttribute& sAttrib = sShaderVertexDecl.m_asAttributes[i];
        if (sAttrib.IsUsed( ))
        {
            SAttribParams& sParams = m_asAttribParams[i];

            // check if this attribute has already been set
            if (sParams.m_nOffset == -1 &&
                sParams.m_nNumComponents == -1)
            {
                // offset and size are all in bytes
                sParams.m_nOffset = nOffset;
                sParams.m_nNumComponents = st_int32(sAttrib.NumUsedComponents( ));

                // determine data type
                sParams.m_eDataType = GetOpenGLDataType(sAttrib.m_eFormat);

                // perform sanity tests
                if (sParams.m_nNumComponents < 1 || sParams.m_nNumComponents > 4)
                {
                    CCore::SetError("CGeometryBufferOpenGL::SetVertexDecl, element count must be 1, 2, 3, or 4");
                    bSuccess = false;
                }

                nOffset += sAttrib.Size( );
            }
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::BindVertexBuffer

st_bool CGeometryBufferOpenGL::BindVertexBuffer(st_uint32 /*uiStream*/, st_uint32 uiVertexSize) const
{
    st_bool bSuccess = false;

#ifndef NDEBUG
    if (VertexBufferIsValid( ) && IsVertexFormatSet( ))
    {
#endif

        // bind the vertex buffer object
        glBindBuffer(GL_ARRAY_BUFFER, m_uiVertexBuffer);

        // use as offset for VBO data
        const st_byte* pBase = NULL;

        // setup each vertex attribute offset/stride
        for (st_uint32 i = st_uint32(VERTEX_ATTRIB_0); i < st_uint32(VERTEX_ATTRIB_COUNT); ++i)
        {
            const SAttribParams* pParams = m_asAttribParams + i;

            if (pParams->IsActive( ))
                glVertexAttribPointer(i, pParams->m_nNumComponents, pParams->m_eDataType, GL_FALSE, uiVertexSize, pBase + pParams->m_nOffset);
        }

        bSuccess = true;

#ifndef NDEBUG
    }
    else
        CCore::SetError("CGeometryBufferOpenGL::BindVertexBuffer, vertex buffer is not valid, cannot bind");
#endif

    return bSuccess;
}

