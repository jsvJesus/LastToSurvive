#pragma once

#include <cstdint>

namespace lts::editor
{
    class DockLayout final
    {
    public:
        [[nodiscard]] std::uint32_t DrawDockSpace() noexcept;
        void RequestReset() noexcept;

    private:
        void BuildDefault(std::uint32_t dockspaceId) noexcept;

        bool resetRequested_ = false;
    };
}
