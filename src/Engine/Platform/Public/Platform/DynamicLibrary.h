#pragma once

#include "Platform/Path.h"

#include <cstdint>
#include <type_traits>
#include <utility>

namespace engine::platform
{
    using DynamicFunction = void (*)();

    class DynamicLibrary final
    {
    public:
        DynamicLibrary() noexcept = default;

        explicit DynamicLibrary(const Path& path);

        ~DynamicLibrary() noexcept;

        DynamicLibrary(const DynamicLibrary&) = delete;
        DynamicLibrary& operator=(const DynamicLibrary&) = delete;

        DynamicLibrary(DynamicLibrary&& other) noexcept;
        DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;

        [[nodiscard]] bool Load(const Path& path);

        void Unload() noexcept;

        [[nodiscard]] bool IsLoaded() const noexcept;

        [[nodiscard]] explicit operator bool() const noexcept;

        [[nodiscard]] DynamicFunction FindFunction(
            const char* functionName) const noexcept;

        template <typename Function>
        [[nodiscard]] Function FindFunctionAs(
            const char* functionName) const noexcept
        {
            static_assert(
                std::is_pointer_v<Function>,
                "Function must be a function pointer type.");

            static_assert(
                std::is_function_v<std::remove_pointer_t<Function>>,
                "Function must point to a function type.");

            return reinterpret_cast<Function>(
                FindFunction(functionName));
        }

        [[nodiscard]] bool HasFunction(
            const char* functionName) const noexcept;

        [[nodiscard]] const Path& GetPath() const noexcept;

        [[nodiscard]] std::uint32_t GetLastErrorCode() const noexcept;

    private:
        std::uintptr_t nativeHandle_ = 0;
        Path loadedPath_;
        mutable std::uint32_t lastErrorCode_ = 0;
    };
}