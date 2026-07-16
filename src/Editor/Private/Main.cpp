#include "Editor/EditorApplication.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

int WINAPI wWinMain(
    HINSTANCE,
    HINSTANCE,
    PWSTR,
    int)
{
    lts::editor::EditorApplication application;

    const auto result =
        application.Run();

    return static_cast<int>(result);
}