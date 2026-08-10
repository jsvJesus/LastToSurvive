#include "Editor/LevelEditor/Rendering/StaticMeshRenderer.h"
#include "Editor/LevelEditor/Rendering/ShaderCompiler.h"

#include <Assets/AssetData.h>
#include <Assets/AssetMetadata.h>
#include <Assets/AssetPath.h>
#include <Assets/AssetResult.h>
#include <Assets/AssetType.h>
#include <Assets/GpuMesh.h>
#include <Assets/MaterialAssetLoader.h>
#include <Assets/MeshAsset.h>
#include <Assets/MeshAssetLoader.h>
#include <Assets/DdsTextureDecoder.h>
#include <Assets/TextureAsset.h>

#include <Core/Log.h>

#include <Graphics/Buffer.h>
#include <Graphics/CommandContext.h>
#include <Graphics/Format.h>
#include <Graphics/InputLayout.h>
#include <Graphics/PipelineState.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/ResourceHandle.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture.h>

#include <DirectXMath.h>
#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <array>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lts::editor
{
    namespace
    {
        constexpr std::uintmax_t MaximumMeshFileSize =
            512U * 1024U * 1024U;
        constexpr std::size_t MaximumInstancesPerDraw = 2048U;
        constexpr std::size_t MaximumMeshLoadsPerFrame = 1U;
        constexpr float StaticMeshRenderDistance = 1600.0F;

        struct alignas(16) ObjectConstants final
        {
            DirectX::XMFLOAT4X4 world;
            DirectX::XMFLOAT4X4 viewProjection;

            DirectX::XMFLOAT4 baseColor;
            DirectX::XMFLOAT4 materialParameters;

            // xyz = направление от поверхности к солнцу.
            // w = нормализованная интенсивность.
            DirectX::XMFLOAT4 sunDirectionIntensity;

            DirectX::XMFLOAT4 sunColor;
            DirectX::XMFLOAT4 ambientColor;
            DirectX::XMFLOAT4 cameraPositionFogDensity;
            DirectX::XMFLOAT4 fogColorEnabled;
            DirectX::XMFLOAT4 fogDistancesHeight;
            DirectX::XMFLOAT4 shadowParameters;
            DirectX::XMFLOAT4 legacySurfaceParameters;
            DirectX::XMFLOAT4 legacyDetailParameters;
            DirectX::XMFLOAT4 legacyTextureFlags;
            DirectX::XMFLOAT4 legacyFeatureFlags;
        };

        static_assert(
            sizeof(ObjectConstants) % 16U == 0U);

        struct alignas(16) InstanceData final
        {
            DirectX::XMFLOAT4X4 world{};
            DirectX::XMFLOAT4 parameters{};
        };

        static_assert(sizeof(InstanceData) == 80U);

        struct LegacyMaterialDefinition final
        {
            engine::assets::MaterialAssetDesc desc;
            float detailScale = 1.0F;
            float detailAmount = 0.0F;
            float displacementValue = 0.0F;
            bool displacementEnabled = false;
            bool camouflage = false;
            std::string type;
            std::string diffuseTexture;
            std::string normalTexture;
            std::string specularTexture;
            std::string specularPowerTexture;
            std::string detailNormalTexture;
            std::string emissiveTexture;
        };

        [[nodiscard]] std::string TrimAscii(std::string value)
        {
            const auto isSpace = [](const unsigned char character)
            {
                return std::isspace(character) != 0;
            };
            value.erase(
                value.begin(),
                std::find_if(value.begin(), value.end(),
                    [&](const char character)
                    {
                        return !isSpace(static_cast<unsigned char>(character));
                    }));
            value.erase(
                std::find_if(value.rbegin(), value.rend(),
                    [&](const char character)
                    {
                        return !isSpace(static_cast<unsigned char>(character));
                    }).base(),
                value.end());
            return value;
        }

        [[nodiscard]] std::string LowercaseAscii(std::string value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](const unsigned char character)
                {
                    return static_cast<char>(std::tolower(character));
                });
            return value;
        }

        [[nodiscard]] float ParseLegacyFloat(
            const std::string& value,
            const float fallback) noexcept
        {
            char* end = nullptr;
            const float parsed = std::strtof(value.c_str(), &end);
            return end != value.c_str() && std::isfinite(parsed)
                ? parsed
                : fallback;
        }

        [[nodiscard]] bool ParseLegacyBool(
            const std::string& value) noexcept
        {
            const std::string lowered = LowercaseAscii(TrimAscii(value));
            return lowered == "1" || lowered == "true" || lowered == "yes";
        }

        [[nodiscard]] bool ParseLegacyMaterial(
            const std::filesystem::path& path,
            LegacyMaterialDefinition& output) noexcept
        {
            try
            {
                std::ifstream input(path);
                if (!input)
                {
                    return false;
                }
                input.imbue(std::locale::classic());

                LegacyMaterialDefinition material;
                bool forceTransparent = false;
                bool alphaTransparent = false;
                bool glows = false;
                float lowQualitySelfIllumination = 0.0F;
                float selfIlluminationMultiplier = 0.0F;
                float alphaReference = 0.0F;
                std::string line;

                while (std::getline(input, line))
                {
                    const std::size_t separator = line.find('=');
                    if (separator == std::string::npos)
                    {
                        continue;
                    }
                    const std::string key = LowercaseAscii(
                        TrimAscii(line.substr(0U, separator)));
                    const std::string value = TrimAscii(
                        line.substr(separator + 1U));

                    if (key == "name") material.desc.debugName = value;
                    else if (key == "texture") material.diffuseTexture = value;
                    else if (key == "normalmap") material.normalTexture = value;
                    else if (key == "specularmap") material.specularTexture = value;
                    else if (key == "specpowmap") material.specularPowerTexture = value;
                    else if (key == "detailnmap") material.detailNormalTexture = value;
                    else if (key == "glowmap") material.emissiveTexture = value;
                    else if (key == "specularpower")
                        material.desc.specularIntensity = (std::max)(
                            ParseLegacyFloat(value, 0.0F), 0.0F);
                    else if (key == "specular1power")
                    {
                        const float gloss = (std::clamp)(
                            ParseLegacyFloat(value, 0.0F), 0.0F, 1.0F);
                        material.desc.specularPower = 4.0F + gloss * 124.0F;
                    }
                    else if (key == "reflectionpower")
                        material.desc.reflectionFactor = (std::max)(
                            ParseLegacyFloat(value, 0.0F), 0.0F);
                    else if (key == "detailscale")
                        material.detailScale = (std::max)(
                            ParseLegacyFloat(value, 1.0F), 0.001F);
                    else if (key == "detailammount" || key == "diffdetailammount")
                        material.detailAmount = (std::max)(
                            ParseLegacyFloat(value, 0.0F), 0.0F);
                    else if (key == "normalscale")
                        material.desc.normalScale = ParseLegacyFloat(value, 1.0F);
                    else if (key == "displace")
                        material.displacementEnabled = ParseLegacyBool(value);
                    else if (key == "displ_val")
                        material.displacementValue = ParseLegacyFloat(value, 0.0F);
                    else if (key == "lowqmetallness")
                        material.desc.metallicFactor = (std::clamp)(
                            ParseLegacyFloat(value, 0.0F), 0.0F, 1.0F);
                    else if (key == "lowqselfillum")
                        lowQualitySelfIllumination = (std::max)(
                            ParseLegacyFloat(value, 0.0F), 0.0F);
                    else if (key == "selfillummultiplier")
                        selfIlluminationMultiplier = (std::max)(
                            ParseLegacyFloat(value, 0.0F), 0.0F);
                    else if (key == "doublesided")
                        material.desc.doubleSided = ParseLegacyBool(value);
                    else if (key == "forcetransparent")
                        forceTransparent = ParseLegacyBool(value);
                    else if (key == "alphatransparent")
                        alphaTransparent = ParseLegacyBool(value);
                    else if (key == "camouflage")
                        material.camouflage = ParseLegacyBool(value);
                    else if (key == "glows")
                        glows = ParseLegacyBool(value);
                    else if (key == "alpharef")
                        alphaReference = ParseLegacyFloat(value, 0.0F);
                    else if (key == "type") material.type = value;
                    else if (key == "color24")
                    {
                        std::istringstream colors(value);
                        colors.imbue(std::locale::classic());
                        int red = 255;
                        int green = 255;
                        int blue = 255;
                        if (colors >> red >> green >> blue)
                        {
                            material.desc.baseColorFactor = {
                                (std::clamp)(red, 0, 255) / 255.0F,
                                (std::clamp)(green, 0, 255) / 255.0F,
                                (std::clamp)(blue, 0, 255) / 255.0F,
                                1.0F};
                        }
                    }
                }

                material.desc.emissiveStrength = (std::max)(
                    lowQualitySelfIllumination,
                    selfIlluminationMultiplier);
                if (glows && material.desc.emissiveStrength <= 0.0F)
                {
                    material.desc.emissiveStrength = 1.0F;
                }
                if (forceTransparent || alphaTransparent)
                {
                    material.desc.alphaMode =
                        engine::assets::MaterialAlphaMode::Blend;
                }
                else if (alphaReference > 0.0F)
                {
                    material.desc.alphaMode =
                        engine::assets::MaterialAlphaMode::Mask;
                    material.desc.alphaCutoff = (std::clamp)(
                        alphaReference > 1.0F
                            ? alphaReference / 255.0F
                            : alphaReference,
                        0.0F,
                        1.0F);
                }

                output = std::move(material);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]] bool DecodeWicRgba(
            const std::filesystem::path& path,
            std::vector<std::byte>& pixels,
            std::uint32_t& width,
            std::uint32_t& height) noexcept
        {
            pixels.clear(); width = 0U; height = 0U;
            try
            {
                Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
                HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                    CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.GetAddressOf()));
                if (FAILED(hr)) return false;
                Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
                hr = factory->CreateDecoderFromFilename(path.c_str(), nullptr,
                    GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
                if (FAILED(hr)) return false;
                Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
                hr = decoder->GetFrame(0U, frame.GetAddressOf());
                if (FAILED(hr) || FAILED(frame->GetSize(&width, &height)) ||
                    width == 0U || height == 0U || width > 16384U || height > 16384U)
                    return false;
                Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
                hr = factory->CreateFormatConverter(converter.GetAddressOf());
                if (FAILED(hr)) return false;
                hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
                    WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
                if (FAILED(hr)) return false;
                const std::size_t rowPitch = static_cast<std::size_t>(width) * 4U;
                const std::size_t byteCount = rowPitch * static_cast<std::size_t>(height);
                if (byteCount > static_cast<std::size_t>((std::numeric_limits<UINT>::max)()))
                    return false;
                pixels.resize(byteCount);
                hr = converter->CopyPixels(nullptr, static_cast<UINT>(rowPitch),
                    static_cast<UINT>(byteCount), reinterpret_cast<BYTE*>(pixels.data()));
                return SUCCEEDED(hr);
            }
            catch (...) { pixels.clear(); width = 0U; height = 0U; return false; }
        }

        [[nodiscard]]
        DirectX::XMMATRIX BuildWorldMatrix(
            const EditorTransform& transform) noexcept
        {
            const DirectX::XMMATRIX scale =
                DirectX::XMMatrixScaling(
                    transform.scale[0],
                    transform.scale[1],
                    transform.scale[2]);

            const DirectX::XMMATRIX rotation =
                DirectX::XMMatrixRotationRollPitchYaw(
                    DirectX::XMConvertToRadians(
                        transform.
                            rotationDegrees[1]),
                    DirectX::XMConvertToRadians(
                        transform.
                            rotationDegrees[0]),
                    DirectX::XMConvertToRadians(
                        transform.
                            rotationDegrees[2]));

            const DirectX::XMMATRIX translation =
                DirectX::XMMatrixTranslation(
                    transform.position[0],
                    transform.position[1],
                    transform.position[2]);

            return
                scale *
                rotation *
                translation;
        }

        [[nodiscard]]
        bool IsTransformedMeshBoundsVisible(
            const engine::assets::MeshBounds& bounds,
            const DirectX::XMMATRIX& worldTransform,
            const DirectX::XMMATRIX& viewProjection) noexcept
        {
            const DirectX::XMMATRIX worldViewProjection =
                worldTransform * viewProjection;

            bool outsideLeft = true;
            bool outsideRight = true;
            bool outsideBottom = true;
            bool outsideTop = true;
            bool outsideNear = true;
            bool outsideFar = true;

            for (std::size_t cornerIndex = 0U;
                 cornerIndex < 8U;
                 ++cornerIndex)
            {
                const float x =
                    (cornerIndex & 1U) != 0U
                        ? bounds.maximum[0]
                        : bounds.minimum[0];
                const float y =
                    (cornerIndex & 2U) != 0U
                        ? bounds.maximum[1]
                        : bounds.minimum[1];
                const float z =
                    (cornerIndex & 4U) != 0U
                        ? bounds.maximum[2]
                        : bounds.minimum[2];

                const DirectX::XMVECTOR clipPosition =
                    DirectX::XMVector4Transform(
                        DirectX::XMVectorSet(x, y, z, 1.0F),
                        worldViewProjection);

                DirectX::XMFLOAT4 clip{};
                DirectX::XMStoreFloat4(&clip, clipPosition);

                outsideLeft = outsideLeft && clip.x < -clip.w;
                outsideRight = outsideRight && clip.x > clip.w;
                outsideBottom = outsideBottom && clip.y < -clip.w;
                outsideTop = outsideTop && clip.y > clip.w;
                outsideNear = outsideNear && clip.z < 0.0F;
                outsideFar = outsideFar && clip.z > clip.w;
            }

            return
                !outsideLeft &&
                !outsideRight &&
                !outsideBottom &&
                !outsideTop &&
                !outsideNear &&
                !outsideFar;
        }

        struct ResolvedDirectionalLight final
        {
            DirectX::XMFLOAT3 direction
            {
                -0.35F,
                0.85F,
                -0.40F
            };

            DirectX::XMFLOAT3 color
            {
                1.0F,
                1.0F,
                1.0F
            };

            float intensity = 1.0F;

            DirectX::XMFLOAT3 ambientColor
            {
                0.28F,
                0.31F,
                0.36F
            };

            float ambientIntensity = 1.0F;

            DirectX::XMFLOAT3 fogColor
            {
                0.45F,
                0.62F,
                0.78F
            };

            float fogStart = 450.0F;
            float fogEnd = 5000.0F;
            float fogDensity = 0.00018F;
            float fogHeightFalloff = 0.0015F;
            bool fogEnabled = false;
            bool sunEnabled = true;
            bool shadowsEnabled = true;
            float shadowStrength = 0.82F;
            float shadowSoftness = 1.25F;
            float shadowDistance = 1800.0F;
        };

        [[nodiscard]]
        ResolvedDirectionalLight ResolveDirectionalLight(
            const SceneDocument& document) noexcept
        {
            ResolvedDirectionalLight result;

            for (const EditorSceneEntity& entity : document.GetEntities())
            {
                if (!entity.environment.has_value() ||
                    !entity.environment->visible)
                {
                    continue;
                }

                const auto& environment =
                    *entity.environment;

                result.ambientColor =
                {
                    (std::max)(
                        environment.ambientColor[0],
                        0.0F),

                    (std::max)(
                        environment.ambientColor[1],
                        0.0F),

                    (std::max)(
                        environment.ambientColor[2],
                        0.0F)
                };

                result.ambientIntensity =
                    (std::max)(
                        environment.ambientIntensity,
                        0.0F);

                result.fogColor =
                {
                    (std::max)(environment.fogColor[0], 0.0F),
                    (std::max)(environment.fogColor[1], 0.0F),
                    (std::max)(environment.fogColor[2], 0.0F)
                };
                result.fogStart = (std::max)(environment.fogStart, 0.0F);
                result.fogEnd = (std::max)(environment.fogEnd, result.fogStart + 1.0F);
                result.fogDensity = (std::max)(environment.fogDensity, 0.0F);
                result.fogHeightFalloff = (std::max)(environment.fogHeightFalloff, 0.0F);
                result.fogEnabled = environment.fogEnabled;
                result.sunEnabled = environment.sunEnabled;
                result.shadowsEnabled = environment.shadowsEnabled;
                result.shadowStrength = std::clamp(environment.shadowStrength, 0.0F, 1.0F);
                result.shadowSoftness = std::clamp(environment.shadowSoftness, 0.05F, 4.0F);
                result.shadowDistance = (std::max)(environment.shadowDistance, 1.0F);

                break;
            }

            for (const EditorSceneEntity& entity : document.GetEntities())
            {
                if (!entity.directionalLight.has_value())
                {
                    continue;
                }

                const auto& light = *entity.directionalLight;

                const float pitch = DirectX::XMConvertToRadians(
                    entity.transform.rotationDegrees[0]);

                const float yaw = DirectX::XMConvertToRadians(
                    entity.transform.rotationDegrees[1]);

                const float cosinePitch = std::cos(pitch);

                DirectX::XMFLOAT3 direction
                {
                    -cosinePitch * std::sin(yaw),
                    -std::sin(pitch),
                    -cosinePitch * std::cos(yaw)
                };

                DirectX::XMStoreFloat3(
                    &result.direction,
                    DirectX::XMVector3Normalize(
                        DirectX::XMLoadFloat3(&direction)));

                result.color =
                {
                    (std::max)(light.color[0], 0.0F),
                    (std::max)(light.color[1], 0.0F),
                    (std::max)(light.color[2], 0.0F)
                };

                /*
                 * Старое значение по умолчанию равно 4.
                 * Для shader нормализуем его к 1.
                 */
                result.intensity =
                    (std::max)(light.intensity, 0.0F) *
                    0.25F;

                if (!light.castShadows)
                {
                    result.shadowsEnabled = false;
                }

                break;
            }

            return result;
        }

        [[nodiscard]]
        std::wstring LowercasePath(
            std::wstring value)
        {
            for (wchar_t& character : value)
            {
                character =
                    static_cast<wchar_t>(
                        std::towlower(
                            character));
            }

            return value;
        }

        void LogGraphicsFailure(
            const char* const operation,
            const engine::graphics::
                GraphicsResult result) noexcept
        {
            std::string message =
                operation != nullptr
                    ? operation
                    : "Static mesh graphics operation";

            message += " failed: ";

            message +=
                engine::graphics::
                    ToString(result);

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                "LTS.Editor.StaticMesh",
                message);
        }

        void LogAssetFailure(
            const std::filesystem::path& path,
            const char* const operation,
            const engine::assets::
                AssetResult result)
        {
            std::string message =
                operation != nullptr
                    ? operation
                    : "Static mesh asset operation";

            message += " failed for '";
            message += path.generic_u8string();
            message += "': ";

            message +=
                engine::assets::
                    ToString(result);

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                "LTS.Editor.StaticMesh",
                message);
        }

        [[nodiscard]]
        engine::assets::AssetResult ReadAssetData(
            const std::filesystem::path& path,
            engine::assets::AssetData& output) noexcept
        {
            output.Clear();

            try
            {
                std::error_code filesystemError;

                const std::uintmax_t fileSize =
                    std::filesystem::file_size(
                        path,
                        filesystemError);

                if (filesystemError)
                {
                    return engine::assets::
                        AssetResult::IoError;
                }

                if (
                    fileSize == 0U ||
                    fileSize >
                        MaximumMeshFileSize ||
                    fileSize >
                        static_cast<std::uintmax_t>(
                            std::numeric_limits<
                                std::streamsize>::
                                    max()))
                {
                    return engine::assets::
                        AssetResult::FileTooLarge;
                }

                const engine::assets::AssetResult
                    resizeResult =
                        output.Resize(
                            static_cast<std::size_t>(
                                fileSize));

                if (
                    engine::assets::Failed(
                        resizeResult))
                {
                    return resizeResult;
                }

                std::ifstream input(
                    path,
                    std::ios::binary);

                if (!input)
                {
                    output.Clear();

                    return engine::assets::
                        AssetResult::IoError;
                }

                input.read(
                    reinterpret_cast<char*>(
                        output.GetData()),
                    static_cast<std::streamsize>(
                        fileSize));

                if (!input)
                {
                    output.Clear();

                    return engine::assets::
                        AssetResult::IoError;
                }

                return engine::assets::
                    AssetResult::Success;
            }
            catch (const std::bad_alloc&)
            {
                output.Clear();

                return engine::assets::
                    AssetResult::OutOfMemory;
            }
            catch (...)
            {
                output.Clear();

                return engine::assets::
                    AssetResult::InternalError;
            }
        }

        [[nodiscard]]
        bool CreateTextureFromFile(
            engine::graphics::RenderDevice& device,
            const std::filesystem::path& file,
            const bool forceSrgb,
            engine::graphics::TextureHandle& output) noexcept
        {
            output = {};

            try
            {
                const std::wstring extension =
                    LowercasePath(
                        file.extension().wstring());

                if (extension == L".dds")
                {
                    engine::assets::AssetData source;

                    if (engine::assets::Failed(
                            ReadAssetData(
                                file,
                                source)))
                    {
                        return false;
                    }

                    engine::assets::TextureAsset texture;

                    engine::assets::DdsTextureDecodeOptions
                        options;

                    options.forceSrgb = forceSrgb;
                    options.allowBc7 = true;

                    const auto decodeResult =
                        engine::assets::DdsTextureDecoder::Decode(
                            source,
                            options,
                            texture);

                    if (engine::assets::Failed(
                            decodeResult) ||
                        !texture.IsValid())
                    {
                        return false;
                    }

                    std::vector<
                        engine::graphics::
                            TextureSubresourceData>
                        subresources;

                    subresources.resize(
                        texture.GetSubresourceCount());

                    for (std::size_t index = 0U;
                         index < subresources.size();
                         ++index)
                    {
                        if (engine::assets::Failed(
                                texture.GetSubresourceData(
                                    index,
                                    subresources[index])))
                        {
                            return false;
                        }
                    }

                    return engine::graphics::Succeeded(
                        device.CreateTexture(
                            texture.GetDesc(),
                            subresources.data(),
                            subresources.size(),
                            output));
                }

                std::vector<std::byte> pixels;

                std::uint32_t width = 0U;
                std::uint32_t height = 0U;

                if (!DecodeWicRgba(
                        file,
                        pixels,
                        width,
                        height))
                {
                    return false;
                }

                engine::graphics::TextureDesc description;

                description.width = width;
                description.height = height;

                description.format =
                    forceSrgb
                        ? engine::graphics::Format::R8G8B8A8UNormSrgb
                        : engine::graphics::Format::R8G8B8A8UNorm;

                engine::graphics::TextureSubresourceData
                    initialData;

                initialData.data = pixels.data();
                initialData.dataSize = pixels.size();

                initialData.rowPitch =
                    static_cast<std::size_t>(
                        width) *
                    4U;

                initialData.slicePitch =
                    pixels.size();

                return engine::graphics::Succeeded(
                    device.CreateTexture(
                        description,
                        &initialData,
                        1U,
                        output));
            }
            catch (...)
            {
                output = {};

                return false;
            }
        }

        [[nodiscard]]
        engine::assets::AssetResult
            CreateMeshMetadata(
                const std::filesystem::path& requestedPath,
                const std::size_t sourceSize,
                engine::assets::AssetMetadata& metadata) noexcept
        {
            try
            {
                std::filesystem::path logicalPath =
                    requestedPath;

                if (logicalPath.is_absolute())
                {
                    logicalPath = logicalPath.relative_path();

                    if (logicalPath.empty())
                    {
                        logicalPath = requestedPath.filename();
                    }
                }

                const std::string logicalName =
                    logicalPath.
                        lexically_normal().
                        generic_u8string();

                engine::assets::AssetPath assetPath;

                const engine::assets::AssetResult
                    pathResult =
                        engine::assets::
                            AssetPath::TryCreate(
                                logicalName,
                                assetPath);

                if (
                    engine::assets::Failed(
                        pathResult))
                {
                    return pathResult;
                }

                metadata = {};

                metadata.path =
                    std::move(assetPath);

                metadata.id =
                    metadata.path.GetId();

                metadata.type =
                    engine::assets::
                        AssetType::Mesh;

                metadata.schemaVersion = 1U;

                metadata.sourceSize =
                    static_cast<std::uint64_t>(
                        sourceSize);

                return engine::assets::
                    AssetResult::Success;
            }
            catch (const std::bad_alloc&)
            {
                return engine::assets::
                    AssetResult::OutOfMemory;
            }
            catch (...)
            {
                return engine::assets::
                    AssetResult::InternalError;
            }
        }
    }

    class StaticMeshRenderer::Impl final
    {
        struct CachedMaterial final
        {
            engine::assets::MaterialAssetDesc desc;
            engine::graphics::TextureHandle baseColorTexture;
            engine::graphics::TextureHandle normalTexture;
            engine::graphics::TextureHandle specularTexture;
            engine::graphics::TextureHandle specularPowerTexture;
            engine::graphics::TextureHandle detailNormalTexture;
            engine::graphics::TextureHandle emissiveTexture;
            engine::graphics::SamplerHandle sampler;
            float detailScale = 1.0F;
            float detailAmount = 0.0F;
            float displacementValue = 0.0F;
            bool displacementEnabled = false;
            bool camouflage = false;
            std::string type;
        };

        struct CachedMesh final
        {
            std::unique_ptr<engine::assets::GpuMesh> gpu;
            std::vector<CachedMaterial> materials;
        };

    public:
        [[nodiscard]]
        bool Initialize(
            engine::graphics::
                RenderDevice& device) noexcept
        {
            if (initialized_)
            {
                return true;
            }

            device_ = &device;

            Microsoft::WRL::ComPtr<ID3DBlob>
                vertexBytecode;

            Microsoft::WRL::ComPtr<ID3DBlob>
                pixelBytecode;

            if (!CompileEditorShaderFile(
                    L"StaticMesh.hlsl",
                    "VSMain",
                    "vs_5_0",
                    "LTS.Editor.StaticMesh",
                    vertexBytecode))
            {
                device_ = nullptr;
                return false;
            }

            if (!CompileEditorShaderFile(
                    L"StaticMesh.hlsl",
                    "PSMain",
                    "ps_5_0",
                    "LTS.Editor.StaticMesh",
                    pixelBytecode))
            {
                device_ = nullptr;
                return false;
            }

            engine::graphics::ShaderDesc
                vertexShaderDescription;

            vertexShaderDescription.stage =
                engine::graphics::
                    ShaderStage::Vertex;

            vertexShaderDescription.bytecode.data =
                vertexBytecode->
                    GetBufferPointer();

            vertexShaderDescription.bytecode.size =
                vertexBytecode->
                    GetBufferSize();

            vertexShaderDescription.debugName =
                "EditorStaticMesh.VertexShader";

            auto result =
                device.CreateShader(
                    vertexShaderDescription,
                    vertexShader_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create static mesh vertex shader",
                    result);

                Shutdown(device);
                return false;
            }

            engine::graphics::ShaderDesc
                pixelShaderDescription;

            pixelShaderDescription.stage =
                engine::graphics::
                    ShaderStage::Pixel;

            pixelShaderDescription.bytecode.data =
                pixelBytecode->
                    GetBufferPointer();

            pixelShaderDescription.bytecode.size =
                pixelBytecode->
                    GetBufferSize();

            pixelShaderDescription.debugName =
                "EditorStaticMesh.PixelShader";

            result =
                device.CreateShader(
                    pixelShaderDescription,
                    pixelShader_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create static mesh pixel shader",
                    result);

                Shutdown(device);
                return false;
            }

            const std::array<
                engine::graphics::
                    VertexElementDesc,
                9U> elements
            {{
                {
                    "POSITION",
                    0U,
                    engine::graphics::
                        Format::R32G32B32Float,
                    0U,
                    0U,
                    engine::graphics::
                        VertexInputRate::PerVertex,
                    0U
                },
                {
                    "NORMAL",
                    0U,
                    engine::graphics::
                        Format::R32G32B32Float,
                    0U,
                    12U,
                    engine::graphics::
                        VertexInputRate::PerVertex,
                    0U
                },
                {
                    "TANGENT",
                    0U,
                    engine::graphics::
                        Format::R32G32B32A32Float,
                    0U,
                    24U,
                    engine::graphics::
                        VertexInputRate::PerVertex,
                    0U
                },
                {
                    "TEXCOORD",
                    0U,
                    engine::graphics::
                        Format::R32G32Float,
                    0U,
                    40U,
                    engine::graphics::
                        VertexInputRate::PerVertex,
                    0U
                },
                {
                    "INSTANCEWORLD", 0U,
                    engine::graphics::Format::R32G32B32A32Float,
                    1U, 0U,
                    engine::graphics::VertexInputRate::PerInstance,
                    1U
                },
                {
                    "INSTANCEWORLD", 1U,
                    engine::graphics::Format::R32G32B32A32Float,
                    1U, 16U,
                    engine::graphics::VertexInputRate::PerInstance,
                    1U
                },
                {
                    "INSTANCEWORLD", 2U,
                    engine::graphics::Format::R32G32B32A32Float,
                    1U, 32U,
                    engine::graphics::VertexInputRate::PerInstance,
                    1U
                },
                {
                    "INSTANCEWORLD", 3U,
                    engine::graphics::Format::R32G32B32A32Float,
                    1U, 48U,
                    engine::graphics::VertexInputRate::PerInstance,
                    1U
                },
                {
                    "INSTANCEPARAM", 0U,
                    engine::graphics::Format::R32G32B32A32Float,
                    1U, 64U,
                    engine::graphics::VertexInputRate::PerInstance,
                    1U
                }
            }};

            engine::graphics::
                InputLayoutDesc
                    inputLayoutDescription;

            inputLayoutDescription.vertexShader =
                vertexShader_;

            inputLayoutDescription.elements =
                elements.data();

            inputLayoutDescription.elementCount =
                elements.size();

            inputLayoutDescription.debugName =
                "EditorStaticMesh.InputLayout";

            result =
                device.CreateInputLayout(
                    inputLayoutDescription,
                    inputLayout_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create static mesh input layout",
                    result);

                Shutdown(device);
                return false;
            }

            engine::graphics::BufferDesc
                constantBufferDescription;

            constantBufferDescription.byteSize =
                sizeof(ObjectConstants);

            constantBufferDescription.stride = 0U;

            constantBufferDescription.usage =
                engine::graphics::
                    ResourceUsage::Default;

            constantBufferDescription.bindFlags =
                engine::graphics::
                    BufferBindFlags::Constant;

            constantBufferDescription.miscFlags =
                engine::graphics::
                    BufferMiscFlags::None;

            constantBufferDescription.cpuAccess =
                engine::graphics::
                    CpuAccessFlags::None;

            constantBufferDescription.indexFormat =
                engine::graphics::
                    IndexFormat::None;

            result =
                device.CreateBuffer(
                    constantBufferDescription,
                    nullptr,
                    objectBuffer_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create static mesh object buffer",
                    result);

                Shutdown(device);
                return false;
            }

            engine::graphics::BufferDesc instanceBufferDescription;
            instanceBufferDescription.byteSize =
                sizeof(InstanceData) * MaximumInstancesPerDraw;
            instanceBufferDescription.stride = sizeof(InstanceData);
            instanceBufferDescription.usage =
                engine::graphics::ResourceUsage::Dynamic;
            instanceBufferDescription.bindFlags =
                engine::graphics::BufferBindFlags::Vertex;
            instanceBufferDescription.miscFlags =
                engine::graphics::BufferMiscFlags::None;
            instanceBufferDescription.cpuAccess =
                engine::graphics::CpuAccessFlags::Write;
            instanceBufferDescription.indexFormat =
                engine::graphics::IndexFormat::None;

            result = device.CreateBuffer(
                instanceBufferDescription,
                nullptr,
                instanceBuffer_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create static mesh instance buffer",
                    result);
                Shutdown(device);
                return false;
            }

            engine::graphics::
                GraphicsPipelineDesc
                    pipelineDescription;

            pipelineDescription.vertexShader =
                vertexShader_;

            pipelineDescription.pixelShader =
                pixelShader_;

            pipelineDescription.inputLayout =
                inputLayout_;

            pipelineDescription.topology =
                engine::graphics::
                    PrimitiveTopology::
                        TriangleList;

            pipelineDescription.rasterizer.fillMode =
                engine::graphics::
                    FillMode::Solid;

            pipelineDescription.rasterizer.cullMode =
                engine::graphics::CullMode::Back;

            pipelineDescription.rasterizer.depthClipEnable =
                true;

            pipelineDescription.blend.renderTargets[0].
                blendEnable = false;

            pipelineDescription.depthStencil.depthEnable =
                true;

            pipelineDescription.depthStencil.depthWriteEnable =
                true;

            pipelineDescription.depthStencil.depthFunction =
                engine::graphics::ComparisonFunction::LessEqual;

            pipelineDescription.debugName =
                "EditorStaticMesh.Pipeline";

            result = device.CreateGraphicsPipeline(
                pipelineDescription,
                pipeline_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create static mesh pipeline",
                    result);

                Shutdown(device);
                return false;
            }

            /*
             * Opaque / Mask, Double Sided.
             */
            pipelineDescription.rasterizer.cullMode =
                engine::graphics::CullMode::None;

            pipelineDescription.debugName =
                "EditorStaticMesh.DoubleSidedPipeline";

            result = device.CreateGraphicsPipeline(
                pipelineDescription,
                doubleSidedPipeline_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create double-sided static mesh pipeline",
                    result);

                Shutdown(device);
                return false;
            }

            /*
             * Blend, односторонний материал.
             */
            pipelineDescription.rasterizer.cullMode =
                engine::graphics::CullMode::Back;

            pipelineDescription.blend.renderTargets[0].
                blendEnable = true;

            pipelineDescription.blend.renderTargets[0].
                sourceColor =
                    engine::graphics::BlendFactor::SourceAlpha;

            pipelineDescription.blend.renderTargets[0].
                destinationColor =
                    engine::graphics::BlendFactor::
                        InverseSourceAlpha;

            pipelineDescription.blend.renderTargets[0].
                sourceAlpha =
                    engine::graphics::BlendFactor::One;

            pipelineDescription.blend.renderTargets[0].
                destinationAlpha =
                    engine::graphics::BlendFactor::
                        InverseSourceAlpha;

            pipelineDescription.depthStencil.depthWriteEnable =
                false;

            pipelineDescription.debugName =
                "EditorStaticMesh.TransparentPipeline";

            result = device.CreateGraphicsPipeline(
                pipelineDescription,
                transparentPipeline_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create transparent static mesh pipeline",
                    result);

                Shutdown(device);
                return false;
            }

            /*
             * Blend, Double Sided.
             */
            pipelineDescription.rasterizer.cullMode =
                engine::graphics::CullMode::None;

            pipelineDescription.debugName =
                "EditorStaticMesh.TransparentDoubleSidedPipeline";

            result = device.CreateGraphicsPipeline(
                pipelineDescription,
                transparentDoubleSidedPipeline_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create transparent double-sided pipeline",
                    result);

                Shutdown(device);
                return false;
            }
            
            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure("Create transparent static mesh pipeline", result);
                Shutdown(device);
                return false;
            }

            BuildLegacyAssetIndex();
            initialized_ = true;

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Information,
                "LTS.Editor.StaticMesh",
                "Editor static mesh renderer initialized.");

            return true;
        }

        void Shutdown(
            engine::graphics::
                RenderDevice& device) noexcept
        {
            initialized_ = false;

            for (auto& entry : meshes_)
            {
                if (entry.second.gpu != nullptr)
                {
                    static_cast<void>(
                        entry.second.gpu->Release(
                            device));
                }
            }

            meshes_.clear();
            failedMeshes_.clear();

            for (const auto& [key, texture] : materialTextures_)
            {
                static_cast<void>(key);
                if (texture.IsValid())
                {
                    static_cast<void>(device.DestroyTexture(texture));
                }
            }
            materialTextures_.clear();
            failedMaterialTextures_.clear();
            legacyMaterialIndex_.clear();
            legacyTextureIndex_.clear();

            if (materialSampler_.IsValid())
            {
                static_cast<void>(device.DestroySampler(materialSampler_));
                materialSampler_ = {};
            }

            if (transparentDoubleSidedPipeline_.IsValid())
            {
                static_cast<void>(
                    device.DestroyGraphicsPipeline(
                        transparentDoubleSidedPipeline_));

                transparentDoubleSidedPipeline_ = {};
            }

            if (transparentPipeline_.IsValid())
            {
                static_cast<void>(
                    device.DestroyGraphicsPipeline(
                        transparentPipeline_));

                transparentPipeline_ = {};
            }

            if (doubleSidedPipeline_.IsValid())
            {
                static_cast<void>(
                    device.DestroyGraphicsPipeline(
                        doubleSidedPipeline_));

                doubleSidedPipeline_ = {};
            }

            if (pipeline_.IsValid())
            {
                static_cast<void>(
                    device.DestroyGraphicsPipeline(
                        pipeline_));

                pipeline_ = {};
            }

            if (inputLayout_.IsValid())
            {
                static_cast<void>(
                    device.DestroyInputLayout(
                        inputLayout_));

                inputLayout_ = {};
            }

            if (pixelShader_.IsValid())
            {
                static_cast<void>(
                    device.DestroyShader(
                        pixelShader_));

                pixelShader_ = {};
            }

            if (vertexShader_.IsValid())
            {
                static_cast<void>(
                    device.DestroyShader(
                        vertexShader_));

                vertexShader_ = {};
            }

            if (instanceBuffer_.IsValid())
            {
                static_cast<void>(
                    device.DestroyBuffer(instanceBuffer_));
                instanceBuffer_ = {};
            }

            if (objectBuffer_.IsValid())
            {
                static_cast<void>(
                    device.DestroyBuffer(
                        objectBuffer_));

                objectBuffer_ = {};
            }

            device_ = nullptr;
        }

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const SceneDocument& document,
            const DirectX::XMFLOAT4X4&
                viewProjection,
            const DirectX::XMFLOAT3& cameraPosition) noexcept
        {
            if (
                !initialized_ ||
                device_ == nullptr)
            {
                return engine::graphics::
                    GraphicsResult::InvalidState;
            }

            engine::graphics::GraphicsResult result =
                context.SetGraphicsPipeline(
                    pipeline_);

            if (engine::graphics::Failed(result))
            {
                return result;
            }

            result =
                context.SetConstantBuffers(
                    engine::graphics::
                        ShaderStage::Vertex,
                    0U,
                    &objectBuffer_,
                    1U);

            if (engine::graphics::Failed(result))
            {
                context.UnbindGraphicsPipeline();
                return result;
            }

            result = context.SetConstantBuffers(
                engine::graphics::ShaderStage::Pixel, 0U, &objectBuffer_, 1U);
            if (engine::graphics::Failed(result))
            {
                static_cast<void>(context.UnbindConstantBuffers(
                    engine::graphics::ShaderStage::Vertex, 0U, 1U));
                context.UnbindGraphicsPipeline();
                return result;
            }

            result =
                engine::graphics::
                    GraphicsResult::Success;

            const auto& entities =
                document.GetEntities();

            const std::size_t selectedIndex =
                document.GetSelectedIndex();
            const ResolvedDirectionalLight lighting =
                ResolveDirectionalLight(document);

            if (instanceBuffer_.IsValid())
            {
                result = RenderInstancedBatches(
                    context,
                    entities,
                    selectedIndex,
                    lighting,
                    viewProjection,
                    cameraPosition);
            }
            else
            {
            for (
                std::size_t entityIndex = 0U;
                entityIndex < entities.size();
                ++entityIndex)
            {
                const EditorSceneEntity& entity =
                    entities[entityIndex];

                if (
                    !entity.staticMesh.has_value() ||
                    !entity.staticMesh->visible ||
                    entity.staticMesh->
                        assetPath.empty())
                {
                    continue;
                }

                const DirectX::XMVECTOR objectOrigin =
                    DirectX::XMVectorSet(
                        entity.transform.position[0],
                        entity.transform.position[1],
                        entity.transform.position[2],
                        1.0F);
                const DirectX::XMVECTOR clipPosition =
                    DirectX::XMVector4Transform(
                        objectOrigin,
                        DirectX::XMLoadFloat4x4(&viewProjection));
                DirectX::XMFLOAT4 clip{};
                DirectX::XMStoreFloat4(&clip, clipPosition);

                if (
                    clip.w <= 0.001F ||
                    clip.x < -clip.w * 1.25F ||
                    clip.x > clip.w * 1.25F ||
                    clip.y < -clip.w * 1.25F ||
                    clip.y > clip.w * 1.25F ||
                    clip.z < -clip.w * 0.10F ||
                    clip.z > clip.w * 1.20F)
                {
                    continue;
                }

                CachedMesh* const cachedMesh =
                    GetOrLoadMesh(
                        entity.staticMesh->
                            assetPath);

                if (cachedMesh == nullptr || cachedMesh->gpu == nullptr)
                {
                    continue;
                }
                engine::assets::GpuMesh* const mesh = cachedMesh->gpu.get();

                ObjectConstants constants{};

                DirectX::XMStoreFloat4x4(
                    &constants.world,
                    BuildWorldMatrix(
                        entity.transform));

                constants.viewProjection =
                    viewProjection;

                constants.baseColor = { 0.58F, 0.63F, 0.66F, 1.0F };
                constants.materialParameters = {
                    entityIndex == selectedIndex ? 1.0F : 0.0F, 0.0F, 0.0F, 0.5F };
                
                constants.sunDirectionIntensity =
                {
                    lighting.direction.x,
                    lighting.direction.y,
                    lighting.direction.z,
                    lighting.intensity
                };

                constants.sunColor =
                {
                    lighting.color.x,
                    lighting.color.y,
                    lighting.color.z,
                    1.0F
                };

                constants.ambientColor =
                {
                    lighting.ambientColor.x *
                        lighting.ambientIntensity,

                    lighting.ambientColor.y *
                        lighting.ambientIntensity,

                    lighting.ambientColor.z *
                        lighting.ambientIntensity,

                    1.0F
                };

                if (!lighting.sunEnabled)
                {
                    constants.sunDirectionIntensity.w = 0.0F;
                }

                constants.cameraPositionFogDensity =
                {
                    cameraPosition.x,
                    cameraPosition.y,
                    cameraPosition.z,
                    lighting.fogDensity
                };

                constants.fogColorEnabled =
                {
                    lighting.fogColor.x,
                    lighting.fogColor.y,
                    lighting.fogColor.z,
                    lighting.fogEnabled ? 1.0F : 0.0F
                };

                constants.fogDistancesHeight =
                {
                    lighting.fogStart,
                    lighting.fogEnd,
                    lighting.fogHeightFalloff,
                    0.0F
                };

                constants.shadowParameters =
                {
                    lighting.shadowsEnabled ? lighting.shadowStrength : 0.0F,
                    lighting.shadowSoftness,
                    lighting.shadowDistance,
                    0.0F
                };

                result =
                    context.UpdateBuffer(
                        objectBuffer_,
                        &constants,
                        sizeof(constants));

                if (engine::graphics::Failed(result))
                {
                    break;
                }

                engine::graphics::
                    VertexBufferBinding
                        vertexBinding;

                vertexBinding.buffer =
                    mesh->GetVertexBuffer();

                vertexBinding.stride =
                    mesh->GetVertexStride();

                vertexBinding.offset = 0U;

                result =
                    context.SetVertexBuffers(
                        0U,
                        &vertexBinding,
                        1U);

                if (engine::graphics::Failed(result))
                {
                    break;
                }

                engine::graphics::
                    IndexBufferBinding
                        indexBinding;

                indexBinding.buffer =
                    mesh->GetIndexBuffer();

                indexBinding.offset = 0U;

                result =
                    context.SetIndexBuffer(
                        indexBinding);

                if (engine::graphics::Failed(result))
                {
                    break;
                }

                for (
                    std::size_t submeshIndex = 0U;
                    submeshIndex <
                        mesh->GetSubmeshCount();
                    ++submeshIndex)
                {
                    const engine::assets::
                        MeshSubmesh* const submesh =
                            mesh->GetSubmesh(
                                submeshIndex);

                    if (submesh == nullptr)
                    {
                        continue;
                    }

                    engine::graphics::TextureHandle texture;
                    engine::graphics::SamplerHandle sampler;

                    bool transparent = false;
                    bool doubleSided = false;
                    
                    if (submesh->materialSlot < cachedMesh->materials.size())
                    {
                        const CachedMaterial& material =
                            cachedMesh->materials[submesh->materialSlot];
                        constants.baseColor = {
                            material.desc.baseColorFactor[0], material.desc.baseColorFactor[1],
                            material.desc.baseColorFactor[2], material.desc.baseColorFactor[3] };
                        texture = material.baseColorTexture;
                        sampler = material.sampler;
                        
                        transparent = material.desc.alphaMode == engine::assets::MaterialAlphaMode::Blend;
                        doubleSided = material.desc.doubleSided;
                        constants.materialParameters.z = material.desc.alphaMode == engine::assets::MaterialAlphaMode::Mask ? 1.0F : 0.0F;
                        constants.materialParameters.w = material.desc.alphaCutoff;
                        constants.materialParameters.y = texture.IsValid() ? 1.0F : 0.0F;
                    }
                    else
                    {
                        constants.baseColor = { 0.58F, 0.63F, 0.66F, 1.0F };
                        constants.materialParameters.y = 0.0F;
                        constants.materialParameters.z = 0.0F;
                        constants.materialParameters.w = 0.5F;
                    }
                    
                    engine::graphics::PipelineStateHandle selectedPipeline;

                    if (transparent)
                    {
                        selectedPipeline =
                            doubleSided
                                ? transparentDoubleSidedPipeline_
                                : transparentPipeline_;
                    }
                    else
                    {
                        selectedPipeline =
                            doubleSided
                                ? doubleSidedPipeline_
                                : pipeline_;
                    }

                    result = context.SetGraphicsPipeline(
                        selectedPipeline);
                    
                    if (engine::graphics::Failed(result)) break;
                    result = context.UpdateBuffer(objectBuffer_, &constants, sizeof(constants));
                    if (engine::graphics::Failed(result)) break;
                    if (texture.IsValid())
                    {
                        result = context.SetShaderResources(
                            engine::graphics::ShaderStage::Pixel, 0U, &texture, 1U);
                        if (engine::graphics::Failed(result)) break;
                        result = context.SetSamplers(
                            engine::graphics::ShaderStage::Pixel, 0U, &sampler, 1U);
                        if (engine::graphics::Failed(result)) break;
                    }
                    else
                    {
                        static_cast<void>(context.UnbindShaderResources(
                            engine::graphics::ShaderStage::Pixel, 0U, 1U));
                        static_cast<void>(context.UnbindSamplers(
                            engine::graphics::ShaderStage::Pixel, 0U, 1U));
                    }

                    result =
                        context.DrawIndexed(
                            submesh->indexCount,
                            submesh->firstIndex,
                            submesh->baseVertex);

                    if (
                        engine::graphics::Failed(
                            result))
                    {
                        break;
                    }
                }

                if (engine::graphics::Failed(result))
                {
                    break;
                }
            }
            }

            context.UnbindIndexBuffer();
            static_cast<void>(context.UnbindShaderResources(
                engine::graphics::ShaderStage::Pixel, 0U, 6U));
            static_cast<void>(context.UnbindSamplers(
                engine::graphics::ShaderStage::Pixel, 0U, 1U));

            static_cast<void>(
                context.UnbindConstantBuffers(
                    engine::graphics::
                        ShaderStage::Vertex,
                    0U,
                    1U));
            static_cast<void>(context.UnbindConstantBuffers(
                engine::graphics::ShaderStage::Pixel, 0U, 1U));

            context.UnbindGraphicsPipeline();

            return result;
        }

        [[nodiscard]]
        engine::graphics::GraphicsResult RenderInstancedBatches(
            engine::graphics::CommandContext& context,
            const std::vector<EditorSceneEntity>& entities,
            const std::size_t selectedIndex,
            const ResolvedDirectionalLight& lighting,
            const DirectX::XMFLOAT4X4& viewProjection,
            const DirectX::XMFLOAT3& cameraPosition) noexcept
        {
            struct VisibleBatch final
            {
                std::wstring assetPath;
                std::vector<InstanceData> instances;
            };

            try
            {
                std::vector<VisibleBatch> batches;
                batches.reserve(1024U);

                std::unordered_map<std::wstring, std::size_t> batchLookup;
                batchLookup.reserve(1024U);

                std::unordered_map<std::wstring, CachedMesh*> cachedMeshLookup;
                cachedMeshLookup.reserve(1024U);

                const DirectX::XMMATRIX viewProjectionMatrix =
                    DirectX::XMLoadFloat4x4(&viewProjection);
                constexpr float renderDistanceSquared =
                    StaticMeshRenderDistance * StaticMeshRenderDistance;

                for (std::size_t entityIndex = 0U;
                     entityIndex < entities.size();
                     ++entityIndex)
                {
                    const EditorSceneEntity& entity = entities[entityIndex];
                    if (!entity.staticMesh.has_value() ||
                        !entity.staticMesh->visible ||
                        entity.staticMesh->assetPath.empty())
                    {
                        continue;
                    }

                    const float deltaX =
                        entity.transform.position[0] - cameraPosition.x;
                    const float deltaZ =
                        entity.transform.position[2] - cameraPosition.z;
                    if (deltaX * deltaX + deltaZ * deltaZ > renderDistanceSquared)
                    {
                        continue;
                    }

                    const DirectX::XMVECTOR objectOrigin =
                        DirectX::XMVectorSet(
                            entity.transform.position[0],
                            entity.transform.position[1],
                            entity.transform.position[2],
                            1.0F);
                    const DirectX::XMVECTOR clipPosition =
                        DirectX::XMVector4Transform(
                            objectOrigin,
                            viewProjectionMatrix);
                    DirectX::XMFLOAT4 clip{};
                    DirectX::XMStoreFloat4(&clip, clipPosition);

                    const bool originVisible =
                        clip.w > 0.001F &&
                        clip.x >= -clip.w * 1.25F &&
                        clip.x <= clip.w * 1.25F &&
                        clip.y >= -clip.w * 1.25F &&
                        clip.y <= clip.w * 1.25F &&
                        clip.z >= -clip.w * 0.10F &&
                        clip.z <= clip.w * 1.20F;

                    const DirectX::XMMATRIX worldMatrix =
                        BuildWorldMatrix(entity.transform);

                    CachedMesh* cachedMesh = nullptr;
                    const auto cachedMeshEntry =
                        cachedMeshLookup.find(entity.staticMesh->assetPath);
                    if (cachedMeshEntry == cachedMeshLookup.end())
                    {
                        cachedMesh = GetOrLoadMesh(
                            entity.staticMesh->assetPath,
                            false);
                        cachedMeshLookup.emplace(
                            entity.staticMesh->assetPath,
                            cachedMesh);
                    }
                    else
                    {
                        cachedMesh = cachedMeshEntry->second;
                    }

                    if (cachedMesh != nullptr && cachedMesh->gpu != nullptr &&
                        cachedMesh->gpu->GetBounds().IsValid())
                    {
                        if (!IsTransformedMeshBoundsVisible(
                                cachedMesh->gpu->GetBounds(),
                                worldMatrix,
                                viewProjectionMatrix))
                        {
                            continue;
                        }
                    }
                    else if (!originVisible)
                    {
                        // Keep unloaded meshes on the old cheap candidate path.
                        // Their bounds become available after the normal
                        // one-mesh-per-frame streaming step below.
                        continue;
                    }

                    const std::wstring key = LowercasePath(
                        std::filesystem::path(entity.staticMesh->assetPath)
                            .lexically_normal()
                            .wstring());
                    auto foundBatch = batchLookup.find(key);
                    std::size_t batchIndex = 0U;
                    if (foundBatch == batchLookup.end())
                    {
                        batchIndex = batches.size();
                        VisibleBatch batch;
                        batch.assetPath = entity.staticMesh->assetPath;
                        batch.instances.reserve(8U);
                        batches.push_back(std::move(batch));
                        batchLookup.emplace(key, batchIndex);
                    }
                    else
                    {
                        batchIndex = foundBatch->second;
                    }

                    InstanceData instance{};
                    DirectX::XMStoreFloat4x4(
                        &instance.world,
                        worldMatrix);
                    instance.parameters.x =
                        entityIndex == selectedIndex ? 1.0F : 0.0F;
                    batches[batchIndex].instances.push_back(instance);
                }

                ObjectConstants constants{};
                DirectX::XMStoreFloat4x4(
                    &constants.world,
                    DirectX::XMMatrixIdentity());
                constants.viewProjection = viewProjection;
                constants.baseColor = {0.58F, 0.63F, 0.66F, 1.0F};
                constants.materialParameters = {0.0F, 0.0F, 0.0F, 0.5F};
                constants.sunDirectionIntensity = {
                    lighting.direction.x,
                    lighting.direction.y,
                    lighting.direction.z,
                    lighting.sunEnabled ? lighting.intensity : 0.0F};
                constants.sunColor = {
                    lighting.color.x,
                    lighting.color.y,
                    lighting.color.z,
                    1.0F};
                constants.ambientColor = {
                    lighting.ambientColor.x * lighting.ambientIntensity,
                    lighting.ambientColor.y * lighting.ambientIntensity,
                    lighting.ambientColor.z * lighting.ambientIntensity,
                    1.0F};
                constants.cameraPositionFogDensity = {
                    cameraPosition.x,
                    cameraPosition.y,
                    cameraPosition.z,
                    lighting.fogDensity};
                constants.fogColorEnabled = {
                    lighting.fogColor.x,
                    lighting.fogColor.y,
                    lighting.fogColor.z,
                    lighting.fogEnabled ? 1.0F : 0.0F};
                constants.fogDistancesHeight = {
                    lighting.fogStart,
                    lighting.fogEnd,
                    lighting.fogHeightFalloff,
                    0.0F};
                constants.shadowParameters = {
                    lighting.shadowsEnabled ? lighting.shadowStrength : 0.0F,
                    lighting.shadowSoftness,
                    lighting.shadowDistance,
                    0.0F};
                
                std::size_t meshLoads = 0U;
                engine::graphics::GraphicsResult result = engine::graphics::GraphicsResult::Success;
                engine::graphics::PipelineStateHandle boundPipeline = pipeline_;
                std::array<engine::graphics::TextureHandle, 6U> boundMaterialTextures{};

                /*
                 * Сбрасываем material SRV один раз перед всем StaticMesh pass.
                 * Дальше меняем только реально изменившиеся slots.
                 */
                result = context.UnbindShaderResources(
                    engine::graphics::ShaderStage::Pixel,
                    0U,
                    boundMaterialTextures.size());

                if (engine::graphics::Failed(result))
                {
                    return result;
                }

                /*
                 * Все StaticMesh материалы сейчас используют один material sampler.
                 * Биндим его один раз на весь pass.
                 */
                if (EnsureMaterialSampler())
                {
                    result = context.SetSamplers(
                        engine::graphics::ShaderStage::Pixel,
                        0U,
                        &materialSampler_,
                        1U);

                    if (engine::graphics::Failed(result))
                    {
                        return result;
                    }
                }

                for (VisibleBatch& batch : batches)
                {
                    bool loadAttempted = false;
                    CachedMesh* const cachedMesh = GetOrLoadMesh(
                        batch.assetPath,
                        meshLoads < MaximumMeshLoadsPerFrame,
                        &loadAttempted);
                    if (loadAttempted)
                    {
                        ++meshLoads;
                    }
                    if (cachedMesh == nullptr || cachedMesh->gpu == nullptr)
                    {
                        continue;
                    }

                    engine::assets::GpuMesh* const mesh = cachedMesh->gpu.get();
                    const std::array<engine::graphics::VertexBufferBinding, 2U>
                        vertexBindings{{
                            {
                                mesh->GetVertexBuffer(),
                                mesh->GetVertexStride(),
                                0U
                            },
                            {
                                instanceBuffer_,
                                static_cast<std::uint32_t>(sizeof(InstanceData)),
                                0U
                            }
                        }};
                    result = context.SetVertexBuffers(
                        0U,
                        vertexBindings.data(),
                        vertexBindings.size());
                    if (engine::graphics::Failed(result))
                    {
                        break;
                    }

                    engine::graphics::IndexBufferBinding indexBinding;
                    indexBinding.buffer = mesh->GetIndexBuffer();
                    indexBinding.offset = 0U;
                    result = context.SetIndexBuffer(indexBinding);
                    if (engine::graphics::Failed(result))
                    {
                        break;
                    }

                    for (std::size_t firstInstance = 0U;
                         firstInstance < batch.instances.size();
                         firstInstance += MaximumInstancesPerDraw)
                    {
                        const std::size_t instanceCount = (std::min)(MaximumInstancesPerDraw, batch.instances.size() - firstInstance);
                        const InstanceData* const instanceData = batch.instances.data() + firstInstance;
                        result = context.UpdateBuffer(instanceBuffer_, instanceData, instanceCount * sizeof(InstanceData));
                        
                        if (engine::graphics::Failed(result))
                        {
                            break;
                        }

                        for (std::size_t submeshIndex = 0U;
                             submeshIndex < mesh->GetSubmeshCount();
                             ++submeshIndex)
                        {
                            const engine::assets::MeshSubmesh* const submesh =
                                mesh->GetSubmesh(submeshIndex);
                            if (submesh == nullptr)
                            {
                                continue;
                            }

                            engine::graphics::TextureHandle texture;
                            engine::graphics::TextureHandle normalTexture;
                            engine::graphics::TextureHandle specularTexture;
                            engine::graphics::TextureHandle detailNormalTexture;
                            engine::graphics::TextureHandle emissiveTexture;
                            engine::graphics::TextureHandle specularPowerTexture;
                            bool transparent = false;
                            bool doubleSided = false;
                            if (submesh->materialSlot < cachedMesh->materials.size())
                            {
                                const CachedMaterial& material =
                                    cachedMesh->materials[submesh->materialSlot];
                                constants.baseColor = {
                                    material.desc.baseColorFactor[0],
                                    material.desc.baseColorFactor[1],
                                    material.desc.baseColorFactor[2],
                                    material.desc.baseColorFactor[3]};
                                texture = material.baseColorTexture;
                                normalTexture = material.normalTexture;
                                specularTexture = material.specularTexture;
                                detailNormalTexture = material.detailNormalTexture;
                                emissiveTexture = material.emissiveTexture;
                                specularPowerTexture = material.specularPowerTexture;
                                transparent = material.desc.alphaMode == engine::assets::MaterialAlphaMode::Blend;
                                doubleSided = material.desc.doubleSided;
                                constants.materialParameters.y = texture.IsValid() ? 1.0F : 0.0F;
                                constants.materialParameters.z = material.desc.alphaMode == engine::assets::MaterialAlphaMode::Mask ? 1.0F : 0.0F;
                                constants.materialParameters.w = material.desc.alphaCutoff;
                                constants.legacySurfaceParameters = {
                                    material.desc.specularIntensity,
                                    material.desc.specularPower,
                                    material.desc.reflectionFactor,
                                    material.desc.metallicFactor
                                };
                                constants.legacyDetailParameters = {
                                    material.desc.normalScale,
                                    material.detailScale,
                                    material.detailAmount,
                                    material.desc.emissiveStrength
                                };
                                constants.legacyTextureFlags = {
                                    normalTexture.IsValid() ? 1.0F : 0.0F,
                                    specularTexture.IsValid() ? 1.0F : 0.0F,
                                    detailNormalTexture.IsValid() ? 1.0F : 0.0F,
                                    emissiveTexture.IsValid() ? 1.0F : 0.0F
                                };
                                constants.legacyFeatureFlags = {
                                    specularPowerTexture.IsValid() ? 1.0F : 0.0F,
                                    material.camouflage ? 1.0F : 0.0F,
                                    material.displacementEnabled ? 1.0F : 0.0F,
                                    material.displacementValue
                                };
                            }
                            else
                            {
                                constants.baseColor = {0.58F, 0.63F, 0.66F, 1.0F};
                                constants.materialParameters.y = 0.0F;
                                constants.materialParameters.z = 0.0F;
                                constants.materialParameters.w = 0.5F;
                                constants.legacySurfaceParameters = {0.0F, 32.0F, 0.0F, 0.0F};
                                constants.legacyDetailParameters = {1.0F, 1.0F, 0.0F, 0.0F};
                                constants.legacyTextureFlags = {};
                                constants.legacyFeatureFlags = {};
                            }

                            const engine::graphics::PipelineStateHandle selectedPipeline = transparent
                                ? (doubleSided ? transparentDoubleSidedPipeline_ : transparentPipeline_) : (doubleSided
                                ? doubleSidedPipeline_ : pipeline_);
                            
                            if (selectedPipeline != boundPipeline)
                            {
                                result = context.SetGraphicsPipeline(
                                    selectedPipeline);

                                if (engine::graphics::Failed(result))
                                {
                                    break;
                                }

                                boundPipeline = selectedPipeline;
                            }
                            
                            result = context.UpdateBuffer(
                                objectBuffer_,
                                &constants,
                                sizeof(constants));
                            if (engine::graphics::Failed(result))
                            {
                                break;
                            }

                            const std::array<engine::graphics::TextureHandle, 6U>
                                materialTextureHandles{{
                                    texture,
                                    normalTexture,
                                    specularTexture,
                                    detailNormalTexture,
                                    emissiveTexture,
                                    specularPowerTexture}};
                            
                            for (std::size_t textureSlot = 0U; textureSlot < materialTextureHandles.size(); ++textureSlot)
                            {
                                const engine::graphics::TextureHandle requestedTexture =
                                    materialTextureHandles[textureSlot];

                                if (requestedTexture ==
                                    boundMaterialTextures[textureSlot])
                                {
                                    continue;
                                }

                                if (requestedTexture.IsValid())
                                {
                                    result = context.SetShaderResources(
                                        engine::graphics::ShaderStage::Pixel,
                                        static_cast<std::uint32_t>(textureSlot),
                                        &requestedTexture,
                                        1U);
                                }
                                else
                                {
                                    result = context.UnbindShaderResources(
                                        engine::graphics::ShaderStage::Pixel,
                                        static_cast<std::uint32_t>(textureSlot),
                                        1U);
                                }

                                if (engine::graphics::Failed(result))
                                {
                                    break;
                                }

                                boundMaterialTextures[textureSlot] =
                                    requestedTexture;
                            }

                            if (engine::graphics::Failed(result))
                            {
                                break;
                            }

                            result = context.DrawIndexedInstanced(
                                submesh->indexCount,
                                static_cast<std::uint32_t>(instanceCount),
                                submesh->firstIndex,
                                submesh->baseVertex,
                                0U);
                            if (engine::graphics::Failed(result))
                            {
                                break;
                            }
                        }

                        if (engine::graphics::Failed(result))
                        {
                            break;
                        }
                    }

                    if (engine::graphics::Failed(result))
                    {
                        break;
                    }
                }

                return result;
            }
            catch (const std::bad_alloc&)
            {
                return engine::graphics::GraphicsResult::OutOfMemory;
            }
            catch (...)
            {
                return engine::graphics::GraphicsResult::BackendFailure;
            }
        }

        [[nodiscard]]
        bool PreviewMaterial(
            const std::wstring& assetPath,
            const std::size_t materialSlot,
            const engine::assets::MaterialAssetDesc& material) noexcept
        {
            if (!initialized_)
            {
                return false;
            }

            try
            {
                CachedMesh* const cachedMesh =
                    GetOrLoadMesh(assetPath);

                if (cachedMesh == nullptr ||
                    materialSlot >= cachedMesh->materials.size())
                {
                    return false;
                }

                /*
                 * TextureHandle и SamplerHandle не трогаем.
                 * Здесь обновляются цвет, прозрачность,
                 * Double Sided и остальные параметры.
                 */
                cachedMesh->materials[materialSlot].desc =
                    material;

                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]]
        bool ReloadMaterials(
            const std::wstring& assetPath) noexcept
        {
            if (!initialized_ || device_ == nullptr)
            {
                return false;
            }

            try
            {
                std::filesystem::path path(assetPath);

                if (!path.is_absolute())
                {
                    std::error_code error;

                    const std::filesystem::path gameRoot =
                        std::filesystem::current_path(error);

                    if (error)
                    {
                        return false;
                    }

                    path = gameRoot / path;
                }

                path = path.lexically_normal();

                const std::wstring key =
                    LowercasePath(path.wstring());

                const auto found =
                    meshes_.find(key);

                /*
                 * Если mesh ещё не загружен, то при первом
                 * рендере он сразу прочитает новый материал.
                 */
                if (found == meshes_.end())
                {
                    return true;
                }

                found->second.materials.clear();

                LoadMaterials(
                    path,
                    found->second.materials);

                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]] bool TryGetMeshBounds(
            const std::wstring& assetPath,
            DirectX::XMFLOAT3& minimum,
            DirectX::XMFLOAT3& maximum) const noexcept
        {
            try
            {
                std::filesystem::path path(assetPath);
                if (!path.is_absolute())
                {
                    path = std::filesystem::current_path() / path;
                }
                const auto found = meshes_.find(
                    LowercasePath(path.lexically_normal().wstring()));
                if (found == meshes_.end() || found->second.gpu == nullptr)
                {
                    return false;
                }
                const engine::assets::MeshBounds& bounds =
                    found->second.gpu->GetBounds();
                minimum = {bounds.minimum[0], bounds.minimum[1], bounds.minimum[2]};
                maximum = {bounds.maximum[0], bounds.maximum[1], bounds.maximum[2]};
                return bounds.IsValid();
            }
            catch (...)
            {
                return false;
            }
        }

    private:
        [[nodiscard]]
        CachedMesh* GetOrLoadMesh(
            const std::wstring& assetPath,
            const bool allowLoad = true,
            bool* const loadAttempted = nullptr) noexcept
        {
            if (loadAttempted != nullptr)
            {
                *loadAttempted = false;
            }

            try
            {
                std::filesystem::path path(
                    assetPath);

                std::error_code filesystemError;

                if (!path.is_absolute())
                {
                    const std::filesystem::path gameRoot =
                        std::filesystem::current_path(
                            filesystemError);

                    if (filesystemError)
                    {
                        return nullptr;
                    }

                    path =
                        gameRoot /
                        path;
                }

                path =
                    path.lexically_normal();

                const std::wstring key =
                    LowercasePath(
                        path.wstring());

                const auto existing =
                    meshes_.find(key);

                if (existing != meshes_.end())
                {
                    return
                        &existing->second;
                }

                if (
                    failedMeshes_.find(key) !=
                    failedMeshes_.end())
                {
                    return nullptr;
                }

                if (!allowLoad)
                {
                    return nullptr;
                }

                if (loadAttempted != nullptr)
                {
                    *loadAttempted = true;
                }

                std::unique_ptr<engine::assets::GpuMesh> mesh =
                        LoadGpuMesh(
                            path,
                            std::filesystem::path(
                                assetPath));

                if (mesh == nullptr)
                {
                    failedMeshes_.insert(key);
                    return nullptr;
                }

                CachedMesh cached;
                cached.gpu = std::move(mesh);
                LoadMaterials(path, cached.materials);
                auto inserted = meshes_.emplace(key, std::move(cached));
                return &inserted.first->second;
            }
            catch (...)
            {
                return nullptr;
            }
        }

        void BuildLegacyAssetIndex() noexcept
        {
            legacyMaterialIndex_.clear();
            legacyTextureIndex_.clear();

            try
            {
                std::filesystem::path objectsDepot =
                    std::filesystem::current_path() / L"Data" / L"ObjectsDepot";
                std::error_code error;
                if (!std::filesystem::is_directory(objectsDepot, error) || error)
                {
                    error.clear();
                    objectsDepot = std::filesystem::current_path() /
                        L"bin" / L"Data" / L"ObjectsDepot";
                }
                if (!std::filesystem::is_directory(objectsDepot, error) || error)
                {
                    return;
                }

                for (std::filesystem::recursive_directory_iterator iterator(
                         objectsDepot,
                         std::filesystem::directory_options::skip_permission_denied,
                         error),
                     end;
                     !error && iterator != end;
                     iterator.increment(error))
                {
                    if (!iterator->is_regular_file(error) || error)
                    {
                        error.clear();
                        continue;
                    }

                    const std::filesystem::path path = iterator->path();
                    const std::wstring extension = LowercasePath(
                        path.extension().wstring());
                    const std::wstring filename = LowercasePath(
                        path.filename().wstring());
                    if (extension == L".mat")
                    {
                        legacyMaterialIndex_.try_emplace(filename, path);
                    }
                    else if (extension == L".dds" || extension == L".png" ||
                             extension == L".jpg" || extension == L".jpeg" ||
                             extension == L".bmp")
                    {
                        legacyTextureIndex_.try_emplace(filename, path);
                    }
                }

                std::string message = "Legacy ObjectsDepot index: ";
                message += std::to_string(legacyMaterialIndex_.size());
                message += " materials, ";
                message += std::to_string(legacyTextureIndex_.size());
                message += " textures.";
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Information,
                    "LTS.Editor.StaticMesh",
                    message);
            }
            catch (...)
            {
                legacyMaterialIndex_.clear();
                legacyTextureIndex_.clear();
            }
        }

        [[nodiscard]]
        engine::graphics::TextureHandle GetOrLoadMaterialTexture(
            const std::filesystem::path& path,
            const bool forceSrgb) noexcept
        {
            if (device_ == nullptr || path.empty())
            {
                return {};
            }

            try
            {
                std::error_code error;
                if (!std::filesystem::is_regular_file(path, error) || error)
                {
                    return {};
                }

                std::wstring key = LowercasePath(
                    path.lexically_normal().wstring());
                key += forceSrgb ? L"|srgb" : L"|linear";
                const auto existing = materialTextures_.find(key);
                if (existing != materialTextures_.end())
                {
                    return existing->second;
                }
                if (failedMaterialTextures_.find(key) !=
                    failedMaterialTextures_.end())
                {
                    return {};
                }

                engine::graphics::TextureHandle texture;
                if (!CreateTextureFromFile(
                        *device_,
                        path,
                        forceSrgb,
                        texture))
                {
                    failedMaterialTextures_.insert(std::move(key));
                    return {};
                }

                materialTextures_.emplace(std::move(key), texture);
                if (materialTextures_.size() == 1U ||
                    materialTextures_.size() % 250U == 0U)
                {
                    std::string message = "Legacy material textures loaded: ";
                    message += std::to_string(materialTextures_.size());
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Information,
                        "LTS.Editor.StaticMesh",
                        message);
                }
                return texture;
            }
            catch (...)
            {
                return {};
            }
        }

        [[nodiscard]]
        bool EnsureMaterialSampler() noexcept
        {
            if (materialSampler_.IsValid()) { return true; }

            if (device_ == nullptr) { return false; }

            engine::graphics::SamplerDesc description;
            description.filter = engine::graphics::TextureFilter::Anisotropic;
            description.addressU = engine::graphics::TextureAddressMode::Wrap;
            description.addressV = engine::graphics::TextureAddressMode::Wrap;
            description.addressW = engine::graphics::TextureAddressMode::Wrap;
            description.maximumAnisotropy = 16U;

            return engine::graphics::Succeeded(
                device_->CreateSampler(description, materialSampler_));
        }

        [[nodiscard]]
        static std::filesystem::path FirstRegularFile(
            const std::vector<std::filesystem::path>& candidates) noexcept
        {
            try
            {
                for (const std::filesystem::path& candidate : candidates)
                {
                    std::error_code error;
                    if (std::filesystem::is_regular_file(candidate, error) && !error)
                    {
                        return candidate.lexically_normal();
                    }
                }
            }
            catch (...)
            {
            }
            return {};
        }

        [[nodiscard]]
        std::filesystem::path ResolveLegacyTexturePath(
            const std::string& value,
            const std::filesystem::path& materialPath,
            const std::filesystem::path& packageRoot,
            const std::filesystem::path& sourceDirectory,
            const std::filesystem::path& workspaceRoot) noexcept
        {
            try
            {
                if (value.empty())
                {
                    return {};
                }
                std::filesystem::path requested =
                    std::filesystem::u8path(value).lexically_normal();
                if (requested.is_absolute())
                {
                    return FirstRegularFile({requested});
                }

                const std::filesystem::path local = FirstRegularFile({
                    materialPath.parent_path().parent_path() / L"Textures" / requested,
                    packageRoot / L"Textures" / requested,
                    sourceDirectory / L"Textures" / requested,
                    materialPath.parent_path() / requested,
                    packageRoot / requested,
                    sourceDirectory / requested,
                    workspaceRoot / L"bin" / requested});
                if (!local.empty())
                {
                    return local;
                }

                const auto global = legacyTextureIndex_.find(
                    LowercasePath(requested.filename().wstring()));
                return global != legacyTextureIndex_.end()
                    ? global->second
                    : std::filesystem::path{};
            }
            catch (...)
            {
                return {};
            }
        }

        [[nodiscard]]
        bool LoadLegacyMaterials(
            const std::filesystem::path& meshPath,
            std::vector<CachedMaterial>& output) noexcept
        {
            try
            {
                std::filesystem::path sidecarPath = meshPath;
                sidecarPath += L".materials";
                std::ifstream sidecar(sidecarPath, std::ios::binary);
                if (!sidecar)
                {
                    return false;
                }

                std::filesystem::path meshesRoot;
                std::filesystem::path cursor = meshPath.parent_path();
                while (!cursor.empty())
                {
                    if (LowercasePath(cursor.filename().wstring()) == L"meshes")
                    {
                        meshesRoot = cursor;
                        break;
                    }
                    const std::filesystem::path parent = cursor.parent_path();
                    if (parent == cursor) break;
                    cursor = parent;
                }
                if (meshesRoot.empty())
                {
                    return false;
                }

                std::filesystem::path workspaceRoot = meshesRoot;
                while (!workspaceRoot.empty())
                {
                    std::error_code error;
                    if (std::filesystem::is_directory(
                            workspaceRoot / L"bin" / L"Data" / L"ObjectsDepot",
                            error) && !error)
                    {
                        break;
                    }
                    const std::filesystem::path parent = workspaceRoot.parent_path();
                    if (parent == workspaceRoot)
                    {
                        workspaceRoot.clear();
                        break;
                    }
                    workspaceRoot = parent;
                }
                if (workspaceRoot.empty())
                {
                    return false;
                }

                std::error_code error;
                std::filesystem::path relativeMesh = std::filesystem::relative(
                    meshPath,
                    meshesRoot,
                    error);
                if (error)
                {
                    return false;
                }
                std::filesystem::path sourcePath =
                    workspaceRoot / L"bin" / relativeMesh;
                sourcePath.replace_extension(L".sco");
                if (!std::filesystem::is_regular_file(sourcePath, error) || error)
                {
                    error.clear();
                    sourcePath.replace_extension(L".scb");
                }
                const std::filesystem::path sourceDirectory =
                    sourcePath.parent_path();

                std::filesystem::path packageRoot = sourceDirectory;
                cursor = sourceDirectory;
                while (!cursor.empty() &&
                       LowercasePath(cursor.filename().wstring()) != L"objectsdepot")
                {
                    error.clear();
                    if (std::filesystem::is_directory(cursor / L"Materials", error) &&
                        !error)
                    {
                        packageRoot = cursor;
                        break;
                    }
                    const std::filesystem::path parent = cursor.parent_path();
                    if (parent == cursor) break;
                    cursor = parent;
                }

                std::vector<std::string> materialNames;
                std::string materialName;
                while (std::getline(sidecar, materialName))
                {
                    if (!materialName.empty() && materialName.back() == '\r')
                    {
                        materialName.pop_back();
                    }
                    materialNames.push_back(TrimAscii(std::move(materialName)));
                }
                if (materialNames.empty())
                {
                    return false;
                }

                static_cast<void>(EnsureMaterialSampler());
                output.clear();
                output.reserve(materialNames.size());

                for (const std::string& name : materialNames)
                {
                    CachedMaterial cached;
                    cached.desc.debugName = name;

                    const std::filesystem::path materialFilename =
                        std::filesystem::u8path(
                            name.empty() || name == "__default"
                                ? "_DEFAULT_.mat"
                                : name + ".mat");
                    std::filesystem::path materialPath = FirstRegularFile({
                        packageRoot / L"Materials" / materialFilename,
                        sourceDirectory / L"Materials" / materialFilename,
                        sourceDirectory / materialFilename,
                        packageRoot / materialFilename});
                    if (materialPath.empty())
                    {
                        const auto global = legacyMaterialIndex_.find(
                            LowercasePath(materialFilename.filename().wstring()));
                        if (global != legacyMaterialIndex_.end())
                        {
                            materialPath = global->second;
                        }
                    }

                    LegacyMaterialDefinition legacy;
                    if (!materialPath.empty() &&
                        ParseLegacyMaterial(materialPath, legacy))
                    {
                        cached.desc = legacy.desc;
                        cached.detailScale = legacy.detailScale;
                        cached.detailAmount = legacy.detailAmount;
                        cached.displacementEnabled = legacy.displacementEnabled;
                        cached.displacementValue = legacy.displacementValue;
                        cached.camouflage = legacy.camouflage;
                        cached.type = std::move(legacy.type);

                        const auto resolveTexture =
                            [&](const std::string& textureName)
                            {
                                return ResolveLegacyTexturePath(
                                    textureName,
                                    materialPath,
                                    packageRoot,
                                    sourceDirectory,
                                    workspaceRoot);
                            };
                        cached.baseColorTexture = GetOrLoadMaterialTexture(
                            resolveTexture(legacy.diffuseTexture), true);
                        cached.normalTexture = GetOrLoadMaterialTexture(
                            resolveTexture(legacy.normalTexture), false);
                        cached.specularTexture = GetOrLoadMaterialTexture(
                            resolveTexture(legacy.specularTexture), false);
                        cached.specularPowerTexture = GetOrLoadMaterialTexture(
                            resolveTexture(legacy.specularPowerTexture), false);
                        cached.detailNormalTexture = GetOrLoadMaterialTexture(
                            resolveTexture(legacy.detailNormalTexture), false);
                        cached.emissiveTexture = GetOrLoadMaterialTexture(
                            resolveTexture(legacy.emissiveTexture), true);
                    }

                    cached.sampler = materialSampler_;
                    output.push_back(std::move(cached));
                }

                return !output.empty();
            }
            catch (...)
            {
                output.clear();
                return false;
            }
        }

        void LoadMaterials(
            const std::filesystem::path& meshPath,
            std::vector<CachedMaterial>& output) noexcept
        {
            output.clear();
            try
            {
                if (LoadLegacyMaterials(meshPath, output))
                {
                    return;
                }

                std::filesystem::path meshesRoot;
                std::filesystem::path cursor = meshPath.parent_path();
                while (!cursor.empty())
                {
                    if (LowercasePath(cursor.filename().wstring()) == L"meshes")
                    {
                        meshesRoot = cursor;
                        break;
                    }
                    const auto parent = cursor.parent_path();
                    if (parent == cursor) break;
                    cursor = parent;
                }
                if (meshesRoot.empty()) return;
                std::error_code filesystemError;
                const auto package = std::filesystem::relative(
                    meshPath.parent_path(), meshesRoot, filesystemError);
                if (filesystemError) return;
                const auto directory = meshesRoot.parent_path() / L"Materials" / package;
                if (!std::filesystem::is_directory(directory, filesystemError) || filesystemError)
                    return;
                
                std::vector<std::filesystem::path>matchingFiles;
                std::vector<std::filesystem::path>legacyFiles;

                const std::wstring materialPrefix =
                    LowercasePath(
                        meshPath.stem().wstring() +
                        L"_");

                for (
                    std::filesystem::directory_iterator
                        iterator(
                            directory,
                            filesystemError),
                        end;

                    !filesystemError &&
                    iterator != end;

                    iterator.increment(filesystemError))
                {
                    if (
                        !iterator->is_regular_file() ||
                        LowercasePath(
                            iterator->path().
                                extension().
                                wstring()) !=
                            L".material")
                    {
                        continue;
                    }

                    const std::filesystem::path file =
                        iterator->path();

                    legacyFiles.push_back(file);

                    const std::wstring filename =
                        LowercasePath(
                            file.filename().wstring());

                    if (
                        filename.rfind(
                            materialPrefix,
                            0U) == 0U)
                    {
                        matchingFiles.push_back(file);
                    }
                }
                
                /*
                 * Новые материалы:
                 *
                 * MeshName_0000_Material.material
                 *
                 * Для старых ресурсов оставляем fallback,
                 * где материалы назывались просто
                 * 0000_Material.material.
                 */
                std::vector<std::filesystem::path> files =
                    !matchingFiles.empty()
                        ? std::move(matchingFiles)
                        : std::move(legacyFiles);

                std::sort(
                    files.begin(),
                    files.end(),
                    [](const auto& left,
                       const auto& right)
                    {
                        return
                            LowercasePath(
                                left.filename().wstring()) <
                            LowercasePath(
                                right.filename().wstring());
                    });
                
                for (const auto& file : files)
                {
                    engine::assets::AssetData data;
                    if (engine::assets::Failed(ReadAssetData(file, data))) continue;
                    const auto gameRoot = meshesRoot.parent_path().parent_path();
                    const auto logical = std::filesystem::relative(file, gameRoot, filesystemError);
                    if (filesystemError) continue;
                    engine::assets::AssetPath assetPath;
                    if (engine::assets::Failed(engine::assets::AssetPath::TryCreate(
                            logical.generic_u8string(), assetPath))) continue;
                    engine::assets::AssetMetadata metadata;
                    metadata.path = std::move(assetPath);
                    metadata.id = metadata.path.GetId();
                    metadata.type = engine::assets::AssetType::Material;
                    metadata.schemaVersion = 2U;
                    metadata.sourceSize = data.GetSize();
                    engine::assets::MaterialAssetLoader loader;
                    std::unique_ptr<engine::assets::LoadedAsset> loaded;
                    if (engine::assets::Failed(loader.Load(metadata, data, loaded)) || !loaded)
                        continue;
                    const auto* loadedMaterial = static_cast<engine::assets::MaterialLoadedAsset*>(loaded.get());
                    CachedMaterial material;
                    material.desc = loadedMaterial->GetMaterial().GetDesc();
                    
                    if (material.desc.baseColorTexture.has_value())
                    {
                        const std::filesystem::path texturePath =
                            gameRoot /
                            std::filesystem::u8path(
                                material.desc.
                                    baseColorTexture->
                                    String());

                        material.baseColorTexture =
                            GetOrLoadMaterialTexture(texturePath, true);
                    }

                    if (material.baseColorTexture.IsValid() &&
                        EnsureMaterialSampler())
                    {
                        material.sampler = materialSampler_;
                    }
                    output.push_back(std::move(material));
                }
            }
            catch (...) { output.clear(); }
        }

        [[nodiscard]]
        std::unique_ptr<
            engine::assets::GpuMesh>
                LoadGpuMesh(
                    const std::filesystem::path& filePath,
                    const std::filesystem::path& logicalPath) noexcept
        {
            try
            {
                engine::assets::AssetData sourceData;

                engine::assets::AssetResult assetResult =
                    ReadAssetData(
                        filePath,
                        sourceData);

                if (
                    engine::assets::Failed(
                        assetResult))
                {
                    LogAssetFailure(
                        filePath,
                        "Read LTS mesh",
                        assetResult);

                    return nullptr;
                }

                engine::assets::AssetMetadata metadata;

                assetResult =
                    CreateMeshMetadata(
                        logicalPath,
                        sourceData.GetSize(),
                        metadata);

                if (
                    engine::assets::Failed(
                        assetResult))
                {
                    LogAssetFailure(
                        filePath,
                        "Create mesh metadata",
                        assetResult);

                    return nullptr;
                }

                engine::assets::
                    MeshAssetLoader loader;

                std::unique_ptr<
                    engine::assets::LoadedAsset>
                        loadedAsset;

                assetResult =
                    loader.Load(
                        metadata,
                        sourceData,
                        loadedAsset);

                if (
                    engine::assets::Failed(
                        assetResult) ||
                    loadedAsset == nullptr ||
                    loadedAsset->GetType() !=
                        engine::assets::
                            AssetType::Mesh)
                {
                    if (
                        engine::assets::Succeeded(
                            assetResult))
                    {
                        assetResult =
                            engine::assets::
                                AssetResult::
                                    TypeMismatch;
                    }

                    LogAssetFailure(
                        filePath,
                        "Load LTS mesh",
                        assetResult);

                    return nullptr;
                }

                auto* const loadedMesh =
                    static_cast<
                        engine::assets::
                            MeshLoadedAsset*>(
                                loadedAsset.get());

                engine::assets::MeshAsset cpuMesh =
                    loadedMesh->ReleaseMesh();

                auto gpuMesh =
                    std::make_unique<
                        engine::assets::GpuMesh>();

                const engine::graphics::
                    GraphicsResult uploadResult =
                        gpuMesh->Upload(
                            *device_,
                            cpuMesh);

                if (
                    engine::graphics::Failed(
                        uploadResult))
                {
                    LogGraphicsFailure(
                        "Upload static mesh",
                        uploadResult);

                    return nullptr;
                }

                return gpuMesh;
            }
            catch (const std::bad_alloc&)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.StaticMesh",
                    "Not enough memory to load a static mesh.");

                return nullptr;
            }
            catch (...)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.StaticMesh",
                    "Unexpected static mesh loading failure.");

                return nullptr;
            }
        }

        engine::graphics::RenderDevice*
            device_ = nullptr;

        engine::graphics::BufferHandle
            objectBuffer_;

        engine::graphics::BufferHandle
            instanceBuffer_;

        engine::graphics::ShaderHandle
            vertexShader_;

        engine::graphics::ShaderHandle
            pixelShader_;

        engine::graphics::InputLayoutHandle
            inputLayout_;

        engine::graphics::PipelineStateHandle pipeline_;

        engine::graphics::PipelineStateHandle
            doubleSidedPipeline_;

        engine::graphics::PipelineStateHandle
            transparentPipeline_;

        engine::graphics::PipelineStateHandle
            transparentDoubleSidedPipeline_;

        std::unordered_map<
            std::wstring,
            CachedMesh> meshes_;

        std::unordered_set<
            std::wstring> failedMeshes_;

        std::unordered_map<
            std::wstring,
            engine::graphics::TextureHandle> materialTextures_;

        std::unordered_set<
            std::wstring> failedMaterialTextures_;

        std::unordered_map<
            std::wstring,
            std::filesystem::path> legacyMaterialIndex_;

        std::unordered_map<
            std::wstring,
            std::filesystem::path> legacyTextureIndex_;

        engine::graphics::SamplerHandle materialSampler_;

        bool initialized_ = false;
    };

    StaticMeshRenderer::
        StaticMeshRenderer() noexcept =
            default;

    StaticMeshRenderer::
        ~StaticMeshRenderer() noexcept =
            default;

    bool StaticMeshRenderer::PreviewMaterial(
        const std::wstring& assetPath,
        const std::size_t materialSlot,
        const engine::assets::MaterialAssetDesc& material) noexcept
    {
        return
            impl_ != nullptr &&
            impl_->PreviewMaterial(
                assetPath,
                materialSlot,
                material);
    }

    bool StaticMeshRenderer::ReloadMaterials(
        const std::wstring& assetPath) noexcept
    {
        return
            impl_ != nullptr &&
            impl_->ReloadMaterials(assetPath);
    }

    bool StaticMeshRenderer::Initialize(
        engine::graphics::
            RenderDevice& device) noexcept
    {
        if (impl_ != nullptr)
        {
            return true;
        }

        try
        {
            impl_ =
                std::make_unique<Impl>();
        }
        catch (...)
        {
            return false;
        }

        if (!impl_->Initialize(device))
        {
            impl_.reset();
            return false;
        }

        return true;
    }

    void StaticMeshRenderer::Shutdown(
        engine::graphics::
            RenderDevice& device) noexcept
    {
        if (impl_ == nullptr)
        {
            return;
        }

        impl_->Shutdown(device);
        impl_.reset();
    }

    engine::graphics::GraphicsResult
        StaticMeshRenderer::Render(
            engine::graphics::
                CommandContext& context,
            const SceneDocument& document,
            const DirectX::XMFLOAT4X4&
                viewProjection,
            const DirectX::XMFLOAT3& cameraPosition) noexcept
    {
        if (impl_ == nullptr)
        {
            return engine::graphics::
                GraphicsResult::InvalidState;
        }

        return impl_->Render(
            context,
            document,
            viewProjection,
            cameraPosition);
    }

    bool StaticMeshRenderer::TryGetMeshBounds(
        const std::wstring& assetPath,
        DirectX::XMFLOAT3& minimum,
        DirectX::XMFLOAT3& maximum) const noexcept
    {
        return impl_ != nullptr &&
            impl_->TryGetMeshBounds(assetPath, minimum, maximum);
    }
}