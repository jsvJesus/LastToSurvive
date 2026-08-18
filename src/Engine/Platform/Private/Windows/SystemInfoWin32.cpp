#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include "Platform/SystemInfo.h"

namespace engine::platform
{
    namespace
    {
        [[nodiscard]] ProcessorArchitecture TranslateArchitecture(
            const WORD architecture) noexcept
        {
            switch (architecture)
            {
                case PROCESSOR_ARCHITECTURE_INTEL:
                    return ProcessorArchitecture::X86;

                case PROCESSOR_ARCHITECTURE_AMD64:
                    return ProcessorArchitecture::X64;

                case PROCESSOR_ARCHITECTURE_ARM64:
                    return ProcessorArchitecture::Arm64;

                default:
                    return ProcessorArchitecture::Unknown;
            }
        }
    }

    SystemInfo QuerySystemInfo() noexcept
    {
        SYSTEM_INFO nativeSystemInfo{};
        ::GetNativeSystemInfo(&nativeSystemInfo);

        SystemInfo result{};

        result.architecture =
            TranslateArchitecture(nativeSystemInfo.wProcessorArchitecture);

        result.logicalProcessorCount =
            static_cast<std::uint32_t>(
                nativeSystemInfo.dwNumberOfProcessors);

        result.pageSize =
            static_cast<std::uint32_t>(
                nativeSystemInfo.dwPageSize);

        result.allocationGranularity =
            static_cast<std::uint32_t>(
                nativeSystemInfo.dwAllocationGranularity);

        result.is64BitProcess = sizeof(void*) == 8;

        result.is64BitOperatingSystem =
            result.architecture == ProcessorArchitecture::X64 ||
            result.architecture == ProcessorArchitecture::Arm64;

        MEMORYSTATUSEX memoryStatus{};
        memoryStatus.dwLength = sizeof(memoryStatus);

        if (::GlobalMemoryStatusEx(&memoryStatus) != FALSE)
        {
            result.totalPhysicalMemoryBytes =
                static_cast<std::uint64_t>(
                    memoryStatus.ullTotalPhys);

            result.availablePhysicalMemoryBytes =
                static_cast<std::uint64_t>(
                    memoryStatus.ullAvailPhys);
        }

        return result;
    }

    const char* ToString(
        const ProcessorArchitecture architecture) noexcept
    {
        switch (architecture)
        {
            case ProcessorArchitecture::X86:
                return "x86";

            case ProcessorArchitecture::X64:
                return "x64";

            case ProcessorArchitecture::Arm64:
                return "ARM64";

            default:
                return "Unknown";
        }
    }
}