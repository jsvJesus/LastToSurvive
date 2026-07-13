#include "D3D11Debug.h"

#include "D3D11ComPtr.h"

#include <d3d11.h>
#include <d3d11sdklayers.h>
#include <cstring>

namespace engine::graphics::d3d11::detail
{
    void SetDebugName(
        ID3D11DeviceChild* object,
        const char* name) noexcept
    {
        if (object == nullptr || name == nullptr || name[0] == '\0')
            return;

        object->SetPrivateData(
            WKPDID_D3DDebugObjectName,
            static_cast<UINT>(std::strlen(name)),
            name);
    }

    void ReportLiveObjects(ID3D11Device* device) noexcept
    {
        if (device == nullptr)
            return;

        ComPtr<ID3D11Debug> debug;
        const HRESULT result = device->QueryInterface(
            __uuidof(ID3D11Debug),
            reinterpret_cast<void**>(debug.Put()));
        if (SUCCEEDED(result))
        {
            debug.Get()->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY);
        }
    }
}
