#pragma once

#include <d3dcompiler.h>
#include <wrl/client.h>

namespace lts::editor
{
    [[nodiscard]]
    bool CompileEditorShaderFile(
        const wchar_t* shaderFileName,
        const char* entryPoint,
        const char* targetProfile,
        const char* logCategory,
        Microsoft::WRL::ComPtr<ID3DBlob>& bytecode) noexcept;
}