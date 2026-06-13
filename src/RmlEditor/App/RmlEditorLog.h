#pragma once

namespace RmlEditorLog
{
    bool Initialize();
    void Shutdown();

    void Write(const char* Format, ...);
}