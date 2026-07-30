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
//  CGeometryBufferOpenGL::SAttribParams::SAttribParams

inline CGeometryBufferOpenGL::SAttribParams::SAttribParams( ) :
    m_nOffset(-1),
    m_nNumComponents(-1),
    m_eDataType(GL_FLOAT)
{
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::SAttribParams::IsActive

inline st_bool CGeometryBufferOpenGL::SAttribParams::IsActive(void) const
{
    return (m_nOffset > -1);
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::CGeometryBufferOpenGL

inline CGeometryBufferOpenGL::CGeometryBufferOpenGL( ) :
    m_uiVertexBuffer(0),
    m_uiIndexBuffer(0),
    m_eIndexFormat(INDEX_FORMAT_UNSIGNED_32BIT)
{
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::~CGeometryBufferOpenGL

inline CGeometryBufferOpenGL::~CGeometryBufferOpenGL( )
{
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::CreateVertexBuffer

inline st_bool CGeometryBufferOpenGL::CreateVertexBuffer(st_bool bDynamic, const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize)
{
    st_bool bSuccess = false;

    if (uiNumVertices > 0 && uiVertexSize > 0)
    {
        glGenBuffers(1, &m_uiVertexBuffer);

        // make sure the generation was okay
        if (VertexBufferIsValid( ))
        {
            glBindBuffer(GL_ARRAY_BUFFER, m_uiVertexBuffer);

            // copy the data
            glBufferData(GL_ARRAY_BUFFER, uiNumVertices * uiVertexSize, pVertexData, bDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

            // unbind this buffer
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            bSuccess = true;
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::OverwriteVertices

inline st_bool CGeometryBufferOpenGL::OverwriteVertices(const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize, st_uint32 uiOffset, st_uint32 /*uiStream*/)
{
    // should have been taken care of in CGeometryBufferRI::OverwriteVertices, but be sure
    assert(pVertexData);
    assert(uiNumVertices > 0);
    assert(VertexBufferIsValid( ));

    glBindBuffer(GL_ARRAY_BUFFER, m_uiVertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, uiOffset * uiVertexSize, uiNumVertices * uiVertexSize, pVertexData);

    // unbind this buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true; // validity checks are performed in CGeometryBufferRI::OverwriteVertices
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::VertexBufferIsValid

inline st_bool CGeometryBufferOpenGL::VertexBufferIsValid(void) const
{
    return (m_uiVertexBuffer != 0);
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::EnableFormat

inline st_bool CGeometryBufferOpenGL::EnableFormat(void) const
{
    // intentionally empty (glEnableVertexAttribArray() calls made in CGeometryBufferOpenGL::CreateVertexBuffer())

    for (st_uint32 i = st_uint32(VERTEX_ATTRIB_0); i < st_uint32(VERTEX_ATTRIB_COUNT); ++i)
    {
        if (m_asAttribParams[i].IsActive( ))
            glEnableVertexAttribArray(i);
        else
            glDisableVertexAttribArray(i);
    }

    return true;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::DisableFormat

inline st_bool CGeometryBufferOpenGL::DisableFormat(void)
{
    // intentionally empty

    return true;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::UnBindVertexBuffer

inline st_bool CGeometryBufferOpenGL::UnBindVertexBuffer(st_uint32 /*uiStream*/)
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::SetIndexFormat

inline st_bool CGeometryBufferOpenGL::SetIndexFormat(EIndexFormat eFormat)
{
    m_eIndexFormat = eFormat;

    return (m_eIndexFormat == INDEX_FORMAT_UNSIGNED_16BIT || m_eIndexFormat == INDEX_FORMAT_UNSIGNED_32BIT);
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::CreateIndexBuffer

inline st_bool CGeometryBufferOpenGL::CreateIndexBuffer(st_bool bDynamic, const void* pIndexData, st_uint32 uiNumIndices)
{
    st_bool bSuccess = false;

    if (uiNumIndices > 0)
    {
        glGenBuffers(1, &m_uiIndexBuffer);

        if (IndexBufferIsValid( ))
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiIndexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(uiNumIndices * IndexSize( )), pIndexData, bDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

            // unbind this buffer
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            bSuccess = true;
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::OverwriteIndices

inline st_bool CGeometryBufferOpenGL::OverwriteIndices(const void* pIndexData, st_uint32 uiNumIndices, st_uint32 uiOffset)
{
    st_bool bSuccess = false;

    if (pIndexData && uiNumIndices > 0)
    {
        if (IndexBufferIsValid( ))
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiIndexBuffer);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, GLint(uiOffset * IndexSize( )), GLsizeiptr(uiNumIndices * IndexSize( )), pIndexData);

            // unbind this buffer
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            bSuccess = true;
        }
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::IndexBufferIsValid

inline st_bool CGeometryBufferOpenGL::IndexBufferIsValid(void) const
{
    return (m_uiIndexBuffer != 0);
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::IndexSize

inline st_uint32 CGeometryBufferOpenGL::IndexSize(void) const
{
    return (m_eIndexFormat == INDEX_FORMAT_UNSIGNED_16BIT) ? 2 : 4;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::ClearIndices

inline st_bool CGeometryBufferOpenGL::ClearIndices(void)
{
    if (IndexBufferIsValid( ))
    {
        glDeleteBuffers(1, &m_uiIndexBuffer);
        m_uiIndexBuffer = 0;
    }

    return true;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::BindIndexBuffer

inline st_bool CGeometryBufferOpenGL::BindIndexBuffer(void) const
{
    // CGeometryBufferRI class checks if the index buffer is valid before calling
    // BindIndexBuffer; no need to check twice
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiIndexBuffer);

    return true;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::UnBindIndexBuffer

inline st_bool CGeometryBufferOpenGL::UnBindIndexBuffer(void)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return true;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::ReleaseGfxResources

inline void CGeometryBufferOpenGL::ReleaseGfxResources(void)
{
    if (VertexBufferIsValid( ))
        glDeleteBuffers(1, &m_uiVertexBuffer);

    if (IndexBufferIsValid( ))
        glDeleteBuffers(1, &m_uiIndexBuffer);
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::RenderIndexed

inline st_bool CGeometryBufferOpenGL::RenderIndexed(EPrimitiveType ePrimType,
                                                    st_uint32 uiStartIndex,
                                                    st_uint32 uiNumIndices,
                                                    st_uint32 /*uiMinIndex*/, // DX9
                                                    st_uint32 /*uiNumVertices*/ // DX9
                                                    ) const
{
    st_bool bSuccess = false;

    if (uiNumIndices > 0)
    {
        // determine the right opengl primitive type
        GLenum eOpenGLPrimitive = GL_TRIANGLE_STRIP;
        switch (ePrimType)
        {
        case PRIMITIVE_QUADS:
            eOpenGLPrimitive = GL_QUADS;
            break;
        case PRIMITIVE_TRIANGLES:
            eOpenGLPrimitive = GL_TRIANGLES;
            break;
        case PRIMITIVE_POINTS:
            eOpenGLPrimitive = GL_POINTS;
            break;
        case PRIMITIVE_LINE_STRIP:
            eOpenGLPrimitive = GL_LINE_STRIP;
            break;
        case PRIMITIVE_LINE_LOOP:
            eOpenGLPrimitive = GL_LINE_LOOP;
            break;
        case PRIMITIVE_LINES:
            eOpenGLPrimitive = GL_LINES;
            break;
        case PRIMITIVE_TRIANGLE_FAN:
            eOpenGLPrimitive = GL_TRIANGLE_FAN;
            break;
        case PRIMITIVE_QUAD_STRIP:
            eOpenGLPrimitive = GL_QUAD_STRIP;
            break;
        case PRIMITIVE_TRIANGLE_STRIP:
        default:
            break;
        }

        const st_byte* pBase = NULL;
        if (m_eIndexFormat == INDEX_FORMAT_UNSIGNED_16BIT)
            glDrawElements(eOpenGLPrimitive, uiNumIndices, GL_UNSIGNED_SHORT, pBase + uiStartIndex * sizeof(st_uint16));
        else
            glDrawElements(eOpenGLPrimitive, uiNumIndices, GL_UNSIGNED_INT, pBase + uiStartIndex * sizeof(st_uint32));

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::RenderIndexedInstanced

inline st_bool CGeometryBufferOpenGL::RenderIndexedInstanced(EPrimitiveType ePrimType,
                                                             st_uint32 uiStartIndex,
                                                             st_uint32 uiNumIndices,
                                                             st_uint32 uiNumInstances,
                                                             st_uint32 /*uiStartInstanceLocation*/) const
{
    st_bool bSuccess = false;

    if (uiNumIndices > 0)
    {
        // determine the right opengl primitive type
        GLenum eOpenGLPrimitive = GL_TRIANGLE_STRIP;
        switch (ePrimType)
        {
        case PRIMITIVE_QUADS:
            eOpenGLPrimitive = GL_QUADS;
            break;
        case PRIMITIVE_TRIANGLES:
            eOpenGLPrimitive = GL_TRIANGLES;
            break;
        case PRIMITIVE_POINTS:
            eOpenGLPrimitive = GL_POINTS;
            break;
        case PRIMITIVE_LINE_STRIP:
            eOpenGLPrimitive = GL_LINE_STRIP;
            break;
        case PRIMITIVE_LINE_LOOP:
            eOpenGLPrimitive = GL_LINE_LOOP;
            break;
        case PRIMITIVE_LINES:
            eOpenGLPrimitive = GL_LINES;
            break;
        case PRIMITIVE_TRIANGLE_FAN:
            eOpenGLPrimitive = GL_TRIANGLE_FAN;
            break;
        case PRIMITIVE_QUAD_STRIP:
            eOpenGLPrimitive = GL_QUAD_STRIP;
            break;
        case PRIMITIVE_TRIANGLE_STRIP:
        default:
            break;
        }

        const st_byte* pBase = NULL;
        if (m_eIndexFormat == INDEX_FORMAT_UNSIGNED_16BIT)
            glDrawElementsInstanced(eOpenGLPrimitive, uiNumIndices, GL_UNSIGNED_SHORT, pBase + uiStartIndex * sizeof(st_uint16), uiNumInstances);
        else
            glDrawElementsInstanced(eOpenGLPrimitive, uiNumIndices, GL_UNSIGNED_INT, pBase + uiStartIndex * sizeof(st_uint32), uiNumInstances);

        bSuccess = true;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::RenderArrays

inline st_bool CGeometryBufferOpenGL::RenderArrays(EPrimitiveType ePrimType, st_uint32 uiStartVertex, st_uint32 uiNumVertices) const
{
    st_bool bSuccess = false;

#ifndef NDEBUG
    if (VertexBufferIsValid( ))
    {
#endif
        GLenum eOpenGLPrimitive = GL_TRIANGLE_STRIP;
        switch (ePrimType)
        {
        case PRIMITIVE_QUADS:
            eOpenGLPrimitive = GL_QUADS;
            break;
        case PRIMITIVE_TRIANGLES:
            eOpenGLPrimitive = GL_TRIANGLES;
            break;
        case PRIMITIVE_POINTS:
            eOpenGLPrimitive = GL_POINTS;
            break;
        case PRIMITIVE_LINE_STRIP:
            eOpenGLPrimitive = GL_LINE_STRIP;
            break;
        case PRIMITIVE_LINE_LOOP:
            eOpenGLPrimitive = GL_LINE_LOOP;
            break;
        case PRIMITIVE_LINES:
            eOpenGLPrimitive = GL_LINES;
            break;
        case PRIMITIVE_TRIANGLE_FAN:
            eOpenGLPrimitive = GL_TRIANGLE_FAN;
            break;
        case PRIMITIVE_QUAD_STRIP:
            eOpenGLPrimitive = GL_QUAD_STRIP;
            break;
        default:
            // let GL_TRIANGLE_STRIP fall through
            break;
        }

        glDrawArrays(eOpenGLPrimitive, uiStartVertex, uiNumVertices);

        bSuccess = true;
#ifndef NDEBUG
    }
#endif

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::CreateInstancingVAO

inline GLuint CGeometryBufferOpenGL::CreateInstancingVAO(const CGeometryBuffer* pObjectBuffer, GLuint uiInstanceBuffer) const
{
    GLuint uiVAO = 0;

    // query currently bound VAO
    GLint iCurrentVAO = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &iCurrentVAO);

    glGenVertexArrays(1, &uiVAO);
    glBindVertexArray(uiVAO);
    {
        // object buffer
        pObjectBuffer->BindVertexBuffer( );

        // active vertex buffer attributes
        for (st_uint32 i = st_uint32(VERTEX_ATTRIB_0); i < st_uint32(VERTEX_ATTRIB_COUNT); ++i)
        {
            if (m_asAttribParams[i].IsActive( ))
                glEnableVertexAttribArray(i);
            else
                glDisableVertexAttribArray(i);

            glVertexAttribDivisor(i, 0);
        }

        // instance buffer
        glBindBuffer(GL_ARRAY_BUFFER, uiInstanceBuffer);

        // instance data
        const st_byte* pBase = NULL;
        for (st_int32 i = 0; i < 3; ++i)
        {
            glEnableVertexAttribArray(VERTEX_ATTRIB_13 + i);
            glVertexAttribPointer(VERTEX_ATTRIB_13 + i, 4, GL_FLOAT, GL_FALSE, 12 * sizeof(st_float32), pBase + i * 4 * sizeof(st_float32));
            glVertexAttribDivisor(VERTEX_ATTRIB_13 + i, 1);
        }

        // index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiIndexBuffer);
    }
    glBindVertexArray(iCurrentVAO);

    return uiVAO;
}


///////////////////////////////////////////////////////////////////////
//  CGeometryBufferOpenGL::IsVertexFormatSet

inline st_bool CGeometryBufferOpenGL::IsVertexFormatSet(void) const
{
    return m_asAttribParams[VERTEX_ATTRIB_0].IsActive( );
}
