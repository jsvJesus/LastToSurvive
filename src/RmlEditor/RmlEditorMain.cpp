#include "App/RmlEditorApplication.h"

#include <windows.h>

int APIENTRY wWinMain(
    HINSTANCE Instance,
    HINSTANCE PreviousInstance,
    PWSTR CommandLine,
    int ShowCommand
)
{
    (void)PreviousInstance;
    (void)CommandLine;
    (void)ShowCommand;

    RmlEditorApplication Application;

    if (!Application.Initialize(Instance))
        return 1;

    return Application.Run();
}