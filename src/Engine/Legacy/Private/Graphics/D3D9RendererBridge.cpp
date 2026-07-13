#include "Legacy/Graphics/D3D9RendererBridge.h"

#include "Legacy/Assets/LegacyTextureAssetBridge.h"

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

        // Remains true until the native Reset finishes successfully.
        bool g_deviceLostNotificationPending = false;

        void DestroyCompatibilityDevice() noexcept
        {
            if (g_compatibilityDevice == nullptr)
            {
                engine::legacy::assets::AbandonLegacyTextureAssetBridge();
                g_deviceLostNotificationPending = false;
                return;
            }

            const engine::legacy::assets::LegacyTextureBridgeResult
                assetShutdownResult =
                    engine::legacy::assets::
                        ShutdownLegacyTextureAssetBridge();

            if (!assetShutdownResult.Succeeded())
            {
                // The adapter is about to clear its own registry. Drop bridge
                // handles so they can never outlive the compatibility device.
                engine::legacy::assets::
                    AbandonLegacyTextureAssetBridge();
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

        // Reinitialization with the same working device.
        if (g_compatibilityDevice != nullptr)
        {
            if (
                g_compatibilityDevice->GetNativeDevice() == device &&
                g_compatibilityDevice->IsReady())
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

        // The adapter currently works only as a compatibility bridge.
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
            g_deviceLostNotificationPending)
        {
            return;
        }

        // Before native Reset the adapter must still be Ready.
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
            !g_deviceLostNotificationPending)
        {
            return false;
        }

        const engine::graphics::GraphicsResult resetResult =
            g_compatibilityDevice->OnDeviceReset(device);

        if (engine::graphics::Failed(resetResult))
        {
            // Adapter remains Lost. A later successful native Reset can retry.
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
