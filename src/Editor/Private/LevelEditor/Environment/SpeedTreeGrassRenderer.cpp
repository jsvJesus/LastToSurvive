#include "Editor/LevelEditor/Environment/SpeedTreeGrassRenderer.h"

#include "Editor/LevelEditor/Environment/GrassEditor.h"

#include <Graphics/GraphicsBackend.h>
#include <Graphics/RenderDevice.h>
#include <GraphicsDX11/D3D11Device.h>

#include <Core/CoordSys.h>
#include <Renderers/DirectX11/DirectX11Renderer.h>

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace lts::editor
{
    namespace
    {
        constexpr float GrassCellSize = 32.0F;
        constexpr float PhysicalNearPlane = 0.1F;
        constexpr float PhysicalFarPlane = 2000.0F;
        constexpr float Pi = 3.14159265358979323846F;

        constexpr SpeedTree::st_int32 MaximumVisibleGrassCells =
            4096;

        constexpr SpeedTree::st_int32 MaximumInstancesPerCell =
            4096;

        struct CellKey final
        {
            SpeedTree::st_int32 row = 0;
            SpeedTree::st_int32 column = 0;

            [[nodiscard]]
            bool operator==(const CellKey& other) const noexcept
            {
                return
                    row == other.row &&
                    column == other.column;
            }
        };

        struct CellKeyHash final
        {
            [[nodiscard]]
            std::size_t operator()(
                const CellKey& key) const noexcept
            {
                const std::size_t rowHash =
                    std::hash<SpeedTree::st_int32>{}(
                        key.row);

                const std::size_t columnHash =
                    std::hash<SpeedTree::st_int32>{}(
                        key.column);

                return
                    rowHash ^
                    (
                        columnHash +
                        static_cast<std::size_t>(
                            0x9E3779B9U) +
                        (rowHash << 6U) +
                        (rowHash >> 2U)
                    );
            }
        };

        [[nodiscard]]
        std::string LowercaseAscii(
            std::string value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](const unsigned char character)
                {
                    if (character >= 'A' &&
                        character <= 'Z')
                    {
                        return static_cast<char>(
                            character -
                            static_cast<unsigned char>('A') +
                            static_cast<unsigned char>('a'));
                    }

                    return static_cast<char>(character);
                });

            return value;
        }

        [[nodiscard]]
        bool IsSafeRelativePath(
            const std::filesystem::path& path)
        {
            if (path.empty() || path.is_absolute())
            {
                return false;
            }

            for (const std::filesystem::path& part : path)
            {
                if (part == L"..")
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]]
        std::string MakeAssetKey(
            const std::filesystem::path& relativePath)
        {
            return LowercaseAscii(
                relativePath.lexically_normal().
                    generic_u8string());
        }

        [[nodiscard]]
        SpeedTree::Mat4x4 ToSpeedTreeMatrix(
            const DirectX::XMFLOAT4X4& matrix)
        {
            SpeedTree::Mat4x4 result;

            result.Set(&matrix._11);

            return result;
        }

        [[nodiscard]]
        SpeedTree::Vec3 ToSpeedTreePosition(
            const DirectX::XMFLOAT3& position)
        {
            return SpeedTree::Vec3(
                position.x,
                position.y,
                position.z);
        }

        void SetCellExtents(
            SpeedTree::CCell& cell)
        {
            const float minimumX =
                static_cast<float>(cell.Col()) *
                GrassCellSize;

            const float minimumZ =
                static_cast<float>(cell.Row()) *
                GrassCellSize;

            const float maximumX =
                minimumX + GrassCellSize;

            const float maximumZ =
                minimumZ + GrassCellSize;

            /*
             * Нам важны X/Z границы клетки.
             *
             * Y намеренно широкий, потому что Terrain может
             * находиться выше или ниже нулевого уровня.
             */
            cell.SetExtents(
                SpeedTree::CExtents(
                    SpeedTree::Vec3(
                        minimumX,
                        -32768.0F,
                        minimumZ),
                    SpeedTree::Vec3(
                        maximumX,
                        32768.0F,
                        maximumZ)));
        }

        [[nodiscard]]
        std::string BuildSpeedTreeError(
            const char* operation)
        {
            std::string result = operation;

            const char* speedTreeError =
                SpeedTree::CCore::GetError();

            if (speedTreeError != nullptr &&
                speedTreeError[0] != '\0')
            {
                result += ": ";
                result += speedTreeError;
            }

            return result;
        }
    }

    class SpeedTreeGrassRenderer::Impl final
    {
    public:
        using CellInstanceMap = std::unordered_map<
            CellKey,
            std::vector<SpeedTree::SInstance>,
            CellKeyHash>;

        class Layer final
        {
        public:
            Layer()
                : visible(
                    SpeedTree::POPULATION_GRASS,
                    false)
            {
                SpeedTree::SHeapReserves reserves;

                reserves.m_nMaxBaseTrees = 1;
                reserves.m_nMaxVisibleGrassCells =
                    MaximumVisibleGrassCells;

                reserves.m_nMaxPerBaseGrassInstancesInAnyCell =
                    MaximumInstancesPerCell;

                visible.SetHeapReserves(reserves);
                visible.SetCellSize(GrassCellSize);
            }

            Layer(const Layer&) = delete;
            Layer& operator=(const Layer&) = delete;

            std::unique_ptr<SpeedTree::CTreeRender> tree;
            SpeedTree::CVisibleInstancesRender visible;

            CellInstanceMap cellInstances;

            float largestOverhang = 1.0F;
        };

        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice& device,
            const std::filesystem::path& workspaceRoot)
        {
            if (initialized)
            {
                return true;
            }

            lastError.clear();

            if (device.GetBackend() !=
                engine::graphics::GraphicsBackend::D3D11)
            {
                lastError =
                    "SpeedTree Grass requires D3D11.";

                return false;
            }

            auto& d3d11Device =
                static_cast<
                    engine::graphics::D3D11Device&>(
                        device);

            ID3D11Device* nativeDevice =
                d3d11Device.GetNativeDevice();

            ID3D11DeviceContext* nativeContext =
                d3d11Device.GetNativeImmediateContext();

            if (nativeDevice == nullptr ||
                nativeContext == nullptr)
            {
                lastError =
                    "D3D11 native device is not available.";

                return false;
            }

            workspaceRootPath =
                workspaceRoot.lexically_normal();

            grassAssetRoot =
                workspaceRootPath /
                L"bin" /
                L"Data" /
                L"SpeedTree" /
                L"Grass";

            shaderRoot =
                workspaceRootPath /
                L"External" /
                L"SpeedTreeSDK" /
                L"bin" /
                L"forests" /
                L"meadow" /
                L"compiled_assets" /
                L"desktop" /
                L"forward" /
                L"shared_shaders" /
                L"shaders_directx11";

            std::error_code filesystemError;

            if (!std::filesystem::is_directory(
                    grassAssetRoot,
                    filesystemError) ||
                filesystemError)
            {
                lastError =
                    "SpeedTree Grass directory not found: " +
                    grassAssetRoot.generic_u8string();

                return false;
            }

            filesystemError.clear();

            if (!std::filesystem::is_directory(
                    shaderRoot,
                    filesystemError) ||
                filesystemError)
            {
                lastError =
                    "SpeedTree DX11 shaders not found: " +
                    shaderRoot.generic_u8string();

                return false;
            }

            SpeedTree::DX11::SetDevice(nativeDevice);
            SpeedTree::DX11::SetDeviceContext(nativeContext);

            SpeedTree::CCoordSys::SetCoordSys(
                SpeedTree::CCoordSys::
                    COORD_SYS_LEFT_HANDED_Y_UP);

            SpeedTree::CCore::SetClipSpaceDepthRange(
                0.0F,
                1.0F);

            SpeedTree::SHeapReserves reserves;

            reserves.m_nMaxBaseTrees = 256;
            reserves.m_nMaxVisibleGrassCells =
                MaximumVisibleGrassCells;

            reserves.m_nMaxPerBaseGrassInstancesInAnyCell =
                MaximumInstancesPerCell;

            forest.SetHeapReserves(reserves);

            SpeedTree::SForestRenderInfo renderInfo;

            renderInfo.m_sAppState.m_bMultisampling =
                false;

            renderInfo.m_sAppState.m_bAlphaToCoverage =
                false;

            renderInfo.m_sAppState.m_bDepthPrepass =
                false;

            renderInfo.m_sAppState.m_bDeferred =
                false;

            renderInfo.m_sAppState.m_eShadowConfig =
                SpeedTree::SRenderState::
                    SHADOW_CONFIG_OFF;

            renderInfo.m_sAppState.m_eOverrideDepthTest =
                SpeedTree::SAppState::
                    OVERRIDE_DEPTH_TEST_ENABLE;

            renderInfo.m_nMaxAnisotropy = 16;
            renderInfo.m_bHorizontalBillboards = false;
            renderInfo.m_bDepthOnlyPrepass = false;

            renderInfo.m_fNearClip =
                PhysicalNearPlane;

            renderInfo.m_fFarClip =
                PhysicalFarPlane;

            renderInfo.m_bTexturingEnabled = true;

            renderInfo.m_fTextureAlphaScalar3d =
                0.5F;

            renderInfo.m_fTextureAlphaScalarGrass =
                0.5F;

            renderInfo.m_fTextureAlphaScalarBillboards =
                0.5F;

            renderInfo.m_bShadowsEnabled = false;
            renderInfo.m_nShadowsNumMaps = 0;

            renderInfo.m_fFogStartDistance =
                PhysicalFarPlane;

            renderInfo.m_fFogEndDistance =
                PhysicalFarPlane;

            forest.SetRenderInfo(renderInfo);

            forest.SetLightDir(
                SpeedTree::Vec3(
                    -0.35F,
                    -0.85F,
                    0.25F));

            if (!forest.InitGfx())
            {
                lastError =
                    BuildSpeedTreeError(
                        "CForestRender::InitGfx failed");

                forest.ReleaseGfxResources();

                SpeedTree::DX11::SetDeviceContext(nullptr);
                SpeedTree::DX11::SetDevice(nullptr);

                return false;
            }

            if (!forest.UpdateFogAndSkyConstantBuffer())
            {
                lastError =
                    BuildSpeedTreeError(
                        "SpeedTree fog constant buffer failed");

                forest.ReleaseGfxResources();

                SpeedTree::DX11::SetDeviceContext(nullptr);
                SpeedTree::DX11::SetDevice(nullptr);

                return false;
            }

            nativeD3D11Device = nativeDevice;
            nativeD3D11Context = nativeContext;

            initialized = true;
            frameIndex = 0;

            return true;
        }

        void Shutdown()
        {
            for (auto& entry : layers)
            {
                Layer& layer = *entry.second;

                layer.visible.ReleaseGfxResources();

                if (layer.tree)
                {
                    layer.tree->ReleaseGfxResources();
                    layer.tree.reset();
                }

                layer.cellInstances.clear();
            }

            layers.clear();

            if (initialized)
            {
                forest.ReleaseGfxResources();
            }

            SpeedTree::DX11::SetDeviceContext(nullptr);
            SpeedTree::DX11::SetDevice(nullptr);

            nativeD3D11Device = nullptr;
            nativeD3D11Context = nullptr;

            initialized = false;
            frameIndex = 0;
        }

        [[nodiscard]]
        Layer* FindOrLoadLayer(
            const std::filesystem::path& relativeSrtPath)
        {
            if (!IsSafeRelativePath(relativeSrtPath))
            {
                lastError =
                    "Unsafe Grass .srt path: " +
                    relativeSrtPath.generic_u8string();

                return nullptr;
            }

            const std::string key =
                MakeAssetKey(relativeSrtPath);

            const auto existing =
                layers.find(key);

            if (existing != layers.end())
            {
                return existing->second.get();
            }

            const std::filesystem::path sourcePath =
                (
                    grassAssetRoot /
                    relativeSrtPath
                ).lexically_normal();

            std::error_code filesystemError;

            if (!std::filesystem::is_regular_file(
                    sourcePath,
                    filesystemError) ||
                filesystemError)
            {
                lastError =
                    "Grass .srt not found: " +
                    sourcePath.generic_u8string();

                return nullptr;
            }

            auto layer =
                std::make_unique<Layer>();

            layer->tree =
                std::make_unique<
                    SpeedTree::CTreeRender>();

            const std::string sourcePathUtf8 =
                sourcePath.generic_u8string();

            if (!layer->tree->LoadTree(
                    sourcePathUtf8.c_str(),
                    true,
                    1.0F))
            {
                lastError =
                    BuildSpeedTreeError(
                        "CTreeRender::LoadTree failed");

                return nullptr;
            }

            SpeedTree::CArray<
                SpeedTree::CFixedString>
                    searchPaths;

            const std::string sourceDirectory =
                sourcePath.parent_path().
                    generic_u8string();

            const std::string grassDirectory =
                grassAssetRoot.generic_u8string();

            const std::string compiledShaderDirectory =
                shaderRoot.generic_u8string();

            searchPaths.push_back(
                SpeedTree::CFixedString(
                    sourceDirectory.c_str()));

            searchPaths.push_back(
                SpeedTree::CFixedString(
                    grassDirectory.c_str()));

            searchPaths.push_back(
                SpeedTree::CFixedString(
                    compiledShaderDirectory.c_str()));

            if (!layer->tree->InitGfx(
                    forest.GetRenderInfo().m_sAppState,
                    searchPaths,
                    forest.GetRenderInfo().
                        m_nMaxAnisotropy,
                    forest.GetRenderInfo().
                        m_fTextureAlphaScalarGrass))
            {
                lastError =
                    BuildSpeedTreeError(
                        "CTreeRender::InitGfx failed");

                layer->tree->ReleaseGfxResources();
                return nullptr;
            }

            layer->largestOverhang =
                (std::max)(
                    1.0F,
                    layer->tree->
                        GetExtents().
                        ComputeRadiusFromCenter3D() *
                    1.25F);

            Layer* result = layer.get();

            layers.emplace(
                key,
                std::move(layer));

            return result;
        }

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            const std::uint32_t viewportWidth,
            const std::uint32_t viewportHeight,
            const DirectX::XMFLOAT4X4& viewMatrix,
            const DirectX::XMFLOAT4X4& projectionMatrix,
            const DirectX::XMFLOAT3& cameraPosition,
            const GrassEditor& grassEditor)
        {
            if (!initialized ||
                nativeD3D11Device == nullptr ||
                nativeD3D11Context == nullptr)
            {
                lastError =
                    "SpeedTree Grass renderer is not initialized.";

                return
                    engine::graphics::
                        GraphicsResult::InvalidState;
            }

            if (viewportWidth == 0U ||
                viewportHeight == 0U)
            {
                return
                    engine::graphics::
                        GraphicsResult::Success;
            }

            SpeedTree::DX11::SetDevice(
                nativeD3D11Device);

            SpeedTree::DX11::SetDeviceContext(
                nativeD3D11Context);

            for (auto& entry : layers)
            {
                entry.second->cellInstances.clear();
            }

            const float viewDistance =
                grassEditor.GetViewDistance();

            const float viewDistanceSquared =
                viewDistance * viewDistance;

            for (const GrassRenderInstance& sourceInstance :
                 grassEditor.GetRenderInstances())
            {
                const float dx =
                    sourceInstance.x -
                    cameraPosition.x;

                const float dz =
                    sourceInstance.z -
                    cameraPosition.z;

                if (dx * dx + dz * dz >
                    viewDistanceSquared)
                {
                    continue;
                }

                Layer* layer =
                    FindOrLoadLayer(
                        sourceInstance.relativeSrtPath);

                if (layer == nullptr ||
                    !layer->tree)
                {
                    return
                        engine::graphics::
                            GraphicsResult::BackendFailure;
                }

                SpeedTree::SInstance instance;

                instance.m_vPos =
                    SpeedTree::Vec3(
                        sourceInstance.x,
                        sourceInstance.y,
                        sourceInstance.z);

                instance.m_fScalar =
                    (std::max)(
                        0.01F,
                        sourceInstance.scale);

                instance.m_vUp =
                    SpeedTree::Vec3(
                        0.0F,
                        1.0F,
                        0.0F);

                const float yawRadians =
                    sourceInstance.yawDegrees *
                    (Pi / 180.0F);

                /*
                 * Только вращение вокруг мировой оси Y.
                 *
                 * Здесь нет pitch/roll и нет прижатия всей
                 * геометрии к normal Terrain. Поэтому .srt
                 * не будет случайно ложиться набок.
                 */
                instance.m_vRight =
                    SpeedTree::Vec3(
                        std::cos(yawRadians),
                        0.0F,
                        -std::sin(yawRadians));

                instance.m_fLodTransition = 1.0F;
                instance.m_fLodValue = 1.0F;

                SpeedTree::st_int32 row = 0;
                SpeedTree::st_int32 column = 0;

                SpeedTree::ComputeCellCoords(
                    instance.m_vPos,
                    GrassCellSize,
                    row,
                    column);

                layer->cellInstances[
                    CellKey{row, column}
                ].push_back(instance);
            }

            SpeedTree::Mat4x4 speedTreeView =
                ToSpeedTreeMatrix(viewMatrix);

            SpeedTree::Mat4x4 speedTreeProjection =
                ToSpeedTreeMatrix(projectionMatrix);

            SpeedTree::CView renderView;

            renderView.Set(
                ToSpeedTreePosition(cameraPosition),
                speedTreeProjection,
                speedTreeView,
                PhysicalNearPlane,
                PhysicalFarPlane,
                false);

            /*
             * Основная Projection в редакторе Reverse-Z.
             * Старый SpeedTree culling ожидает обычную Z projection.
             *
             * Поэтому отдельно создаём обычную projection только
             * для Rough/Fine culling. В GPU constant buffer всё равно
             * передаётся настоящая Reverse-Z projection.
             */
            const float aspectRatio =
                static_cast<float>(viewportWidth) /
                static_cast<float>(viewportHeight);

            const DirectX::XMMATRIX normalCullProjection =
                DirectX::XMMatrixPerspectiveFovLH(
                    DirectX::XMConvertToRadians(60.0F),
                    aspectRatio,
                    PhysicalNearPlane,
                    PhysicalFarPlane);

            DirectX::XMFLOAT4X4 normalCullProjectionFloat{};

            DirectX::XMStoreFloat4x4(
                &normalCullProjectionFloat,
                normalCullProjection);

            SpeedTree::CView cullView;

            cullView.Set(
                ToSpeedTreePosition(cameraPosition),
                ToSpeedTreeMatrix(
                    normalCullProjectionFloat),
                speedTreeView,
                PhysicalNearPlane,
                PhysicalFarPlane,
                false);

            const SpeedTree::st_int32 currentFrame =
                frameIndex;

            ++frameIndex;

            if (frameIndex < 0 ||
                frameIndex ==
                    (std::numeric_limits<
                        SpeedTree::st_int32>::max)())
            {
                frameIndex = 0;
            }

            std::vector<Layer*> activeLayers;
            activeLayers.reserve(layers.size());

            static const SpeedTree::SInstance
                EmptyInstance;

            for (auto& entry : layers)
            {
                Layer& layer = *entry.second;

                if (!layer.tree ||
                    layer.cellInstances.empty())
                {
                    continue;
                }

                layer.visible.NotifyOfPopulationChange();

                if (!layer.visible.RoughCullCells(
                        cullView,
                        currentFrame,
                        layer.largestOverhang))
                {
                    lastError =
                        BuildSpeedTreeError(
                            "SpeedTree RoughCullCells failed");

                    return
                        engine::graphics::
                            GraphicsResult::BackendFailure;
                }

                SpeedTree::TCellArray& roughCells =
                    layer.visible.RoughCells();

                for (std::size_t index = 0U;
                     index < roughCells.size();
                     ++index)
                {
                    SetCellExtents(
                        roughCells[index]);
                }

                if (!layer.visible.FineCullGrassCells(
                        cullView,
                        currentFrame,
                        layer.largestOverhang))
                {
                    lastError =
                        BuildSpeedTreeError(
                            "SpeedTree FineCullGrassCells failed");

                    return
                        engine::graphics::
                            GraphicsResult::BackendFailure;
                }

                SpeedTree::TCellPtrArray& newCells =
                    layer.visible.NewlyVisibleCells();

                for (std::size_t index = 0U;
                     index < newCells.size();
                     ++index)
                {
                    SpeedTree::CCell* cell =
                        newCells[index];

                    if (cell == nullptr)
                    {
                        continue;
                    }

                    const CellKey key
                    {
                        cell->Row(),
                        cell->Col()
                    };

                    const auto found =
                        layer.cellInstances.find(key);

                    if (found ==
                        layer.cellInstances.end() ||
                        found->second.empty())
                    {
                        cell->SetGrassInstances(
                            &EmptyInstance,
                            0);

                        continue;
                    }

                    const std::vector<
                        SpeedTree::SInstance>&
                            instances =
                                found->second;

                    const std::size_t safeCount =
                        (std::min)(
                            instances.size(),
                            static_cast<std::size_t>(
                                MaximumInstancesPerCell));

                    cell->SetGrassInstances(
                        instances.data(),
                        static_cast<
                            SpeedTree::st_int32>(
                                safeCount));
                }

                if (!layer.visible.
                        UpdateGrassInstanceBuffers(
                            layer.tree.get()))
                {
                    lastError =
                        BuildSpeedTreeError(
                            "SpeedTree grass instance upload failed");

                    return
                        engine::graphics::
                            GraphicsResult::BackendFailure;
                }

                activeLayers.push_back(&layer);
            }

            if (!forest.UpdateFrameConstantBuffer(
                    renderView,
                    static_cast<SpeedTree::st_int32>(
                        viewportWidth),
                    static_cast<SpeedTree::st_int32>(
                        viewportHeight)))
            {
                lastError =
                    BuildSpeedTreeError(
                        "SpeedTree frame constant buffer failed");

                return
                    engine::graphics::
                        GraphicsResult::BackendFailure;
            }

            if (!forest.StartRender())
            {
                lastError =
                    BuildSpeedTreeError(
                        "SpeedTree StartRender failed");

                return
                    engine::graphics::
                        GraphicsResult::BackendFailure;
            }

            bool renderSucceeded = true;

            for (const Layer* layer : activeLayers)
            {
                renderSucceeded =
                    forest.RenderGrass(
                        SpeedTree::RENDER_PASS_MAIN,
                        layer->tree.get(),
                        layer->visible) &&
                    renderSucceeded;
            }

            const bool endSucceeded =
                forest.EndRender();

            if (!renderSucceeded ||
                !endSucceeded)
            {
                lastError =
                    BuildSpeedTreeError(
                        "SpeedTree RenderGrass failed");

                return
                    engine::graphics::
                        GraphicsResult::BackendFailure;
            }

            lastError.clear();

            return
                engine::graphics::
                    GraphicsResult::Success;
        }

        std::filesystem::path workspaceRootPath;
        std::filesystem::path grassAssetRoot;
        std::filesystem::path shaderRoot;

        SpeedTree::CForestRender forest;

        std::unordered_map<
            std::string,
            std::unique_ptr<Layer>>
                layers;

        ID3D11Device* nativeD3D11Device = nullptr;
        ID3D11DeviceContext* nativeD3D11Context = nullptr;

        SpeedTree::st_int32 frameIndex = 0;

        std::string lastError;
        bool initialized = false;
    };

    SpeedTreeGrassRenderer::
        SpeedTreeGrassRenderer() noexcept
        : impl_(
            new (std::nothrow) Impl())
    {
    }

    SpeedTreeGrassRenderer::
        ~SpeedTreeGrassRenderer() noexcept = default;

    bool SpeedTreeGrassRenderer::Initialize(
        engine::graphics::RenderDevice& device,
        const std::filesystem::path& workspaceRoot) noexcept
    {
        try
        {
            if (!impl_)
            {
                impl_.reset(
                    new (std::nothrow) Impl());
            }

            return
                impl_ != nullptr &&
                impl_->Initialize(
                    device,
                    workspaceRoot);
        }
        catch (...)
        {
            return false;
        }
    }

    void SpeedTreeGrassRenderer::Shutdown(
        engine::graphics::RenderDevice&) noexcept
    {
        if (!impl_)
        {
            return;
        }

        try
        {
            impl_->Shutdown();
        }
        catch (...)
        {
        }
    }

    engine::graphics::GraphicsResult
        SpeedTreeGrassRenderer::Render(
            const std::uint32_t viewportWidth,
            const std::uint32_t viewportHeight,
            const DirectX::XMFLOAT4X4& view,
            const DirectX::XMFLOAT4X4& projection,
            const DirectX::XMFLOAT3& cameraPosition,
            const GrassEditor& grassEditor) noexcept
    {
        if (!impl_)
        {
            return
                engine::graphics::
                    GraphicsResult::InvalidState;
        }

        try
        {
            return impl_->Render(
                viewportWidth,
                viewportHeight,
                view,
                projection,
                cameraPosition,
                grassEditor);
        }
        catch (...)
        {
            return
                engine::graphics::
                    GraphicsResult::BackendFailure;
        }
    }

    const std::string&
        SpeedTreeGrassRenderer::
            GetLastError() const noexcept
    {
        static const std::string NoImplementation =
            "SpeedTree Grass renderer allocation failed.";

        return impl_
            ? impl_->lastError
            : NoImplementation;
    }
}