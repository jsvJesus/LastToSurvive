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

#ifdef ST_OPENGL

    ///////////////////////////////////////////////////////////////////////
    //  Preprocessor

    #include "MyDebugRenderFunctions.h"
    using namespace SpeedTree;


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::RenderAxes
    //
    //  Render XYZ ortho axes out in front of the camera

    void CMyDebugRenderFunctions::RenderAxes(const CView& cView)
    {
        BeginFixedPipelineRender(cView);
        {
            glPushAttrib(GL_LINE_BIT);

            glLineWidth(3.0f);

            const st_float32 c_fScalar = 25.0f;
            const Vec3& c_vCameraPos = cView.GetCameraPos( );
            const Vec3 c_vCameraDir = cView.GetCameraDir( ) * (CCoordSys::IsYAxisUp( ) ? -1.0f : 1.0f);

            // push the axes out in front of the camera
            const Vec3 c_vAxesCenter = c_vCameraPos + c_vCameraDir * 50.0f;
            glTranslatef(c_vAxesCenter.x, c_vAxesCenter.y, c_vAxesCenter.z);

            // right axis
            glColor3f(1.0f, 0.0f, 0.0f);
            glBegin(GL_LINES);
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3fv(Vec3(1.0f, 0.0f, 0.0f) * c_fScalar);
            glEnd( );

            // out axis
            glColor3f(0.3f, 1.0f, 0.3f);
            glBegin(GL_LINES);
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3fv(Vec3(0.0f, 1.0f, 0.0f) * c_fScalar);
            glEnd( );

            // up axis
            glColor3f(0.0f, 0.0f, 1.0f);
            glBegin(GL_LINES);
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3fv(Vec3(0.0f, 0.0f, 1.0f) * c_fScalar);
            glEnd( );

            glPopAttrib( );
        }
        EndFixedPipelineRender( );
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::RenderFrustum

    void CMyDebugRenderFunctions::RenderFrustum(const CView& cView)
    {
        const Vec3* pFrustumPoints = cView.GetFrustumPoints( );

        BeginFixedPipelineRender(cView);
        {
            glPushAttrib(GL_LINE_BIT);
            glLineWidth(2.0f);
            glColor3f(0.4f, 1.0f, 0.4f);

            // near
            glBegin(GL_LINE_LOOP);
                glVertex3fv(pFrustumPoints[0]);
                glVertex3fv(pFrustumPoints[1]);
                glVertex3fv(pFrustumPoints[2]);
                glVertex3fv(pFrustumPoints[3]);
            glEnd( );

            // far
            glBegin(GL_LINE_LOOP);
                glVertex3fv(pFrustumPoints[4]);
                glVertex3fv(pFrustumPoints[5]);
                glVertex3fv(pFrustumPoints[6]);
                glVertex3fv(pFrustumPoints[7]);
            glEnd( );

            // connect
            glBegin(GL_LINES);
                glVertex3fv(pFrustumPoints[0]);
                glVertex3fv(pFrustumPoints[4]);
                glVertex3fv(pFrustumPoints[1]);
                glVertex3fv(pFrustumPoints[5]);
                glVertex3fv(pFrustumPoints[2]);
                glVertex3fv(pFrustumPoints[6]);
                glVertex3fv(pFrustumPoints[3]);
                glVertex3fv(pFrustumPoints[7]);
            glEnd( );

            glPopAttrib( );
        }
        EndFixedPipelineRender( );
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::RenderTreeBoundingBoxes
    //
    //  Displays a yellow wireframe box around each 3D tree instance

    void CMyDebugRenderFunctions::RenderTreeBoundingBoxes(const CView& cView, const CVisibleInstancesRender& cVisibleInstances)
    {
        BeginFixedPipelineRender(cView);
        {
            glColor3f(1.0f, 1.0f, 0.0f);

            const TDetailedCullDataArray& aInstanceLods = cVisibleInstances.Get3dInstanceLods( );
            for (st_int32 nBaseTree = 0; nBaseTree < st_int32(aInstanceLods.size( )); ++nBaseTree)
            {
                if (!aInstanceLods[nBaseTree].m_a3dInstanceLods.empty( ))
                {
                    // get extents of base tree
                    const CTree* pBaseTree = aInstanceLods[nBaseTree].m_a3dInstanceLods[0].m_pInstance->m_pInstanceOf;
                    const CExtents& cExtents = pBaseTree->GetExtents( );

                    // render each instance
                    for (st_int32 nInstance = 0; nInstance < st_int32(aInstanceLods[nBaseTree].m_a3dInstanceLods.size( )); ++nInstance)
                    {
                        const S3dTreeInstanceLod& sInstanceLod = aInstanceLods[nBaseTree].m_a3dInstanceLods[nInstance];

                        // modify base tree's extents to compensate for instance's transform
                        CExtents cInstanceExtents = cExtents;
                        cInstanceExtents.Orient(sInstanceLod.m_pInstance->m_vUp, sInstanceLod.m_pInstance->m_vRight);
                        cInstanceExtents.Scale(sInstanceLod.m_pInstance->m_fScalar);
                        cInstanceExtents.Translate(sInstanceLod.m_pInstance->m_vPos);

                        DrawBox(cInstanceExtents.Min( ), cInstanceExtents.Max( ), true);
                    }
                }
            }
        }
        EndFixedPipelineRender( );
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::RenderTreeCullSpheres
    //
    //  Renders yellow wireframe sphere around each 3D tree, illustrating its cull radius

    void CMyDebugRenderFunctions::RenderTreeCullSpheres(const CView& cView, const CVisibleInstancesRender& cVisibleInstances)
    {
        BeginFixedPipelineRender(cView);
        {
            glColor3f(1.0f, 1.0f, 0.0f);

            const TDetailedCullDataArray& aInstanceLods = cVisibleInstances.Get3dInstanceLods( );
            for (st_int32 nBaseTree = 0; nBaseTree < st_int32(aInstanceLods.size( )); ++nBaseTree)
            {
                if (!aInstanceLods[nBaseTree].m_a3dInstanceLods.empty( ))
                {
                    // render each instance
                    for (st_int32 nInstance = 0; nInstance < st_int32(aInstanceLods[nBaseTree].m_a3dInstanceLods.size( )); ++nInstance)
                    {
                        const S3dTreeInstanceLod& sInstanceLod = aInstanceLods[nBaseTree].m_a3dInstanceLods[nInstance];
                        const Vec3& vGeometricCenter = sInstanceLod.m_pInstance->m_vGeometricCenter;
                        const st_float32 c_fCullingRadius = sInstanceLod.m_pInstance->m_fCullingRadius;

                        DrawSphere(vGeometricCenter, c_fCullingRadius);
                    }
                }
            }
        }
        EndFixedPipelineRender( );
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::RenderVisibleCellBoundingBoxes
    //
    //  Renders bounding boxes for the visible cells: green for boxes fully inside
    //  the frustum, yellow for intersecting the frustum, and red otherwise (which
    //  should never be visible from the camera's point of view)

    void CMyDebugRenderFunctions::RenderVisibleCellBoundingBoxes(const CView& cView, const CVisibleInstancesRender& cVisibleInstances)
    {
        BeginFixedPipelineRender(cView);
        {
            const TRowColCellPtrMap& mVisibleCells = cVisibleInstances.VisibleCells( );
            for (TRowColCellPtrMap::const_iterator i = mVisibleCells.begin( ); i != mVisibleCells.end( ); ++i)
            {
                const CCell* pCell = i->second;

                // color code based on cull status
                if (pCell->GetCullStatus( ) == CS_FULLY_INSIDE_FRUSTUM)
                    glColor3f(0.0f, 1.0f, 0.0f);
                else if (pCell->GetCullStatus( ) == CS_INTERSECTS_FRUSTUM)
                    glColor3f(1.0f, 1.0f, 0.0f);
                else
                    glColor3f(1.0f, 0.0f, 0.0f);
                
                DrawBox(pCell->GetExtents( ).Min( ), pCell->GetExtents( ).Max( ), true);
            }
        }

        EndFixedPipelineRender( );
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::RenderRoughCellBoundingBoxes

    void CMyDebugRenderFunctions::RenderRoughCellBoundingBoxes(const CView& cView, CVisibleInstancesRender& cVisibleInstances)
    {
        BeginFixedPipelineRender(cView);
        {
            glColor3f(1.0f, 0.0f, 1.0f);
            for (st_int32 nCell = 0; nCell < st_int32(cVisibleInstances.RoughCells( ).size( )); ++nCell)
            {
                const CCell& cCell = cVisibleInstances.RoughCells( )[nCell];

                DrawBox(cCell.GetExtents( ).Min( ), cCell.GetExtents( ).Max( ), true);
            }
        }
        EndFixedPipelineRender( );
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::RenderCellCullSpheres

    void CMyDebugRenderFunctions::RenderCellCullSpheres(const CView& cView, CVisibleInstancesRender& cVisibleInstances)
    {
        glColor3f(1.0f, 0.0f, 0.0f);
        BeginFixedPipelineRender(cView);
        {
            const TRowColCellPtrMap& mVisibleCells = cVisibleInstances.VisibleCells( );
            for (TRowColCellPtrMap::const_iterator i = mVisibleCells.begin( ); i != mVisibleCells.end( ); ++i)
            {
                const CCell* pCell = i->second;

                DrawSphere(pCell->GetExtents( ).GetCenter( ), pCell->GetExtents( ).ComputeRadiusFromCenter3D( ));
            }
        }
        EndFixedPipelineRender( );
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::RenderTerrainNormals

    void CMyDebugRenderFunctions::RenderTerrainNormals(const CView& cView, const CMyTerrain& cTerrain)
    {
        BeginFixedPipelineRender(cView);
        {
            glPushAttrib(GL_LINE_BIT);

            glLineWidth(3.0f);
    
            const st_float32 c_fRadius = 500.0f;
            const st_float32 c_fSpacing = 10.0f;
            const st_float32 c_fNormalSize = 3.0f;
            const Vec3& c_vCamera = cView.GetCameraPos( );

            glBegin(GL_LINES);
            for (st_float32 x = c_vCamera.x - c_fRadius; x < c_vCamera.x + c_fRadius; x += c_fSpacing)
            {
                for (st_float32 y = c_vCamera.y - c_fRadius; y < c_vCamera.y + c_fRadius; y += c_fSpacing)
                {
                    st_float32 z = cTerrain.GetHeightFromXY(x, y);

                    Vec3 vNormal = cTerrain.GetNormalFromXY(x, y) * c_fNormalSize;

                    glColor3f(0.0f, 0.0f, 1.0f);
                    glVertex3f(x, y, z);
                    glVertex3f(x + vNormal.x, y + vNormal.y, z + vNormal.z);
                }
            }
            glEnd( );

            glPopAttrib( );
        }
        EndFixedPipelineRender( );
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::RenderDistanceCues
    //
    //  Render concentric circles around the camera that hug the terrain; useful
    //  for gaging distances of cascaded shadow maps.

    void CMyDebugRenderFunctions::RenderDistanceCues(const CView& cView, const CMyTerrain& cTerrain)
    {
        const st_int32 c_nNumRings = 5;
        const st_int32 c_nRingRes = 100;
        const st_float32 c_nRingInterval = 100.0f;

        BeginFixedPipelineRender(cView);
        {
            glPushAttrib(GL_LINE_BIT);

            glColor3f(0.0f, 0.0f, 1.0f);
            glLineWidth(3.0f);
    
            const Vec3& c_vCamera = cView.GetCameraPos( );

            for (st_int32 nRing = 0; nRing < c_nNumRings; ++nRing)
            {
                st_float32 c_fRadius = c_nRingInterval * (nRing + 1);

                glBegin(GL_LINE_LOOP);
                    for (st_int32 i = 0; i < c_nRingRes; ++i)
                    {
                        st_float32 c_fAngle = i * (c_fTwoPi / c_nRingRes);

                        st_float32 x = c_vCamera.x + c_fRadius * cosf(c_fAngle);
                        st_float32 y = c_vCamera.y + c_fRadius * sinf(c_fAngle);
                        st_float32 z = cTerrain.GetHeightFromXY(x, y);
                        glVertex3f(x, y, z);
                    }
                glEnd( );
            }

            glPopAttrib( );
        }
        EndFixedPipelineRender( );
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::RenderBillboardSpikes
    //
    //  Renders a short purple spike at every tree and billboard location

    void CMyDebugRenderFunctions::RenderBillboardSpikes(const CView& cView, const CVisibleInstancesRender& cVisibleInstances)
    {
        BeginFixedPipelineRender(cView);
        {
            glPushAttrib(GL_LINE_BIT);
            glColor3f(1.0f, 0.0f, 1.0f);
            glLineWidth(3.0f);
            
            glBegin(GL_LINES);
            {
                const TRowColCellPtrMap& mVisibleCells = cVisibleInstances.VisibleCells( );
                for (TRowColCellPtrMap::const_iterator i = mVisibleCells.begin( ); i != mVisibleCells.end( ); ++i)
                {
                    const CCell* pCell = i->second;

                    const TTreeInstConstPtrArray& aInstances = pCell->GetTreeInstances( );
                    for (st_int32 j = 0; j < st_int32(aInstances.size( )); ++j)
                    {
                        const STreeInstance* pInstance = aInstances[j];

                        glVertex3fv(pInstance->m_vPos);
                        Vec3 vUp = pInstance->m_vUp * 10.0f;
                        glVertex3f(pInstance->m_vPos.x + vUp.x, pInstance->m_vPos.y + vUp.y, pInstance->m_vPos.z + vUp.z);
                    }
                }
            }
            glEnd( );
            glPopAttrib( );
        }
        EndFixedPipelineRender( );
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::RenderCollisionObjects

    void CMyDebugRenderFunctions::RenderCollisionObjects(const CView& cView, const CVisibleInstancesRender& cVisibleInstances)
    {
        BeginFixedPipelineRender(cView);
        {
            glPushAttrib(GL_LINE_BIT | GL_POLYGON_BIT);
            glColor3f(1.0f, 0.0f, 1.0f);
            glLineWidth(1.0f);
            glPolygonMode(GL_FRONT, GL_LINE);
            glPolygonMode(GL_BACK, GL_LINE);
            
            // render collision objects only for 3D instances
            const TDetailedCullDataArray& aInstanceLods = cVisibleInstances.Get3dInstanceLods( );
            for (st_int32 nBaseTree = 0; nBaseTree < st_int32(aInstanceLods.size( )); ++nBaseTree)
            {
                if (!aInstanceLods[nBaseTree].m_a3dInstanceLods.empty( ))
                {
                    // render each instance
                    for (st_int32 nInstance = 0; nInstance < st_int32(aInstanceLods[nBaseTree].m_a3dInstanceLods.size( )); ++nInstance)
                    {
                        // get this instance
                        const S3dTreeInstanceLod& sInstanceLod = aInstanceLods[nBaseTree].m_a3dInstanceLods[nInstance];
                        const SInstance* pInstance = sInstanceLod.m_pInstance;

                        // get the base tree of this instance
                        const CTree* pBaseTree = sInstanceLod.m_pInstance->m_pInstanceOf;

                        // collision objects
                        st_int32 nNumObjs = 0;
                        const SCollisionObject* pObjs = pBaseTree->GetCollisionObjects(nNumObjs);
                        for (st_int32 i = 0; i < nNumObjs; ++i)
                        {
                            const Vec3& vRight = pInstance->m_vRight;
                            const Vec3& vUp = pInstance->m_vUp;

                            Vec3 vOut = vUp.Cross(vRight).Normalize( );
                            if ( (!CCoordSys::IsLeftHanded( ) && CCoordSys::IsYAxisUp( )) ||
                                 (CCoordSys::IsLeftHanded( ) && !CCoordSys::IsYAxisUp( )) )
                                 vOut = -vOut;

                            const Mat3x3 mRotation(vRight, vOut, vUp);

                            const Vec3 vCenter = pInstance->m_vPos + (mRotation * CCoordSys::ConvertToStd(pObjs[i].m_vCenter1)) * pInstance->m_fScalar;
                            const st_float32 fRadius = pObjs[i].m_fRadius * pInstance->m_fScalar;

                            DrawSphere(vCenter, fRadius);
                        }
                    }
                }
            }

            glPopAttrib( );
        }
        EndFixedPipelineRender( );
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::DrawSphereAt

    void CMyDebugRenderFunctions::DrawSphereAt(const CView& cView, const Vec3& vPos)
    {
        BeginFixedPipelineRender(cView);
        {
            //glPushAttrib(GL_LINE_BIT | GL_POLYGON_BIT);
            glColor3f(1.0f, 1.0f, 0.0f);

            DrawSphere(vPos, 5.0f);

            //glPopAttrib( );
        }
        EndFixedPipelineRender( );
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::RenderWindDirection

    void CMyDebugRenderFunctions::RenderWindDirection(const CView& cView, const Vec3& vWindDir)
    {
        BeginFixedPipelineRender(cView);
        {
            glPushAttrib(GL_LINE_BIT);

            glLineWidth(3.0f);

            const st_float32 c_fScalar = 25.0f;
            const Vec3& c_vCameraPos = cView.GetCameraPos( );
            const Vec3 c_vCameraDir = cView.GetCameraDir( ) * (CCoordSys::IsYAxisUp( ) ? -1.0f : 1.0f);

            // push the axes out in front of the camera
            const Vec3 c_vAxesCenter = c_vCameraPos + c_vCameraDir * 50.0f;
            glTranslatef(c_vAxesCenter.x, c_vAxesCenter.y, c_vAxesCenter.z);

            // wind indicator
            glBegin(GL_LINES);

            glColor3f(0.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);

            glColor3f(1.0f, 1.0f, 0.0f);
            glVertex3fv(vWindDir * c_fScalar);

            glEnd( );

            glPopAttrib( );
        }
        EndFixedPipelineRender( );
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::DrawBox

    void CMyDebugRenderFunctions::DrawBox(const st_float32 afMin[3], const st_float32 afMax[3], st_bool bLines)
    {
        if (bLines)
        {
            glBegin(GL_LINE_LOOP);
            glVertex3f(afMin[0], afMin[1], afMin[2]);
            glVertex3f(afMax[0], afMin[1], afMin[2]);
            glVertex3f(afMax[0], afMax[1], afMin[2]);
            glVertex3f(afMin[0], afMax[1], afMin[2]);
            glEnd( );

            glBegin(GL_LINE_LOOP);
            glVertex3f(afMin[0], afMin[1], afMax[2]);
            glVertex3f(afMax[0], afMin[1], afMax[2]);
            glVertex3f(afMax[0], afMax[1], afMax[2]);
            glVertex3f(afMin[0], afMax[1], afMax[2]);
            glEnd( );

            glBegin(GL_LINES);
            glVertex3f(afMin[0], afMin[1], afMin[2]);
            glVertex3f(afMin[0], afMin[1], afMax[2]);

            glVertex3f(afMax[0], afMin[1], afMin[2]);
            glVertex3f(afMax[0], afMin[1], afMax[2]);

            glVertex3f(afMax[0], afMax[1], afMin[2]);
            glVertex3f(afMax[0], afMax[1], afMax[2]);

            glVertex3f(afMin[0], afMax[1], afMin[2]);
            glVertex3f(afMin[0], afMax[1], afMax[2]);
            glEnd( );
        }
        else
        {
            glBegin(GL_QUADS);

            // -z
            glVertex3f(afMin[0], afMin[1], afMin[2]);
            glVertex3f(afMin[0], afMax[1], afMin[2]);
            glVertex3f(afMax[0], afMax[1], afMin[2]);
            glVertex3f(afMax[0], afMin[1], afMin[2]);

            // +z
            glVertex3f(afMin[0], afMin[1], afMax[2]);
            glVertex3f(afMax[0], afMin[1], afMax[2]);
            glVertex3f(afMax[0], afMax[1], afMax[2]);
            glVertex3f(afMin[0], afMax[1], afMax[2]);

            // -x
            glVertex3f(afMin[0], afMin[1], afMin[2]);
            glVertex3f(afMin[0], afMin[1], afMax[2]);
            glVertex3f(afMin[0], afMax[1], afMax[2]);
            glVertex3f(afMin[0], afMax[1], afMin[2]);

            // +x
            glVertex3f(afMax[0], afMin[1], afMin[2]);
            glVertex3f(afMax[0], afMax[1], afMin[2]);
            glVertex3f(afMax[0], afMax[1], afMax[2]);
            glVertex3f(afMax[0], afMin[1], afMax[2]);

            // -y
            glVertex3f(afMin[0], afMin[1], afMin[2]);
            glVertex3f(afMax[0], afMin[1], afMin[2]);
            glVertex3f(afMax[0], afMin[1], afMax[2]);
            glVertex3f(afMin[0], afMin[1], afMax[2]);

            // +y
            glVertex3f(afMin[0], afMax[1], afMin[2]);
            glVertex3f(afMin[0], afMax[1], afMax[2]);
            glVertex3f(afMax[0], afMax[1], afMax[2]);
            glVertex3f(afMax[0], afMax[1], afMin[2]);

            glEnd( );
        }
    }



    ///////////////////////////////////////////////////////////////////////
    //  Sphere-rendering asset functions
    //
    //  Sphere-rendering code taken from the OpenGL Red Book

    #define SPHERE_X 0.525731112119133606f
    #define SPHERE_Z 0.850650808352039932f

    static GLfloat vdata[12][3] = 
    {
        { -SPHERE_X, 0.0f, SPHERE_Z }, { SPHERE_X, 0.0f, SPHERE_Z }, { -SPHERE_X, 0.0f, -SPHERE_Z }, { SPHERE_X, 0.0f, -SPHERE_Z },
        { 0.0f, SPHERE_Z, SPHERE_X }, { 0.0f, SPHERE_Z, -SPHERE_X }, { 0.0f, -SPHERE_Z, SPHERE_X }, { 0.0f, -SPHERE_Z, -SPHERE_X },
        { SPHERE_Z, SPHERE_X, 0.0f }, { -SPHERE_Z, SPHERE_X, 0.0f }, { SPHERE_Z, -SPHERE_X, 0.0f }, { -SPHERE_Z, -SPHERE_X, 0.0f }
    };
    static GLuint tindices[20][3] =
    {
        { 0, 4, 1 }, { 0, 9, 4 }, { 9, 5, 4 }, { 4, 5, 8 }, { 4, 8, 1 },
        { 8, 10, 1 }, { 8, 3, 10 }, { 5, 3, 8 }, { 5, 2, 3 }, { 2, 7, 3 },
        { 7, 10, 3 }, { 7, 6, 10 }, { 7, 11, 6 }, { 11, 0, 6 }, { 0, 1, 6 },
        { 6, 1, 10 }, { 9, 0, 11 }, { 9, 11, 2 }, { 9, 2, 5 }, { 7, 2, 11 } };

    static void SphereNormalize(GLfloat *a)
    {
        GLfloat d = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
        a[0] /= d; a[1] /= d; a[2] /= d;
    }

    static void SphereDrawTri(GLfloat *a, GLfloat *b, GLfloat *c, int div, float r) 
    {
        if (div <= 0)
        {
            glNormal3fv(a); glVertex3f(a[0] * r, a[1] * r, a[2] * r);
            glNormal3fv(b); glVertex3f(b[0] * r, b[1] * r, b[2] * r);
            glNormal3fv(c); glVertex3f(c[0] * r, c[1] * r, c[2] * r);
        }
        else 
        {
            GLfloat ab[3], ac[3], bc[3];
            for (int i = 0; i < 3; i++)
            {
                ab[i] = (a[i] + b[i]) / 2;
                ac[i] = (a[i] + c[i]) / 2;
                bc[i] = (b[i] + c[i]) / 2;
            }

            SphereNormalize(ab); SphereNormalize(ac); SphereNormalize(bc);
            SphereDrawTri(a, ab, ac, div - 1, r);
            SphereDrawTri(b, bc, ab, div - 1, r);
            SphereDrawTri(c, ac, bc, div - 1, r);
            SphereDrawTri(ab, bc, ac, div - 1, r);  //<--Comment this line and sphere looks really cool!
        }
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::DrawSphere

    void CMyDebugRenderFunctions::DrawSphere(const Vec3& vCenter, st_float32 fRadius)
    {
        static GLuint uiDisplayList = 0;

        if (uiDisplayList == 0)
        {
            uiDisplayList = glGenLists(1);
            glNewList(uiDisplayList, GL_COMPILE);
            {
                const st_int32 c_nNumDivs = 2;
                const st_float32 c_fRadius = 1.0f;

                glBegin(GL_TRIANGLES);
                for (int i = 0; i < 20; i++)
                    SphereDrawTri(vdata[tindices[i][0]], vdata[tindices[i][1]], vdata[tindices[i][2]], c_nNumDivs, c_fRadius);
                glEnd( );
            }
            glEndList( );
        }

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix( );
            glTranslatef(vCenter.x, vCenter.y, vCenter.z);
            glScalef(fRadius, fRadius, fRadius);
            glCallList(uiDisplayList);
        glPopMatrix( );
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::BeginFixedPipelineRender

    void CMyDebugRenderFunctions::BeginFixedPipelineRender(const CView& cView)
    {
        // setup matrix for fixed-function rendering
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(cView.GetProjection( ));
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(cView.GetModelview( ));

        // set up render state
        glPushAttrib(GL_ENABLE_BIT);
            glDisable(GL_VERTEX_PROGRAM_ARB);
            glDisable(GL_FRAGMENT_PROGRAM_ARB);
            glUseProgram(0);
            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_ALPHA_TEST);
            glDisable(GL_BLEND);

        // matching glPopAttrib appears in ExdFixedPipelineRender() below
    }


    ///////////////////////////////////////////////////////////////////////
    //  CMyDebugRenderFunctions::EndFixedPipelineRender

    void CMyDebugRenderFunctions::EndFixedPipelineRender(void)
    {
        glPopAttrib( );
    }

#endif // ST_WIN_OPENGL
