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


//  NOTE: If ST_RENDER_STATS is not defined then these functions will be empty and
//        will be optimized out by the compiler


///////////////////////////////////////////////////////////////////////  
//  CRenderStats::CStatPrimitive::CStatPrimitive

template <class T>
inline CRenderStats::CStatPrimitive<T>::CStatPrimitive( )
{
    Zero( );
}


///////////////////////////////////////////////////////////////////////  
//  CRenderStats::CStatPrimitive::CStatPrimitive

template <class T>
inline CRenderStats::CStatPrimitive<T>::CStatPrimitive(const T& tRight)
{
    #ifdef ST_RENDER_STATS
        m_tData = tRight;
    #else
        (tRight);
    #endif
}


///////////////////////////////////////////////////////////////////////  
//  CRenderStats::CStatPrimitive::Zero

template <class T>
inline void CRenderStats::CStatPrimitive<T>::Zero(void)
{
    #ifdef ST_RENDER_STATS
        memset(&m_tData, 0, sizeof(T));
    #endif
}


///////////////////////////////////////////////////////////////////////  
//  CRenderStats::CStatPrimitive::operator +

template <class T>
inline T CRenderStats::CStatPrimitive<T>::operator +(const T& tRight) const
{
    #ifdef ST_RENDER_STATS
        return m_tData + tRight;
    #else
        return m_tData;
    #endif
}


///////////////////////////////////////////////////////////////////////  
//  CRenderStats::CStatPrimitive::operator +

template <class T>
inline T CRenderStats::CStatPrimitive<T>::operator +=(const T& tRight)
{
    #ifdef ST_RENDER_STATS
        m_tData += tRight;
    #else
        (tRight);
    #endif

    return m_tData;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderStats::CStatPrimitive::operator T

template <class T>
inline CRenderStats::CStatPrimitive<T>::operator T(void) const
{
    return m_tData;
}


///////////////////////////////////////////////////////////////////////  
//  CRenderStats::SObjectStats::SObjectStats

inline CRenderStats::SObjectStats::SObjectStats( )
{
    Reset( );
}


///////////////////////////////////////////////////////////////////////  
//  CRenderStats::CRenderStats

inline CRenderStats::CRenderStats( )
{
    #ifdef ST_RENDER_STATS
        m_asObjects.SetHeapDescription("CRenderStats::CRenderStats");
        const size_t c_siMaxAnticipatedStatsObjects = 60;
        m_asObjects.reserve(c_siMaxAnticipatedStatsObjects);

        Reset( );
    #endif
}


///////////////////////////////////////////////////////////////////////  
//  CRenderStats::~CRenderStats

inline CRenderStats::~CRenderStats( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CRenderStats::SObjectStats::Reset

inline void CRenderStats::SObjectStats::Reset(void)
{
    #ifdef ST_RENDER_STATS
        m_nNumInstances = 0;
        m_nNumDrawCalls = 0;
        m_nNumTextureBinds = 0;
        m_nNumShaderBinds = 0;
        m_nNumTrianglesRendered = 0;
    #endif
}


///////////////////////////////////////////////////////////////////////  
//  CRenderStats::Reset

inline void CRenderStats::Reset(void)
{
    #ifdef ST_RENDER_STATS
        m_fSpeedTreeCullAndUpdateTime = 0.0f;
        m_fOtherCullAndUpdateTime = 0.0f;
        m_fOverallCullAndUpdateTime = 0.0f;
        m_asObjects.resize(0);
    #endif
}


///////////////////////////////////////////////////////////////////////  
//  CRenderStats::GetObjectStats

inline CRenderStats::SObjectStats& CRenderStats::GetObjectStats(const CFixedString& strName)
{
    #ifdef ST_RENDER_STATS
        // scan for occurrence of pBaseTree or first empty slot
        for (st_int32 i = 0; i < st_int32(m_asObjects.size( )); ++i)
        {
            if (m_asObjects[i].m_strName == strName)
                return m_asObjects[i];
        }

        SObjectStats sNewObject;
        sNewObject.m_strName = strName;
        m_asObjects.push_back(sNewObject);

        return m_asObjects.back( );
    #else
        (strName);

        return m_sStub;
    #endif
}


///////////////////////////////////////////////////////////////////////  
//  CRenderStats::GetObjectsArray

inline const CArray<CRenderStats::SObjectStats>& CRenderStats::GetObjectsArray(void) const
{
    return m_asObjects;
}




