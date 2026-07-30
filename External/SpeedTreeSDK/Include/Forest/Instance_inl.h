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
//  SInstance::SInstance

ST_INLINE SInstance::SInstance( ) :
    m_fScalar(1.0f),
    m_fLodTransition(1.0f),
    m_fLodValue(1.0f)
{
}



///////////////////////////////////////////////////////////////////////
//  STreeInstance::STreeInstance

ST_INLINE STreeInstance::STreeInstance( ) :
    m_pInstanceOf(NULL),
    m_fCullingRadius(-1.0f),
    m_pUserData(NULL)
{
}


///////////////////////////////////////////////////////////////////////
//  STreeInstance::ComputeCullParameters

ST_INLINE void STreeInstance::ComputeCullParameters(void)
{
    st_assert(m_pInstanceOf, "CTreeInstance must know its base tree before most operations can occur");

    // the geometric center is computed by rotating and scaling the base tree's
    // extents based on this instance's parameters and then computing the center
    // of the new extents
    CExtents cBaseExtents = m_pInstanceOf->GetExtents( );
    cBaseExtents.Scale(m_fScalar);
    cBaseExtents.Orient(m_vUp, m_vRight);

    m_vGeometricCenter = cBaseExtents.GetCenter( ) + m_vPos;

    // the culling radius is measured as the longest distance from the center of
    // new box to one of the corners (all corners are equidistant from the center)
    m_fCullingRadius = cBaseExtents.ComputeRadiusFromCenter3D( );
}
