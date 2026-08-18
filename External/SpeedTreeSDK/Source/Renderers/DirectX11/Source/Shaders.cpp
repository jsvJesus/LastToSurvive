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

#include "Renderers/DirectX11/DirectX11Renderer.h"
#include "Core/FileSystem.h"
#include "Utilities/Utility.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//  Shader static member variables

// CShaderConstantDirectX11
ID3D11ShaderResourceView* CShaderConstantBufferDirectX11::c_apCachedTextureSets[TEXTURE_REGISTER_COUNT] = { NULL };
st_int32 CShaderConstantBufferDirectX11::m_anTextureRegisterRange[2] = { TEXTURE_REGISTER_COUNT, 0 };
