#include "r3dPCH.h"
#include "r3d.h"

#include "rendering/DX11/LevelEditorStaticBridge.h"
#include "rendering/World/WorldDX11.h"

#if LTS_STUDIO_DX11

#include "rendering/DX11/RenderDX11Core.h"

#include "gameobjects/GameObj.h"
#include "gameobjects/ObjManag.h"
#include "gameobjects/obj_Mesh.h"

#include <Assets/AssetLoaderRegistry.h>
#include <Assets/AssetManager.h>
#include <Assets/DdsTextureLoader.h>
#include <Assets/FileAssetSource.h>
#include <Assets/MaterialAssetLoader.h>
#include <Assets/MeshAssetLoader.h>
#include <Assets/ShaderAssetLoader.h>
#include <Assets/StaticModelAssetLoader.h>

#include <Graphics/GraphicsResult.h>
#include <Graphics/Viewport.h>
#include <GraphicsDX11/D3D11Device.h>

#include <Math/Matrix4.h>
#include <Renderer/RenderView.h>
#include <Renderer/StaticModelRenderer.h>

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <new>
#include <string>
#include <vector>

extern char __r3dCmdLine[1024];
PCHAR* CommandLineToArgvA(PCHAR commandLine, int* argumentCount);

extern bool g_bEditMode;
extern int CurHUDID;

namespace
{
    constexpr int LevelEditorHudIndex = 1;
    constexpr std::size_t MaximumBridgeInstances = 4096U;
    constexpr std::size_t MaximumLegacyPathLength = 1024U;

    struct BridgeState final
    {
        engine::graphics::d3d11::D3D11Device device;
        engine::graphics::CommandContext* context = nullptr;

        engine::assets::FileAssetSource assetSource;
        engine::assets::AssetManager assetManager;
        engine::assets::AssetLoaderRegistry assetRegistry;

        engine::assets::StaticModelAssetLoader staticModelLoader;
        engine::assets::MeshAssetLoader meshLoader;
        engine::assets::MaterialAssetLoader materialLoader;
        engine::assets::DdsTextureLoader textureLoader;
        engine::assets::ShaderAssetLoader shaderLoader;

        engine::renderer::StaticModelRenderer renderer;
        engine::renderer::StaticModelRenderHandle model;

        std::vector<engine::renderer::StaticModelInstance> instances;

        std::string legacyPath;
        engine::assets::AssetPath cookedModelPath;

        bool ready = false;
        bool failed = false;
        bool firstFrameLogged = false;
        bool noMatchingObjectsLogged = false;
        bool instanceLimitLogged = false;
    };

    BridgeState gBridge;

    void BridgeLog(const char* format, ...)
    {
        if (!format)
        {
            return;
        }

        char text[2048] = {};

        va_list arguments;
        va_start(arguments, format);

        vsnprintf_s(
            text,
            sizeof(text),
            _TRUNCATE,
            format,
            arguments);

        va_end(arguments);

        OutputDebugStringA(text);
        r3dOutToLog("%s", text);
    }

    bool HasCommandLineSwitch(const char* switchName)
    {
        if (!switchName || !switchName[0])
        {
            return false;
        }

        int argumentCount = 0;
        PCHAR* arguments = CommandLineToArgvA(
            __r3dCmdLine,
            &argumentCount);

        if (!arguments)
        {
            return false;
        }

        bool found = false;

        for (int index = 0; index < argumentCount; ++index)
        {
            if (_stricmp(arguments[index], switchName) == 0)
            {
                found = true;
                break;
            }
        }

        GlobalFree(arguments);
        return found;
    }

    std::string GetCommandLineValue(const char* prefix)
    {
        if (!prefix || !prefix[0])
        {
            return {};
        }

        int argumentCount = 0;
        PCHAR* arguments = CommandLineToArgvA(
            __r3dCmdLine,
            &argumentCount);

        if (!arguments)
        {
            return {};
        }

        std::string value;
        const std::size_t prefixLength = std::strlen(prefix);

        for (int index = 0; index < argumentCount; ++index)
        {
            if (
                _strnicmp(
                    arguments[index],
                    prefix,
                    prefixLength) == 0)
            {
                value.assign(arguments[index] + prefixLength);
                break;
            }
        }

        GlobalFree(arguments);
        return value;
    }

    char NormalizePathCharacter(const char value) noexcept
    {
        if (value == '\\')
        {
            return '/';
        }

        return static_cast<char>(
            std::tolower(
                static_cast<unsigned char>(value)));
    }

    bool StartsWithDataPrefix(
        const char* value) noexcept
    {
        return
            value != nullptr &&
            value[0] == 'd' &&
            value[1] == 'a' &&
            value[2] == 't' &&
            value[3] == 'a' &&
            value[4] == '/';
    }

    bool NormalizeLegacyPath(
        const char* input,
        char* output,
        const std::size_t outputCapacity) noexcept
    {
        if (
            !input ||
            !input[0] ||
            !output ||
            outputCapacity < 2U)
        {
            return false;
        }

        char temporary[MaximumLegacyPathLength] = {};
        std::size_t writeIndex = 0U;
        bool previousWasSlash = false;

        for (
            const char* cursor = input;
            *cursor != '\0';
            ++cursor)
        {
            char value = NormalizePathCharacter(*cursor);

            if (value == '/')
            {
                if (previousWasSlash)
                {
                    continue;
                }

                previousWasSlash = true;
            }
            else
            {
                previousWasSlash = false;
            }

            if (writeIndex + 1U >= sizeof(temporary))
            {
                return false;
            }

            temporary[writeIndex++] = value;
        }

        temporary[writeIndex] = '\0';

        while (
            writeIndex > 0U &&
            temporary[writeIndex - 1U] == '/')
        {
            temporary[--writeIndex] = '\0';
        }

        const char* normalized = temporary;

        while (*normalized == '/')
        {
            ++normalized;
        }

        while (
            normalized[0] == '.' &&
            normalized[1] == '/')
        {
            normalized += 2;
        }

        if (StartsWithDataPrefix(normalized))
        {
            normalized += 5;
        }

        const std::size_t normalizedLength =
            std::strlen(normalized);

        if (
            normalizedLength == 0U ||
            normalizedLength + 1U > outputCapacity)
        {
            return false;
        }

        std::memcpy(
            output,
            normalized,
            normalizedLength + 1U);

        return true;
    }

    bool MatchesLegacyPath(
        const char* candidate,
        const std::string& expected) noexcept
    {
        if (expected.empty())
        {
            return false;
        }

        char normalized[MaximumLegacyPathLength] = {};

        if (!NormalizeLegacyPath(
                candidate,
                normalized,
                sizeof(normalized)))
        {
            return false;
        }

        return _stricmp(
            normalized,
            expected.c_str()) == 0;
    }

    engine::math::Matrix4 ConvertMatrix(
        const D3DXMATRIX& source) noexcept
    {
        static_assert(
            sizeof(D3DXMATRIX) ==
            sizeof(engine::math::Matrix4));

        engine::math::Matrix4 result;

        std::memcpy(
            result.Data(),
            &source._11,
            sizeof(result.m));

        return result;
    }

    engine::assets::AssetResult LoadTypedAsset(
        const engine::assets::AssetPath& path,
        const engine::assets::AssetType expectedType,
        std::unique_ptr<engine::assets::LoadedAsset>& output) noexcept
    {
        using engine::assets::AssetResult;

        output.reset();

        engine::assets::AssetHandle handle;
        AssetResult result =
            gBridge.assetManager.FindByPath(path, handle);

        if (result == AssetResult::NotFound)
        {
            engine::assets::AssetMetadata metadata;
            metadata.path = path;
            metadata.id = path.GetId();
            metadata.type = expectedType;

            result = gBridge.assetManager.Register(
                metadata,
                handle);
        }

        if (engine::assets::Failed(result))
        {
            return result;
        }

        result = gBridge.assetManager.Load(handle);

        if (engine::assets::Failed(result))
        {
            return result;
        }

        engine::assets::AssetMetadata metadata;
        result = gBridge.assetManager.GetMetadata(
            handle,
            metadata);

        if (engine::assets::Failed(result))
        {
            return result;
        }

        if (metadata.type != expectedType)
        {
            return AssetResult::TypeMismatch;
        }

        const engine::assets::AssetData* data = nullptr;
        result = gBridge.assetManager.GetData(
            handle,
            data);

        if (engine::assets::Failed(result))
        {
            return result;
        }

        if (!data)
        {
            return AssetResult::IoError;
        }

        result = gBridge.assetRegistry.Load(
            metadata,
            *data,
            output);

        if (engine::assets::Failed(result))
        {
            return result;
        }

        if (
            !output ||
            output->GetType() != expectedType)
        {
            output.reset();
            return AssetResult::TypeMismatch;
        }

        return AssetResult::Success;
    }

    engine::assets::AssetResult LoadShader(
        const engine::assets::AssetPath& path,
        engine::assets::ShaderAsset& output) noexcept
    {
        using engine::assets::AssetResult;

        std::unique_ptr<engine::assets::LoadedAsset> loaded;

        const AssetResult result = LoadTypedAsset(
            path,
            engine::assets::AssetType::Shader,
            loaded);

        if (engine::assets::Failed(result))
        {
            return result;
        }

        auto* shaderAsset =
            static_cast<engine::assets::ShaderLoadedAsset*>(
                loaded.get());

        if (!shaderAsset->GetShader().IsValid())
        {
            return AssetResult::TypeMismatch;
        }

        output = shaderAsset->ReleaseShader();
        return AssetResult::Success;
    }

    bool IsLevelEditorModeActive() noexcept
    {
        return
            g_bEditMode &&
            CurHUDID == LevelEditorHudIndex;
    }

    bool IsObjectVisibleForBridge(
        const GameObject* object) noexcept
    {
        if (!object)
        {
            return false;
        }

        if (
            !object->isActive() ||
            !object->bLoaded)
        {
            return false;
        }

        if (
            object->ObjFlags &
            (
                OBJFLAG_SkipDraw |
                OBJFLAG_PlayerCollisionOnly |
                OBJFLAG_Removed
            ))
        {
            return false;
        }

        if (
            !object->InMainFrustum &&
            !(object->ObjFlags & OBJFLAG_AlwaysDraw))
        {
            return false;
        }

        return true;
    }

    void ReleaseBridgeResources() noexcept
    {
        gBridge.instances.clear();

        if (gBridge.model.IsValid())
        {
            (void)gBridge.renderer.DestroyModel(
                gBridge.model);
        }

        gBridge.model = {};
        gBridge.renderer.Shutdown();

        // Освобождает только собственные AddRef и registry resources.
        // Реальный native device остаётся владельцем RenderDX11Core.
        gBridge.device.Shutdown();
        gBridge.context = nullptr;

        gBridge.assetRegistry.Clear();
        gBridge.assetManager.Shutdown();
        gBridge.assetSource.Shutdown();

        gBridge.ready = false;
        gBridge.firstFrameLogged = false;
        gBridge.noMatchingObjectsLogged = false;
        gBridge.instanceLimitLogged = false;
    }

    bool FailInitialization(
        const char* operation,
        const engine::assets::AssetResult result) noexcept
    {
        BridgeLog(
            "[DX11][StaticBridge] %s failed: %s\n",
            operation,
            engine::assets::ToString(result));

        ReleaseBridgeResources();
        gBridge.failed = true;
        return false;
    }

    bool FailInitialization(
        const char* operation,
        const engine::graphics::GraphicsResult result) noexcept
    {
        BridgeLog(
            "[DX11][StaticBridge] %s failed: %s\n",
            operation,
            engine::graphics::ToString(result));

        ReleaseBridgeResources();
        gBridge.failed = true;
        return false;
    }

    bool BuildInstances()
    {
        gBridge.instances.clear();

        ObjectManager& world = GameWorld();
        const int objectCount = world.GetStaticObjectCount();

        const std::size_t requiredCapacity =
            (std::min)(
                static_cast<std::size_t>(
                    objectCount > 0 ? objectCount : 0),
                MaximumBridgeInstances);

        if (gBridge.instances.capacity() < requiredCapacity)
        {
            gBridge.instances.reserve(requiredCapacity);
        }

        for (int index = 0; index < objectCount; ++index)
        {
            GameObject* object = world.GetStaticObject(index);

            if (!IsObjectVisibleForBridge(object))
            {
                continue;
            }

            if (!LevelEditorStaticBridge_ShouldReplaceObject(object))
            {
                continue;
            }

            if (
                gBridge.instances.size() >=
                MaximumBridgeInstances)
            {
                if (!gBridge.instanceLimitLogged)
                {
                    gBridge.instanceLimitLogged = true;

                    BridgeLog(
                        "[DX11][StaticBridge] Instance limit reached: %zu\n",
                        MaximumBridgeInstances);
                }

                break;
            }

            engine::renderer::StaticModelInstance instance;
            instance.model = gBridge.model;
            instance.world = ConvertMatrix(
                object->GetTransformMatrix());
            instance.objectId = object->GetHashID();
            instance.visible = true;

            if (object->isObjType(OBJTYPE_Mesh))
            {
                const auto* meshObject =
                    static_cast<const MeshGameObject*>(object);

                instance.tint =
                {
                    meshObject->m_ObjectColor.R / 255.0F,
                    meshObject->m_ObjectColor.G / 255.0F,
                    meshObject->m_ObjectColor.B / 255.0F,
                    meshObject->m_ObjectColor.A / 255.0F
                };
            }

            gBridge.instances.push_back(instance);
        }

        return true;
    }

    engine::renderer::RenderView BuildRenderView(
        const D3D11_VIEWPORT& viewport) noexcept
    {
        engine::renderer::RenderView view;

        view.view = ConvertMatrix(
            r3dRenderer->ViewMatrix);

        view.projection = ConvertMatrix(
            r3dRenderer->ProjMatrix);

        view.viewProjection = ConvertMatrix(
            r3dRenderer->ViewProjMatrix);

        view.cameraPosition =
        {
            r3dRenderer->CameraPosition.x,
            r3dRenderer->CameraPosition.y,
            r3dRenderer->CameraPosition.z
        };

        view.viewport.x = viewport.TopLeftX;
        view.viewport.y = viewport.TopLeftY;
        view.viewport.width = viewport.Width;
        view.viewport.height = viewport.Height;
        view.viewport.minDepth = viewport.MinDepth;
        view.viewport.maxDepth = viewport.MaxDepth;

        view.ambientColor =
        {
            r3dRenderer->AmbientColor.R / 255.0F,
            r3dRenderer->AmbientColor.G / 255.0F,
            r3dRenderer->AmbientColor.B / 255.0F
        };

        view.lightDirection =
        {
            -0.45F,
            0.75F,
            -0.55F
        };

        view.lightColor =
        {
            1.0F,
            0.97F,
            0.92F
        };

        view.lightIntensity = 1.15F;
        view.elapsedTime = r3dGetTime();
        view.debugMode =
            engine::renderer::MaterialDebugMode::Lit;

        return view;
    }
}

bool LevelEditorStaticBridge_IsRequested()
{
    static int requested = -1;

    if (requested < 0)
    {
        requested =
            HasCommandLineSwitch("-dx11staticbridge") ||
            HasCommandLineSwitch("/dx11staticbridge")
                ? 1
                : 0;
    }

    return requested != 0;
}

bool LevelEditorStaticBridge_IsReady()
{
    return
        gBridge.ready &&
        !gBridge.failed &&
        gBridge.model.IsValid() &&
        gBridge.renderer.IsInitialized();
}

bool LevelEditorStaticBridge_IsActive()
{
    return
        LevelEditorStaticBridge_IsReady() &&
        IsLevelEditorModeActive();
}

bool LevelEditorStaticBridge_Initialize()
{
    using engine::assets::AssetPath;
    using engine::assets::AssetResult;
    using engine::graphics::GraphicsBackend;
    using engine::graphics::GraphicsResult;

    if (!LevelEditorStaticBridge_IsRequested())
    {
        return true;
    }

    if (LevelEditorStaticBridge_IsReady())
    {
        return true;
    }

    if (gBridge.failed)
    {
        return false;
    }

    ReleaseBridgeResources();

    try
    {
        const std::string rootText =
            GetCommandLineValue("-dx11assetroot=");

        const std::string legacyText =
            GetCommandLineValue("-dx11bridgelegacy=");

        const std::string modelText =
            GetCommandLineValue("-dx11bridgemodel=");

        std::string vertexShaderText =
            GetCommandLineValue("-dx11staticvs=");

        std::string pixelShaderText =
            GetCommandLineValue("-dx11staticps=");

        if (vertexShaderText.empty())
        {
            vertexShaderText =
                "cookedshaders/"
                "staticmodelcompatibility.vs.ltsshader";
        }

        if (pixelShaderText.empty())
        {
            pixelShaderText =
                "cookedshaders/"
                "staticmodelcompatibility.ps.ltsshader";
        }

        if (
            rootText.empty() ||
            legacyText.empty() ||
            modelText.empty())
        {
            BridgeLog(
                "[DX11][StaticBridge] Required arguments:\n"
                "  -dx11assetroot=<Data root>\n"
                "  -dx11bridgelegacy=<legacy GameObject FileName>\n"
                "  -dx11bridgemodel=<cooked .ltsmodel>\n");

            gBridge.failed = true;
            return false;
        }

        char normalizedLegacyPath[
            MaximumLegacyPathLength] = {};

        if (!NormalizeLegacyPath(
                legacyText.c_str(),
                normalizedLegacyPath,
                sizeof(normalizedLegacyPath)))
        {
            BridgeLog(
                "[DX11][StaticBridge] Invalid legacy path: %s\n",
                legacyText.c_str());

            gBridge.failed = true;
            return false;
        }

        gBridge.legacyPath.assign(
            normalizedLegacyPath);

        AssetPath vertexShaderPath;
        AssetPath pixelShaderPath;

        AssetResult assetResult =
            AssetPath::TryCreate(
                modelText,
                gBridge.cookedModelPath);

        if (engine::assets::Failed(assetResult))
        {
            return FailInitialization(
                "model AssetPath",
                assetResult);
        }

        assetResult = AssetPath::TryCreate(
            vertexShaderText,
            vertexShaderPath);

        if (engine::assets::Failed(assetResult))
        {
            return FailInitialization(
                "vertex shader AssetPath",
                assetResult);
        }

        assetResult = AssetPath::TryCreate(
            pixelShaderText,
            pixelShaderPath);

        if (engine::assets::Failed(assetResult))
        {
            return FailInitialization(
                "pixel shader AssetPath",
                assetResult);
        }

        std::error_code pathError;

        const std::filesystem::path root =
            std::filesystem::absolute(
                std::filesystem::u8path(rootText),
                pathError).lexically_normal();

        if (pathError)
        {
            BridgeLog(
                "[DX11][StaticBridge] Invalid asset root: %s\n",
                rootText.c_str());

            gBridge.failed = true;
            return false;
        }

        assetResult =
            gBridge.assetSource.Initialize(root);

        if (engine::assets::Failed(assetResult))
        {
            return FailInitialization(
                "FileAssetSource",
                assetResult);
        }

        assetResult =
            gBridge.assetManager.Initialize(
                gBridge.assetSource);

        if (engine::assets::Failed(assetResult))
        {
            return FailInitialization(
                "AssetManager",
                assetResult);
        }

        const auto registerLoader =
            [](engine::assets::AssetLoader& loader)
            {
                return gBridge.assetRegistry.Register(loader);
            };

        assetResult = registerLoader(
            gBridge.staticModelLoader);

        if (engine::assets::Succeeded(assetResult))
        {
            assetResult = registerLoader(
                gBridge.meshLoader);
        }

        if (engine::assets::Succeeded(assetResult))
        {
            assetResult = registerLoader(
                gBridge.materialLoader);
        }

        if (engine::assets::Succeeded(assetResult))
        {
            assetResult = registerLoader(
                gBridge.textureLoader);
        }

        if (engine::assets::Succeeded(assetResult))
        {
            assetResult = registerLoader(
                gBridge.shaderLoader);
        }

        if (engine::assets::Failed(assetResult))
        {
            return FailInitialization(
                "AssetLoaderRegistry",
                assetResult);
        }

        engine::assets::ShaderAsset vertexShader;
        engine::assets::ShaderAsset pixelShader;

        assetResult = LoadShader(
            vertexShaderPath,
            vertexShader);

        if (engine::assets::Failed(assetResult))
        {
            return FailInitialization(
                "vertex ShaderAsset",
                assetResult);
        }

        assetResult = LoadShader(
            pixelShaderPath,
            pixelShader);

        if (engine::assets::Failed(assetResult))
        {
            return FailInitialization(
                "pixel ShaderAsset",
                assetResult);
        }

        RenderDX11Core& nativeCore =
            RenderDX11_GetCore();

        if (
            !nativeCore.IsReady() ||
            !nativeCore.GetDevice() ||
            !nativeCore.GetContext())
        {
            BridgeLog(
                "[DX11][StaticBridge] "
                "RenderDX11Core is not ready\n");

            gBridge.failed = true;
            return false;
        }

        engine::graphics::RenderDeviceDesc deviceDescription;
        deviceDescription.backend = GraphicsBackend::D3D11;
        deviceDescription.enableValidation = false;

        GraphicsResult graphicsResult =
            gBridge.device.AttachExternal(
                nativeCore.GetDevice(),
                nativeCore.GetContext(),
                deviceDescription);

        if (engine::graphics::Failed(graphicsResult))
        {
            return FailInitialization(
                "D3D11 external attach",
                graphicsResult);
        }

        gBridge.context =
            gBridge.device.GetImmediateCommandContext();

        if (
            !gBridge.context ||
            !gBridge.context->IsValid())
        {
            return FailInitialization(
                "external CommandContext",
                GraphicsResult::InvalidState);
        }

        graphicsResult = gBridge.renderer.Initialize(
            gBridge.device,
            *gBridge.context,
            vertexShader,
            pixelShader);

        if (engine::graphics::Failed(graphicsResult))
        {
            return FailInitialization(
                "StaticModelRenderer",
                graphicsResult);
        }

        assetResult = gBridge.renderer.CreateModel(
            gBridge.assetManager,
            gBridge.assetRegistry,
            gBridge.cookedModelPath,
            gBridge.model);

        if (engine::assets::Failed(assetResult))
        {
            return FailInitialization(
                "StaticModelRenderer::CreateModel",
                assetResult);
        }

        gBridge.ready = true;

        BridgeLog(
            "[DX11][StaticBridge] Ready\n"
            "  Legacy: %s\n"
            "  Cooked: %s\n"
            "  VS: %s\n"
            "  PS: %s\n",
            gBridge.legacyPath.c_str(),
            gBridge.cookedModelPath.String().c_str(),
            vertexShaderPath.String().c_str(),
            pixelShaderPath.String().c_str());

        return true;
    }
    catch (const std::bad_alloc&)
    {
        ReleaseBridgeResources();
        gBridge.failed = true;

        BridgeLog(
            "[DX11][StaticBridge] "
            "Initialization failed: out of memory\n");

        return false;
    }
    catch (...)
    {
        ReleaseBridgeResources();
        gBridge.failed = true;

        BridgeLog(
            "[DX11][StaticBridge] "
            "Initialization failed: unexpected exception\n");

        return false;
    }
}

void LevelEditorStaticBridge_Shutdown()
{
    ReleaseBridgeResources();

    gBridge.failed = false;
    gBridge.legacyPath.clear();
    gBridge.cookedModelPath = {};
}

bool LevelEditorStaticBridge_ShouldReplaceObject(
    const GameObject* object)
{
    if (
        !LevelEditorStaticBridge_IsActive() ||
        !object)
    {
        return false;
    }

    return MatchesLegacyPath(
        object->FileName.c_str(),
        gBridge.legacyPath);
}

bool LevelEditorStaticBridge_Render(
    const WorldDX11FrameDesc& frame,
    ID3D11RenderTargetView* colorTarget,
    ID3D11DepthStencilView* depthTarget,
    const D3D11_VIEWPORT& viewport)
{
    using engine::graphics::GraphicsResult;

    if (!LevelEditorStaticBridge_IsActive())
    {
        return true;
    }

    if (
        !colorTarget ||
        !depthTarget ||
        !gBridge.context ||
        frame.Width <= 0 ||
        frame.Height <= 0)
    {
        return false;
    }

    try
    {
        if (!BuildInstances())
        {
            return false;
        }
    }
    catch (const std::bad_alloc&)
    {
        BridgeLog(
            "[DX11][StaticBridge] "
            "Instance collection ran out of memory\n");

        return false;
    }
    catch (...)
    {
        BridgeLog(
            "[DX11][StaticBridge] "
            "Instance collection failed\n");

        return false;
    }

    if (gBridge.instances.empty())
    {
        if (!gBridge.noMatchingObjectsLogged)
        {
            gBridge.noMatchingObjectsLogged = true;

            BridgeLog(
                "[DX11][StaticBridge] "
                "No visible objects match legacy path: %s\n",
                gBridge.legacyPath.c_str());
        }

        return true;
    }

    gBridge.noMatchingObjectsLogged = false;

    ID3D11DeviceContext* nativeContext =
        RenderDX11_GetCore().GetContext();

    if (!nativeContext)
    {
        return false;
    }

    nativeContext->OMSetRenderTargets(
        1,
        &colorTarget,
        depthTarget);

    engine::graphics::Viewport neutralViewport;
    neutralViewport.x = viewport.TopLeftX;
    neutralViewport.y = viewport.TopLeftY;
    neutralViewport.width = viewport.Width;
    neutralViewport.height = viewport.Height;
    neutralViewport.minDepth = viewport.MinDepth;
    neutralViewport.maxDepth = viewport.MaxDepth;

    GraphicsResult result =
        gBridge.context->SetViewport(
            neutralViewport);

    if (engine::graphics::Failed(result))
    {
        BridgeLog(
            "[DX11][StaticBridge] SetViewport failed: %s\n",
            engine::graphics::ToString(result));

        return false;
    }

    engine::graphics::ScissorRect scissor;
    scissor.left =
        static_cast<std::int32_t>(viewport.TopLeftX);
    scissor.top =
        static_cast<std::int32_t>(viewport.TopLeftY);
    scissor.right =
        scissor.left +
        static_cast<std::int32_t>(viewport.Width);
    scissor.bottom =
        scissor.top +
        static_cast<std::int32_t>(viewport.Height);

    result = gBridge.context->SetScissorRect(scissor);

    if (engine::graphics::Failed(result))
    {
        BridgeLog(
            "[DX11][StaticBridge] SetScissorRect failed: %s\n",
            engine::graphics::ToString(result));

        return false;
    }

    const engine::renderer::RenderView view =
        BuildRenderView(viewport);

    if (!view.IsValid())
    {
        BridgeLog(
            "[DX11][StaticBridge] RenderView is invalid\n");

        return false;
    }

    engine::renderer::StaticModelRenderStats statistics;

    result = gBridge.renderer.Render(
        view,
        gBridge.instances.data(),
        gBridge.instances.size(),
        statistics);

    gBridge.renderer.Unbind();

    if (engine::graphics::Failed(result))
    {
        BridgeLog(
            "[DX11][StaticBridge] Render failed: %s\n",
            engine::graphics::ToString(result));

        return false;
    }

    if (!gBridge.firstFrameLogged)
    {
        gBridge.firstFrameLogged = true;

        const auto diagnostics =
            gBridge.renderer.GetDiagnostics();

        BridgeLog(
            "[DX11][StaticBridge] First Level Editor frame\n"
            "  Instances: %zu\n"
            "  Draws: %zu\n"
            "  Triangles: %zu\n"
            "  Model resources: %zu\n"
            "  Texture entries: %zu\n"
            "  Texture references: %zu\n",
            statistics.acceptedInstances,
            statistics.drawCalls,
            statistics.triangles,
            diagnostics.liveModelResources,
            diagnostics.liveTextureEntries,
            diagnostics.liveTextureReferences);
    }

    return true;
}

#endif