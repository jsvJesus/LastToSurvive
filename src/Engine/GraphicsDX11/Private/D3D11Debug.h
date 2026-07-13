#pragma once

struct ID3D11Device;
struct ID3D11DeviceChild;

namespace engine::graphics::d3d11::detail
{
    void SetDebugName(
        ID3D11DeviceChild* object,
        const char* name) noexcept;

    void ReportLiveObjects(
        ID3D11Device* device) noexcept;
}
