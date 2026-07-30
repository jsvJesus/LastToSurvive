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


///////////////////////////////////////////////////////////////////////
//  Preprocessor

#include "MyCmdLineOptions.h"
#include "Core/String.h"
#include "Utilities/Utility.h"

#ifdef __CELLOS_LV2__
    #include <sys/paths.h>
#endif

using namespace std;
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//  PrintUsage

void PrintUsage(const char* pExeName)
{
    Report("\tSpeedTree 7.1.0 Application (c) 2002-2016");
    Report("\nUsage:\n\n%s {<options>}\n", CFixedString(pExeName).NoPath( ).c_str( ));
    Report("where options can be (all units are same as the SRT & terrain files):\n");

    Report("\nFile options");
    Report("\t-sfc <text file>              loads SFC (SpeedTree Forest Config) file");

    Report("\nWindow options");
    Report("\t-res <width> <height>         window resolution");
    Report("\t-windowed                     makes the application run in a window");
    Report("\t-fullscreen                   makes the application run in fullscreen mode");
    Report("\t-samples <count>              multisampling configuration");
    Report("\t-anisotropy <value>           set the max texture anisotropy");
    Report("\t-gamma <value>                sets the screen gamma");
}


///////////////////////////////////////////////////////////////////////
//  SMyCmdLineOptions::SMyCmdLineOptions

SMyCmdLineOptions::SMyCmdLineOptions( ) :
    // display
    m_nWindowWidth(1280),
    m_nWindowHeight(720),
    m_bFullscreen(false),
    m_bFullscreenResOverride(false),
    m_bConsole(false),
    m_bDeferred(false),
    m_nNumSamples(1),
    m_nMaxAnisotropy(1),
    m_fGamma(1.0f)
{
}


///////////////////////////////////////////////////////////////////////
//  CMyCmdLineParser::Parse

st_bool CMyCmdLineParser::Parse(st_int32 argc, char* argv[], SMyCmdLineOptions& sConfig)
{
    st_bool bSuccess = false;
 
    st_bool bNeedsPrintUsage = false;
    if (argc == 1)
    {
        #ifdef _XBOX
            sConfig.m_strSfcFilename = "game:\\forests\\meadow\\xbox_360.sfc";
            sConfig.m_bDeferred = false;
            sConfig.m_nWindowWidth = 1280;
            sConfig.m_nWindowHeight = 720;
            bSuccess = true;
        #elif defined(_DURANGO)
            // Golden Mountain forests, forward & deferred (choose one)
            sConfig.m_nWindowWidth = 1920;
            sConfig.m_nWindowHeight = 1080;
            sConfig.m_strSfcFilename = "./meadow/xbox_one.sfc";
            sConfig.m_nMaxAnisotropy = 0; // (>1) enabled anisotropic filtering
            sConfig.m_bDeferred = false;
            sConfig.m_nNumSamples = 2; // (>1) enables multisampling
            
            // deferred
            //sConfig.m_bDeferred = true;
            //sConfig.m_nNumSamples = 1; // (>1) enables multisampling (currently unsupported for deferred)

            bSuccess = true;
        #elif defined(__CELLOS_LV2__) // PS3
            sConfig.m_strSfcFilename = "/app_home/forests/meadow/ps3.sfc";
            sConfig.m_nNumSamples = 1;
            bSuccess = true;
        #elif defined(__ORBIS__)
            sConfig.m_strSfcFilename = "/hostapp/forests/meadow/ps4.sfc";
            sConfig.m_nWindowWidth = 1920;
            sConfig.m_nWindowHeight = 1080;
            sConfig.m_nNumSamples = 1; // MSAA CURRENTLY UNSUPPORTED ON PS4
            sConfig.m_nMaxAnisotropy = 0;
            sConfig.m_bDeferred = false;
            bSuccess = true;
        #elif defined(WIN32) || defined(__APPLE__) || defined(__GNUC__)
            sConfig.m_strSfcFilename = "../../forests/meadow/dx11_dx9_opengl.sfc";
            sConfig.m_nWindowWidth = 1920;
            sConfig.m_nWindowHeight = 1080;
            sConfig.m_nNumSamples = 4;
            sConfig.m_bDeferred = false;
            sConfig.m_bConsole = true;
            bSuccess = true;
        #else
            bNeedsPrintUsage = true;
        #endif
    }
    else
    {
        // scan the options
        for (st_int32 i = 1; i < argc; ++i)
        {
            CFixedString strCommand = argv[i];
            if (strCommand == "-sfc")
            {
                if (i + 1 < argc)
                {
                    sConfig.m_strSfcFilename = CFileSystemInterface::Get( )->CleanPlatformFilename(argv[++i]);
                }
                else
                    bNeedsPrintUsage = true;
            }
            else if (strCommand == "-res")
            {
                if (i + 2 < argc)
                {
                    sConfig.m_nWindowWidth = atoi(argv[++i]);
                    sConfig.m_nWindowHeight = atoi(argv[++i]);
                    sConfig.m_bFullscreenResOverride = true;
                }
                else
                    bNeedsPrintUsage = true;
            }
            else if (strCommand == "-samples")
            {
                if (i + 1 < argc)
                    sConfig.m_nNumSamples = st_max(1, atoi(argv[++i]));
                else
                    bNeedsPrintUsage = true;
            }
            else if (strCommand == "-windowed" ||
                     strCommand == "-window")
            {
                sConfig.m_bFullscreen = false;
            }
            else if (strCommand == "-fullscreen")
            {
                sConfig.m_bFullscreen = true;
            }
            else if (strCommand == "-anisotropy")
            {
                if (i + 1 < argc)
                {
                    sConfig.m_nMaxAnisotropy = atoi(argv[++i]);
                }
                else
                    bNeedsPrintUsage = true;
            }
            else if (strCommand == "-gamma")
            {
                if (i + 1 < argc)
                    sConfig.m_fGamma = st_float32(atof(argv[++i]));
                else
                    bNeedsPrintUsage = true;
            }
            else if (strCommand == "-console")
            {
                sConfig.m_bConsole = true;
            }
            else if (strCommand == "-deferred")
            {
                sConfig.m_bDeferred = true;
            }
            else
            {
                Warning("Unknown command [%s] (cmd-line argument #%d)\n", strCommand.c_str( ), i);
                bNeedsPrintUsage = true;
            }
        }
    }

    if (bNeedsPrintUsage)
        PrintUsage(argv[0]);
    else
        bSuccess = true;

    return bSuccess;
}



