//--------------------------------------------------------------------------------------
// File: SDKMisc.h
//
// Various helper functionality that is shared between SDK samples
//
// Copyright (c) Microsoft Corporation. All rights reserved
//--------------------------------------------------------------------------------------
#pragma once
#ifndef SDKMISC_H
#define SDKMISC_H

#include "dxut.h"

//--------------------------------------------------------------------------------------
// Manages the insertion point when drawing text
//--------------------------------------------------------------------------------------
class CDXUTTextHelper
{
public:
            CDXUTTextHelper( ID3DXFont* pFont9 = NULL, ID3DXSprite* pSprite9 = NULL, ID3DX10Font* pFont10 = NULL,
                             ID3DX10Sprite* pSprite10 = NULL, int nLineHeight = 15 );
            CDXUTTextHelper( ID3DXFont* pFont9, ID3DXSprite* pSprite9, int nLineHeight = 15 );
            CDXUTTextHelper( ID3DX10Font* pFont10, ID3DX10Sprite* pSprite10, int nLineHeight = 15 );
            ~CDXUTTextHelper();

    void    Init( ID3DXFont* pFont9 = NULL, ID3DXSprite* pSprite9 = NULL, ID3DX10Font* pFont10 = NULL,
                  ID3DX10Sprite* pSprite10 = NULL, int nLineHeight = 15 );

    void    SetInsertionPos( int x, int y )
    {
        m_pt.x = x; m_pt.y = y;
    }
    void    SetForegroundColor( D3DXCOLOR clr )
    {
        m_clr = clr;
    }

    void    Begin();
    HRESULT DrawFormattedTextLine( const char* strMsg, ... );
    HRESULT DrawTextLine( const char* strMsg );
    HRESULT DrawFormattedTextLine( RECT& rc, DWORD dwFlags, const char* strMsg, ... );
    HRESULT DrawTextLine( RECT& rc, DWORD dwFlags, const char* strMsg );
    void    End();

protected:
    ID3DXFont* m_pFont9;
    ID3DXSprite* m_pSprite9;
    ID3DX10Font* m_pFont10;
    ID3DX10Sprite* m_pSprite10;
    D3DXCOLOR m_clr;
    POINT m_pt;
    int m_nLineHeight;

    ID3D10BlendState* m_pFontBlendState10;
};


#endif
