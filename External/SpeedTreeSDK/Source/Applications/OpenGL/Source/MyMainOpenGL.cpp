///////////////////////////////////////////////////////////////////////
//  MyMainOpenGL.cpp
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


///////////////////////////////////////////////////////////////////////
//  Preprocessor

#ifndef ST_OPENGL
#error ST_OPENGL should be defined for the entire application before it can build correctly
#endif

#include "MyApplication.h"
#include "MyMouseAndKeyboardNavigation.h"
//#define ST_OVERRIDE_GLOBAL_NEW_AND_DELETE
#define ST_USE_ALLOCATOR_INTERFACE // must be active for Debug MT and Release MT builds
#define ST_OVERRIDE_FILESYSTEM
#include "MyCustomAllocator.h"
#include "MyFileSystem.h"
#define GLEW_STATIC
#define GLEW_NO_GLU
#include "Utilities/glew.h"
#include "Utilities/glfw/include/GL/glfw.h"
#ifdef __GNUC__
#else
    #include "Utilities/wglew.h"
#endif
#include "Renderers/OpenGL/OpenGLRenderer.h"
#include "MyCmdLineOptions.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//  File-scope functions

static  void                            InitGlfw(void);
static  void                            InitOpenGL(void);
static  void                            GlfwMainLoop(void);

        // glfw callback functions
static  void                            GlfwKey(st_int32 chKey, st_int32 nAction);
static  void                            GlfwMouseButton(st_int32 nButton, st_int32 nAction);
static  void                            GlfwMouseMotion(st_int32 nX, st_int32 nY);
static  void                            GlfwReshape(st_int32 nWindowWidth, st_int32 nWindowHeight);
static  int                             GlfwClose(void);


///////////////////////////////////////////////////////////////////////
//  File-scope variables

        // memory management
#ifdef ST_USE_ALLOCATOR_INTERFACE
        // passing the allocator to a static member helps get it placed as
        // soon as possible to hopefully trap all relevant heap allocations
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
static  st_bool                         g_bRunning = false;


///////////////////////////////////////////////////////////////////////
//  main

st_int32 main(st_int32 argc, char* argv[])
{
    st_int32 nErrorCode = -1;

    #ifdef _WIN32
        // set current path to that of the app
        if (argc > 0)
            SetCurrentDirectoryA(CFixedString(argv[0]).Path( ).c_str( ));
    #endif

    g_pApplication = st_new(CMyApplication, "CMyApplication");
    if (g_pApplication->ParseCmdLine(argc, argv))
    {
        if (g_pApplication->Init( ))
        {
            // main init/loop
            InitGlfw( );

            // hide cursor
            ShowCursor(FALSE);

            GlfwMainLoop( );
            GlfwClose( );
        }
        else
            Error("Failed to initialize forest");
    }

    glfwTerminate( );

    return nErrorCode;
}


///////////////////////////////////////////////////////////////////////
//  InitGlfw

void InitGlfw(void)
{
    if (!glfwInit( ))
    {
        Error("Failed to initialize GLFW\n");
        exit(EXIT_FAILURE);
    }

    // extract command-line settings to set up the window
    assert(g_pApplication);
    const SMyCmdLineOptions& sCmdLineOptions = g_pApplication->GetCmdLineOptions( );

    if (sCmdLineOptions.m_nNumSamples > 0)
        glfwOpenWindowHint(GLFW_FSAA_SAMPLES, sCmdLineOptions.m_nNumSamples);

    #ifdef ST_OPENGL_USE_CORE_3_2
        glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
        glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
        glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #endif

    if (!glfwOpenWindow(sCmdLineOptions.m_nWindowWidth, sCmdLineOptions.m_nWindowHeight,
                        8, 8, 8, 8, 24, 0,
                        sCmdLineOptions.m_bFullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW))
    {
        Error("Failed to open GLFW window\n");
        glfwTerminate( );
        exit(EXIT_FAILURE);
    }

    // center window on main display
    GLFWvidmode sVideoMode;
    glfwGetDesktopMode(&sVideoMode);
    glfwSetWindowPos(sVideoMode.Width / 2 - sCmdLineOptions.m_nWindowWidth / 2, sVideoMode.Height / 2 - sCmdLineOptions.m_nWindowHeight / 2);

    // center mouse pointer in window
    glfwSetMousePos(sCmdLineOptions.m_nWindowWidth / 2, sCmdLineOptions.m_nWindowHeight / 2);

    // set title and other values
    st_bool bDeferred = g_pApplication->GetCmdLineOptions( ).m_bDeferred;
    CFixedString strTitle = CFixedString::Format("SpeedTree SDK v%s OpenGL Application (%s)", ST_VERSION_STRING, bDeferred ? "Deferred" : "Forward");
    glfwSetWindowTitle(strTitle.c_str( ));
    glfwSwapInterval(0);
    glfwSetTime(0.0);
    glfwEnable(GLFW_AUTO_POLL_EVENTS);

    // init user input
    g_pUserInput = st_new(CMyMouseAndKeyboardNavigation, "CMyMouseAndKeyboardNavigation")(g_pApplication);

    // setup glfw callback functions
    glfwSetWindowSizeCallback(GlfwReshape);
    glfwSetWindowCloseCallback(GlfwClose);
    glfwSetKeyCallback(GlfwKey);
    glfwSetMousePosCallback(GlfwMouseMotion);
    glfwSetMouseButtonCallback(GlfwMouseButton);
    glfwDisable(GLFW_KEY_REPEAT);

    // get rid of any glfw-specific warnings
    while (glGetError( ) != GL_NO_ERROR)
    {
    }
}


///////////////////////////////////////////////////////////////////////
//  GlfwClose

int GlfwClose(void)
{
    glFinish( );

    if (g_pApplication != NULL)
    {
        st_delete<CMyMouseAndKeyboardNavigation>(g_pUserInput);
        g_pApplication->ReleaseGfxResources( );
        st_delete<CMyApplication>(g_pApplication);

        CCore::ShutDown( );
        #ifdef ST_MEMORY_STATS
            CAllocator::Report("st_memory_report_windows_opengl.csv", true);
        #endif
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////
//  GlfwMainLoop

void GlfwMainLoop(void)
{
    InitOpenGL( );

    assert(g_pApplication);
    if (!g_pApplication->InitGfx( ))
    {
        system("pause");
        return;
    }
    else
        g_pUserInput->GotoPresetCamera(0);

    // center the mouse pointer
    POINT sCenter;
    sCenter.x = g_pApplication->GetCmdLineOptions( ).m_nWindowWidth / 2;
    sCenter.y = g_pApplication->GetCmdLineOptions( ).m_nWindowHeight / 2;
    ClientToScreen(GetActiveWindow( ), &sCenter);
    SetCursorPos(sCenter.x, sCenter.y);

    g_bRunning = true;
    do
    {
        // update the scene before the render; this can be outside of the render thread/function, but must be
        // completed before the render can proceed
        assert(g_pUserInput);
        g_pUserInput->Tick(g_pApplication->GetLastFrameTime( ));
        g_pApplication->SetWorldTransform(g_pUserInput->GetTransform( ), g_pUserInput->GetCamera( ).m_vPos);

        g_pApplication->Advance( );
        g_pApplication->Cull( );
        g_pApplication->ReportStats( );

        // draw calls
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        g_pApplication->Render( );

        glfwSwapBuffers( );

        PrintOpenGLErrors("Render loop");

        // make sure window is still open
        g_bRunning &= (g_pApplication != NULL) && (glfwGetWindowParam(GLFW_OPENED) != 0);
    }
    while(g_bRunning);
}


///////////////////////////////////////////////////////////////////////
//  InitOpenGL

void InitOpenGL(void)
{
    // initialize OpenGL extensions (for DDS files, compression, etc)
    #ifdef ST_OPENGL_USE_CORE_3_2
        glewExperimental = GL_TRUE; // seems to be needed to get proper extensions for NVIDIA hardware
    #endif

    GLenum eReturn = CGraphicsCapsOpenGL::InitGlew( );
    if (eReturn != GLEW_OK)
    {
        Error("GLEW initialization failed OpenGL Reference Application: %s\n", glewGetErrorString(eReturn));
        exit(EXIT_FAILURE); // completely fatal, must have DDS support
    }

    eReturn = CGraphicsCapsOpenGL::InitGlew( ); // depending on the linkage, the OpenGL rendering library may
                                                // have its own copy of glew that needs separate initialization
    if (eReturn != GLEW_OK)
    {
        Error("GLEW initialization failed for OpenGL rendering library: %s\n", glewGetErrorString(eReturn));
        exit(EXIT_FAILURE); // completely fatal, must have DDS support
    }

    // check that OpenGL 2.0 is available as the SpeedTree renderer requires it
    if (!GLEW_VERSION_2_0)
    {
        Error("The SpeedTree OpenGL implementation requires OpenGL 2.0 or better to run; this system has version %s", glGetString(GL_VERSION));
        exit(EXIT_FAILURE);
    }

    // get rid of any glew-specific warnings
    while (glGetError( ) != GL_NO_ERROR)
    {
    }

    // identify program
    CMyApplication::PrintId( );

    // print some useful information to the console
    Report("\n         OpenGL Version: %s", glGetString(GL_VERSION));
    Report("        OpenGL Renderer: %s", glGetString(GL_RENDERER));
    Report("           glew Version: %s\n", glewGetString(GLEW_VERSION));

    // set OpenGL render states
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    PrintOpenGLErrors("InitOpenGL()");

    CMyApplication::PrintKeys( );
}


///////////////////////////////////////////////////////////////////////
//  GlfwKey

void GlfwKey(st_int32 chKey, st_int32 nAction)
{
    if (g_pApplication != NULL)
    {
        st_int32 x = 0;
        st_int32 y = 0;
        glfwGetMousePos(&x, &y);

        if (nAction == GLFW_PRESS)
        {
            switch (chKey)
            {
            case GLFW_KEY_ESC: // esc, quit
                g_bRunning = false;
                break;
            default:
                if (g_pUserInput)
                    g_pUserInput->KeyDown(static_cast<st_uchar>(chKey));
                break;
            }
        }
        else
        {
            if (g_pUserInput)
                g_pUserInput->KeyUp(static_cast<st_uchar>(chKey));
        }
    }
}


///////////////////////////////////////////////////////////////////////
//  GlfwMouseButton

void GlfwMouseButton(st_int32 nButton, st_int32 nAction)
{
    if (g_pApplication != NULL)
    {
        st_int32 x = 0;
        st_int32 y = 0;
        glfwGetMousePos(&x, &y);

        bool bPressed = (nAction == GLFW_PRESS);
        if (nButton == GLFW_MOUSE_BUTTON_1)
            g_pUserInput->MouseClick(MOUSE_BUTTON_LEFT, bPressed, x, y);
        else if (nButton == GLFW_MOUSE_BUTTON_2)
            g_pUserInput->MouseClick(MOUSE_BUTTON_MIDDLE, bPressed, x, y);
        else
            g_pUserInput->MouseClick(MOUSE_BUTTON_RIGHT, bPressed, x, y);
    }
}


///////////////////////////////////////////////////////////////////////
//  GlfwMouseMotion

void GlfwMouseMotion(st_int32 x, st_int32 y)
{
    if (g_pApplication != NULL)
    {
        g_pUserInput->MouseMotion(x, y);
    }
}


///////////////////////////////////////////////////////////////////////
//  GlfwReshape

void GlfwReshape(st_int32 nWindowWidth, st_int32 nWindowHeight)
{
    if (g_pApplication != NULL)
    {
        glViewport(0, 0, nWindowWidth, nWindowHeight);
        g_pApplication->WindowReshape(nWindowWidth, nWindowHeight);
    }
}


