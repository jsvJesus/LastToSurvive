#include "Editor/EditorApplication.h"

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