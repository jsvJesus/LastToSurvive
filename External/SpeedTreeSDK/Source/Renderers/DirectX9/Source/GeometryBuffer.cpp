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

#include "Renderers/DirectX9/DirectX9Renderer.h"
#include "Utilities/Utility.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  Function: GetMatchingDx9Type

_D3DDECLTYPE GetMatchingDx9Type(EVertexFormat eVertexFormat, st_int32 nNumComponents)
{
    _D3DDECLTYPE eDx9Type = D3DDECLTYPE_UNUSED; // not found

    if (nNumComponents > 0 && nNumComponents < 5)
    {
        if (eVertexFormat == VERTEX_FORMAT_BYTE)
        {
            // DX9 does not have a signed single byte value, so we use an unsigned
            // byte and the SpeedTree shader will perform the appropriate 
            // decompression (for DX9 only)
            assert(nNumComponents == 4);
            eDx9Type = D3DDECLTYPE_UBYTE4;
        }
        else if (eVertexFormat == VERTEX_FORMAT_HALF_FLOAT)
        {
            if (nNumComponents == 2)
                eDx9Type = D3DDECLTYPE_FLOAT16_2;
            else if (nNumComponents == 4)
                eDx9Type = D3DDECLTYPE_FLOAT16_4;
            else
                assert(false);
        }
        else if (eVertexFormat == VERTEX_FORMAT_FULL_FLOAT)
        {
            assert(nNumComponents >= 1 && nNumComponents <= 4);
            eDx9Type = _D3DDECLTYPE(D3DDECLTYPE_FLOAT1 + nNumComponents - 1);
        }
        else
            assert(false);
    }

    return eDx9Type;
}


///////////////////////////////////////////////////////////////////////  
//  Function: GetMatchingDx9Semantic

st_bool GetMatchingDx9Semantic(EVertexAttribute eAttrib, D3DVERTEXELEMENT9& sVertexElement)
{
    st_bool bSuccess = true;

    sVertexElement.UsageIndex = 0;
    switch (eAttrib)
    {
    case VERTEX_ATTRIB_0:
        sVertexElement.Usage = D3DDECLUSAGE_POSITION;
        break;
    case VERTEX_ATTRIB_1:
    case VERTEX_ATTRIB_2:
    case VERTEX_ATTRIB_3:
    case VERTEX_ATTRIB_4:
    case VERTEX_ATTRIB_5:
    case VERTEX_ATTRIB_6:
    case VERTEX_ATTRIB_7:
    case VERTEX_ATTRIB_8:
    case VERTEX_ATTRIB_9:
    case VERTEX_ATTRIB_10:
    case VERTEX_ATTRIB_11:
    case VERTEX_ATTRIB_12:
    case VERTEX_ATTRIB_13:
    case VERTEX_ATTRIB_14:
    case VERTEX_ATTRIB_15:
        sVertexElement.Usage = D3DDECLUSAGE_TEXCOORD;
        sVertexElement.UsageIndex = BYTE(eAttrib - VERTEX_ATTRIB_1);
        break;
    default:
        bSuccess = false;
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CGeometryBufferDirectX9::SetVertexDecl

st_bool CGeometryBufferDirectX9::SetVertexDecl(const SVertexDecl& sVertexDecl, const CShaderTechnique* pTechnique, const SVertexDecl& sInstanceVertexDecl)
{
    ST_UNREF_PARAM(pTechnique);
    ST_UNREF_PARAM(sInstanceVertexDecl);

    st_bool bSuccess = false;

    // count # of used attributes
    size_t siNumElements = 0;
    for (st_int32 i = 0; i < VERTEX_ATTRIB_COUNT; ++i)
        if (sVertexDecl.m_asAttributes[i].IsUsed( ))
            ++siNumElements;

    if (siNumElements > 0)
    {
        st_uint32 auiOffsetsByStream[2] = { 0 };
        CStaticArray<D3DVERTEXELEMENT9> aElements(siNumElements + 1, "CGeometryBufferDirectX9::SetVertexDecl", false);

        for (st_uint8 uiStream = 0; uiStream < 2; ++uiStream)
        {
            for (st_int32 i = 0; i < VERTEX_ATTRIB_COUNT; ++i)
            {
                const SVertexDecl::SAttribute& sAttrib = sVertexDecl.m_asAttributes[i];
                if (sAttrib.IsUsed( ) && sAttrib.m_uiStream == uiStream)
                {
                    _D3DDECLTYPE eDx9Type = GetMatchingDx9Type(sAttrib.m_eFormat, sAttrib.NumUsedComponents( ));
                    if (eDx9Type != D3DDECLTYPE_UNUSED)
                    {
                        // setup element to match this attribute
                        D3DVERTEXELEMENT9 sElement;

                        assert(sAttrib.m_uiStream == 0 || sAttrib.m_uiStream == 1);
                        sElement.Stream = WORD(sAttrib.m_uiStream);

                        sElement.Method = D3DDECLMETHOD_DEFAULT;

                        // use verified dx9 data type 
                        sElement.Type = BYTE(eDx9Type);

                        // offset of this element in the vertex struct
                        sElement.Offset = WORD(auiOffsetsByStream[sElement.Stream]);

                        // semantic/method
                        if (GetMatchingDx9Semantic(EVertexAttribute(i), sElement)) // assigns both .Usage and .UsageIndex
                        {
                            aElements.push_back(sElement);
                            bSuccess = true;
                        }
                        else
                        {
                            CCore::SetError("CGeometryBufferDirectX9::SetFormat, cannot find matching DX9 semantic");
                            bSuccess = false;
                            break;
                        }
                    }
                    else
                    {
                        CCore::SetError("CGeometryBufferDirectX9::SetFormat, vertex attribute not supported by DX9: (%s, # elements: %d, data type: %s)\n",
                            SVertexDecl::AttributeName(EVertexAttribute(i)), sAttrib.NumUsedComponents( ), SVertexDecl::FormatName(sAttrib.m_eFormat));
                        bSuccess = false;
                        break;
                    }

                    auiOffsetsByStream[sAttrib.m_uiStream] += WORD(sAttrib.Size( ));
                }
            }
        }

        if (bSuccess)
        {
            // add ending vertex element
            D3DVERTEXELEMENT9 sTerminator = D3DDECL_END();
            aElements.push_back(sTerminator);

            assert(DX9::Device( ));
            bSuccess = SUCCEEDED(DX9::Device( )->CreateVertexDeclaration(&aElements[0], &m_pVertexDeclaration));
        }
    }

    return bSuccess;
}
