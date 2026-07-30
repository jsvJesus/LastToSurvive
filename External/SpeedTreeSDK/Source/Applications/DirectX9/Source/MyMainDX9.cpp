///////////////////////////////////////////////////////////////////////  
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization and may
//  not be copied or disclosed except in accordance with the terms of
//  that agreement.
//
//      Copyright (c) 2003-2016 IDV, Inc.
//      All Rights Reserved.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#pragma warning (disable : 4995)
#include "DXUT.h"
#include "MyApplication.h"
#include "MyMouseAndKeyboardNavigation.h"
//#define ST_OVERRIDE_GLOBAL_NEW_AND_DELETE
#define ST_USE_ALLOCATOR_INTERFACE // must be active for Debug MT and Release MT builds
#define ST_OVERRIDE_FILESYSTEM
#include "MyCustomAllocator.h"
#include "MyFileSystem.h"
using namespace SpeedTree;

// enable to allow debugging with NVPerfHUD
//#define ST_NVPERFHUD


///////////////////////////////////////////////////////////////////////  
//  File-scope function prototypes

static  PCHAR*                  CommandLineToArgvA(PCHAR CmdLine, int* _argc);


///////////////////////////////////////////////////////////////////////  
//  File-scope variables

        // memory management
#ifdef ST_USE_ALLOCATOR_INTERFACE
static  CMyCustomAllocator              g_cCustomAllocator;
static  CAllocatorInterface             g_cAllocatorInterface(&g_cCustomAllocator);
#endif

// file system
#ifdef ST_OVERRIDE_FILESYSTEM
static  CMyFileSystem                   g_cMyFileSystem;
static  CFileSystemInterface            g_cFileSystemInterface(&g_cMyFileSystem);
#endif

// app
static  CMyApplication*                 g_pApplication = NULL;
static  CMyMouseAndKeyboardNavigation*  g_pUserInput = NULL;


///////////////////////////////////////////////////////////////////////  
//  ModifyDeviceSettings
//
//  Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed

bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{
    if (pDeviceSettings->ver == DXUT_D3D9_DEVICE)
    {
        IDirect3D9* pD3D = DXUTGetD3D9Object( );
        D3DCAPS9 sCaps;
        pD3D->GetDeviceCaps(pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &sCaps);

        // if device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
        // then switch to SWVP.
        if ((sCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 || sCaps.VertexShaderVersion < D3DVS_VERSION(1,1))
            pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

        // Request the multisampling quality specified on the command-line
        st_assert(g_pApplication, "g_pApplication should be allocated before this point");
        if (g_pApplication->GetCmdLineOptions( ).m_nNumSamples > 0)
            pDeviceSettings->d3d9.pp.MultiSampleType = D3DMULTISAMPLE_TYPE(g_pApplication->GetCmdLineOptions( ).m_nNumSamples);

        // debugging vertex shaders requires either REF or software vertex processing 
        // and debugging pixel shaders requires REF.
        #ifdef DEBUG_VS
            if (pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF)
            {
                pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
                pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;                            
                pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
            }
        #endif
        #ifdef DEBUG_PS
            pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
        #endif

        // handle nVidia PerfHUD if it is enabled
        #ifdef ST_NVPERFHUD
            for (UINT uiAdapter = 0; uiAdapter < pD3D->GetAdapterCount( ); ++uiAdapter)  
            { 
                D3DADAPTER_IDENTIFIER9 sIdentifier; 
                HRESULT hResult = pD3D->GetAdapterIdentifier(uiAdapter, 0, &sIdentifier); 
                if (SUCCEEDED(hResult) && strstr(sIdentifier.Description, "PerfHUD") != 0) 
                {
                    pDeviceSettings->d3d9.AdapterOrdinal = uiAdapter; 
                    pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
                    printf("Detected NVIDIA PerfHUD\n");
                    break; 
                } 
            }
        #endif
    }
    
    // for the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if (DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF)
        {
            printf("\n\n\t*** Currently using a reference renderer ***\n\n");
        }
    }

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  IsD3D9DeviceAcceptable
//
//  Rejects any D3D9 devices that aren't acceptable to the app by returning false

bool CALLBACK IsD3D9DeviceAcceptable(D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, 
                                     bool bWindowed, void* pUserContext)
{
    // reject any device that doesn't support at least ps1.1
    if (pCaps->PixelShaderVersion < D3DPS_VERSION(1, 1))
        return false;

    // Typically want to skip back buffer formats that don't support alpha blending
    IDirect3D9* pD3D = DXUTGetD3D9Object( ); 
    if (FAILED(pD3D->CheckDeviceFormat(pCaps->AdapterOrdinal, 
                                       pCaps->DeviceType, 
                                       AdapterFormat, 
                                       D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, 
                                       D3DRTYPE_TEXTURE, 
                                       BackBufferFormat)))
        return false;
    
    // need to support D3DFMT_R32F render target
    if (FAILED(pD3D->CheckDeviceFormat(pCaps->AdapterOrdinal, 
                                       pCaps->DeviceType,
                                       AdapterFormat, 
                                       D3DUSAGE_RENDERTARGET,
                                       D3DRTYPE_CUBETEXTURE, 
                                       D3DFMT_R32F)))
        return false;

    // need to support D3DFMT_A8R8G8B8 render target
    if (FAILED(pD3D->CheckDeviceFormat(pCaps->AdapterOrdinal, 
                                       pCaps->DeviceType,
                                       AdapterFormat, 
                                       D3DUSAGE_RENDERTARGET,
                                       D3DRTYPE_CUBETEXTURE, 
                                       D3DFMT_A8R8G8B8)))
        return false;

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  OnD3D9CreateDevice
//
//  Create any D3D9 resources that will live through a device reset (D3DPOOL_MANAGED) and aren't tied to the back buffer size

HRESULT CALLBACK OnD3D9CreateDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
    st_assert(pd3dDevice, "OnD3D9CreateDevice should always get a valid device pointer");
    SpeedTree::DX9::SetDevice(pd3dDevice);

    return S_OK;
}


///////////////////////////////////////////////////////////////////////  
//  OnD3D9ResetDevice
//
//  Create any D3D9 resources that won't live through a device reset (D3DPOOL_DEFAULT) 
//  or that are tied to the back buffer size 

HRESULT CALLBACK OnD3D9ResetDevice(IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
    if (g_pApplication != NULL)
    {
        g_pApplication->WindowReshape(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
        g_pApplication->OnResetDevice( );
    }

    return S_OK;
}


///////////////////////////////////////////////////////////////////////  
//  OnD3D9LostDevice
//
//  Release D3D9 resources created in the OnD3D9ResetDevice callback 

void CALLBACK OnD3D9LostDevice(void* pUserContext)
{
    if (g_pApplication != NULL)
        g_pApplication->OnLostDevice( );
}


///////////////////////////////////////////////////////////////////////  
//  OnD3D9DestroyDevice
//
//  Release D3D9 resources created in the OnD3D9CreateDevice callback 

void CALLBACK OnD3D9DestroyDevice(void* pUserContext)
{
}


///////////////////////////////////////////////////////////////////////  
//  OnD3D9FrameRender
//
//  Render the scene using the D3D9 device

void CALLBACK OnD3D9FrameRender(IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTimeInSec, void* pUserContext)
{
    HRESULT hr;

    // render the scene
    if (SUCCEEDED(pd3dDevice->BeginScene( )))
    {
        st_assert(g_pApplication, "g_pApplication should be allocated before this point");

        bool bInitError = false;
        static bool bFirstDisplay = true;
        if (bFirstDisplay)
        {
            if (!g_pApplication->InitGfx( ))
            {
                bInitError = true;
                PrintSpeedTreeErrors( );
                system("pause");
                exit(-1);
            }
            else
                g_pUserInput->GotoPresetCamera(0);

            // center the mouse pointer
            POINT sCenter;
            sCenter.x = g_pApplication->GetCmdLineOptions( ).m_nWindowWidth / 2;
            sCenter.y = g_pApplication->GetCmdLineOptions( ).m_nWindowHeight / 2;
            ClientToScreen(GetActiveWindow( ), &sCenter);
            SetCursorPos(sCenter.x, sCenter.y);

            bFirstDisplay = false;
        }

        if (!bInitError)
        {
            if (g_pApplication->ReadyToRender( ))
            {
                // update the scene before the render; this can be outside of the render thread/function, but must be
                // completed before the render can proceed
                assert(g_pUserInput);
                g_pUserInput->Tick(fElapsedTimeInSec);
                g_pApplication->SetWorldTransform(g_pUserInput->GetTransform( ), g_pUserInput->GetCamera( ).m_vPos);

                g_pApplication->Advance( );
                g_pApplication->Cull( );

                // draw calls
                DX9::Device( )->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_COLORVALUE(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
                g_pApplication->Render( );

                // stats
                g_pApplication->ReportStats( );
            }
        }

        // scene complete
        V(pd3dDevice->EndScene( ));
    }
}


///////////////////////////////////////////////////////////////////////  
//  MsgProc
//
//  Handle messages to the application

LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext)
{
    // handle keyboard and mouse events
    int x = int(LOWORD(lParam));
    int y = int(HIWORD(lParam));

    st_assert(g_pApplication, "g_pApplication should be allocated before this point");
    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
        g_pUserInput->MouseClick(MOUSE_BUTTON_LEFT, true, x, y);
        SetCapture(hWnd);
        break;
    case WM_LBUTTONUP:
        g_pUserInput->MouseClick(MOUSE_BUTTON_LEFT, false, x, y);
        ReleaseCapture( );
        break;
    case WM_MBUTTONDOWN:
        g_pUserInput->MouseClick(MOUSE_BUTTON_MIDDLE, true, x, y);
        SetCapture(hWnd);
        break;
    case WM_MBUTTONUP:
        g_pUserInput->MouseClick(MOUSE_BUTTON_MIDDLE, false, x, y);
        ReleaseCapture( );
        break;
    case WM_RBUTTONDOWN:
        g_pUserInput->MouseClick(MOUSE_BUTTON_RIGHT, true, x, y);
        SetCapture(hWnd);
        break;
    case WM_RBUTTONUP:
        g_pUserInput->MouseClick(MOUSE_BUTTON_RIGHT, false, x, y);
        ReleaseCapture( );
        break;
    case WM_MOUSEMOVE:
        g_pUserInput->MouseMotion(x, y);
        break;
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////  
//  OnKeyboard
//
//  Handle key presses

void CALLBACK OnKeyboard(UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext)
{
    st_assert(g_pApplication, "g_pApplication should be allocated before this point");

    if (nChar == VK_OEM_COMMA)
        nChar = ',';
    else if (nChar == VK_OEM_PERIOD)
        nChar = '.';
    else if (nChar == VK_OEM_2)
        nChar = '/';

    if (bKeyDown)
        g_pUserInput->KeyDown(static_cast<st_uchar>(nChar));
    else
        g_pUserInput->KeyUp(static_cast<st_uchar>(nChar));
}


///////////////////////////////////////////////////////////////////////  
//  OnMouse
//
//  Handle mouse button presses

void CALLBACK OnMouse(bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
                      bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
                      int xPos, int yPos, void* pUserContext)
{
}


///////////////////////////////////////////////////////////////////////  
//  OnDeviceRemoved
//
//  Call if device was removed. Return true to find a new device, false to quit

bool CALLBACK OnDeviceRemoved(void* pUserContext)
{
    return true;
}


///////////////////////////////////////////////////////////////////////  
//  GetCmdLineAsAscii

st_char** GetCmdLineAsAscii(st_int32& argc)
{
    // return value
    st_char** argv = NULL;

    // extract wide version of command-line
    LPWSTR* pstrArgList = CommandLineToArgvW(GetCommandLine(), &argc);
    if (argc > 0)
    {
        // allocate array of char*s
        argv = st_new_array<st_char*>(argc, "GetCmdLineAsAscii");

        const st_int32 c_nMaxArgSize = 1024;
        st_char szArg[c_nMaxArgSize];
        for (int i = 0; i < argc; ++i)
        {
            // convert each wide argument to ascii and copy
            wcstombs(szArg, pstrArgList[i], c_nMaxArgSize - 1);
            argv[i] = st_new_array<st_char>(strlen(szArg) + 1, "GetCmdLineAsAscii");
            strcpy(argv[i], szArg);
        }
    }

    return argv;
}


///////////////////////////////////////////////////////////////////////  
//  GetCmdLineAsAscii

void FreeCmdLineAscii(st_int32 argc, st_char** argv)
{
    if (argv)
    {
        for (int i = 0; i < argc; ++i)
            st_delete_array<st_char>(argv[i]);

        st_delete_array<st_char*>(argv);
    }
}


///////////////////////////////////////////////////////////////////////  
//  wWinMain
//
//  Initialize everything and go into a render loop

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Enable run-time memory check for debug builds.
    #if defined(DEBUG) | defined(_DEBUG)
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif

    // set current path to that of the app
    char szBuf[1024];
    GetModuleFileNameA(hInstance, szBuf, 1024);
    SetCurrentDirectoryA(SpeedTree::CFixedString(szBuf).Path( ).c_str( ));

    CScopeTrace::Init( );

    // check DirectX version
    if (!D3DXCheckVersion(D3D_SDK_VERSION, D3DX_SDK_VERSION))
    {
        printf("This SpeedTree Reference Application was built with a different version of the DirectX SDK than is installed.\n");
        exit(1);
    }

    // Set general DXUT callbacks
    DXUTSetCallbackKeyboard(OnKeyboard);
    DXUTSetCallbackMouse(OnMouse);
    DXUTSetCallbackMsgProc(MsgProc);
    DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
    DXUTSetCallbackDeviceRemoved(OnDeviceRemoved);

    // D3D9 DXUT callbacks
    DXUTSetCallbackD3D9DeviceAcceptable(IsD3D9DeviceAcceptable);
    DXUTSetCallbackD3D9DeviceCreated(OnD3D9CreateDevice);
    DXUTSetCallbackD3D9DeviceReset(OnD3D9ResetDevice);
    DXUTSetCallbackD3D9FrameRender(OnD3D9FrameRender);
    DXUTSetCallbackD3D9DeviceLost(OnD3D9LostDevice);
    DXUTSetCallbackD3D9DeviceDestroyed(OnD3D9DestroyDevice);

    // perform any application-level initialization here
    {
        // convert the command-line to standard argc and argv
        int argc = 0;
        char** argv = GetCmdLineAsAscii(argc);

        g_pApplication = st_new(CMyApplication, "CMyApplication");
        if (g_pApplication->ParseCmdLine(argc, argv))
        {
            // create console so that prints can be seen
            if (g_pApplication->GetCmdLineOptions( ).m_bConsole)
            {
                AllocConsole( );                 // allocate console window
                FILE* pStream = NULL;
                freopen_s(&pStream, "CONOUT$", "a", stderr); // redirect stderr to console
                freopen_s(&pStream, "CONOUT$", "a", stdout); // redirect stdout also
                freopen_s(&pStream, "CONIN$", "r", stdin);
                SetConsoleTitleA("SpeedTree Console Window");
            }

            if (!g_pApplication->Init( ))
            {
                Error("Failed to initialize forest");
                return DXUTGetExitCode( );
            }

            // init user input
            g_pUserInput = st_new(CMyMouseAndKeyboardNavigation, "CMyMouseAndKeyboardNavigation")(g_pApplication);

            // hide cursor
            ShowCursor(FALSE);
        }

        GlobalFree(argv);
    }

    CMyApplication::PrintId( );
    CMyApplication::PrintKeys( );

    // extract command-line settings to set up the window
    st_assert(g_pApplication, "g_pApplication should be allocated before this point");
    const SMyCmdLineOptions& sCmdLineOptions= g_pApplication->GetCmdLineOptions( );

    DXUTSetIsInGammaCorrectMode(false);
    DXUTInit(false, true, NULL); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings(true, true); // Show the cursor and clip it when in full screen

    {
        const st_bool bDeferred = g_pApplication->GetCmdLineOptions( ).m_bDeferred;
        SpeedTree::CFixedString strTitle = SpeedTree::CFixedString::Format("SpeedTree SDK v%s DirectX 9.0c Application (%s)", ST_VERSION_STRING, bDeferred ? "Deferred" : "Forward");
        WCHAR wstrAppTitle[64];
        MultiByteToWideChar(CP_ACP, 0, strTitle.c_str( ), -1, wstrAppTitle, 64);
        DXUTCreateWindow(wstrAppTitle);
    }

    if (sCmdLineOptions.m_bFullscreen)
    {
        if (sCmdLineOptions.m_bFullscreenResOverride)
            DXUTCreateDevice(false, sCmdLineOptions.m_nWindowWidth, sCmdLineOptions.m_nWindowHeight);
        else
            DXUTCreateDevice(false);
    }
    else
        DXUTCreateDevice(true, sCmdLineOptions.m_nWindowWidth, sCmdLineOptions.m_nWindowHeight);

    DXUTMainLoop( ); // Enter into the DXUT render loop

    st_delete<CMyMouseAndKeyboardNavigation>(g_pUserInput);
    g_pApplication->ReleaseGfxResources( );
    st_delete<CMyApplication>(g_pApplication);

    PrintSpeedTreeErrors( );

    CCore::ShutDown( );
    #ifdef ST_MEMORY_STATS
        SpeedTree::CAllocator::Report("st_memory_report_windows_directx9.csv", true);
    #endif

    // cleanup console
    fclose(stderr);
    fclose(stdout);
    fclose(stdin);
    FreeConsole( );

    return DXUTGetExitCode( );
}



