///////////////////////////////////////////////////////////////////////  
//
//  All source files prefixed with "My" indicate an example implementation, 
//  meant to detail what an application might (and, in most cases, should)
//  do when interfacing with the SpeedTree SDK.  These files constitute 
//  the bulk of the SpeedTree Reference Application and are not part
//  of the official SDK, but merely example or "reference" implementations.
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

#include "DXUT.h"
#pragma warning (disable : 4995)
#include "MyApplication.h"
#include "MyMouseAndKeyboardNavigation.h"
//#define ST_OVERRIDE_GLOBAL_NEW_AND_DELETE
#define ST_USE_ALLOCATOR_INTERFACE // must be active for Debug MT and Release MT builds
#define ST_OVERRIDE_FILESYSTEM
#include "MyCustomAllocator.h"
#include "MyFileSystem.h"
using namespace SpeedTree;

#ifndef NDEBUG
    #pragma comment( lib, "dxguid.lib") 
#endif

// enable to allow debugging with NVPerfHUD
//#define ST_NVPERFHUD


///////////////////////////////////////////////////////////////////////  
//  File-scope function prototypes

static  PCHAR*                          CommandLineToArgvA(PCHAR CmdLine, int* _argc);


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

// app & support
static  CMyApplication*                 g_pApplication = NULL;
static  CMyMouseAndKeyboardNavigation*  g_pUserInput = NULL;


///////////////////////////////////////////////////////////////////////  
//  ModifyDeviceSettings
//
//  Called right before creating a D3D9 or d3d11 device, allowing the app to modify the device settings as needed

bool CALLBACK ModifyDeviceSettings(DXUTDeviceSettings* pDeviceSettings, void* pUserContext)
{
    UNREFERENCED_PARAMETER(pUserContext);

    if (pDeviceSettings->MinimumFeatureLevel == D3D_FEATURE_LEVEL_11_0)
    {
        // Debugging vertex shaders requires either REF or software vertex processing 
        // and debugging pixel shaders requires REF.  
        #ifdef DEBUG_VS
            if (pDeviceSettings->d3d11.DriverType != D3D_DRIVER_TYPE_REFERENCE)
                pDeviceSettings->d3d11.CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
        #endif
        #ifdef DEBUG_PS
            pDeviceSettings->d3d11.DriverType = D3D_DRIVER_TYPE_REFERENCE;
        #endif

        // Request the multisampling quality specified on the command-line
        assert(g_pApplication);
        #ifdef ST_FW_POSTFX
            pDeviceSettings->d3d11.sd.SampleDesc.Count = 1;
        #else
            pDeviceSettings->d3d11.sd.SampleDesc.Count = g_pApplication->GetCmdLineOptions( ).m_nNumSamples;    
        #endif

        // don't use gamma correction
        pDeviceSettings->d3d11.sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        // handle nVidia PerfHUD if it is enabled
        #ifdef ST_NVPERFHUD 
            UINT uiAdapter = 0; 
            IDXGIAdapter* pAdapter = NULL;
            IDXGIFactory* pDXGIFactory = DXUTGetDXGIFactory( );
            while (pDXGIFactory->EnumAdapters(uiAdapter, &pAdapter) != DXGI_ERROR_NOT_FOUND) 
            {
                DXGI_ADAPTER_DESC sDescription; 
                if (pAdapter != NULL && SUCCEEDED(pAdapter->GetDesc(&sDescription)) &&
                    wcscmp(sDescription.Description, L"NVIDIA PerfHUD") == 0) 
                {                        
                    pDeviceSettings->d3d11.AdapterOrdinal = uiAdapter;
                    pDeviceSettings->d3d11.DriverType = D3D_DRIVER_TYPE_REFERENCE;
                    printf("Detected NVIDIA PerfHUD\n");
                }
                    
                ST_SAFE_RELEASE(pAdapter);
                ++uiAdapter;
            }
        #endif
    }

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  Isd3d11DeviceAcceptable
//
//  Reject any D3D11 devices that aren't acceptable by returning false

bool CALLBACK IsD3D11DeviceAcceptable(const CD3D11EnumAdapterInfo* pAdapterInfo, UINT uiOutput, const CD3D11EnumDeviceInfo* pDeviceInfo,
                                       DXGI_FORMAT sBackBufferFormat, bool bWindowed, void* pUserContext)
{
    UNREFERENCED_PARAMETER(pAdapterInfo);
    UNREFERENCED_PARAMETER(uiOutput);
    UNREFERENCED_PARAMETER(pDeviceInfo);
    UNREFERENCED_PARAMETER(sBackBufferFormat);
    UNREFERENCED_PARAMETER(bWindowed);
    UNREFERENCED_PARAMETER(pUserContext);

    return true;
}


///////////////////////////////////////////////////////////////////////  
//  Ond3d11CreateDevice
//
//  Create any d3d11 resources that aren't dependant on the back buffer

HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext)
{
    UNREFERENCED_PARAMETER(pBackBufferSurfaceDesc);
    UNREFERENCED_PARAMETER(pUserContext);

    assert(pd3dDevice);
    SpeedTree::DX11::SetDevice(pd3dDevice);

    return S_OK;
}


///////////////////////////////////////////////////////////////////////  
//  Ond3d11ResizedSwapChain
//
//  Create any d3d11 resources that depend on the back buffer

HRESULT CALLBACK OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice, 
                                         IDXGISwapChain* pSwapChain, 
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, 
                                         void* pUserContext)
{
    UNREFERENCED_PARAMETER(pd3dDevice);
    UNREFERENCED_PARAMETER(pSwapChain);
    UNREFERENCED_PARAMETER(pUserContext);

    if (g_pApplication != NULL)
    {
        g_pApplication->WindowReshape(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
    }

    return S_OK;
}


///////////////////////////////////////////////////////////////////////  
//  Ond3d11ReleasingSwapChain
//
//  Release d3d11 resources created in Ond3d11ResizedSwapChain 

void CALLBACK OnD3D11ReleasingSwapChain(void* pUserContext)
{
    UNREFERENCED_PARAMETER(pUserContext);
}


///////////////////////////////////////////////////////////////////////  
//  Ond3d11DestroyDevice
//
//  Release D3D11 resources created in Ond3d11CreateDevice 

void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
    UNREFERENCED_PARAMETER(pUserContext);
}


///////////////////////////////////////////////////////////////////////  
//  OnFrameMove
//
//  Handle updates to the scene.  This is called regardless of which D3D API is used

void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
    UNREFERENCED_PARAMETER(fTime);
    UNREFERENCED_PARAMETER(fElapsedTime);
    UNREFERENCED_PARAMETER(pUserContext);

}


///////////////////////////////////////////////////////////////////////  
//  Ond3d11FrameRender
//
//  Render the scene using the d3d11 device

void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, 
                                 ID3D11DeviceContext* pd3dImmediateContext,
                                 double fTime, 
                                 float fElapsedTimeInSec, 
                                 void* pUserContext)
{
    UNREFERENCED_PARAMETER(pd3dDevice);
    UNREFERENCED_PARAMETER(fTime);
    UNREFERENCED_PARAMETER(fElapsedTimeInSec);
    UNREFERENCED_PARAMETER(pUserContext);

    bool bInitError = false;
    static bool bFirstDisplay = true;

    CScopeTrace::Init( );

    if (bFirstDisplay)
    {
        DX11::SetDeviceContext(pd3dImmediateContext);

        assert(g_pApplication);
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
        assert(g_pApplication);
        if (g_pApplication->ReadyToRender( ))
        {
            // update the scene before the render; this can be outside of the render thread/function, but must be
            // completed before the render can proceed
            assert(g_pUserInput);
            g_pUserInput->Tick(fElapsedTimeInSec);
            g_pApplication->SetWorldTransform(g_pUserInput->GetTransform( ), g_pUserInput->GetCamera( ).m_vPos);

            g_pApplication->Advance( );
            g_pApplication->Cull( );
            g_pApplication->ReportStats( );

            // draw calls
            const FLOAT c_afBgColor[4] = { 0.0 };
            DX11::DeviceContext()->ClearRenderTargetView(DXUTGetD3D11RenderTargetView(), c_afBgColor);
            DX11::DeviceContext( )->ClearDepthStencilView(DXUTGetD3D11DepthStencilView( ), D3D11_CLEAR_DEPTH, 1.0f, 0);

            g_pApplication->Render( );
        }
    }
}


///////////////////////////////////////////////////////////////////////  
//  MsgProc
//
//  Handle messages to the application

LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(pbNoFurtherProcessing);
    UNREFERENCED_PARAMETER(pUserContext);

    // handle keyboard and mouse events
    int x = int(LOWORD(lParam));
    int y = int(HIWORD(lParam));

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
    UNREFERENCED_PARAMETER(bAltDown);
    UNREFERENCED_PARAMETER(pUserContext);

    if (bKeyDown)
        g_pUserInput->KeyDown(static_cast<st_uchar>(nChar));
    else
        g_pUserInput->KeyUp(static_cast<st_uchar>(nChar));
}


///////////////////////////////////////////////////////////////////////  
//  OnDeviceRemoved
//
//  Call if device was removed. Return true to find a new device, false to quit

bool CALLBACK OnDeviceRemoved(void* pUserContext)
{
    UNREFERENCED_PARAMETER(pUserContext);

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
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Enable run-time memory check for debug builds.
    #if defined(DEBUG) | defined(_DEBUG)
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif

    // set current path to that of the app
    char szBuf[1024];
    GetModuleFileNameA(hInstance, szBuf, 1024);
    SetCurrentDirectoryA(SpeedTree::CString(szBuf).Path( ).c_str( ));

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
                AllocConsole( );
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

        FreeCmdLineAscii(argc, argv);
    }

    // Set general DXUT callbacks
    DXUTSetCallbackFrameMove(OnFrameMove);
    DXUTSetCallbackKeyboard(OnKeyboard);
    DXUTSetCallbackMsgProc(MsgProc);
    DXUTSetCallbackDeviceChanging(ModifyDeviceSettings);
    DXUTSetCallbackDeviceRemoved(OnDeviceRemoved);

    // D3D11 DXUT callbacks
    DXUTSetCallbackD3D11DeviceAcceptable(IsD3D11DeviceAcceptable);
    DXUTSetCallbackD3D11DeviceCreated(OnD3D11CreateDevice);
    DXUTSetCallbackD3D11SwapChainResized(OnD3D11ResizedSwapChain);
    DXUTSetCallbackD3D11FrameRender(OnD3D11FrameRender);
    DXUTSetCallbackD3D11SwapChainReleasing(OnD3D11ReleasingSwapChain);
    DXUTSetCallbackD3D11DeviceDestroyed(OnD3D11DestroyDevice);

    CMyApplication::PrintId( );
    CMyApplication::PrintKeys( );

    // extract command-line settings to set up the window
    assert(g_pApplication);
    const SMyCmdLineOptions& sCmdLine = g_pApplication->GetCmdLineOptions( );

    DXUTSetIsInGammaCorrectMode(false);
    DXUTInit(false, true, NULL); // parse the command line, show msgboxes on error, no extra command line params
    
    DXUTSetCursorSettings(true, true); // show the cursor and clip it when in full screen
    SetCursor(LoadCursor(NULL, IDC_ARROW));

    {
        SpeedTree::CString strTitle = SpeedTree::CString::Format("SpeedTree SDK v%s DirectX 11.0 Application", ST_VERSION_STRING);
        WCHAR wstrAppTitle[64];
        MultiByteToWideChar(CP_ACP, 0, strTitle.c_str( ), -1, wstrAppTitle, 64);
        DXUTCreateWindow(wstrAppTitle);
    }

    if (sCmdLine.m_bFullscreen)
    {
        if (sCmdLine.m_bFullscreenResOverride)
            DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, false, sCmdLine.m_nWindowWidth, sCmdLine.m_nWindowHeight);
        else
            DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, false);
    }
    else
        DXUTCreateDevice(D3D_FEATURE_LEVEL_11_0, true, sCmdLine.m_nWindowWidth, sCmdLine.m_nWindowHeight);

    DXUTMainLoop( ); // Enter into the DXUT render loop

    // save preset cameras and destroy navigation object
    (void) g_pUserInput->SavePresetCameraFile(g_pApplication->GetConfig( ).m_sNavigation.m_strCameraFilename);
    st_delete<CMyMouseAndKeyboardNavigation>(g_pUserInput);

    // destroy ST application object
    st_delete<CMyMouseAndKeyboardNavigation>(g_pUserInput);
    g_pApplication->ReleaseGfxResources( );
    st_delete<CMyApplication>(g_pApplication);

    CCore::ShutDown( );
    #ifdef ST_MEMORY_STATS
        SpeedTree::CAllocator::Report("st_memory_report_windows_directx11.csv", true);
    #endif

    // cleanup console
    fclose(stderr);
    fclose(stdout);
    fclose(stdin);
    FreeConsole( );

    return DXUTGetExitCode( );;
}





