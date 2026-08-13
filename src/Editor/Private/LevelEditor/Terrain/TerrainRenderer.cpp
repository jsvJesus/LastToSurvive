#include "Editor/LevelEditor/Terrain/TerrainRenderer.h"
#include "Editor/LevelEditor/Rendering/ShaderCompiler.h"

#include <Assets/AssetData.h>
#include <Assets/AssetResult.h>
#include <Assets/DdsTextureDecoder.h>
#include <Assets/TerrainAsset.h>
#include <Assets/TextureAsset.h>
#include <Core/Log.h>
#include <Graphics/Buffer.h>
#include <Graphics/CommandContext.h>
#include <Graphics/Format.h>
#include <Graphics/GraphicsResult.h>
#include <Graphics/InputLayout.h>
#include <Graphics/PipelineState.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/Sampler.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture.h>
#include <GraphicsDX11/D3D11Device.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace lts::editor
{
    namespace
    {
        constexpr std::uint32_t MaximumTerrainGpuCellsPerAxis = 1024U;
        constexpr std::uint32_t TerrainChunkCellCount = 64U;
        constexpr std::uint32_t TerrainBrushSegmentCount = 96U;
        constexpr std::uint32_t TerrainBrushVertexCount = TerrainBrushSegmentCount * 2U;
        constexpr std::size_t MaximumTerrainMaskCount = 6U;
        constexpr std::size_t MaximumTerrainLayerCount = 18U;
        constexpr std::size_t MaximumPaintedLayerCount = 17U;
        constexpr std::uint32_t MaximumPaintHistorySize = 32U;
        constexpr std::uint32_t MaximumHeightHistorySize = 32U;
        constexpr std::uintmax_t MaximumTextureFileSize =
            512ULL * 1024ULL * 1024ULL;

        constexpr std::uint32_t PaintFileSignature = 0x5053544CU;
        constexpr std::uint32_t PaintFileVersion = 1U;

        struct Vertex final
        {
            DirectX::XMFLOAT3 position{};
            DirectX::XMFLOAT3 normal{};
            DirectX::XMFLOAT2 uv{};
        };

        struct alignas(16) Constants final
        {
            DirectX::XMFLOAT4X4 world{};
            DirectX::XMFLOAT4X4 viewProjection{};

            std::array<DirectX::XMFLOAT4, MaximumTerrainLayerCount>
                placement{};

            std::array<DirectX::XMFLOAT4, MaximumTerrainLayerCount>
                layerParameters{};

            // x = terrain local width.
            // y = terrain local depth.
            // z = active layer count.
            // w = reserved.
            DirectX::XMFLOAT4 terrainInfo{};
            DirectX::XMFLOAT4 sunDirectionIntensity{};
            DirectX::XMFLOAT4 sunColor{};
            DirectX::XMFLOAT4 ambientColor{};
            DirectX::XMFLOAT4 cameraPositionFogDensity{};
            DirectX::XMFLOAT4 fogColorEnabled{};
            DirectX::XMFLOAT4 fogDistancesHeight{};
            DirectX::XMFLOAT4 shadowParameters{};
        };

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

        static_assert(sizeof(Constants) % 16U == 0U);

        struct BrushVertex final
        {
            DirectX::XMFLOAT3 position{};
            DirectX::XMFLOAT4 color{};
        };

        struct alignas(16) BrushConstants final
        {
            DirectX::XMFLOAT4X4 viewProjection{};
        };

        static_assert(sizeof(BrushConstants) % 16U == 0U);

        struct Chunk final
        {
            DirectX::XMFLOAT3 minimum{};
            DirectX::XMFLOAT3 maximum{};
            DirectX::XMFLOAT3 center{};

            std::array<std::uint32_t, 3U> first{};
            std::array<std::uint32_t, 3U> count{};
        };

        struct PaintChange final
        {
            std::uint32_t pixel = 0U;
            std::array<std::uint8_t, MaximumPaintedLayerCount> before{};
            std::array<std::uint8_t, MaximumPaintedLayerCount> after{};
        };

        using PaintCommand = std::vector<PaintChange>;

        struct HeightChange final
        {
            std::uint32_t sample = 0U;
            std::int16_t before = 0;
            std::int16_t after = 0;
        };

        using HeightCommand =
            std::vector<HeightChange>;

        [[nodiscard]]
        std::uint32_t CalculateTerrainSampleStep(
            const std::uint32_t sourceWidth,
            const std::uint32_t sourceHeight) noexcept
        {
            if (sourceWidth < 2U || sourceHeight < 2U)
            {
                return 1U;
            }

            const std::uint32_t maximumCellCount =
                (std::max)(sourceWidth - 1U, sourceHeight - 1U);

            return (std::max)(
                1U,
                (maximumCellCount + MaximumTerrainGpuCellsPerAxis - 1U) /
                    MaximumTerrainGpuCellsPerAxis);
        }

        [[nodiscard]]
        std::uint32_t CalculateTerrainGpuVertexCount(
            const std::uint32_t sourceVertexCount,
            const std::uint32_t sampleStep) noexcept
        {
            if (sourceVertexCount < 2U || sampleStep == 0U)
            {
                return sourceVertexCount;
            }

            return
                (sourceVertexCount - 1U + sampleStep - 1U) / sampleStep + 1U;
        }

        void LogTerrainGraphicsFailure(
            const char* const operation,
            const engine::graphics::GraphicsResult result)
        {
            std::string message =
                operation != nullptr
                    ? operation
                    : "Terrain graphics operation";

            message += " failed: ";
            message += engine::graphics::ToString(result);

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                "LTS.Editor.Terrain",
                message);
        }

        void LogTerrainAssetFailure(
            const char* const operation,
            const engine::assets::AssetResult result)
        {
            std::string message =
                operation != nullptr
                    ? operation
                    : "Terrain asset operation";

            message += " failed: ";
            message += engine::assets::ToString(result);

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                "LTS.Editor.Terrain",
                message);
        }

        void LogTerrainGeometryInfo(
            const std::uint32_t sourceWidth,
            const std::uint32_t sourceHeight,
            const std::uint32_t gpuWidth,
            const std::uint32_t gpuHeight,
            const std::uint32_t sampleStep)
        {
            std::string message = "Preparing terrain GPU grid. Source=";
            message += std::to_string(sourceWidth);
            message += "x";
            message += std::to_string(sourceHeight);
            message += ", GPU=";
            message += std::to_string(gpuWidth);
            message += "x";
            message += std::to_string(gpuHeight);
            message += ", sample step=";
            message += std::to_string(sampleStep);
            message += ".";

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Information,
                "LTS.Editor.Terrain",
                message);
        }

        [[nodiscard]]
        bool IsChunkVisible(
            const Chunk& chunk,
            DirectX::FXMMATRIX worldViewProjection) noexcept
        {
            std::array<DirectX::XMVECTOR, 8U> clipPoints{};
            std::size_t pointIndex = 0U;

            for (std::uint32_t z = 0U; z < 2U; ++z)
            {
                for (std::uint32_t y = 0U; y < 2U; ++y)
                {
                    for (std::uint32_t x = 0U; x < 2U; ++x)
                    {
                        const DirectX::XMVECTOR position =
                            DirectX::XMVectorSet(
                                x == 0U
                                    ? chunk.minimum.x
                                    : chunk.maximum.x,
                                y == 0U
                                    ? chunk.minimum.y
                                    : chunk.maximum.y,
                                z == 0U
                                    ? chunk.minimum.z
                                    : chunk.maximum.z,
                                1.0F);

                        clipPoints[pointIndex++] =
                            DirectX::XMVector4Transform(
                                position,
                                worldViewProjection);
                    }
                }
            }

            const auto allOutside =
                [&clipPoints](const auto& predicate)
                {
                    for (const DirectX::XMVECTOR& clipPoint : clipPoints)
                    {
                        DirectX::XMFLOAT4 value{};
                        DirectX::XMStoreFloat4(&value, clipPoint);

                        if (!predicate(value))
                        {
                            return false;
                        }
                    }

                    return true;
                };

            return
                !allOutside(
                    [](const DirectX::XMFLOAT4& point)
                    {
                        return point.x < -point.w;
                    }) &&
                !allOutside(
                    [](const DirectX::XMFLOAT4& point)
                    {
                        return point.x > point.w;
                    }) &&
                !allOutside(
                    [](const DirectX::XMFLOAT4& point)
                    {
                        return point.y < -point.w;
                    }) &&
                !allOutside(
                    [](const DirectX::XMFLOAT4& point)
                    {
                        return point.y > point.w;
                    }) &&
                !allOutside(
                    [](const DirectX::XMFLOAT4& point)
                    {
                        return point.z < 0.0F;
                    }) &&
                !allOutside(
                    [](const DirectX::XMFLOAT4& point)
                    {
                        return point.z > point.w;
                    });
        }

        [[nodiscard]]
        bool ReadEmbeddedTextureData(
            const engine::assets::TerrainAsset& terrain,
            const engine::assets::TerrainEmbeddedTexture& embedded,
            engine::assets::AssetData& output)
        {
            if (embedded.size == 0U ||
                embedded.size > MaximumTextureFileSize)
            {
                return false;
            }

            std::ifstream stream(
                terrain.sourcePath,
                std::ios::binary);

            if (!stream)
            {
                return false;
            }

            stream.seekg(
                static_cast<std::streamoff>(embedded.offset),
                std::ios::beg);

            if (!stream)
            {
                return false;
            }

            const engine::assets::AssetResult resizeResult =
                output.Resize(
                    static_cast<std::size_t>(embedded.size));

            if (engine::assets::Failed(resizeResult))
            {
                return false;
            }

            return static_cast<bool>(
                stream.read(
                    reinterpret_cast<char*>(output.GetData()),
                    static_cast<std::streamsize>(embedded.size)));
        }

        [[nodiscard]]
        std::uint8_t Expand5(const std::uint16_t value) noexcept
        {
            return static_cast<std::uint8_t>(
                (value << 3U) | (value >> 2U));
        }

        [[nodiscard]]
        std::uint8_t Expand6(const std::uint16_t value) noexcept
        {
            return static_cast<std::uint8_t>(
                (value << 2U) | (value >> 4U));
        }

        void DecodeColorBlock(
            const std::uint8_t* const block,
            const bool forceFourColorMode,
            std::array<std::array<std::uint8_t, 4U>, 4U>& colors,
            std::uint32_t& selectors) noexcept
        {
            const std::uint16_t color0 =
                static_cast<std::uint16_t>(
                    block[0] |
                    (static_cast<std::uint16_t>(block[1]) << 8U));

            const std::uint16_t color1 =
                static_cast<std::uint16_t>(
                    block[2] |
                    (static_cast<std::uint16_t>(block[3]) << 8U));

            colors[0] =
            {
                Expand5((color0 >> 11U) & 31U),
                Expand6((color0 >> 5U) & 63U),
                Expand5(color0 & 31U),
                255U
            };

            colors[1] =
            {
                Expand5((color1 >> 11U) & 31U),
                Expand6((color1 >> 5U) & 63U),
                Expand5(color1 & 31U),
                255U
            };

            const bool fourColorMode =
                forceFourColorMode || color0 > color1;

            for (std::size_t channel = 0U; channel < 3U; ++channel)
            {
                if (fourColorMode)
                {
                    colors[2][channel] =
                        static_cast<std::uint8_t>(
                            (2U * colors[0][channel] +
                             colors[1][channel]) /
                            3U);

                    colors[3][channel] =
                        static_cast<std::uint8_t>(
                            (colors[0][channel] +
                             2U * colors[1][channel]) /
                            3U);
                }
                else
                {
                    colors[2][channel] =
                        static_cast<std::uint8_t>(
                            (colors[0][channel] +
                             colors[1][channel]) /
                            2U);

                    colors[3][channel] = 0U;
                }
            }

            colors[2][3] = 255U;
            colors[3][3] = fourColorMode ? 255U : 0U;

            selectors =
                static_cast<std::uint32_t>(block[4]) |
                (static_cast<std::uint32_t>(block[5]) << 8U) |
                (static_cast<std::uint32_t>(block[6]) << 16U) |
                (static_cast<std::uint32_t>(block[7]) << 24U);
        }

        [[nodiscard]]
        bool DecodeEmbeddedMask(
            const engine::assets::TerrainAsset& terrain,
            const engine::assets::TerrainEmbeddedTexture& embedded,
            std::vector<std::byte>& rgba,
            std::uint32_t& width,
            std::uint32_t& height)
        {
            engine::assets::AssetData data;

            if (!ReadEmbeddedTextureData(terrain, embedded, data))
            {
                return false;
            }

            engine::assets::TextureAsset decoded;

            const engine::assets::AssetResult decodeResult =
                engine::assets::DdsTextureDecoder::Decode(
                    data,
                    decoded);

            if (engine::assets::Failed(decodeResult))
            {
                return false;
            }

            const engine::graphics::TextureDesc& description =
                decoded.GetDesc();

            width = description.width;
            height = description.height;

            if (width == 0U || height == 0U)
            {
                return false;
            }

            engine::graphics::TextureSubresourceData source{};

            if (engine::assets::Failed(
                    decoded.GetSubresourceData(0U, source)))
            {
                return false;
            }

            const std::uint64_t byteCount =
                static_cast<std::uint64_t>(width) *
                static_cast<std::uint64_t>(height) *
                4ULL;

            if (byteCount >
                static_cast<std::uint64_t>(
                    (std::numeric_limits<std::size_t>::max)()))
            {
                return false;
            }

            rgba.assign(
                static_cast<std::size_t>(byteCount),
                std::byte{0});

            if (description.format ==
                    engine::graphics::Format::R8G8B8A8UNorm ||
                description.format ==
                    engine::graphics::Format::R8G8B8A8UNormSrgb)
            {
                for (std::uint32_t y = 0U; y < height; ++y)
                {
                    std::memcpy(
                        rgba.data() +
                            static_cast<std::size_t>(y) *
                                width * 4U,
                        source.data +
                            static_cast<std::size_t>(y) *
                                source.rowPitch,
                        static_cast<std::size_t>(width) * 4U);
                }

                return true;
            }

            const bool isBc1 =
                description.format ==
                    engine::graphics::Format::BC1UNorm ||
                description.format ==
                    engine::graphics::Format::BC1UNormSrgb;

            const bool isBc2 =
                description.format ==
                    engine::graphics::Format::BC2UNorm ||
                description.format ==
                    engine::graphics::Format::BC2UNormSrgb;

            const bool isBc3 =
                description.format ==
                    engine::graphics::Format::BC3UNorm ||
                description.format ==
                    engine::graphics::Format::BC3UNormSrgb;

            if (!isBc1 && !isBc2 && !isBc3)
            {
                std::string message =
                    "Unsupported terrain splat-mask format: ";

                message +=
                    engine::graphics::ToString(
                        description.format);

                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.Terrain",
                    message);

                return false;
            }

            const std::size_t blockSize = isBc1 ? 8U : 16U;
            const std::size_t colorOffset = isBc1 ? 0U : 8U;
            const bool forceFourColorMode = !isBc1;

            for (std::uint32_t blockY = 0U;
                 blockY < (height + 3U) / 4U;
                 ++blockY)
            {
                const auto* const row =
                    reinterpret_cast<const std::uint8_t*>(
                        source.data +
                        static_cast<std::size_t>(blockY) *
                            source.rowPitch);

                for (std::uint32_t blockX = 0U;
                     blockX < (width + 3U) / 4U;
                     ++blockX)
                {
                    const std::uint8_t* const block =
                        row +
                        static_cast<std::size_t>(blockX) *
                            blockSize +
                        colorOffset;

                    std::array<std::array<std::uint8_t, 4U>, 4U>
                        colors{};

                    std::uint32_t selectors = 0U;

                    DecodeColorBlock(
                        block,
                        forceFourColorMode,
                        colors,
                        selectors);

                    for (std::uint32_t pixelY = 0U;
                         pixelY < 4U;
                         ++pixelY)
                    {
                        for (std::uint32_t pixelX = 0U;
                             pixelX < 4U;
                             ++pixelX)
                        {
                            const std::uint32_t x =
                                blockX * 4U + pixelX;

                            const std::uint32_t y =
                                blockY * 4U + pixelY;

                            if (x >= width || y >= height)
                            {
                                continue;
                            }

                            const std::uint32_t selector =
                                (selectors >>
                                 (2U * (pixelY * 4U + pixelX))) &
                                3U;

                            const auto& color = colors[selector];

                            std::memcpy(
                                rgba.data() +
                                    (static_cast<std::size_t>(y) *
                                         width +
                                     x) *
                                        4U,
                                color.data(),
                                4U);
                        }
                    }
                }
            }

            return true;
        }

        [[nodiscard]]
        bool CreateSolidTexture(
            engine::graphics::RenderDevice& device,
            const std::array<std::byte, 4U>& pixel,
            engine::graphics::TextureHandle& output)
        {
            engine::graphics::TextureDesc description{};
            description.width = 1U;
            description.height = 1U;
            description.format =
                engine::graphics::Format::R8G8B8A8UNorm;
            description.bindFlags =
                engine::graphics::TextureBindFlags::ShaderResource;

            const engine::graphics::TextureSubresourceData data
            {
                pixel.data(),
                pixel.size(),
                4U,
                4U
            };

            return engine::graphics::Succeeded(
                device.CreateTexture(
                    description,
                    &data,
                    1U,
                    output));
        }

        [[nodiscard]]
        bool CreateFallbackMask(
            engine::graphics::RenderDevice& device,
            engine::graphics::TextureHandle& output)
        {
            constexpr std::array<std::byte, 4U> pixel
            {
                std::byte{0},
                std::byte{0},
                std::byte{0},
                std::byte{0}
            };

            return CreateSolidTexture(device, pixel, output);
        }

        [[nodiscard]]
        bool CreateFallbackMaterial(
            engine::graphics::RenderDevice& device,
            engine::graphics::TextureHandle& output)
        {
            constexpr std::array<std::byte, 4U> pixel
            {
                std::byte{96},
                std::byte{96},
                std::byte{96},
                std::byte{255}
            };

            return CreateSolidTexture(device, pixel, output);
        }

        bool CreateFallbackNormalMaterial(
            engine::graphics::RenderDevice& device,
            engine::graphics::TextureHandle& output)
        {
            constexpr std::array<std::byte, 4U> pixel
            {
                std::byte{128},
                std::byte{128},
                std::byte{255},
                std::byte{255}
            };

            return CreateSolidTexture(device, pixel, output);
        }

        [[nodiscard]]
        bool CreateEditableMaskTexture(
            engine::graphics::RenderDevice& device,
            const std::vector<std::byte>& rgba,
            const std::uint32_t width,
            const std::uint32_t height,
            engine::graphics::TextureHandle& output)
        {
            const std::uint64_t expectedSize =
                static_cast<std::uint64_t>(width) *
                static_cast<std::uint64_t>(height) *
                4ULL;

            if (width == 0U ||
                height == 0U ||
                expectedSize != rgba.size())
            {
                return false;
            }

            engine::graphics::TextureDesc description{};
            description.width = width;
            description.height = height;
            description.format =
                engine::graphics::Format::R8G8B8A8UNorm;
            description.bindFlags =
                engine::graphics::TextureBindFlags::ShaderResource;

            const engine::graphics::TextureSubresourceData data
            {
                rgba.data(),
                rgba.size(),
                static_cast<std::size_t>(width) * 4U,
                rgba.size()
            };

            return engine::graphics::Succeeded(
                device.CreateTexture(
                    description,
                    &data,
                    1U,
                    output));
        }

        [[nodiscard]]
        bool CreateDdsFileTexture(
            engine::graphics::RenderDevice& device,
            const std::filesystem::path& path,
            const bool forceSrgb,
            engine::graphics::TextureHandle& output)
        {
            std::error_code filesystemError;

            const std::uintmax_t fileSize =
                std::filesystem::file_size(
                    path,
                    filesystemError);

            if (filesystemError ||
                fileSize == 0U ||
                fileSize > MaximumTextureFileSize)
            {
                return false;
            }

            engine::assets::AssetData data;

            const engine::assets::AssetResult resizeResult =
                data.Resize(
                    static_cast<std::size_t>(fileSize));

            if (engine::assets::Failed(resizeResult))
            {
                return false;
            }

            std::ifstream stream(
                path,
                std::ios::binary);

            if (!stream ||
                !stream.read(
                    reinterpret_cast<char*>(data.GetData()),
                    static_cast<std::streamsize>(fileSize)))
            {
                return false;
            }

            engine::assets::TextureAsset decoded;
            engine::assets::DdsTextureDecodeOptions options{};
            options.forceSrgb = forceSrgb;

            const engine::assets::AssetResult decodeResult =
                engine::assets::DdsTextureDecoder::Decode(
                    data,
                    options,
                    decoded);

            if (engine::assets::Failed(decodeResult))
            {
                return false;
            }

            std::vector<engine::graphics::TextureSubresourceData>
                initialData(decoded.GetSubresourceCount());

            for (std::size_t index = 0U;
                 index < initialData.size();
                 ++index)
            {
                if (engine::assets::Failed(
                        decoded.GetSubresourceData(
                            index,
                            initialData[index])))
                {
                    return false;
                }
            }

            return engine::graphics::Succeeded(
                device.CreateTexture(
                    decoded.GetDesc(),
                    initialData.data(),
                    initialData.size(),
                    output));
        }
    }

    class TerrainRenderer::Impl final
    {
    public:
        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice& device) noexcept
        {
            device_ = &device;

            if (terrainPath_.empty())
            {
                return true;
            }

            engine::assets::TerrainAsset terrain;

            const engine::assets::AssetResult loadResult =
                engine::assets::TerrainAsset::Load(
                    terrainPath_,
                    terrain);

            if (engine::assets::Failed(loadResult))
            {
                LogTerrainAssetFailure(
                    "Load terrain asset",
                    loadResult);

                return false;
            }

            if (!terrain.IsValid())
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.Terrain",
                    "Terrain asset failed validation before GPU creation.");

                return false;
            }

            const std::uint32_t sampleStep =
                CalculateTerrainSampleStep(
                    terrain.width,
                    terrain.height);

            const std::uint32_t gpuWidth =
                CalculateTerrainGpuVertexCount(
                    terrain.width,
                    sampleStep);

            const std::uint32_t gpuHeight =
                CalculateTerrainGpuVertexCount(
                    terrain.height,
                    sampleStep);

            if (gpuWidth < 2U || gpuHeight < 2U)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.Terrain",
                    "Terrain GPU grid is too small.");

                return false;
            }

            LogTerrainGeometryInfo(
                terrain.width,
                terrain.height,
                gpuWidth,
                gpuHeight,
                sampleStep);

            const auto getSourceX =
                [&terrain, sampleStep](
                    const std::uint32_t gpuX) noexcept
                {
                    return (std::min)(
                        gpuX * sampleStep,
                        terrain.width - 1U);
                };

            const auto getSourceZ =
                [&terrain, sampleStep](
                    const std::uint32_t gpuZ) noexcept
                {
                    return (std::min)(
                        gpuZ * sampleStep,
                        terrain.height - 1U);
                };

            const std::uint64_t baseVertexCount =
                static_cast<std::uint64_t>(gpuWidth) *
                static_cast<std::uint64_t>(gpuHeight);

            if (baseVertexCount >
                static_cast<std::uint64_t>(
                    (std::numeric_limits<std::size_t>::max)() /
                    sizeof(Vertex)))
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.Terrain",
                    "Terrain GPU vertex count overflow.");

                return false;
            }

            std::vector<Vertex> vertices(
                static_cast<std::size_t>(baseVertexCount));

            for (std::uint32_t gpuZ = 0U;
                 gpuZ < gpuHeight;
                 ++gpuZ)
            {
                const std::uint32_t sourceZ =
                    getSourceZ(gpuZ);

                for (std::uint32_t gpuX = 0U;
                     gpuX < gpuWidth;
                     ++gpuX)
                {
                    const std::uint32_t sourceX =
                        getSourceX(gpuX);

                    const std::uint32_t sourceLeft =
                        sourceX > sampleStep
                            ? sourceX - sampleStep
                            : 0U;

                    const std::uint32_t sourceRight =
                        (std::min)(
                            sourceX + sampleStep,
                            terrain.width - 1U);

                    const std::uint32_t sourceDown =
                        sourceZ > sampleStep
                            ? sourceZ - sampleStep
                            : 0U;

                    const std::uint32_t sourceUp =
                        (std::min)(
                            sourceZ + sampleStep,
                            terrain.height - 1U);

                    const float heightLeft =
                        terrain.GetHeight(
                            sourceLeft,
                            sourceZ);

                    const float heightRight =
                        terrain.GetHeight(
                            sourceRight,
                            sourceZ);

                    const float heightDown =
                        terrain.GetHeight(
                            sourceX,
                            sourceDown);

                    const float heightUp =
                        terrain.GetHeight(
                            sourceX,
                            sourceUp);

                    const float normalScaleX =
                        static_cast<float>(
                            (std::max)(
                                sourceRight - sourceLeft,
                                1U)) *
                        terrain.tileSize;

                    const float normalScaleZ =
                        static_cast<float>(
                            (std::max)(
                                sourceUp - sourceDown,
                                1U)) *
                        terrain.tileSize;

                    const DirectX::XMVECTOR normal =
                        DirectX::XMVector3Normalize(
                            DirectX::XMVectorSet(
                                (heightLeft - heightRight) *
                                    normalScaleZ,
                                normalScaleX * normalScaleZ,
                                (heightDown - heightUp) *
                                    normalScaleX,
                                0.0F));

                    Vertex& vertex =
                        vertices[
                            static_cast<std::size_t>(gpuZ) *
                                gpuWidth +
                            gpuX];

                    vertex.position =
                    {
                        static_cast<float>(sourceX) *
                            terrain.tileSize,
                        terrain.GetHeight(sourceX, sourceZ),
                        static_cast<float>(sourceZ) *
                            terrain.tileSize
                    };

                    DirectX::XMStoreFloat3(
                        &vertex.normal,
                        normal);

                    vertex.uv =
                    {
                        static_cast<float>(sourceX) /
                            static_cast<float>(terrain.width - 1U),
                        static_cast<float>(sourceZ) /
                            static_cast<float>(terrain.height - 1U)
                    };
                }
            }

            std::vector<std::uint32_t> indices;
            chunks_.clear();

            for (std::uint32_t chunkZ = 0U;
                 chunkZ < gpuHeight - 1U;
                 chunkZ += TerrainChunkCellCount)
            {
                for (std::uint32_t chunkX = 0U;
                     chunkX < gpuWidth - 1U;
                     chunkX += TerrainChunkCellCount)
                {
                    const std::uint32_t endX =
                        (std::min)(
                            chunkX + TerrainChunkCellCount,
                            gpuWidth - 1U);

                    const std::uint32_t endZ =
                        (std::min)(
                            chunkZ + TerrainChunkCellCount,
                            gpuHeight - 1U);

                    Chunk chunk{};

                    const Vertex& firstVertex =
                        vertices[
                            static_cast<std::size_t>(chunkZ) *
                                gpuWidth +
                            chunkX];

                    float minimumHeight =
                        firstVertex.position.y;

                    float maximumHeight =
                        firstVertex.position.y;

                    for (std::uint32_t z = chunkZ;
                         z <= endZ;
                         ++z)
                    {
                        for (std::uint32_t x = chunkX;
                             x <= endX;
                             ++x)
                        {
                            const float height =
                                vertices[
                                    static_cast<std::size_t>(z) *
                                        gpuWidth +
                                    x]
                                    .position.y;

                            minimumHeight =
                                (std::min)(minimumHeight, height);

                            maximumHeight =
                                (std::max)(maximumHeight, height);
                        }
                    }

                    const Vertex& minimumCorner =
                        vertices[
                            static_cast<std::size_t>(chunkZ) *
                                gpuWidth +
                            chunkX];

                    const Vertex& maximumCorner =
                        vertices[
                            static_cast<std::size_t>(endZ) *
                                gpuWidth +
                            endX];

                    chunk.minimum =
                    {
                        minimumCorner.position.x,
                        minimumHeight,
                        minimumCorner.position.z
                    };

                    chunk.maximum =
                    {
                        maximumCorner.position.x,
                        maximumHeight,
                        maximumCorner.position.z
                    };

                    chunk.center =
                    {
                        (chunk.minimum.x + chunk.maximum.x) * 0.5F,
                        (minimumHeight + maximumHeight) * 0.5F,
                        (chunk.minimum.z + chunk.maximum.z) * 0.5F
                    };

                    const float skirtDepth =
                        (std::max)(
                            20.0F,
                            terrain.heightScale * 0.025F);

                    std::array<
                        std::vector<std::uint32_t>,
                        4U> skirtBottom{};

                    const auto appendSkirtVertex =
                        [&vertices, skirtDepth](
                            const std::uint32_t sourceIndex)
                        {
                            Vertex skirt = vertices[sourceIndex];
                            skirt.position.y -= skirtDepth;

                            const std::uint32_t index =
                                static_cast<std::uint32_t>(
                                    vertices.size());

                            vertices.push_back(skirt);
                            return index;
                        };

                    for (std::uint32_t x = chunkX;
                         x <= endX;
                         ++x)
                    {
                        skirtBottom[0].push_back(
                            appendSkirtVertex(
                                chunkZ * gpuWidth + x));

                        skirtBottom[1].push_back(
                            appendSkirtVertex(
                                endZ * gpuWidth + x));
                    }

                    for (std::uint32_t z = chunkZ;
                         z <= endZ;
                         ++z)
                    {
                        skirtBottom[2].push_back(
                            appendSkirtVertex(
                                z * gpuWidth + chunkX));

                        skirtBottom[3].push_back(
                            appendSkirtVertex(
                                z * gpuWidth + endX));
                    }

                    for (std::uint32_t lod = 0U;
                         lod < 3U;
                         ++lod)
                    {
                        const std::uint32_t stride = 1U << lod;

                        chunk.first[lod] =
                            static_cast<std::uint32_t>(
                                indices.size());

                        for (std::uint32_t z = chunkZ;
                             z < endZ;
                             z += stride)
                        {
                            const std::uint32_t nextZ =
                                (std::min)(z + stride, endZ);

                            for (std::uint32_t x = chunkX;
                                 x < endX;
                                 x += stride)
                            {
                                const std::uint32_t nextX =
                                    (std::min)(x + stride, endX);

                                const std::uint32_t a =
                                    z * gpuWidth + x;

                                const std::uint32_t b =
                                    z * gpuWidth + nextX;

                                const std::uint32_t c =
                                    nextZ * gpuWidth + x;

                                const std::uint32_t diagonal =
                                    nextZ * gpuWidth + nextX;

                                indices.insert(
                                    indices.end(),
                                    {a, c, b, b, c, diagonal});
                            }
                        }

                        const auto addSkirt =
                            [&indices, stride](
                                const std::uint32_t length,
                                const auto& topIndex,
                                const std::vector<std::uint32_t>& bottom)
                            {
                                for (std::uint32_t edge = 0U;
                                     edge < length;
                                     edge += stride)
                                {
                                    const std::uint32_t next =
                                        (std::min)(
                                            edge + stride,
                                            length);

                                    const std::uint32_t topA =
                                        topIndex(edge);

                                    const std::uint32_t topB =
                                        topIndex(next);

                                    indices.insert(
                                        indices.end(),
                                        {
                                            topA,
                                            bottom[edge],
                                            topB,
                                            topB,
                                            bottom[edge],
                                            bottom[next]
                                        });
                                }
                            };

                        addSkirt(
                            endX - chunkX,
                            [chunkZ, chunkX, gpuWidth](
                                const std::uint32_t edge)
                            {
                                return
                                    chunkZ * gpuWidth +
                                    chunkX + edge;
                            },
                            skirtBottom[0]);

                        addSkirt(
                            endX - chunkX,
                            [endZ, chunkX, gpuWidth](
                                const std::uint32_t edge)
                            {
                                return
                                    endZ * gpuWidth +
                                    chunkX + edge;
                            },
                            skirtBottom[1]);

                        addSkirt(
                            endZ - chunkZ,
                            [chunkZ, chunkX, gpuWidth](
                                const std::uint32_t edge)
                            {
                                return
                                    (chunkZ + edge) * gpuWidth +
                                    chunkX;
                            },
                            skirtBottom[2]);

                        addSkirt(
                            endZ - chunkZ,
                            [chunkZ, endX, gpuWidth](
                                const std::uint32_t edge)
                            {
                                return
                                    (chunkZ + edge) * gpuWidth +
                                    endX;
                            },
                            skirtBottom[3]);

                        chunk.count[lod] =
                            static_cast<std::uint32_t>(
                                indices.size()) -
                            chunk.first[lod];
                    }

                    chunks_.push_back(chunk);
                }
            }

            indexCount_ =
                static_cast<std::uint32_t>(indices.size());

            engine::graphics::BufferDesc bufferDescription{};
            bufferDescription.byteSize =
                vertices.size() * sizeof(Vertex);
            bufferDescription.stride = sizeof(Vertex);
            bufferDescription.bindFlags =
                engine::graphics::BufferBindFlags::Vertex;

            const engine::graphics::BufferInitialData vertexInitialData
            {
                reinterpret_cast<const std::byte*>(vertices.data()),
                bufferDescription.byteSize
            };

            engine::graphics::GraphicsResult graphicsResult =
                device.CreateBuffer(
                    bufferDescription,
                    &vertexInitialData,
                    vertexBuffer_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain vertex buffer",
                    graphicsResult);

                return false;
            }

            bufferDescription = {};
            bufferDescription.byteSize =
                indices.size() * sizeof(std::uint32_t);
            bufferDescription.stride = sizeof(std::uint32_t);
            bufferDescription.bindFlags =
                engine::graphics::BufferBindFlags::Index;
            bufferDescription.indexFormat =
                engine::graphics::IndexFormat::UInt32;

            const engine::graphics::BufferInitialData indexInitialData
            {
                reinterpret_cast<const std::byte*>(indices.data()),
                bufferDescription.byteSize
            };

            graphicsResult =
                device.CreateBuffer(
                    bufferDescription,
                    &indexInitialData,
                    indexBuffer_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain index buffer",
                    graphicsResult);

                return false;
            }

            Microsoft::WRL::ComPtr<ID3DBlob> vertexBytecode;
            Microsoft::WRL::ComPtr<ID3DBlob> pixelBytecode;

            if (!CompileEditorShaderFile(
                    L"Terrain.hlsl",
                    "VSMain",
                    "vs_5_0",
                    "LTS.Editor.Terrain",
                    vertexBytecode) ||
                !CompileEditorShaderFile(
                    L"Terrain.hlsl",
                    "PSMain",
                    "ps_5_0",
                    "LTS.Editor.Terrain",
                    pixelBytecode))
            {
                return false;
            }

            engine::graphics::ShaderDesc shaderDescription{};
            shaderDescription.stage =
                engine::graphics::ShaderStage::Vertex;
            shaderDescription.bytecode =
            {
                vertexBytecode->GetBufferPointer(),
                vertexBytecode->GetBufferSize()
            };

            graphicsResult =
                device.CreateShader(
                    shaderDescription,
                    vertexShader_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain vertex shader",
                    graphicsResult);

                return false;
            }

            shaderDescription.stage =
                engine::graphics::ShaderStage::Pixel;
            shaderDescription.bytecode =
            {
                pixelBytecode->GetBufferPointer(),
                pixelBytecode->GetBufferSize()
            };

            graphicsResult =
                device.CreateShader(
                    shaderDescription,
                    pixelShader_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain pixel shader",
                    graphicsResult);

                return false;
            }

            const std::array<engine::graphics::VertexElementDesc, 3U>
                elements
                {{
                    {
                        "POSITION",
                        0U,
                        engine::graphics::Format::R32G32B32Float,
                        0U,
                        0U,
                        engine::graphics::VertexInputRate::PerVertex,
                        0U
                    },
                    {
                        "NORMAL",
                        0U,
                        engine::graphics::Format::R32G32B32Float,
                        0U,
                        12U,
                        engine::graphics::VertexInputRate::PerVertex,
                        0U
                    },
                    {
                        "TEXCOORD",
                        0U,
                        engine::graphics::Format::R32G32Float,
                        0U,
                        24U,
                        engine::graphics::VertexInputRate::PerVertex,
                        0U
                    }
                }};

            engine::graphics::InputLayoutDesc inputLayoutDescription{};
            inputLayoutDescription.vertexShader = vertexShader_;
            inputLayoutDescription.elements = elements.data();
            inputLayoutDescription.elementCount = elements.size();

            graphicsResult =
                device.CreateInputLayout(
                    inputLayoutDescription,
                    inputLayout_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain input layout",
                    graphicsResult);

                return false;
            }

            bufferDescription = {};
            bufferDescription.byteSize = sizeof(Constants);
            bufferDescription.bindFlags =
                engine::graphics::BufferBindFlags::Constant;

            graphicsResult =
                device.CreateBuffer(
                    bufferDescription,
                    nullptr,
                    constantBuffer_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain constant buffer",
                    graphicsResult);

                return false;
            }

            engine::graphics::GraphicsPipelineDesc pipelineDescription{};
            pipelineDescription.vertexShader = vertexShader_;
            pipelineDescription.pixelShader = pixelShader_;
            pipelineDescription.inputLayout = inputLayout_;
            pipelineDescription.topology =
                engine::graphics::PrimitiveTopology::TriangleList;
            pipelineDescription.rasterizer.cullMode =
                engine::graphics::CullMode::None;
            pipelineDescription.depthStencil.depthEnable = true;
            pipelineDescription.depthStencil.depthWriteEnable = true;
            pipelineDescription.depthStencil.depthFunction =
                engine::graphics::ComparisonFunction::GreaterEqual;

            graphicsResult =
                device.CreateGraphicsPipeline(
                    pipelineDescription,
                    pipeline_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain graphics pipeline",
                    graphicsResult);

                return false;
            }

            Microsoft::WRL::ComPtr<ID3DBlob> brushVertexBytecode;
            Microsoft::WRL::ComPtr<ID3DBlob> brushPixelBytecode;

            if (!CompileEditorShaderFile(
                    L"Grid.hlsl",
                    "VSMain",
                    "vs_5_0",
                    "LTS.Editor.TerrainBrush",
                    brushVertexBytecode) ||
                !CompileEditorShaderFile(
                    L"Grid.hlsl",
                    "PSMain",
                    "ps_5_0",
                    "LTS.Editor.TerrainBrush",
                    brushPixelBytecode))
            {
                return false;
            }

            shaderDescription = {};
            shaderDescription.stage =
                engine::graphics::ShaderStage::Vertex;
            shaderDescription.bytecode =
            {
                brushVertexBytecode->GetBufferPointer(),
                brushVertexBytecode->GetBufferSize()
            };

            graphicsResult =
                device.CreateShader(
                    shaderDescription,
                    brushVertexShader_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain brush vertex shader",
                    graphicsResult);

                return false;
            }

            shaderDescription.stage =
                engine::graphics::ShaderStage::Pixel;
            shaderDescription.bytecode =
            {
                brushPixelBytecode->GetBufferPointer(),
                brushPixelBytecode->GetBufferSize()
            };

            graphicsResult =
                device.CreateShader(
                    shaderDescription,
                    brushPixelShader_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain brush pixel shader",
                    graphicsResult);

                return false;
            }

            const std::array<engine::graphics::VertexElementDesc, 2U>
                brushElements
                {{
                    {
                        "POSITION",
                        0U,
                        engine::graphics::Format::R32G32B32Float,
                        0U,
                        0U,
                        engine::graphics::VertexInputRate::PerVertex,
                        0U
                    },
                    {
                        "COLOR",
                        0U,
                        engine::graphics::Format::R32G32B32A32Float,
                        0U,
                        12U,
                        engine::graphics::VertexInputRate::PerVertex,
                        0U
                    }
                }};

            engine::graphics::InputLayoutDesc brushLayoutDescription{};
            brushLayoutDescription.vertexShader = brushVertexShader_;
            brushLayoutDescription.elements = brushElements.data();
            brushLayoutDescription.elementCount = brushElements.size();

            graphicsResult =
                device.CreateInputLayout(
                    brushLayoutDescription,
                    brushInputLayout_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain brush input layout",
                    graphicsResult);

                return false;
            }

            bufferDescription = {};
            bufferDescription.byteSize = TerrainBrushVertexCount * sizeof(BrushVertex);
            bufferDescription.stride = sizeof(BrushVertex);
            bufferDescription.bindFlags =
                engine::graphics::BufferBindFlags::Vertex;

            graphicsResult =
                device.CreateBuffer(
                    bufferDescription,
                    nullptr,
                    brushVertexBuffer_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain brush vertex buffer",
                    graphicsResult);

                return false;
            }

            bufferDescription = {};
            bufferDescription.byteSize = sizeof(BrushConstants);
            bufferDescription.bindFlags =
                engine::graphics::BufferBindFlags::Constant;

            graphicsResult =
                device.CreateBuffer(
                    bufferDescription,
                    nullptr,
                    brushConstantBuffer_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain brush constant buffer",
                    graphicsResult);

                return false;
            }

            engine::graphics::GraphicsPipelineDesc brushPipelineDescription{};
            brushPipelineDescription.vertexShader = brushVertexShader_;
            brushPipelineDescription.pixelShader = brushPixelShader_;
            brushPipelineDescription.inputLayout = brushInputLayout_;
            brushPipelineDescription.topology =
                engine::graphics::PrimitiveTopology::LineList;
            brushPipelineDescription.rasterizer.cullMode =
                engine::graphics::CullMode::None;
            brushPipelineDescription.depthStencil.depthEnable = true;
            brushPipelineDescription.depthStencil.depthWriteEnable = false;
            brushPipelineDescription.depthStencil.depthFunction =
                engine::graphics::ComparisonFunction::GreaterEqual;

            graphicsResult =
                device.CreateGraphicsPipeline(
                    brushPipelineDescription,
                    brushPipeline_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain brush graphics pipeline",
                    graphicsResult);

                return false;
            }

            if (terrain.masks.size() > MaximumTerrainMaskCount)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.Terrain",
                    "Terrain contains more than six splat masks.");

                return false;
            }

            activeMaskCount_ = terrain.masks.size();
            maskWidth_ = terrain.splatWidth;
            maskHeight_ = terrain.splatHeight;

            bool maskDimensionsInitialized = false;

            for (std::size_t maskIndex = 0U;
                 maskIndex < activeMaskCount_;
                 ++maskIndex)
            {
                std::uint32_t decodedWidth = 0U;
                std::uint32_t decodedHeight = 0U;

                if (!DecodeEmbeddedMask(
                        terrain,
                        terrain.masks[maskIndex],
                        maskPixels_[maskIndex],
                        decodedWidth,
                        decodedHeight) ||
                    decodedWidth == 0U ||
                    decodedHeight == 0U)
                {
                    std::string message =
                        "Decode terrain splat mask ";
                    message += std::to_string(maskIndex);
                    message += " failed.";

                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Error,
                        "LTS.Editor.Terrain",
                        message);

                    return false;
                }

                if (!maskDimensionsInitialized)
                {
                    maskWidth_ = decodedWidth;
                    maskHeight_ = decodedHeight;
                    maskDimensionsInitialized = true;
                }
                else if (decodedWidth != maskWidth_ ||
                         decodedHeight != maskHeight_)
                {
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Error,
                        "LTS.Editor.Terrain",
                        "Terrain splat masks use different dimensions.");

                    return false;
                }
            }

            if (maskWidth_ == 0U || maskHeight_ == 0U)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.Terrain",
                    "Terrain splat-mask dimensions are invalid.");

                return false;
            }

            for (std::size_t maskIndex = activeMaskCount_;
                 maskIndex < maskPixels_.size();
                 ++maskIndex)
            {
                maskPixels_[maskIndex].clear();
            }

            LoadPaintData(terrain.sourcePath);

            for (std::size_t maskIndex = 0U;
                 maskIndex < masks_.size();
                 ++maskIndex)
            {
                const bool created =
                    maskIndex < activeMaskCount_
                        ? CreateEditableMaskTexture(
                            device,
                            maskPixels_[maskIndex],
                            maskWidth_,
                            maskHeight_,
                            masks_[maskIndex])
                        : CreateFallbackMask(
                            device,
                            masks_[maskIndex]);

                if (!created)
                {
                    std::string message =
                        "Create terrain mask texture ";
                    message += std::to_string(maskIndex);
                    message += " failed.";

                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Error,
                        "LTS.Editor.Terrain",
                        message);

                    return false;
                }
            }

            std::filesystem::path workspace =
                std::filesystem::current_path();

            if (workspace.filename() == L"game")
            {
                workspace = workspace.parent_path();
            }

            std::error_code workspaceError;

            while (!std::filesystem::is_directory(
                       workspace / L"bin" / L"Data" / L"TerrainData" / L"Materials",
                       workspaceError))
            {
                const std::filesystem::path parent = workspace.parent_path();

                if (parent.empty() || parent == workspace)
                {
                    break;
                }

                workspace = parent;
                workspaceError.clear();
            }

            workspaceRoot_ = workspace;

            const auto loadLayerTexture =
            [&device, &workspace](
                const std::string& logicalPath,
                const bool forceSrgb,
                engine::graphics::TextureHandle& output)
            {
                if (logicalPath.empty())
                {
                    return false;
                }

                const std::filesystem::path path =
                    std::filesystem::u8path(logicalPath);

                if (CreateDdsFileTexture(
                        device,
                        workspace / L"game" / path,
                        forceSrgb,
                        output))
                {
                    return true;
                }

                return CreateDdsFileTexture(
                    device,
                    workspace / L"bin" / path,
                    forceSrgb,
                    output);
            };

            for (std::size_t layerIndex = 0U;
                 layerIndex < materials_.size();
                 ++layerIndex)
            {
                bool diffuseCreated = false;
                bool normalCreated = false;

                if (layerIndex < terrain.layers.size())
                {
                    materialPaths_[layerIndex] =
                        terrain.layers[layerIndex].diffusePath;

                    normalMaterialPaths_[layerIndex] =
                        terrain.layers[layerIndex].normalPath;

                    diffuseCreated = loadLayerTexture(
                        materialPaths_[layerIndex],
                        true,
                        materials_[layerIndex]);

                    normalCreated = loadLayerTexture(
                        normalMaterialPaths_[layerIndex],
                        false,
                        normalMaterials_[layerIndex]);
                }

                if (!diffuseCreated &&
                    !CreateFallbackMaterial(device, materials_[layerIndex]))
                {
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Error,
                        "LTS.Editor.Terrain",
                        "Failed to create terrain fallback diffuse texture.");

                    return false;
                }

                if (!normalCreated &&
                    !CreateFallbackNormalMaterial(device, normalMaterials_[layerIndex]))
                {
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Error,
                        "LTS.Editor.Terrain",
                        "Failed to create terrain fallback normal texture.");

                    return false;
                }
            }

            engine::graphics::SamplerDesc materialSamplerDescription{};

            materialSamplerDescription.filter =
                engine::graphics::TextureFilter::Anisotropic;

            materialSamplerDescription.addressU =
                engine::graphics::TextureAddressMode::Wrap;

            materialSamplerDescription.addressV =
                engine::graphics::TextureAddressMode::Wrap;

            materialSamplerDescription.addressW =
                engine::graphics::TextureAddressMode::Wrap;

            materialSamplerDescription.mipLodBias = 0.0F;
            materialSamplerDescription.maximumAnisotropy = 16U;

            materialSamplerDescription.debugName =
                "EditorTerrain.MaterialSampler";

            graphicsResult =
                device.CreateSampler(
                    materialSamplerDescription,
                    materialSampler_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain material sampler",
                    graphicsResult);

                return false;
            }


            engine::graphics::SamplerDesc maskSamplerDescription{};

            maskSamplerDescription.filter =
                engine::graphics::TextureFilter::Linear;

            maskSamplerDescription.addressU =
                engine::graphics::TextureAddressMode::Clamp;

            maskSamplerDescription.addressV =
                engine::graphics::TextureAddressMode::Clamp;

            maskSamplerDescription.addressW =
                engine::graphics::TextureAddressMode::Clamp;

            maskSamplerDescription.debugName =
                "EditorTerrain.MaskSampler";

            graphicsResult =
                device.CreateSampler(
                    maskSamplerDescription,
                    maskSampler_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogTerrainGraphicsFailure(
                    "Create terrain mask sampler",
                    graphicsResult);

                return false;
            }

            terrainVertices_ =
                std::move(vertices);

            terrainSampleStep_ =
                sampleStep;

            terrainGpuWidth_ =
                gpuWidth;

            terrainGpuHeight_ =
                gpuHeight;

            activeHeightBefore_.clear();
            heightUndo_.clear();
            heightRedo_.clear();
            heightStrokeActive_ = false;

            terrainAsset_ = std::move(terrain);
            loaded_ = true;

            return true;
        }

        void Shutdown(
            engine::graphics::RenderDevice& device) noexcept
        {
            for (engine::graphics::TextureHandle& texture : materials_)
            {
                if (texture.IsValid())
                {
                    static_cast<void>(
                        device.DestroyTexture(texture));
                }

                texture = {};
            }

            for (engine::graphics::TextureHandle& texture : normalMaterials_)
            {
                if (texture.IsValid())
                {
                    static_cast<void>(device.DestroyTexture(texture));
                }

                texture = {};
            }

            for (engine::graphics::TextureHandle& mask : masks_)
            {
                if (mask.IsValid())
                {
                    static_cast<void>(
                        device.DestroyTexture(mask));
                }

                mask = {};
            }

            if (materialSampler_.IsValid())
            {
                static_cast<void>(
                    device.DestroySampler(
                        materialSampler_));
            }

            if (maskSampler_.IsValid())
            {
                static_cast<void>(
                    device.DestroySampler(
                        maskSampler_));
            }

            if (brushPipeline_.IsValid())
            {
                static_cast<void>(
                    device.DestroyGraphicsPipeline(
                        brushPipeline_));
            }

            if (brushInputLayout_.IsValid())
            {
                static_cast<void>(
                    device.DestroyInputLayout(
                        brushInputLayout_));
            }

            if (brushVertexShader_.IsValid())
            {
                static_cast<void>(
                    device.DestroyShader(
                        brushVertexShader_));
            }

            if (brushPixelShader_.IsValid())
            {
                static_cast<void>(
                    device.DestroyShader(
                        brushPixelShader_));
            }

            if (brushConstantBuffer_.IsValid())
            {
                static_cast<void>(
                    device.DestroyBuffer(
                        brushConstantBuffer_));
            }

            if (brushVertexBuffer_.IsValid())
            {
                static_cast<void>(
                    device.DestroyBuffer(
                        brushVertexBuffer_));
            }

            if (pipeline_.IsValid())
            {
                static_cast<void>(
                    device.DestroyGraphicsPipeline(
                        pipeline_));
            }

            if (inputLayout_.IsValid())
            {
                static_cast<void>(
                    device.DestroyInputLayout(
                        inputLayout_));
            }

            if (vertexShader_.IsValid())
            {
                static_cast<void>(
                    device.DestroyShader(
                        vertexShader_));
            }

            if (pixelShader_.IsValid())
            {
                static_cast<void>(
                    device.DestroyShader(
                        pixelShader_));
            }

            if (constantBuffer_.IsValid())
            {
                static_cast<void>(
                    device.DestroyBuffer(
                        constantBuffer_));
            }

            if (indexBuffer_.IsValid())
            {
                static_cast<void>(
                    device.DestroyBuffer(
                        indexBuffer_));
            }

            if (vertexBuffer_.IsValid())
            {
                static_cast<void>(
                    device.DestroyBuffer(
                        vertexBuffer_));
            }

            materialSampler_ = {};
            maskSampler_ = {};
            brushPipeline_ = {};
            brushInputLayout_ = {};
            brushVertexShader_ = {};
            brushPixelShader_ = {};
            brushConstantBuffer_ = {};
            brushVertexBuffer_ = {};
            pipeline_ = {};
            inputLayout_ = {};
            vertexShader_ = {};
            pixelShader_ = {};
            constantBuffer_ = {};
            indexBuffer_ = {};
            vertexBuffer_ = {};

            terrainAsset_ = {};
            chunks_.clear();

            terrainVertices_.clear();

            activeHeightBefore_.clear();
            heightUndo_.clear();
            heightRedo_.clear();

            terrainSampleStep_ = 1U;
            terrainGpuWidth_ = 0U;
            terrainGpuHeight_ = 0U;
            heightStrokeActive_ = false;

            for (std::vector<std::byte>& pixels : maskPixels_)
            {
                pixels.clear();
            }

            for (std::string& path : materialPaths_)
            {
                path.clear();
            }

            for (std::string& path : normalMaterialPaths_)
            {
                path.clear();
            }

            activePaintBefore_.clear();
            paintUndo_.clear();
            paintRedo_.clear();
            paintPath_.clear();

            activeMaskCount_ = 0U;
            maskWidth_ = 0U;
            maskHeight_ = 0U;
            indexCount_ = 0U;
            paintStrokeActive_ = false;
            loaded_ = false;
            device_ = nullptr;
        }

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const SceneDocument& document,
            const DirectX::XMFLOAT4X4& viewProjection,
            const DirectX::XMFLOAT3& cameraPosition) noexcept
        {
            if (!loaded_)
            {
                return engine::graphics::GraphicsResult::Success;
            }

            const EditorSceneEntity* actor = nullptr;

            for (const EditorSceneEntity& entity : document.GetEntities())
            {
                if (entity.terrain.has_value() && entity.terrain->visible)
                {
                    actor = &entity;
                    break;
                }
            }

            if (actor == nullptr)
            {
                return engine::graphics::GraphicsResult::Success;
            }

            const bool hasSceneOverrides = !actor->terrain->layers.empty();

            const std::size_t sourceLayerCount = hasSceneOverrides
                ? actor->terrain->layers.size()
                : terrainAsset_.layers.size();

            if (sourceLayerCount == 0U ||
                sourceLayerCount > MaximumTerrainLayerCount ||
                !SetMaterialLayerCount(sourceLayerCount))
            {
                return engine::graphics::GraphicsResult::InvalidArgument;
            }

            const std::size_t layerCount =
                (std::min)(sourceLayerCount, MaximumTerrainLayerCount);

            if (device_ != nullptr)
            {
                const auto reloadTexture =
                    [this](
                        const std::string& path,
                        const bool forceSrgb,
                        const bool normalMap,
                        engine::graphics::TextureHandle& texture,
                        std::string& cachedPath)
                {
                    if (path == cachedPath)
                    {
                        return true;
                    }

                    if (texture.IsValid())
                    {
                        static_cast<void>(device_->DestroyTexture(texture));
                    }

                    texture = {};

                    const std::filesystem::path logicalPath =
                        std::filesystem::u8path(path);

                    bool created = false;

                    if (!path.empty())
                    {
                        created = CreateDdsFileTexture(
                            *device_,
                            workspaceRoot_ / L"game" / logicalPath,
                            forceSrgb,
                            texture);

                        if (!created)
                        {
                            created = CreateDdsFileTexture(
                                *device_,
                                workspaceRoot_ / L"bin" / logicalPath,
                                forceSrgb,
                                texture);
                        }
                    }

                    if (!created)
                    {
                        created = normalMap
                            ? CreateFallbackNormalMaterial(*device_, texture)
                            : CreateFallbackMaterial(*device_, texture);
                    }

                    if (created)
                    {
                        cachedPath = path;
                    }

                    return created;
                };

                for (std::size_t layerIndex = 0U;
                     layerIndex < layerCount;
                     ++layerIndex)
                {
                    const std::string& diffusePath =
                        hasSceneOverrides
                            ? actor->terrain->layers[layerIndex].diffusePath
                            : terrainAsset_.layers[layerIndex].diffusePath;

                    const std::string& normalPath =
                        hasSceneOverrides
                            ? actor->terrain->layers[layerIndex].normalPath
                            : terrainAsset_.layers[layerIndex].normalPath;

                    if (!reloadTexture(
                            diffusePath,
                            true,
                            false,
                            materials_[layerIndex],
                            materialPaths_[layerIndex]) ||
                        !reloadTexture(
                            normalPath,
                            false,
                            true,
                            normalMaterials_[layerIndex],
                            normalMaterialPaths_[layerIndex]))
                    {
                        return engine::graphics::GraphicsResult::BackendFailure;
                    }
                }
            }

            Constants constants{};

            const DirectX::XMMATRIX worldMatrix =
                DirectX::XMMatrixScaling(
                    actor->transform.scale[0],
                    actor->transform.scale[1],
                    actor->transform.scale[2]) *
                DirectX::XMMatrixRotationRollPitchYaw(
                    DirectX::XMConvertToRadians(
                        actor->transform.rotationDegrees[0]),
                    DirectX::XMConvertToRadians(
                        actor->transform.rotationDegrees[1]),
                    DirectX::XMConvertToRadians(
                        actor->transform.rotationDegrees[2])) *
                DirectX::XMMatrixTranslation(
                    actor->transform.position[0],
                    actor->transform.position[1],
                    actor->transform.position[2]);

            DirectX::XMStoreFloat4x4(
                &constants.world,
                worldMatrix);

            constants.viewProjection = viewProjection;

            for (std::size_t layerIndex = 0U;
                 layerIndex < constants.placement.size();
                 ++layerIndex)
            {
                constants.placement[layerIndex] =
                {
                    1.0F,
                    1.0F,
                    0.0F,
                    0.0F
                };

                constants.layerParameters[layerIndex] =
                {
                    0.0F,
                    0.0F,
                    0.0F,
                    0.0F
                };
            }

            for (std::size_t layerIndex = 0U;
                 layerIndex < layerCount;
                 ++layerIndex)
            {
                if (hasSceneOverrides)
                {
                    const auto& layer = actor->terrain->layers[layerIndex];
            
                    constants.placement[layerIndex] =
                    {
                        layer.scaleU,
                        layer.scaleV,
                        layer.offsetU,
                        layer.offsetV
                    };
            
                    constants.layerParameters[layerIndex].x =
                        layer.visible ? 1.0F : 0.0F;
                }
                else
                {
                    const auto& layer = terrainAsset_.layers[layerIndex];
            
                    constants.placement[layerIndex] =
                    {
                        layer.scaleU,
                        layer.scaleV,
                        0.0F,
                        0.0F
                    };
            
                    constants.layerParameters[layerIndex].x = 1.0F;
                }
            }

            const float terrainWidth =
                static_cast<float>(terrainAsset_.width - 1U) *
                terrainAsset_.tileSize;

            const float terrainDepth =
                static_cast<float>(terrainAsset_.height - 1U) *
                terrainAsset_.tileSize;

            constants.terrainInfo =
            {
                terrainWidth,
                terrainDepth,
                static_cast<float>(layerCount),
                0.0F
            };

            const ResolvedDirectionalLight lighting =
                ResolveDirectionalLight(document);

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

            engine::graphics::GraphicsResult result =
                context.UpdateBuffer(
                    constantBuffer_,
                    &constants,
                    sizeof(constants));

            if (engine::graphics::Failed(result))
            {
                return result;
            }

            result = context.SetGraphicsPipeline(pipeline_);

            if (engine::graphics::Failed(result))
            {
                return result;
            }

            const engine::graphics::VertexBufferBinding vertexBinding
            {
                vertexBuffer_,
                sizeof(Vertex),
                0U
            };

            const engine::graphics::IndexBufferBinding indexBinding
            {
                indexBuffer_,
                0U
            };

            result =
                context.SetVertexBuffers(
                    0U,
                    &vertexBinding,
                    1U);

            if (!engine::graphics::Failed(result))
            {
                result = context.SetIndexBuffer(indexBinding);
            }

            if (!engine::graphics::Failed(result))
            {
                result =
                    context.SetConstantBuffers(
                        engine::graphics::ShaderStage::Vertex,
                        0U,
                        &constantBuffer_,
                        1U);
            }

            if (!engine::graphics::Failed(result))
            {
                result =
                    context.SetConstantBuffers(
                        engine::graphics::ShaderStage::Pixel,
                        0U,
                        &constantBuffer_,
                        1U);
            }

            if (!engine::graphics::Failed(result))
            {
                result =
                    context.SetShaderResources(
                        engine::graphics::ShaderStage::Pixel,
                        0U,
                        masks_.data(),
                        masks_.size());
            }

            if (!engine::graphics::Failed(result))
            {
                result =
                    context.SetShaderResources(
                        engine::graphics::ShaderStage::Pixel,
                        6U,
                        materials_.data(),
                        materials_.size());
            }

            if (!engine::graphics::Failed(result))
            {
                result = context.SetShaderResources(
                    engine::graphics::ShaderStage::Pixel,
                    24U,
                    normalMaterials_.data(),
                    normalMaterials_.size());
            }

            if (!engine::graphics::Failed(result))
            {
                const std::array<engine::graphics::SamplerHandle, 2U> terrainSamplers
                {
                    materialSampler_,
                    maskSampler_
                };

                result =
                    context.SetSamplers(
                        engine::graphics::ShaderStage::Pixel,
                        0U,
                        terrainSamplers.data(),
                        terrainSamplers.size());
            }

            if (!engine::graphics::Failed(result))
            {
                const DirectX::XMMATRIX viewProjectionMatrix =
                    DirectX::XMLoadFloat4x4(&viewProjection);

                const DirectX::XMMATRIX worldViewProjection =
                    worldMatrix * viewProjectionMatrix;

                for (const Chunk& chunk : chunks_)
                {
                    if (!IsChunkVisible(
                            chunk,
                            worldViewProjection))
                    {
                        continue;
                    }

                    const DirectX::XMVECTOR localCenter =
                        DirectX::XMLoadFloat3(&chunk.center);

                    DirectX::XMFLOAT3 worldCenter{};

                    DirectX::XMStoreFloat3(
                        &worldCenter,
                        DirectX::XMVector3TransformCoord(
                            localCenter,
                            worldMatrix));

                    const float deltaX =
                        worldCenter.x - cameraPosition.x;
                    const float deltaY =
                        worldCenter.y - cameraPosition.y;
                    const float deltaZ =
                        worldCenter.z - cameraPosition.z;

                    const float distanceSquared =
                        deltaX * deltaX +
                        deltaY * deltaY +
                        deltaZ * deltaZ;

                    const std::size_t lod =
                        distanceSquared < 1200.0F * 1200.0F
                            ? 0U
                            : distanceSquared < 2800.0F * 2800.0F
                                ? 1U
                                : 2U;

                    result =
                        context.DrawIndexed(
                            chunk.count[lod],
                            chunk.first[lod],
                            0);

                    if (engine::graphics::Failed(result))
                    {
                        break;
                    }
                }
            }

            static_cast<void>(context.UnbindShaderResources(engine::graphics::ShaderStage::Pixel, 0U, 42U));
            static_cast<void>(context.UnbindSamplers(engine::graphics::ShaderStage::Pixel, 0U, 2U));
            
            context.UnbindIndexBuffer();
            context.UnbindGraphicsPipeline();

            return result;
        }

        [[nodiscard]]
        engine::graphics::GraphicsResult RenderBrush(
            engine::graphics::CommandContext& context,
            const SceneDocument& document,
            const DirectX::XMFLOAT4X4& viewProjection,
            const float worldX,
            const float worldZ,
            const float radius,
            const bool erase) noexcept
        {
            if (!loaded_ ||
                !brushPipeline_.IsValid() ||
                radius <= 0.0F)
            {
                return engine::graphics::GraphicsResult::Success;
            }

            std::array<BrushVertex, TerrainBrushVertexCount> vertices{};

            const DirectX::XMFLOAT4 color =
                erase
                    ? DirectX::XMFLOAT4
                        {0.95F, 0.18F, 0.12F, 1.0F}
                    : DirectX::XMFLOAT4
                        {1.0F, 0.62F, 0.08F, 1.0F};

            std::uint32_t vertexCount = 0U;

            for (std::uint32_t segment = 0U;
                 segment < TerrainBrushSegmentCount;
                 ++segment)
            {
                const float angle0 =
                    DirectX::XM_2PI *
                    static_cast<float>(segment) /
                    static_cast<float>(TerrainBrushSegmentCount);

                const float angle1 =
                    DirectX::XM_2PI *
                    static_cast<float>(segment + 1U) /
                    static_cast<float>(TerrainBrushSegmentCount);

                const float x0 =
                    worldX + std::cos(angle0) * radius;
                const float z0 =
                    worldZ + std::sin(angle0) * radius;
                const float x1 =
                    worldX + std::cos(angle1) * radius;
                const float z1 =
                    worldZ + std::sin(angle1) * radius;

                float y0 = 0.0F;
                float y1 = 0.0F;

                if (!TryGetSurfaceHeight(
                        document,
                        x0,
                        z0,
                        y0) ||
                    !TryGetSurfaceHeight(
                        document,
                        x1,
                        z1,
                        y1))
                {
                    continue;
                }

                vertices[vertexCount++] =
                {
                    {x0, y0 + 0.35F, z0},
                    color
                };

                vertices[vertexCount++] =
                {
                    {x1, y1 + 0.35F, z1},
                    color
                };
            }

            if (vertexCount == 0U)
            {
                return engine::graphics::GraphicsResult::Success;
            }

            const BrushConstants constants{viewProjection};

            engine::graphics::GraphicsResult result =
                context.UpdateBuffer(
                brushVertexBuffer_,
                vertices.data(),
                sizeof(vertices));

            if (engine::graphics::Failed(result))
            {
                return result;
            }

            result =
                context.UpdateBuffer(
                    brushConstantBuffer_,
                    &constants,
                    sizeof(constants));

            if (engine::graphics::Failed(result))
            {
                return result;
            }

            result =
                context.SetGraphicsPipeline(
                    brushPipeline_);

            const engine::graphics::VertexBufferBinding binding
            {
                brushVertexBuffer_,
                sizeof(BrushVertex),
                0U
            };

            if (!engine::graphics::Failed(result))
            {
                result =
                    context.SetVertexBuffers(
                        0U,
                        &binding,
                        1U);
            }

            if (!engine::graphics::Failed(result))
            {
                result =
                    context.SetConstantBuffers(
                        engine::graphics::ShaderStage::Vertex,
                        0U,
                        &brushConstantBuffer_,
                        1U);
            }

            if (!engine::graphics::Failed(result))
            {
                result = context.Draw(vertexCount, 0U);
            }

            static_cast<void>(
                context.UnbindConstantBuffers(
                    engine::graphics::ShaderStage::Vertex,
                    0U,
                    1U));

            context.UnbindGraphicsPipeline();

            return result;
        }

        [[nodiscard]]
        bool SetMaterialLayerCount(
            const std::size_t layerCount) noexcept
        {
            if (!loaded_ ||
                device_ == nullptr ||
                layerCount == 0U ||
                layerCount > MaximumTerrainLayerCount ||
                maskWidth_ == 0U ||
                maskHeight_ == 0U)
            {
                return false;
            }

            const std::size_t requiredMaskCount =
                layerCount <= 1U
                    ? 0U
                    : (layerCount - 1U + 2U) / 3U;

            if (requiredMaskCount > MaximumTerrainMaskCount)
            {
                return false;
            }

            if (requiredMaskCount == activeMaskCount_)
            {
                return true;
            }

            const std::uint64_t bytesPerMask64 =
                static_cast<std::uint64_t>(maskWidth_) *
                static_cast<std::uint64_t>(maskHeight_) *
                4ULL;

            if (bytesPerMask64 == 0U ||
                bytesPerMask64 >
                    static_cast<std::uint64_t>(
                        (std::numeric_limits<std::size_t>::max)()))
            {
                return false;
            }

            const std::size_t bytesPerMask =
                static_cast<std::size_t>(bytesPerMask64);

            const std::size_t previousMaskCount = activeMaskCount_;

            for (std::size_t maskIndex = 0U;
                 maskIndex < requiredMaskCount;
                 ++maskIndex)
            {
                if (maskPixels_[maskIndex].size() != bytesPerMask)
                {
                    maskPixels_[maskIndex].assign(
                        bytesPerMask,
                        std::byte{0});
                }
            }

            activeMaskCount_ = requiredMaskCount;

            /*
             * При загрузке уровня Terrain Asset может содержать только Base Layer,
             * а дополнительные слои хранятся в Level Scene.
             *
             * После вычисления нужного количества масок повторно читаем .paint.
             */
            if (requiredMaskCount > previousMaskCount)
            {
                LoadPaintData(terrainAsset_.sourcePath);
            }

            paintStrokeActive_ = false;
            activePaintBefore_.clear();
            paintUndo_.clear();
            paintRedo_.clear();

            RefreshMaskTextures();

            if (activeMaskCount_ == 0U)
            {
                std::error_code error;
                std::filesystem::remove(paintPath_, error);
            }
            else
            {
                SavePaintData();
            }

            return true;
        }

        [[nodiscard]]
        bool RemoveMaterialLayer(
            const std::size_t layerIndex,
            const std::size_t oldLayerCount) noexcept
        {
            if (layerIndex == 0U ||
                oldLayerCount <= 1U ||
                oldLayerCount > MaximumTerrainLayerCount ||
                layerIndex >= oldLayerCount)
            {
                return false;
            }

            if (!SetMaterialLayerCount(oldLayerCount))
            {
                return false;
            }

            const std::size_t removedPaintIndex = layerIndex - 1U;
            const std::size_t paintedLayerCount = oldLayerCount - 1U;

            const std::uint64_t pixelCount64 =
                static_cast<std::uint64_t>(maskWidth_) *
                static_cast<std::uint64_t>(maskHeight_);

            if (pixelCount64 >
                static_cast<std::uint64_t>(
                    (std::numeric_limits<std::uint32_t>::max)()))
            {
                return false;
            }

            const std::uint32_t pixelCount =
                static_cast<std::uint32_t>(pixelCount64);

            /*
             * Удаляем канал выбранного слоя и сдвигаем последующие слои.
             */
            for (std::uint32_t pixel = 0U; pixel < pixelCount; ++pixel)
            {
                auto weights = ReadWeights(pixel);

                for (std::size_t index = removedPaintIndex;
                     index + 1U < paintedLayerCount;
                     ++index)
                {
                    weights[index] = weights[index + 1U];
                }

                weights[paintedLayerCount - 1U] = 0U;
                WriteWeights(pixel, weights);
            }

            const std::size_t newLayerCount = oldLayerCount - 1U;
            const std::size_t newMaskCount =
                newLayerCount <= 1U
                    ? 0U
                    : (newLayerCount - 1U + 2U) / 3U;

            activeMaskCount_ = newMaskCount;

            paintStrokeActive_ = false;
            activePaintBefore_.clear();
            paintUndo_.clear();
            paintRedo_.clear();

            RefreshMaskTextures();

            if (activeMaskCount_ == 0U)
            {
                std::error_code error;
                std::filesystem::remove(paintPath_, error);
            }
            else
            {
                SavePaintData();
            }

            return true;
        }

        [[nodiscard]]
        bool LoadTerrain(
            engine::graphics::RenderDevice& device,
            const std::filesystem::path& path) noexcept
        {
            Shutdown(device);
            terrainPath_ = path;

            const bool loaded = Initialize(device);

            engine::core::GetLogger().Write(
                loaded
                    ? engine::core::LogLevel::Information
                    : engine::core::LogLevel::Error,
                "LTS.Editor.Terrain",
                loaded
                    ? path.empty()
                        ? "Terrain cleared."
                        : "Terrain GPU resources loaded."
                    : "Failed to load terrain GPU resources.");

            return loaded;
        }

        [[nodiscard]]
        bool HasTerrain() const noexcept
        {
            return loaded_;
        }

        [[nodiscard]]
        bool TryGetSurfaceHeight(
            const SceneDocument& document,
            const float worldX,
            const float worldZ,
            float& worldHeight) const noexcept
        {
            if (!loaded_ || !terrainAsset_.IsValid())
            {
                return false;
            }

            const EditorSceneEntity* actor = nullptr;

            for (const EditorSceneEntity& entity : document.GetEntities())
            {
                if (entity.terrain.has_value() &&
                    entity.terrain->visible)
                {
                    actor = &entity;
                    break;
                }
            }

            if (actor == nullptr ||
                std::abs(actor->transform.scale[0]) < 0.00001F ||
                std::abs(actor->transform.scale[2]) < 0.00001F)
            {
                return false;
            }

            // Terrain editing currently keeps the heightfield aligned
            // to world X/Z. Scale and translation are respected.
            const float localX =
                (worldX - actor->transform.position[0]) /
                actor->transform.scale[0];

            const float localZ =
                (worldZ - actor->transform.position[2]) /
                actor->transform.scale[2];

            const float sampleX =
                localX / terrainAsset_.tileSize;

            const float sampleZ =
                localZ / terrainAsset_.tileSize;

            if (sampleX < 0.0F ||
                sampleZ < 0.0F ||
                sampleX >
                    static_cast<float>(terrainAsset_.width - 1U) ||
                sampleZ >
                    static_cast<float>(terrainAsset_.height - 1U))
            {
                return false;
            }

            const std::uint32_t x0 =
                static_cast<std::uint32_t>(
                    std::floor(sampleX));

            const std::uint32_t z0 =
                static_cast<std::uint32_t>(
                    std::floor(sampleZ));

            const std::uint32_t x1 =
                (std::min)(
                    x0 + 1U,
                    terrainAsset_.width - 1U);

            const std::uint32_t z1 =
                (std::min)(
                    z0 + 1U,
                    terrainAsset_.height - 1U);

            const float blendX =
                sampleX - static_cast<float>(x0);

            const float blendZ =
                sampleZ - static_cast<float>(z0);

            const float height0 =
                terrainAsset_.GetHeight(x0, z0) *
                    (1.0F - blendX) +
                terrainAsset_.GetHeight(x1, z0) *
                    blendX;

            const float height1 =
                terrainAsset_.GetHeight(x0, z1) *
                    (1.0F - blendX) +
                terrainAsset_.GetHeight(x1, z1) *
                    blendX;

            const float localHeight =
                height0 * (1.0F - blendZ) +
                height1 * blendZ;

            worldHeight =
                actor->transform.position[1] +
                localHeight * actor->transform.scale[1];

            return true;
        }

        [[nodiscard]]
        bool CanSculpt() const noexcept
        {
            if (
                !loaded_ ||
                !terrainAsset_.IsValid() ||
                terrainPath_.empty())
            {
                return false;
            }

            std::error_code error;

            std::filesystem::path normalized =
                std::filesystem::weakly_canonical(
                    terrainPath_,
                    error);

            if (error)
            {
                error.clear();

                normalized =
                    std::filesystem::absolute(
                        terrainPath_,
                        error);
            }

            if (error)
            {
                return false;
            }

            std::string path =
                normalized.
                    lexically_normal().
                    generic_u8string();

            std::transform(
                path.begin(),
                path.end(),
                path.begin(),
                [](const unsigned char character)
                {
                    return static_cast<char>(
                        std::tolower(character));
                });

            return
                path.find("/bin/levels/") !=
                std::string::npos;
        }

        [[nodiscard]]
        bool RefreshHeightGeometry() noexcept
        {
            if (
                device_ == nullptr ||
                !vertexBuffer_.IsValid() ||
                !terrainAsset_.IsValid() ||
                terrainGpuWidth_ < 2U ||
                terrainGpuHeight_ < 2U)
            {
                return false;
            }

            const std::size_t baseVertexCount =
                static_cast<std::size_t>(
                    terrainGpuWidth_) *
                static_cast<std::size_t>(
                    terrainGpuHeight_);

            if (terrainVertices_.size() < baseVertexCount)
            {
                return false;
            }

            const auto calculateNormal =
                [this](
                    const std::uint32_t sourceX,
                    const std::uint32_t sourceZ)
                {
                    const std::uint32_t sourceLeft =
                        sourceX > terrainSampleStep_
                            ? sourceX - terrainSampleStep_
                            : 0U;

                    const std::uint32_t sourceRight =
                        (std::min)(
                            sourceX + terrainSampleStep_,
                            terrainAsset_.width - 1U);

                    const std::uint32_t sourceDown =
                        sourceZ > terrainSampleStep_
                            ? sourceZ - terrainSampleStep_
                            : 0U;

                    const std::uint32_t sourceUp =
                        (std::min)(
                            sourceZ + terrainSampleStep_,
                            terrainAsset_.height - 1U);

                    const float heightLeft =
                        terrainAsset_.GetHeight(
                            sourceLeft,
                            sourceZ);

                    const float heightRight =
                        terrainAsset_.GetHeight(
                            sourceRight,
                            sourceZ);

                    const float heightDown =
                        terrainAsset_.GetHeight(
                            sourceX,
                            sourceDown);

                    const float heightUp =
                        terrainAsset_.GetHeight(
                            sourceX,
                            sourceUp);

                    const float scaleX =
                        static_cast<float>(
                            (std::max)(
                                sourceRight - sourceLeft,
                                1U)) *
                        terrainAsset_.tileSize;

                    const float scaleZ =
                        static_cast<float>(
                            (std::max)(
                                sourceUp - sourceDown,
                                1U)) *
                        terrainAsset_.tileSize;

                    const DirectX::XMVECTOR normal =
                        DirectX::XMVector3Normalize(
                            DirectX::XMVectorSet(
                                (heightLeft - heightRight) *
                                    scaleZ,
                                scaleX * scaleZ,
                                (heightDown - heightUp) *
                                    scaleX,
                                0.0F));

                    DirectX::XMFLOAT3 result{};

                    DirectX::XMStoreFloat3(
                        &result,
                        normal);

                    return result;
                };

            for (std::uint32_t gpuZ = 0U;
                 gpuZ < terrainGpuHeight_;
                 ++gpuZ)
            {
                const std::uint32_t sourceZ =
                    (std::min)(
                        gpuZ * terrainSampleStep_,
                        terrainAsset_.height - 1U);

                for (std::uint32_t gpuX = 0U;
                     gpuX < terrainGpuWidth_;
                     ++gpuX)
                {
                    const std::uint32_t sourceX =
                        (std::min)(
                            gpuX * terrainSampleStep_,
                            terrainAsset_.width - 1U);

                    Vertex& vertex =
                        terrainVertices_[
                            static_cast<std::size_t>(gpuZ) *
                                terrainGpuWidth_ +
                            gpuX];

                    vertex.position.y =
                        terrainAsset_.GetHeight(
                            sourceX,
                            sourceZ);

                    vertex.normal =
                        calculateNormal(
                            sourceX,
                            sourceZ);
                }
            }

            const float skirtDepth =
                (std::max)(
                    20.0F,
                    terrainAsset_.heightScale *
                        0.025F);

            for (std::size_t index = baseVertexCount;
                 index < terrainVertices_.size();
                 ++index)
            {
                Vertex& vertex =
                    terrainVertices_[index];

                const std::uint32_t sourceX =
                    (std::min)(
                        static_cast<std::uint32_t>(
                            std::lround(
                                vertex.position.x /
                                terrainAsset_.tileSize)),
                        terrainAsset_.width - 1U);

                const std::uint32_t sourceZ =
                    (std::min)(
                        static_cast<std::uint32_t>(
                            std::lround(
                                vertex.position.z /
                                terrainAsset_.tileSize)),
                        terrainAsset_.height - 1U);

                vertex.position.y =
                    terrainAsset_.GetHeight(
                        sourceX,
                        sourceZ) -
                    skirtDepth;

                vertex.normal =
                    calculateNormal(
                        sourceX,
                        sourceZ);
            }

            const float gpuSpacing =
                terrainAsset_.tileSize *
                static_cast<float>(
                    terrainSampleStep_);

            for (Chunk& chunk : chunks_)
            {
                const std::uint32_t minimumX =
                    (std::min)(
                        static_cast<std::uint32_t>(
                            std::lround(
                                chunk.minimum.x /
                                gpuSpacing)),
                        terrainGpuWidth_ - 1U);

                const std::uint32_t maximumX =
                    (std::min)(
                        static_cast<std::uint32_t>(
                            std::lround(
                                chunk.maximum.x /
                                gpuSpacing)),
                        terrainGpuWidth_ - 1U);

                const std::uint32_t minimumZ =
                    (std::min)(
                        static_cast<std::uint32_t>(
                            std::lround(
                                chunk.minimum.z /
                                gpuSpacing)),
                        terrainGpuHeight_ - 1U);

                const std::uint32_t maximumZ =
                    (std::min)(
                        static_cast<std::uint32_t>(
                            std::lround(
                                chunk.maximum.z /
                                gpuSpacing)),
                        terrainGpuHeight_ - 1U);

                float minimumHeight =
                    std::numeric_limits<float>::max();

                float maximumHeight =
                    std::numeric_limits<float>::lowest();

                for (std::uint32_t z = minimumZ;
                     z <= maximumZ;
                     ++z)
                {
                    for (std::uint32_t x = minimumX;
                         x <= maximumX;
                         ++x)
                    {
                        const float height =
                            terrainVertices_[
                                static_cast<std::size_t>(z) *
                                    terrainGpuWidth_ +
                                x].
                                position.y;

                        minimumHeight =
                            (std::min)(
                                minimumHeight,
                                height);

                        maximumHeight =
                            (std::max)(
                                maximumHeight,
                                height);
                    }
                }

                chunk.minimum.y =
                    minimumHeight;

                chunk.maximum.y =
                    maximumHeight;

                chunk.center.y =
                    (minimumHeight +
                     maximumHeight) *
                    0.5F;
            }

            auto* const d3d11Device =
                static_cast<
                    engine::graphics::d3d11::D3D11Device*>(
                        device_);

            ID3D11Buffer* const nativeBuffer =
                d3d11Device->GetNativeBuffer(
                    vertexBuffer_);

            ID3D11DeviceContext* const nativeContext =
                d3d11Device->
                    GetNativeImmediateContext();

            if (
                nativeBuffer == nullptr ||
                nativeContext == nullptr)
            {
                return false;
            }

            nativeContext->UpdateSubresource(
                nativeBuffer,
                0U,
                nullptr,
                terrainVertices_.data(),
                0U,
                0U);

            return true;
        }

        [[nodiscard]]
        bool SaveHeightField() noexcept
        {
            const engine::assets::AssetResult result =
                terrainAsset_.SaveHeightsAtomic();

            if (engine::assets::Failed(result))
            {
                std::string message =
                    "Save terrain heights failed: ";

                message +=
                    engine::assets::ToString(
                        result);

                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.Terrain",
                    message);

                return false;
            }

            return true;
        }

        void ApplyHeightCommand(
            const HeightCommand& command,
            const bool useBefore) noexcept
        {
            for (const HeightChange& change : command)
            {
                if (
                    change.sample >=
                    terrainAsset_.heights.size())
                {
                    continue;
                }

                terrainAsset_.heights[
                    change.sample] =
                        useBefore
                            ? change.before
                            : change.after;
            }

            static_cast<void>(
                RefreshHeightGeometry());
        }

        [[nodiscard]]
        bool BeginSculptStroke() noexcept
        {
            if (
                !CanSculpt() ||
                heightStrokeActive_)
            {
                return false;
            }

            activeHeightBefore_.clear();
            heightStrokeActive_ = true;

            return true;
        }

        [[nodiscard]]
        bool Sculpt(
            const SceneDocument& document,
            const TerrainSculptMode mode,
            const float worldX,
            const float worldZ,
            const float radius,
            const float hardness,
            const float strength,
            const float deltaValue,
            const float levelHeight,
            const float smoothBoxHalfSize,
            const float smoothSeconds,
            const float deltaSeconds) noexcept
        {
            if (
                !heightStrokeActive_ ||
                !CanSculpt() ||
                radius <= 0.0F)
            {
                return false;
            }

            const EditorSceneEntity* actor = nullptr;

            for (const EditorSceneEntity& entity :
                 document.GetEntities())
            {
                if (
                    entity.terrain.has_value() &&
                    entity.terrain->visible)
                {
                    actor = &entity;
                    break;
                }
            }

            if (
                actor == nullptr ||
                std::abs(actor->transform.scale[0]) <
                    0.00001F ||
                std::abs(actor->transform.scale[1]) <
                    0.00001F ||
                std::abs(actor->transform.scale[2]) <
                    0.00001F)
            {
                return false;
            }

            const float sampleWorldX =
                terrainAsset_.tileSize *
                actor->transform.scale[0];

            const float sampleWorldZ =
                terrainAsset_.tileSize *
                actor->transform.scale[2];

            const float centerX =
                (worldX -
                 actor->transform.position[0]) /
                sampleWorldX;

            const float centerZ =
                (worldZ -
                 actor->transform.position[2]) /
                sampleWorldZ;

            const float radiusSamplesX =
                radius /
                std::abs(sampleWorldX);

            const float radiusSamplesZ =
                radius /
                std::abs(sampleWorldZ);

            const int minimumX =
                (std::max)(
                    0,
                    static_cast<int>(
                        std::floor(
                            centerX -
                            radiusSamplesX)));

            const int maximumX =
                (std::min)(
                    static_cast<int>(
                        terrainAsset_.width - 1U),
                    static_cast<int>(
                        std::ceil(
                            centerX +
                            radiusSamplesX)));

            const int minimumZ =
                (std::max)(
                    0,
                    static_cast<int>(
                        std::floor(
                            centerZ -
                            radiusSamplesZ)));

            const int maximumZ =
                (std::min)(
                    static_cast<int>(
                        terrainAsset_.height - 1U),
                    static_cast<int>(
                        std::ceil(
                            centerZ +
                            radiusSamplesZ)));

            const float amplitude =
                (std::max)(
                    std::fabs(
                        terrainAsset_.heightOffset),
                    std::fabs(
                        terrainAsset_.heightOffset +
                        terrainAsset_.heightScale));

            if (amplitude <= 0.0F)
            {
                return false;
            }

            const auto sampleToHeight =
                [amplitude](
                    const std::int16_t sample)
                {
                    return
                        static_cast<float>(sample) *
                        (amplitude / 32767.0F);
                };

            const auto heightToSample =
                [amplitude](
                    const float height)
                {
                    const long value =
                        std::lround(
                            height *
                            32767.0F /
                            amplitude);

                    return static_cast<std::int16_t>(
                        std::clamp(
                            value,
                            static_cast<long>(
                                (std::numeric_limits<
                                    std::int16_t>::min)()),
                            static_cast<long>(
                                (std::numeric_limits<
                                    std::int16_t>::max)())));
                };

            struct PendingHeight final
            {
                std::uint32_t sample = 0U;
                std::int16_t value = 0;
            };

            std::vector<PendingHeight> pending;

            const float safeHardness =
                std::clamp(
                    hardness,
                    0.0F,
                    1.0F);

            const float safeStrength =
                std::clamp(
                    strength,
                    0.0F,
                    1.0F);

            const float safeDeltaSeconds =
                std::clamp(
                    deltaSeconds,
                    0.0F,
                    0.25F);

            const float frameFactor =
                safeDeltaSeconds *
                60.0F;

            const int smoothRadius =
                (std::max)(
                    1,
                    static_cast<int>(
                        std::lround(
                            smoothBoxHalfSize)));

            for (int z = minimumZ;
                 z <= maximumZ;
                 ++z)
            {
                for (int x = minimumX;
                     x <= maximumX;
                     ++x)
                {
                    const float sampleX =
                        actor->transform.position[0] +
                        static_cast<float>(x) *
                            sampleWorldX;

                    const float sampleZ =
                        actor->transform.position[2] +
                        static_cast<float>(z) *
                            sampleWorldZ;

                    const float deltaX =
                        sampleX - worldX;

                    const float deltaZ =
                        sampleZ - worldZ;

                    const float distance =
                        std::sqrt(
                            deltaX * deltaX +
                            deltaZ * deltaZ);

                    const float normalizedDistance =
                        distance / radius;

                    if (normalizedDistance > 1.0F)
                    {
                        continue;
                    }

                    const float influence =
                        normalizedDistance <=
                                safeHardness
                            ? 1.0F
                            : 1.0F -
                                (
                                    normalizedDistance -
                                    safeHardness
                                ) /
                                (std::max)(
                                    1.0F -
                                        safeHardness,
                                    0.0001F);

                    const std::uint32_t sampleIndex =
                        static_cast<std::uint32_t>(z) *
                            terrainAsset_.width +
                        static_cast<std::uint32_t>(x);

                    const std::int16_t oldSample =
                        terrainAsset_.heights[
                            sampleIndex];

                    const float oldHeight =
                        sampleToHeight(
                            oldSample);

                    float newHeight =
                        oldHeight;

                    switch (mode)
                    {
                    case TerrainSculptMode::Down:

                        newHeight -=
                            deltaValue *
                            safeStrength *
                            influence *
                            frameFactor /
                            actor->transform.scale[1];

                        break;

                    case TerrainSculptMode::Up:

                        newHeight +=
                            deltaValue *
                            safeStrength *
                            influence *
                            frameFactor /
                            actor->transform.scale[1];

                        break;

                    case TerrainSculptMode::Level:
                    {
                        const float targetHeight =
                            (
                                levelHeight -
                                actor->transform.position[1]
                            ) /
                            actor->transform.scale[1];

                        const float amount =
                            std::clamp(
                                safeStrength *
                                    influence *
                                    safeDeltaSeconds *
                                    8.0F,
                                0.0F,
                                1.0F);

                        newHeight +=
                            (targetHeight -
                             oldHeight) *
                            amount;

                        break;
                    }

                    case TerrainSculptMode::Smooth:
                    {
                        float total = 0.0F;
                        std::uint32_t count = 0U;

                        const int smoothMinimumX =
                            (std::max)(
                                0,
                                x - smoothRadius);

                        const int smoothMaximumX =
                            (std::min)(
                                static_cast<int>(
                                    terrainAsset_.width - 1U),
                                x + smoothRadius);

                        const int smoothMinimumZ =
                            (std::max)(
                                0,
                                z - smoothRadius);

                        const int smoothMaximumZ =
                            (std::min)(
                                static_cast<int>(
                                    terrainAsset_.height - 1U),
                                z + smoothRadius);

                        for (int sampleZIndex =
                                 smoothMinimumZ;
                             sampleZIndex <=
                                 smoothMaximumZ;
                             ++sampleZIndex)
                        {
                            for (int sampleXIndex =
                                     smoothMinimumX;
                                 sampleXIndex <=
                                     smoothMaximumX;
                                 ++sampleXIndex)
                            {
                                const std::uint32_t index =
                                    static_cast<std::uint32_t>(
                                        sampleZIndex) *
                                        terrainAsset_.width +
                                    static_cast<std::uint32_t>(
                                        sampleXIndex);

                                total +=
                                    sampleToHeight(
                                        terrainAsset_.
                                            heights[index]);

                                ++count;
                            }
                        }

                        if (count > 0U)
                        {
                            const float average =
                                total /
                                static_cast<float>(
                                    count);

                            const float amount =
                                std::clamp(
                                    safeStrength *
                                        influence *
                                        safeDeltaSeconds /
                                        (std::max)(
                                            smoothSeconds,
                                            0.01F),
                                    0.0F,
                                    1.0F);

                            newHeight +=
                                (average -
                                 oldHeight) *
                                amount;
                        }

                        break;
                    }
                    }

                    const std::int16_t newSample =
                        heightToSample(
                            newHeight);

                    if (newSample == oldSample)
                    {
                        continue;
                    }

                    pending.push_back(
                        {
                            sampleIndex,
                            newSample
                        });
                }
            }

            if (pending.empty())
            {
                return false;
            }

            for (const PendingHeight& value :
                 pending)
            {
                activeHeightBefore_.try_emplace(
                    value.sample,
                    terrainAsset_.heights[
                        value.sample]);

                terrainAsset_.heights[
                    value.sample] =
                        value.value;
            }

            return RefreshHeightGeometry();
        }

        [[nodiscard]]
        bool EndSculptStroke() noexcept
        {
            if (!heightStrokeActive_)
            {
                return false;
            }

            heightStrokeActive_ = false;

            HeightCommand command;

            command.reserve(
                activeHeightBefore_.size());

            for (const auto& [sample, before] :
                 activeHeightBefore_)
            {
                const std::int16_t after =
                    terrainAsset_.heights[sample];

                if (before != after)
                {
                    command.push_back(
                        {
                            sample,
                            before,
                            after
                        });
                }
            }

            activeHeightBefore_.clear();

            if (command.empty())
            {
                return false;
            }

            if (!SaveHeightField())
            {
                ApplyHeightCommand(
                    command,
                    true);

                return false;
            }

            heightUndo_.push_back(
                std::move(command));

            if (
                heightUndo_.size() >
                MaximumHeightHistorySize)
            {
                heightUndo_.erase(
                    heightUndo_.begin());
            }

            heightRedo_.clear();

            return true;
        }

        [[nodiscard]]
        bool UndoSculpt() noexcept
        {
            if (
                heightStrokeActive_ ||
                heightUndo_.empty())
            {
                return false;
            }

            HeightCommand command =
                std::move(
                    heightUndo_.back());

            heightUndo_.pop_back();

            ApplyHeightCommand(
                command,
                true);

            if (!SaveHeightField())
            {
                ApplyHeightCommand(
                    command,
                    false);

                heightUndo_.push_back(
                    std::move(command));

                return false;
            }

            heightRedo_.push_back(
                std::move(command));

            return true;
        }

        [[nodiscard]]
        bool RedoSculpt() noexcept
        {
            if (
                heightStrokeActive_ ||
                heightRedo_.empty())
            {
                return false;
            }

            HeightCommand command =
                std::move(
                    heightRedo_.back());

            heightRedo_.pop_back();

            ApplyHeightCommand(
                command,
                false);

            if (!SaveHeightField())
            {
                ApplyHeightCommand(
                    command,
                    true);

                heightRedo_.push_back(
                    std::move(command));

                return false;
            }

            heightUndo_.push_back(
                std::move(command));

            return true;
        }

        [[nodiscard]]
        bool CanUndoSculpt() const noexcept
        {
            return
                !heightStrokeActive_ &&
                !heightUndo_.empty();
        }

        [[nodiscard]]
        bool CanRedoSculpt() const noexcept
        {
            return
                !heightStrokeActive_ &&
                !heightRedo_.empty();
        }

        [[nodiscard]]
        bool IsSculptStrokeActive() const noexcept
        {
            return heightStrokeActive_;
        }

        [[nodiscard]]
        bool BeginPaintStroke() noexcept
        {
            if (!loaded_ || paintStrokeActive_)
            {
                return false;
            }

            activePaintBefore_.clear();
            paintStrokeActive_ = true;

            return true;
        }

        [[nodiscard]]
        bool Paint(
            const SceneDocument& document,
            const float worldX,
            const float worldZ,
            const float radius,
            const float strength,
            const float falloff,
            const std::size_t layerIndex,
            const bool erase) noexcept
        {
            if (!paintStrokeActive_ || maskWidth_ == 0U || maskHeight_ == 0U)
            {
                return false;
            }

            const EditorSceneEntity* actor = nullptr;

            for (const EditorSceneEntity& entity : document.GetEntities())
            {
                if (entity.terrain.has_value() && entity.terrain->visible)
                {
                    actor = &entity;
                    break;
                }
            }

            if (actor == nullptr ||
                std::abs(actor->transform.scale[0]) < 0.00001F ||
                std::abs(actor->transform.scale[2]) < 0.00001F)
            {
                return false;
            }

            const std::size_t layerCount = actor->terrain->layers.empty()
                ? terrainAsset_.layers.size()
                : actor->terrain->layers.size();

            if (layerIndex >= layerCount ||
                layerCount > MaximumTerrainLayerCount ||
                !SetMaterialLayerCount(layerCount))
            {
                return false;
            }

            if (layerIndex > 0U &&
                (layerIndex - 1U) / 3U >= activeMaskCount_)
            {
                return false;
            }

            const float terrainWidth =
                static_cast<float>(terrainAsset_.width - 1U) *
                terrainAsset_.tileSize *
                actor->transform.scale[0];

            const float terrainDepth =
                static_cast<float>(terrainAsset_.height - 1U) *
                terrainAsset_.tileSize *
                actor->transform.scale[2];

            const float u =
                (worldX - actor->transform.position[0]) /
                terrainWidth;

            const float v =
                (worldZ - actor->transform.position[2]) /
                terrainDepth;

            if (u < 0.0F ||
                v < 0.0F ||
                u > 1.0F ||
                v > 1.0F)
            {
                return false;
            }

            const float centerX =
                u * static_cast<float>(maskWidth_ - 1U);

            const float centerY =
                v * static_cast<float>(maskHeight_ - 1U);

            const float pixelsPerWorldX =
                static_cast<float>(maskWidth_ - 1U) /
                std::abs(terrainWidth);

            const float pixelsPerWorldZ =
                static_cast<float>(maskHeight_ - 1U) /
                std::abs(terrainDepth);

            const float radiusPixels =
                (std::max)(
                    1.0F,
                    radius *
                        (pixelsPerWorldX + pixelsPerWorldZ) *
                        0.5F);

            const int minimumX =
                (std::max)(
                    0,
                    static_cast<int>(
                        std::floor(centerX - radiusPixels)));

            const int maximumX =
                (std::min)(
                    static_cast<int>(maskWidth_ - 1U),
                    static_cast<int>(
                        std::ceil(centerX + radiusPixels)));

            const int minimumY =
                (std::max)(
                    0,
                    static_cast<int>(
                        std::floor(centerY - radiusPixels)));

            const int maximumY =
                (std::min)(
                    static_cast<int>(maskHeight_ - 1U),
                    static_cast<int>(
                        std::ceil(centerY + radiusPixels)));

            bool changed = false;

            for (int y = minimumY; y <= maximumY; ++y)
            {
                for (int x = minimumX; x <= maximumX; ++x)
                {
                    const float deltaX =
                        static_cast<float>(x) - centerX;
                    const float deltaY =
                        static_cast<float>(y) - centerY;

                    const float normalizedDistance =
                        std::sqrt(
                            deltaX * deltaX +
                            deltaY * deltaY) /
                        radiusPixels;

                    if (normalizedDistance > 1.0F)
                    {
                        continue;
                    }

                    const float hardPart =
                        std::clamp(
                            1.0F - falloff,
                            0.0F,
                            1.0F);

                    const float influence =
                        normalizedDistance <= hardPart
                            ? 1.0F
                            : 1.0F -
                                (normalizedDistance - hardPart) /
                                (std::max)(falloff, 0.0001F);

                    const float amount =
                        std::clamp(strength, 0.0F, 1.0F) *
                        std::clamp(influence, 0.0F, 1.0F);

                    if (amount <= 0.0F)
                    {
                        continue;
                    }

                    const std::uint32_t pixel =
                        static_cast<std::uint32_t>(y) *
                            maskWidth_ +
                        static_cast<std::uint32_t>(x);

                    auto weights = ReadWeights(pixel);

                    activePaintBefore_.try_emplace(
                        pixel,
                        weights);

                    if (layerIndex == 0U)
                    {
                        for (std::uint8_t& weight : weights)
                        {
                            weight =
                                static_cast<std::uint8_t>(
                                    std::lround(
                                        static_cast<float>(weight) *
                                        (1.0F - amount)));
                        }
                    }
                    else
                    {
                        const std::size_t selectedLayer =
                            layerIndex - 1U;

                        const float target =
                            erase ? 0.0F : 255.0F;

                        const auto newSelectedWeight =
                            static_cast<std::uint8_t>(
                                std::clamp(
                                    std::lround(
                                        static_cast<float>(
                                            weights[selectedLayer]) +
                                        (target -
                                         static_cast<float>(
                                             weights[selectedLayer])) *
                                            amount),
                                    0L,
                                    255L));

                        weights[selectedLayer] =
                            newSelectedWeight;

                        if (!erase)
                        {
                            std::uint32_t otherTotal = 0U;

                            for (std::size_t index = 0U;
                                 index < weights.size();
                                 ++index)
                            {
                                if (index != selectedLayer)
                                {
                                    otherTotal += weights[index];
                                }
                            }

                            const std::uint32_t remaining =
                                255U - newSelectedWeight;

                            if (otherTotal > remaining &&
                                otherTotal > 0U)
                            {
                                for (std::size_t index = 0U;
                                     index < weights.size();
                                     ++index)
                                {
                                    if (index != selectedLayer)
                                    {
                                        weights[index] =
                                            static_cast<std::uint8_t>(
                                                static_cast<std::uint32_t>(
                                                    weights[index]) *
                                                remaining /
                                                otherTotal);
                                    }
                                }
                            }
                        }
                    }

                    WriteWeights(pixel, weights);
                    changed = true;
                }
            }

            if (changed)
            {
                UploadMaskRegion(
                    static_cast<std::uint32_t>(minimumX),
                    static_cast<std::uint32_t>(minimumY),
                    static_cast<std::uint32_t>(maximumX + 1),
                    static_cast<std::uint32_t>(maximumY + 1));
            }

            return changed;
        }

        [[nodiscard]]
        bool EndPaintStroke() noexcept
        {
            if (!paintStrokeActive_)
            {
                return false;
            }

            paintStrokeActive_ = false;

            PaintCommand command;
            command.reserve(activePaintBefore_.size());

            for (const auto& [pixel, before] : activePaintBefore_)
            {
                const auto after = ReadWeights(pixel);

                if (before != after)
                {
                    command.push_back(
                        {pixel, before, after});
                }
            }

            activePaintBefore_.clear();

            if (command.empty())
            {
                return false;
            }

            paintUndo_.push_back(std::move(command));

            if (paintUndo_.size() > MaximumPaintHistorySize)
            {
                paintUndo_.erase(paintUndo_.begin());
            }

            paintRedo_.clear();
            SavePaintData();

            return true;
        }

        [[nodiscard]]
        bool UndoPaint() noexcept
        {
            if (paintUndo_.empty())
            {
                return false;
            }

            PaintCommand command =
                std::move(paintUndo_.back());

            paintUndo_.pop_back();

            for (const PaintChange& change : command)
            {
                WriteWeights(change.pixel, change.before);
            }

            paintRedo_.push_back(std::move(command));
            RefreshMaskTextures();
            SavePaintData();

            return true;
        }

        [[nodiscard]]
        bool RedoPaint() noexcept
        {
            if (paintRedo_.empty())
            {
                return false;
            }

            PaintCommand command =
                std::move(paintRedo_.back());

            paintRedo_.pop_back();

            for (const PaintChange& change : command)
            {
                WriteWeights(change.pixel, change.after);
            }

            paintUndo_.push_back(std::move(command));
            RefreshMaskTextures();
            SavePaintData();

            return true;
        }

        [[nodiscard]]
        bool CanUndoPaint() const noexcept
        {
            return !paintUndo_.empty();
        }

        [[nodiscard]]
        bool CanRedoPaint() const noexcept
        {
            return !paintRedo_.empty();
        }

    private:
        [[nodiscard]]
        std::array<std::uint8_t, MaximumPaintedLayerCount>
            ReadWeights(
                const std::uint32_t pixel) const noexcept
        {
            std::array<std::uint8_t, MaximumPaintedLayerCount>
                weights{};

            const std::size_t byteOffset =
                static_cast<std::size_t>(pixel) * 4U;

            for (std::size_t layer = 0U;
                 layer < weights.size();
                 ++layer)
            {
                const std::size_t maskIndex = layer / 3U;
                const std::size_t channel = layer % 3U;

                if (maskIndex >= activeMaskCount_ ||
                    byteOffset + channel >=
                        maskPixels_[maskIndex].size())
                {
                    weights[layer] = 0U;
                    continue;
                }

                weights[layer] =
                    std::to_integer<std::uint8_t>(
                        maskPixels_[maskIndex][
                            byteOffset + channel]);
            }

            return weights;
        }

        void WriteWeights(
            const std::uint32_t pixel,
            const std::array<
                std::uint8_t,
                MaximumPaintedLayerCount>& weights) noexcept
        {
            const std::size_t byteOffset =
                static_cast<std::size_t>(pixel) * 4U;

            for (std::size_t layer = 0U;
                 layer < weights.size();
                 ++layer)
            {
                const std::size_t maskIndex = layer / 3U;
                const std::size_t channel = layer % 3U;

                if (maskIndex >= activeMaskCount_ ||
                    byteOffset + channel >=
                        maskPixels_[maskIndex].size())
                {
                    continue;
                }

                maskPixels_[maskIndex][byteOffset + channel] =
                    static_cast<std::byte>(weights[layer]);
            }
        }

        void RefreshMaskTextures() noexcept
        {
            if (device_ == nullptr)
            {
                return;
            }

            for (std::size_t maskIndex = 0U;
                 maskIndex < masks_.size();
                 ++maskIndex)
            {
                if (masks_[maskIndex].IsValid())
                {
                    static_cast<void>(
                        device_->DestroyTexture(
                            masks_[maskIndex]));
                }

                masks_[maskIndex] = {};

                const bool created =
                    maskIndex < activeMaskCount_
                        ? CreateEditableMaskTexture(
                            *device_,
                            maskPixels_[maskIndex],
                            maskWidth_,
                            maskHeight_,
                            masks_[maskIndex])
                        : CreateFallbackMask(
                            *device_,
                            masks_[maskIndex]);

                if (!created)
                {
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Error,
                        "LTS.Editor.Terrain",
                        "Failed to recreate terrain mask texture.");
                }
            }
        }

        void UploadMaskRegion(
            const std::uint32_t left,
            const std::uint32_t top,
            const std::uint32_t right,
            const std::uint32_t bottom) noexcept
        {
            if (device_ == nullptr ||
                right <= left ||
                bottom <= top)
            {
                return;
            }

            auto* const d3d11Device =
                static_cast<
                    engine::graphics::d3d11::D3D11Device*>(
                        device_);

            ID3D11DeviceContext* const context =
                d3d11Device->GetNativeImmediateContext();

            if (context == nullptr)
            {
                return;
            }

            const D3D11_BOX box
            {
                left,
                top,
                0U,
                right,
                bottom,
                1U
            };

            for (std::size_t maskIndex = 0U;
                 maskIndex < activeMaskCount_;
                 ++maskIndex)
            {
                ID3D11Resource* const resource =
                    d3d11Device->GetNativeTexture(
                        masks_[maskIndex]);

                if (resource == nullptr)
                {
                    continue;
                }

                const std::byte* const source =
                    maskPixels_[maskIndex].data() +
                    (static_cast<std::size_t>(top) *
                         maskWidth_ +
                     left) *
                        4U;

                context->UpdateSubresource(
                    resource,
                    0U,
                    &box,
                    source,
                    maskWidth_ * 4U,
                    0U);
            }
        }

        void LoadPaintData(
            const std::filesystem::path& terrainPath) noexcept
        {
            paintPath_ = terrainPath;
            paintPath_ += L".paint";

            std::ifstream stream(
                paintPath_,
                std::ios::binary);

            if (!stream)
            {
                return;
            }

            std::uint32_t signature = 0U;
            std::uint32_t version = 0U;
            std::uint32_t width = 0U;
            std::uint32_t height = 0U;
            std::uint32_t maskCount = 0U;

            if (!stream.read(
                    reinterpret_cast<char*>(&signature),
                    sizeof(signature)) ||
                !stream.read(
                    reinterpret_cast<char*>(&version),
                    sizeof(version)) ||
                !stream.read(
                    reinterpret_cast<char*>(&width),
                    sizeof(width)) ||
                !stream.read(
                    reinterpret_cast<char*>(&height),
                    sizeof(height)) ||
                !stream.read(
                    reinterpret_cast<char*>(&maskCount),
                    sizeof(maskCount)) ||
                signature != PaintFileSignature ||
                version != PaintFileVersion ||
                width != maskWidth_ ||
                height != maskHeight_ ||
                maskCount == 0U ||
                maskCount > MaximumTerrainMaskCount)
            {
                return;
            }

            const std::uint64_t bytesPerMask64 =
                static_cast<std::uint64_t>(width) *
                static_cast<std::uint64_t>(height) *
                4ULL;

            if (bytesPerMask64 >
                static_cast<std::uint64_t>(
                    (std::numeric_limits<std::size_t>::max)()))
            {
                return;
            }

            const std::size_t bytesPerMask =
                static_cast<std::size_t>(bytesPerMask64);

            for (std::uint32_t maskIndex = 0U;
                 maskIndex < maskCount;
                 ++maskIndex)
            {
                std::vector<std::byte> loaded(bytesPerMask);

                if (!stream.read(
                        reinterpret_cast<char*>(loaded.data()),
                        static_cast<std::streamsize>(
                            bytesPerMask)))
                {
                    return;
                }

                if (maskIndex < activeMaskCount_)
                {
                    maskPixels_[maskIndex] =
                        std::move(loaded);
                }
            }
        }

        void SavePaintData() const noexcept
        {
            if (paintPath_.empty() ||
                activeMaskCount_ == 0U)
            {
                return;
            }

            std::filesystem::path temporary = paintPath_;
            temporary += L".tmp";

            std::ofstream stream(
                temporary,
                std::ios::binary |
                    std::ios::trunc);

            if (!stream)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.Terrain",
                    "Failed to create temporary terrain paint file.");

                return;
            }

            const std::uint32_t maskCount =
                static_cast<std::uint32_t>(
                    activeMaskCount_);

            stream.write(
                reinterpret_cast<const char*>(
                    &PaintFileSignature),
                sizeof(PaintFileSignature));

            stream.write(
                reinterpret_cast<const char*>(
                    &PaintFileVersion),
                sizeof(PaintFileVersion));

            stream.write(
                reinterpret_cast<const char*>(&maskWidth_),
                sizeof(maskWidth_));

            stream.write(
                reinterpret_cast<const char*>(&maskHeight_),
                sizeof(maskHeight_));

            stream.write(
                reinterpret_cast<const char*>(&maskCount),
                sizeof(maskCount));

            for (std::size_t maskIndex = 0U;
                 maskIndex < activeMaskCount_;
                 ++maskIndex)
            {
                stream.write(
                    reinterpret_cast<const char*>(
                        maskPixels_[maskIndex].data()),
                    static_cast<std::streamsize>(
                        maskPixels_[maskIndex].size()));
            }

            stream.close();

            if (!stream)
            {
                std::error_code removeError;
                static_cast<void>(
                    std::filesystem::remove(
                        temporary,
                        removeError));

                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.Terrain",
                    "Failed to write terrain paint data.");

                return;
            }

            std::error_code filesystemError;
            static_cast<void>(
                std::filesystem::remove(
                    paintPath_,
                    filesystemError));

            filesystemError.clear();

            std::filesystem::rename(
                temporary,
                paintPath_,
                filesystemError);

            if (filesystemError)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.Terrain",
                    "Failed to commit terrain paint data.");
            }
        }

        engine::graphics::RenderDevice* device_ = nullptr;

        std::filesystem::path terrainPath_;
        std::filesystem::path workspaceRoot_;
        std::filesystem::path paintPath_;

        engine::assets::TerrainAsset terrainAsset_;
        std::vector<Chunk> chunks_;

        std::vector<Vertex> terrainVertices_;

        std::unordered_map<
            std::uint32_t,
            std::int16_t>
            activeHeightBefore_;

        std::vector<HeightCommand> heightUndo_;
        std::vector<HeightCommand> heightRedo_;

        std::uint32_t terrainSampleStep_ = 1U;
        std::uint32_t terrainGpuWidth_ = 0U;
        std::uint32_t terrainGpuHeight_ = 0U;

        bool heightStrokeActive_ = false;

        engine::graphics::BufferHandle vertexBuffer_{};
        engine::graphics::BufferHandle indexBuffer_{};
        engine::graphics::BufferHandle constantBuffer_{};
        engine::graphics::BufferHandle brushVertexBuffer_{};
        engine::graphics::BufferHandle brushConstantBuffer_{};

        engine::graphics::ShaderHandle vertexShader_{};
        engine::graphics::ShaderHandle pixelShader_{};
        engine::graphics::ShaderHandle brushVertexShader_{};
        engine::graphics::ShaderHandle brushPixelShader_{};

        engine::graphics::InputLayoutHandle inputLayout_{};
        engine::graphics::InputLayoutHandle brushInputLayout_{};

        engine::graphics::PipelineStateHandle pipeline_{};
        engine::graphics::PipelineStateHandle brushPipeline_{};

        std::array<engine::graphics::TextureHandle, MaximumTerrainMaskCount> masks_{};
        std::array<std::vector<std::byte>, MaximumTerrainMaskCount> maskPixels_{};
        std::array<engine::graphics::TextureHandle, MaximumTerrainLayerCount> materials_{};
        std::array<std::string, MaximumTerrainLayerCount> materialPaths_{};
        std::array<engine::graphics::TextureHandle, MaximumTerrainLayerCount> normalMaterials_{};
        std::array<std::string, MaximumTerrainLayerCount> normalMaterialPaths_{};
        engine::graphics::SamplerHandle materialSampler_{};
        engine::graphics::SamplerHandle maskSampler_{};

        std::unordered_map<std::uint32_t, std::array<std::uint8_t, MaximumPaintedLayerCount>>activePaintBefore_;

        std::vector<PaintCommand> paintUndo_;
        std::vector<PaintCommand> paintRedo_;

        std::size_t activeMaskCount_ = 0U;
        std::uint32_t maskWidth_ = 0U;
        std::uint32_t maskHeight_ = 0U;
        std::uint32_t indexCount_ = 0U;

        bool paintStrokeActive_ = false;
        bool loaded_ = false;
    };

    TerrainRenderer::TerrainRenderer() noexcept
        : impl_(std::make_unique<Impl>())
    {
    }

    TerrainRenderer::~TerrainRenderer() noexcept = default;

    bool TerrainRenderer::Initialize(
        engine::graphics::RenderDevice& device) noexcept
    {
        return impl_->Initialize(device);
    }

    bool TerrainRenderer::LoadTerrain(
        engine::graphics::RenderDevice& device,
        const std::filesystem::path& path) noexcept
    {
        return impl_->LoadTerrain(device, path);
    }

    bool TerrainRenderer::HasTerrain() const noexcept
    {
        return impl_->HasTerrain();
    }

    bool TerrainRenderer::CanSculpt() const noexcept
    {
        return impl_->CanSculpt();
    }

    bool TerrainRenderer::BeginSculptStroke() noexcept
    {
        return impl_->BeginSculptStroke();
    }

    bool TerrainRenderer::Sculpt(
        const SceneDocument& document,
        const TerrainSculptMode mode,
        const float worldX,
        const float worldZ,
        const float radius,
        const float hardness,
        const float strength,
        const float deltaValue,
        const float levelHeight,
        const float smoothBoxHalfSize,
        const float smoothSeconds,
        const float deltaSeconds) noexcept
    {
        return impl_->Sculpt(
            document,
            mode,
            worldX,
            worldZ,
            radius,
            hardness,
            strength,
            deltaValue,
            levelHeight,
            smoothBoxHalfSize,
            smoothSeconds,
            deltaSeconds);
    }

    bool TerrainRenderer::EndSculptStroke() noexcept
    {
        return impl_->EndSculptStroke();
    }

    bool TerrainRenderer::UndoSculpt() noexcept
    {
        return impl_->UndoSculpt();
    }

    bool TerrainRenderer::RedoSculpt() noexcept
    {
        return impl_->RedoSculpt();
    }

    bool TerrainRenderer::CanUndoSculpt() const noexcept
    {
        return impl_->CanUndoSculpt();
    }

    bool TerrainRenderer::CanRedoSculpt() const noexcept
    {
        return impl_->CanRedoSculpt();
    }

    bool TerrainRenderer::IsSculptStrokeActive() const noexcept
    {
        return impl_->IsSculptStrokeActive();
    }

    bool TerrainRenderer::SetMaterialLayerCount(
    const std::size_t layerCount) noexcept
    {
        return impl_->SetMaterialLayerCount(layerCount);
    }

    bool TerrainRenderer::RemoveMaterialLayer(
        const std::size_t layerIndex,
        const std::size_t oldLayerCount) noexcept
    {
        return impl_->RemoveMaterialLayer(
            layerIndex,
            oldLayerCount);
    }

    bool TerrainRenderer::TryGetSurfaceHeight(
        const SceneDocument& document,
        const float worldX,
        const float worldZ,
        float& worldHeight) const noexcept
    {
        return impl_->TryGetSurfaceHeight(
            document,
            worldX,
            worldZ,
            worldHeight);
    }

    bool TerrainRenderer::BeginPaintStroke() noexcept
    {
        return impl_->BeginPaintStroke();
    }

    bool TerrainRenderer::Paint(
        const SceneDocument& document,
        const float worldX,
        const float worldZ,
        const float radius,
        const float strength,
        const float falloff,
        const std::size_t layerIndex,
        const bool erase) noexcept
    {
        return impl_->Paint(
            document,
            worldX,
            worldZ,
            radius,
            strength,
            falloff,
            layerIndex,
            erase);
    }

    bool TerrainRenderer::EndPaintStroke() noexcept
    {
        return impl_->EndPaintStroke();
    }

    bool TerrainRenderer::UndoPaint() noexcept
    {
        return impl_->UndoPaint();
    }

    bool TerrainRenderer::RedoPaint() noexcept
    {
        return impl_->RedoPaint();
    }

    bool TerrainRenderer::CanUndoPaint() const noexcept
    {
        return impl_->CanUndoPaint();
    }

    bool TerrainRenderer::CanRedoPaint() const noexcept
    {
        return impl_->CanRedoPaint();
    }

    void TerrainRenderer::Shutdown(
        engine::graphics::RenderDevice& device) noexcept
    {
        impl_->Shutdown(device);
    }

    engine::graphics::GraphicsResult TerrainRenderer::Render(
        engine::graphics::CommandContext& context,
        const SceneDocument& document,
        const DirectX::XMFLOAT4X4& viewProjection,
        const DirectX::XMFLOAT3& cameraPosition) noexcept
    {
        return impl_->Render(
            context,
            document,
            viewProjection,
            cameraPosition);
    }

    engine::graphics::GraphicsResult TerrainRenderer::RenderBrush(
        engine::graphics::CommandContext& context,
        const SceneDocument& document,
        const DirectX::XMFLOAT4X4& viewProjection,
        const float worldX,
        const float worldZ,
        const float radius,
        const bool erase) noexcept
    {
        return impl_->RenderBrush(
            context,
            document,
            viewProjection,
            worldX,
            worldZ,
            radius,
            erase);
    }
}
