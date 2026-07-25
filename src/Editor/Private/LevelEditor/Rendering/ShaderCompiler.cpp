#include "Editor/LevelEditor/Rendering/ShaderCompiler.h"

#include <Core/Log.h>

#include <filesystem>
#include <string>
#include <system_error>

namespace lts::editor
{
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

            const std::filesystem::path gameRoot =
                std::filesystem::current_path(
                    filesystemError);

            if (filesystemError)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    category,
                    "Failed to resolve the editor working directory.");

                return false;
            }

            const std::filesystem::path shaderPath =
                gameRoot /
                L"Data" /
                L"Shaders" /
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