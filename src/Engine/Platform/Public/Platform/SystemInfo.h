#pragma once

#include <cstdint>

namespace engine::platform
{
    enum class ProcessorArchitecture : std::uint8_t
    {
        Unknown,
        X86,
        X64,
        Arm64
    };

    struct SystemInfo final
    {
        ProcessorArchitecture architecture = ProcessorArchitecture::Unknown;

        std::uint32_t logicalProcessorCount = 0;
        std::uint32_t pageSize = 0;
        std::uint32_t allocationGranularity = 0;

        std::uint64_t totalPhysicalMemoryBytes = 0;
        std::uint64_t availablePhysicalMemoryBytes = 0;

        bool is64BitProcess = false;
        bool is64BitOperatingSystem = false;
    };

    [[nodiscard]] SystemInfo QuerySystemInfo() noexcept;

    [[nodiscard]] const char* ToString(
        ProcessorArchitecture architecture) noexcept;
}