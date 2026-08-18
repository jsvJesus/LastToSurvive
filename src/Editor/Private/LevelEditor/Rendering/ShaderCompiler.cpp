#include "Editor/LevelEditor/Rendering/ShaderCompiler.h"

#include <Core/Log.h>

#include <filesystem>
#include <string>
#include <system_error>

namespace lts::editor
{
    namespace
    {
        [[nodiscard]] std::filesystem::path FindBinRoot(
            std::filesystem::path current)
        {
            std::error_code error;

            for (;;)
            {
                if (current.filename() == L"bin" &&
                    std::filesystem::is_directory(
                        current / L"Data" / L"Shaders" / L"DX11_P1" / L"Editor",
                        error))
                {
                    return current;
                }

                error.clear();
                const std::filesystem::path nested = current / L"bin";

                if (std::filesystem::is_directory(
                        nested / L"Data" / L"Shaders" / L"DX11_P1" / L"Editor",
                        error))
                {
                    return nested;
                }

                const std::filesystem::path parent = current.parent_path();

                if (parent.empty() || parent == current)
                {
                    return {};
                }

                current = parent;
                error.clear();
            }
        }
    }

    bool CompileEditorShaderFile(
        const wchar_t* const shaderFileName,
        const char* const entryPoint,
        const char* const targetProfile,
        const char* const logCategory,
        Microsoft::WRL::ComPtr<ID3DBlob>& bytecode) noexcept
    {
        bytecode.Reset();

        const char* const category =
            logCategory != nullptr
                ? logCategory
                : "LTS.Editor.Shader";

        if (
            shaderFileName == nullptr ||
            shaderFileName[0] == L'\0' ||
            entryPoint == nullptr ||
            targetProfile == nullptr)
        {
            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                category,
                "Invalid editor shader compilation arguments.");

            return false;
        }

        try
        {
            std::error_code filesystemError;

            const std::filesystem::path workingDirectory =
                std::filesystem::current_path(filesystemError);

            const std::filesystem::path binRoot =
                FindBinRoot(workingDirectory);

            if (filesystemError || binRoot.empty())
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    category,
                    "Failed to resolve the Studio bin working directory.");

                return false;
            }

            const std::filesystem::path shaderPath =
                binRoot /
                L"Data" /
                L"Shaders" /
                L"DX11_P1" /
                L"Editor" /
                shaderFileName;

            if (!std::filesystem::is_regular_file(
                    shaderPath,
                    filesystemError) ||
                filesystemError)
            {
                std::string message =
                    "Editor shader file does not exist: ";

                message +=
                    shaderPath.generic_u8string();

                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    category,
                    message);

                return false;
            }

            Microsoft::WRL::ComPtr<ID3DBlob>
                errors;

            constexpr UINT compileFlags =
                D3DCOMPILE_ENABLE_STRICTNESS |
                D3DCOMPILE_WARNINGS_ARE_ERRORS |
                D3DCOMPILE_OPTIMIZATION_LEVEL3;

            const HRESULT result =
                D3DCompileFromFile(
                    shaderPath.c_str(),
                    nullptr,
                    D3D_COMPILE_STANDARD_FILE_INCLUDE,
                    entryPoint,
                    targetProfile,
                    compileFlags,
                    0U,
                    bytecode.GetAddressOf(),
                    errors.GetAddressOf());

            if (SUCCEEDED(result))
            {
                return true;
            }

            if (
                errors != nullptr &&
                errors->GetBufferPointer() != nullptr)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    category,
                    static_cast<const char*>(
                        errors->GetBufferPointer()));
            }
            else
            {
                std::string message =
                    "Failed to compile editor shader: ";

                message +=
                    shaderPath.generic_u8string();

                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    category,
                    message);
            }

            bytecode.Reset();
            return false;
        }
        catch (...)
        {
            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                category,
                "Unexpected editor shader compilation failure.");

            bytecode.Reset();
            return false;
        }
    }
}
