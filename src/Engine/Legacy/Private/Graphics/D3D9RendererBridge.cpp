#include "Legacy/Graphics/D3D9RendererBridge.h"

#include "Graphics/GraphicsBackend.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/RenderDevice.h"
#include "GraphicsDX9/D3D9Device.h"

#include <new>

namespace engine::legacy::graphics
{
    namespace
    {
        using CompatibilityDevice =
            engine::graphics::d3d9::D3D9Device;

        CompatibilityDevice* g_compatibilityDevice = nullptr;

        // Остаётся true, пока native Reset не завершится успешно.
        bool g_deviceLostNotificationPending = false;

        void DestroyCompatibilityDevice() noexcept
        {
            if (g_compatibilityDevice == nullptr)
            {
                g_deviceLostNotificationPending = false;
                return;
            }

            g_compatibilityDevice->Shutdown();

            delete g_compatibilityDevice;
            g_compatibilityDevice = nullptr;

            g_deviceLostNotificationPending = false;
        }
    }

    bool InitializeD3D9RendererBridge(
        IDirect3DDevice9* device) noexcept
    {
        if (device == nullptr)
        {
            return false;
        }

        // Повторная инициализация тем же работающим устройством.
        if (g_compatibilityDevice != nullptr)
        {
            if (
                g_compatibilityDevice->GetNativeDevice() == device &&
                g_compatibilityDevice->IsReady()
            )
            {
                return true;
            }

            DestroyCompatibilityDevice();
        }

        CompatibilityDevice* compatibilityDevice =
            new (std::nothrow) CompatibilityDevice();

        if (compatibilityDevice == nullptr)
        {
            return false;
        }

        const engine::graphics::GraphicsResult attachResult =
            compatibilityDevice->AttachExternalDevice(device);

        if (engine::graphics::Failed(attachResult))
        {
            delete compatibilityDevice;
            return false;
        }

        engine::graphics::RenderDeviceDesc deviceDesc;
        deviceDesc.backend =
            engine::graphics::GraphicsBackend::D3D9;

        // Пока adapter работает только как compatibility bridge.
        deviceDesc.enableValidation = false;
        deviceDesc.enableDebugMarkers = false;

        const engine::graphics::GraphicsResult initializeResult =
            compatibilityDevice->Initialize(deviceDesc);

        if (engine::graphics::Failed(initializeResult))
        {
            compatibilityDevice->Shutdown();
            delete compatibilityDevice;
            return false;
        }

        g_compatibilityDevice = compatibilityDevice;
        g_deviceLostNotificationPending = false;

        return true;
    }

    void ShutdownD3D9RendererBridge() noexcept
    {
        DestroyCompatibilityDevice();
    }

    void NotifyD3D9DeviceLost() noexcept
    {
        if (
            g_compatibilityDevice == nullptr ||
            g_deviceLostNotificationPending
        )
        {
            return;
        }

        // Перед native Reset adapter должен находиться в Ready.
        if (!g_compatibilityDevice->IsReady())
        {
            return;
        }

        g_compatibilityDevice->OnDeviceLost();
        g_deviceLostNotificationPending = true;
    }

    bool NotifyD3D9DeviceReset(
        IDirect3DDevice9* device) noexcept
    {
        if (
            g_compatibilityDevice == nullptr ||
            !g_deviceLostNotificationPending
        )
        {
            return false;
        }

        const engine::graphics::GraphicsResult resetResult =
            g_compatibilityDevice->OnDeviceReset(device);

        if (engine::graphics::Failed(resetResult))
        {
            // Adapter остаётся в состоянии Lost.
            // Следующая успешная попытка native Reset снова вызовет эту функцию.
            return false;
        }

        g_deviceLostNotificationPending = false;
        return true;
    }

    bool IsD3D9RendererBridgeReady() noexcept
    {
        return
            g_compatibilityDevice != nullptr &&
            g_compatibilityDevice->IsReady();
    }

    engine::graphics::d3d9::D3D9Device*
    GetD3D9CompatibilityDevice() noexcept
    {
        return g_compatibilityDevice;
    }
}