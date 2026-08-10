#include "../StudioGraphicsShell.h"

#include <Windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    if (!studio::WantsDX11Shell())
    {
        MessageBoxW(
            nullptr,
            L"This Release x64 Studio build requires the -dx11 switch.",
            L"Studio",
            MB_OK | MB_ICONINFORMATION);
        return static_cast<int>(
            studio::StudioGraphicsShellResult::NotRequested);
    }

    return static_cast<int>(studio::RunDX11Shell());
}
