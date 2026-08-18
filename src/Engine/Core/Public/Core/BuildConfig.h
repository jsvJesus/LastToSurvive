#pragma once

#include <cstddef>
#include <cstdint>

#if !defined(_MSC_VER)
    #error LTS.Core currently supports the Microsoft C++ compiler only.
#endif

#if _MSC_VER < 1930
    #error Visual Studio 2022 or newer is required.
#endif

#if !defined(_WIN64)
    #error LTS.Core must be compiled for x64.
#endif

#if !defined(_MSVC_LANG) || _MSVC_LANG < 201703L
    #error C++17 or newer is required.
#endif

#if defined(ENGINE_BUILD_RELEASE) && defined(ENGINE_BUILD_FINAL)
    #error Only one engine build configuration may be selected.
#endif

#if !defined(ENGINE_BUILD_RELEASE) && !defined(ENGINE_BUILD_FINAL)
    #error Engine build configuration is not defined.
#endif

namespace engine::core
{
    enum class BuildConfiguration : std::uint8_t
    {
        Release,
        Final
    };

#if defined(ENGINE_BUILD_FINAL)
    inline constexpr BuildConfiguration CurrentBuildConfiguration =
        BuildConfiguration::Final;
#else
    inline constexpr BuildConfiguration CurrentBuildConfiguration =
        BuildConfiguration::Release;
#endif

    inline constexpr bool Is64Bit = sizeof(void*) == 8;
    inline constexpr std::size_t PointerSize = sizeof(void*);

    static_assert(Is64Bit, "The engine supports x64 builds only.");
    static_assert(PointerSize == 8, "Unexpected pointer size.");
}