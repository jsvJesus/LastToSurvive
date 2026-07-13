#pragma once

struct IDirect3DDevice9;

namespace engine::graphics::d3d9
{
    class D3D9Device;
}

namespace engine::legacy::graphics
{
    // Подключает новый DX9 compatibility adapter к уже существующему
    // IDirect3DDevice9 старого renderer.
    //
    // Adapter не владеет native device и не вызывает AddRef/Release.
    [[nodiscard]] bool InitializeD3D9RendererBridge(
        IDirect3DDevice9* device) noexcept;

    // Уничтожает adapter и все зарегистрированные через него ресурсы.
    // Вызывать до Release старого IDirect3DDevice9.
    void ShutdownD3D9RendererBridge() noexcept;

    // Вызывать после освобождения legacy D3DPOOL_DEFAULT ресурсов,
    // но до IDirect3DDevice9::Reset.
    void NotifyD3D9DeviceLost() noexcept;

    // Вызывать только после успешного IDirect3DDevice9::Reset.
    [[nodiscard]] bool NotifyD3D9DeviceReset(
        IDirect3DDevice9* device) noexcept;

    [[nodiscard]] bool IsD3D9RendererBridgeReady() noexcept;

    // Non-owning pointer. Нужен будущему Legacy resource migration.
    [[nodiscard]]
    engine::graphics::d3d9::D3D9Device*
    GetD3D9CompatibilityDevice() noexcept;
}