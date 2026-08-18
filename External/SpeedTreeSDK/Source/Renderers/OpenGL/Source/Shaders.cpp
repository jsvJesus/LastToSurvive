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
//  Static variables

CShaderTechniqueOpenGL::CLinkedProgramCache* CShaderTechniqueOpenGL::m_pLinkedProgramCache = NULL;

