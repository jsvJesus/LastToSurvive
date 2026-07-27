#include "Editor/Tools/Import/LegacyMeshPreview.h"
#include "Editor/LevelEditor/Rendering/ShaderCompiler.h"

#include <imgui.h>

#include <DirectXMath.h>
#include <Windows.h>
#include <d3d11.h>
#include <dxgiformat.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace lts::editor
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        constexpr float PreviewFieldOfView =
            DirectX::XMConvertToRadians(45.0F);

        constexpr std::uint32_t MaximumPreviewSize = 2048U;
        constexpr std::uintmax_t MaximumDdsFileSize =
            512U * 1024U * 1024U;

        constexpr std::uint32_t DdsMagic = 0x20534444U;
        constexpr std::uint32_t DdsFourCcFlag = 0x00000004U;
        constexpr std::uint32_t DdsRgbFlag = 0x00000040U;

        [[nodiscard]]
        constexpr std::uint32_t MakeFourCc(
            const char a,
            const char b,
            const char c,
            const char d) noexcept
        {
            return
                static_cast<std::uint32_t>(
                    static_cast<unsigned char>(a)) |
                static_cast<std::uint32_t>(
                    static_cast<unsigned char>(b)) << 8U |
                static_cast<std::uint32_t>(
                    static_cast<unsigned char>(c)) << 16U |
                static_cast<std::uint32_t>(
                    static_cast<unsigned char>(d)) << 24U;
        }

#pragma pack(push, 1)

        struct DdsPixelFormat final
        {
            std::uint32_t size = 0U;
            std::uint32_t flags = 0U;
            std::uint32_t fourCc = 0U;
            std::uint32_t rgbBitCount = 0U;
            std::uint32_t redMask = 0U;
            std::uint32_t greenMask = 0U;
            std::uint32_t blueMask = 0U;
            std::uint32_t alphaMask = 0U;
        };

        struct DdsHeader final
        {
            std::uint32_t size = 0U;
            std::uint32_t flags = 0U;
            std::uint32_t height = 0U;
            std::uint32_t width = 0U;
            std::uint32_t pitchOrLinearSize = 0U;
            std::uint32_t depth = 0U;
            std::uint32_t mipMapCount = 0U;

            std::uint32_t reserved1[11]{};

            DdsPixelFormat pixelFormat;

            std::uint32_t caps = 0U;
            std::uint32_t caps2 = 0U;
            std::uint32_t caps3 = 0U;
            std::uint32_t caps4 = 0U;
            std::uint32_t reserved2 = 0U;
        };

        struct DdsHeaderDx10 final
        {
            std::uint32_t dxgiFormat = 0U;
            std::uint32_t resourceDimension = 0U;
            std::uint32_t miscFlag = 0U;
            std::uint32_t arraySize = 0U;
            std::uint32_t miscFlags2 = 0U;
        };

#pragma pack(pop)

        static_assert(sizeof(DdsPixelFormat) == 32U);
        static_assert(sizeof(DdsHeader) == 124U);
        static_assert(sizeof(DdsHeaderDx10) == 20U);

        struct PreviewVertex final
        {
            DirectX::XMFLOAT3 position;
            DirectX::XMFLOAT3 normal;
            DirectX::XMFLOAT4 tangent;
            DirectX::XMFLOAT2 uv;
        };

        static_assert(sizeof(PreviewVertex) == 48U);

        struct alignas(16) PreviewConstants final
        {
            DirectX::XMFLOAT4X4 viewProjection;

            DirectX::XMFLOAT4 cameraPosition;
            DirectX::XMFLOAT4 lightDirectionIntensity;
            DirectX::XMFLOAT4 lightColor;
            DirectX::XMFLOAT4 ambientColor;

            DirectX::XMFLOAT4 baseColor;

            // x = SpecularPower
            // y = Specular1Power
            // z = ReflectionPower
            // w = SelfIllumMultiplier
            DirectX::XMFLOAT4 materialParameters;

            // x = Diffuse
            // y = Normal
            // z = Specular / Metalness
            // w = Roughness
            DirectX::XMFLOAT4 textureFlags;
        };

        static_assert(sizeof(PreviewConstants) % 16U == 0U);

        struct GpuMaterial final
        {
            ComPtr<ID3D11ShaderResourceView> diffuse;
            ComPtr<ID3D11ShaderResourceView> normal;
            ComPtr<ID3D11ShaderResourceView> specular;
            ComPtr<ID3D11ShaderResourceView> roughness;

            DirectX::XMFLOAT4 baseColor
            {
                1.0F,
                1.0F,
                1.0F,
                1.0F
            };

            DirectX::XMFLOAT4 parameters
            {
                0.0F,
                0.5F,
                0.0F,
                0.0F
            };

            DirectX::XMFLOAT4 textureFlags{};
        };

        struct CameraState final
        {
            DirectX::XMMATRIX viewProjection =
                DirectX::XMMatrixIdentity();

            DirectX::XMFLOAT3 cameraPosition{};
            DirectX::XMFLOAT3 right{};
            DirectX::XMFLOAT3 up{};
        };

        [[nodiscard]]
        bool IsRegularFile(
            const std::filesystem::path& path) noexcept
        {
            std::error_code error;

            return
                std::filesystem::is_regular_file(path, error) &&
                !error;
        }

        [[nodiscard]]
        bool ReadFileBytes(
            const std::filesystem::path& path,
            std::vector<std::byte>& output,
            std::string& error) noexcept
        {
            output.clear();

            try
            {
                std::error_code sizeError;

                const std::uintmax_t fileSize =
                    std::filesystem::file_size(
                        path,
                        sizeError);

                if (sizeError ||
                    fileSize == 0U ||
                    fileSize > MaximumDdsFileSize ||
                    fileSize >
                        static_cast<std::uintmax_t>(
                            (std::numeric_limits<
                                std::streamsize>::max)()))
                {
                    error =
                        "Invalid DDS file size.";

                    return false;
                }

                output.resize(
                    static_cast<std::size_t>(
                        fileSize));

                std::ifstream stream(
                    path,
                    std::ios::binary);

                if (!stream)
                {
                    error =
                        "Failed to open DDS file.";

                    output.clear();
                    return false;
                }

                stream.read(
                    reinterpret_cast<char*>(
                        output.data()),
                    static_cast<std::streamsize>(
                        output.size()));

                if (!stream)
                {
                    error =
                        "Failed to read DDS file.";

                    output.clear();
                    return false;
                }

                return true;
            }
            catch (const std::exception& exception)
            {
                error =
                    "DDS read failed: " +
                    std::string(exception.what());

                output.clear();
                return false;
            }
            catch (...)
            {
                error =
                    "DDS read failed with an unknown error.";

                output.clear();
                return false;
            }
        }

        [[nodiscard]]
        DXGI_FORMAT NormalizeDxgiFormat(
            const DXGI_FORMAT format) noexcept
        {
            switch (format)
            {
                case DXGI_FORMAT_R8G8B8A8_TYPELESS:
                    return DXGI_FORMAT_R8G8B8A8_UNORM;

                case DXGI_FORMAT_B8G8R8A8_TYPELESS:
                    return DXGI_FORMAT_B8G8R8A8_UNORM;

                case DXGI_FORMAT_B8G8R8X8_TYPELESS:
                    return DXGI_FORMAT_B8G8R8X8_UNORM;

                case DXGI_FORMAT_BC1_TYPELESS:
                    return DXGI_FORMAT_BC1_UNORM;

                case DXGI_FORMAT_BC2_TYPELESS:
                    return DXGI_FORMAT_BC2_UNORM;

                case DXGI_FORMAT_BC3_TYPELESS:
                    return DXGI_FORMAT_BC3_UNORM;

                case DXGI_FORMAT_BC4_TYPELESS:
                    return DXGI_FORMAT_BC4_UNORM;

                case DXGI_FORMAT_BC5_TYPELESS:
                    return DXGI_FORMAT_BC5_UNORM;

                case DXGI_FORMAT_BC7_TYPELESS:
                    return DXGI_FORMAT_BC7_UNORM;

                default:
                    return format;
            }
        }

        [[nodiscard]]
        DXGI_FORMAT MakeSrgbFormat(
            const DXGI_FORMAT format) noexcept
        {
            switch (format)
            {
                case DXGI_FORMAT_R8G8B8A8_UNORM:
                    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

                case DXGI_FORMAT_B8G8R8A8_UNORM:
                    return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

                case DXGI_FORMAT_B8G8R8X8_UNORM:
                    return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

                case DXGI_FORMAT_BC1_UNORM:
                    return DXGI_FORMAT_BC1_UNORM_SRGB;

                case DXGI_FORMAT_BC2_UNORM:
                    return DXGI_FORMAT_BC2_UNORM_SRGB;

                case DXGI_FORMAT_BC3_UNORM:
                    return DXGI_FORMAT_BC3_UNORM_SRGB;

                case DXGI_FORMAT_BC7_UNORM:
                    return DXGI_FORMAT_BC7_UNORM_SRGB;

                default:
                    return format;
            }
        }

        [[nodiscard]]
        bool IsBlockCompressed(
            const DXGI_FORMAT format) noexcept
        {
            switch (format)
            {
                case DXGI_FORMAT_BC1_UNORM:
                case DXGI_FORMAT_BC1_UNORM_SRGB:
                case DXGI_FORMAT_BC2_UNORM:
                case DXGI_FORMAT_BC2_UNORM_SRGB:
                case DXGI_FORMAT_BC3_UNORM:
                case DXGI_FORMAT_BC3_UNORM_SRGB:
                case DXGI_FORMAT_BC4_UNORM:
                case DXGI_FORMAT_BC4_SNORM:
                case DXGI_FORMAT_BC5_UNORM:
                case DXGI_FORMAT_BC5_SNORM:
                case DXGI_FORMAT_BC6H_UF16:
                case DXGI_FORMAT_BC6H_SF16:
                case DXGI_FORMAT_BC7_UNORM:
                case DXGI_FORMAT_BC7_UNORM_SRGB:
                    return true;

                default:
                    return false;
            }
        }

        [[nodiscard]]
        std::size_t GetBlockSize(
            const DXGI_FORMAT format) noexcept
        {
            switch (format)
            {
                case DXGI_FORMAT_BC1_UNORM:
                case DXGI_FORMAT_BC1_UNORM_SRGB:
                case DXGI_FORMAT_BC4_UNORM:
                case DXGI_FORMAT_BC4_SNORM:
                    return 8U;

                case DXGI_FORMAT_BC2_UNORM:
                case DXGI_FORMAT_BC2_UNORM_SRGB:
                case DXGI_FORMAT_BC3_UNORM:
                case DXGI_FORMAT_BC3_UNORM_SRGB:
                case DXGI_FORMAT_BC5_UNORM:
                case DXGI_FORMAT_BC5_SNORM:
                case DXGI_FORMAT_BC6H_UF16:
                case DXGI_FORMAT_BC6H_SF16:
                case DXGI_FORMAT_BC7_UNORM:
                case DXGI_FORMAT_BC7_UNORM_SRGB:
                    return 16U;

                default:
                    return 0U;
            }
        }

        [[nodiscard]]
        bool GetSurfaceInfo(
            const std::uint32_t width,
            const std::uint32_t height,
            const DXGI_FORMAT format,
            std::size_t& rowPitch,
            std::size_t& slicePitch) noexcept
        {
            if (width == 0U || height == 0U)
            {
                return false;
            }

            if (IsBlockCompressed(format))
            {
                const std::size_t blockSize =
                    GetBlockSize(format);

                const std::size_t blockWidth =
                    (std::max)(
                        (static_cast<std::size_t>(
                            width) + 3U) / 4U,
                        1U);

                const std::size_t blockHeight =
                    (std::max)(
                        (static_cast<std::size_t>(
                            height) + 3U) / 4U,
                        1U);

                rowPitch =
                    blockWidth *
                    blockSize;

                slicePitch =
                    rowPitch *
                    blockHeight;

                return true;
            }

            switch (format)
            {
                case DXGI_FORMAT_R8G8B8A8_UNORM:
                case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                case DXGI_FORMAT_B8G8R8A8_UNORM:
                case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                case DXGI_FORMAT_B8G8R8X8_UNORM:
                case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
                    rowPitch =
                        static_cast<std::size_t>(
                            width) * 4U;

                    slicePitch =
                        rowPitch *
                        static_cast<std::size_t>(
                            height);

                    return true;

                default:
                    return false;
            }
        }

        [[nodiscard]]
        DXGI_FORMAT ResolveLegacyDdsFormat(
            const DdsPixelFormat& pixelFormat) noexcept
        {
            if ((pixelFormat.flags &
                 DdsFourCcFlag) != 0U)
            {
                switch (pixelFormat.fourCc)
                {
                    case MakeFourCc('D', 'X', 'T', '1'):
                        return DXGI_FORMAT_BC1_UNORM;

                    case MakeFourCc('D', 'X', 'T', '3'):
                        return DXGI_FORMAT_BC2_UNORM;

                    case MakeFourCc('D', 'X', 'T', '5'):
                        return DXGI_FORMAT_BC3_UNORM;

                    case MakeFourCc('A', 'T', 'I', '1'):
                    case MakeFourCc('B', 'C', '4', 'U'):
                        return DXGI_FORMAT_BC4_UNORM;

                    case MakeFourCc('B', 'C', '4', 'S'):
                        return DXGI_FORMAT_BC4_SNORM;

                    case MakeFourCc('A', 'T', 'I', '2'):
                    case MakeFourCc('B', 'C', '5', 'U'):
                        return DXGI_FORMAT_BC5_UNORM;

                    case MakeFourCc('B', 'C', '5', 'S'):
                        return DXGI_FORMAT_BC5_SNORM;

                    default:
                        return DXGI_FORMAT_UNKNOWN;
                }
            }

            if ((pixelFormat.flags &
                 DdsRgbFlag) == 0U ||
                pixelFormat.rgbBitCount != 32U)
            {
                return DXGI_FORMAT_UNKNOWN;
            }

            if (pixelFormat.redMask == 0x000000FFU &&
                pixelFormat.greenMask == 0x0000FF00U &&
                pixelFormat.blueMask == 0x00FF0000U)
            {
                return DXGI_FORMAT_R8G8B8A8_UNORM;
            }

            if (pixelFormat.redMask == 0x00FF0000U &&
                pixelFormat.greenMask == 0x0000FF00U &&
                pixelFormat.blueMask == 0x000000FFU)
            {
                return pixelFormat.alphaMask != 0U
                    ? DXGI_FORMAT_B8G8R8A8_UNORM
                    : DXGI_FORMAT_B8G8R8X8_UNORM;
            }

            return DXGI_FORMAT_UNKNOWN;
        }

        [[nodiscard]]
        bool LoadDdsTexture(
            ID3D11Device& device,
            const std::filesystem::path& path,
            const bool useSrgb,
            ComPtr<ID3D11ShaderResourceView>& output,
            std::string& error) noexcept
        {
            output.Reset();

            if (!IsRegularFile(path))
            {
                error =
                    "DDS file does not exist: " +
                    path.generic_u8string();

                return false;
            }

            std::vector<std::byte> bytes;

            if (!ReadFileBytes(
                    path,
                    bytes,
                    error))
            {
                return false;
            }

            if (bytes.size() <
                sizeof(std::uint32_t) +
                sizeof(DdsHeader))
            {
                error =
                    "DDS file is too small.";

                return false;
            }

            std::size_t offset = 0U;

            std::uint32_t magic = 0U;

            std::memcpy(
                &magic,
                bytes.data(),
                sizeof(magic));

            offset += sizeof(magic);

            if (magic != DdsMagic)
            {
                error =
                    "Invalid DDS magic.";

                return false;
            }

            DdsHeader header;

            std::memcpy(
                &header,
                bytes.data() + offset,
                sizeof(header));

            offset += sizeof(header);

            if (header.size != sizeof(DdsHeader) ||
                header.pixelFormat.size !=
                    sizeof(DdsPixelFormat) ||
                header.width == 0U ||
                header.height == 0U)
            {
                error =
                    "Invalid DDS header.";

                return false;
            }

            DXGI_FORMAT format =
                DXGI_FORMAT_UNKNOWN;

            std::uint32_t arraySize = 1U;

            if (header.pixelFormat.fourCc ==
                MakeFourCc(
                    'D',
                    'X',
                    '1',
                    '0'))
            {
                if (bytes.size() <
                    offset +
                    sizeof(DdsHeaderDx10))
                {
                    error =
                        "DDS DX10 header is missing.";

                    return false;
                }

                DdsHeaderDx10 dx10;

                std::memcpy(
                    &dx10,
                    bytes.data() + offset,
                    sizeof(dx10));

                offset += sizeof(dx10);

                if (dx10.arraySize != 1U)
                {
                    error =
                        "DDS arrays are not supported by the preview.";

                    return false;
                }

                arraySize = dx10.arraySize;

                format =
                    NormalizeDxgiFormat(
                        static_cast<DXGI_FORMAT>(
                            dx10.dxgiFormat));
            }
            else
            {
                format =
                    ResolveLegacyDdsFormat(
                        header.pixelFormat);
            }

            if (format == DXGI_FORMAT_UNKNOWN ||
                arraySize != 1U)
            {
                error =
                    "Unsupported DDS format.";

                return false;
            }

            if (useSrgb)
            {
                format =
                    MakeSrgbFormat(format);
            }

            const std::uint32_t mipCount =
                (std::max)(
                    header.mipMapCount,
                    1U);

            std::vector<D3D11_SUBRESOURCE_DATA>
                subresources;

            subresources.reserve(mipCount);

            std::uint32_t mipWidth =
                header.width;

            std::uint32_t mipHeight =
                header.height;

            for (std::uint32_t mipIndex = 0U;
                 mipIndex < mipCount;
                 ++mipIndex)
            {
                std::size_t rowPitch = 0U;
                std::size_t slicePitch = 0U;

                if (!GetSurfaceInfo(
                        mipWidth,
                        mipHeight,
                        format,
                        rowPitch,
                        slicePitch) ||
                    rowPitch >
                        static_cast<std::size_t>(
                            (std::numeric_limits<UINT>::max)()) ||
                    slicePitch >
                        static_cast<std::size_t>(
                            (std::numeric_limits<UINT>::max)()) ||
                    offset > bytes.size() ||
                    slicePitch >
                        bytes.size() - offset)
                {
                    error =
                        "DDS mip data is invalid or truncated.";

                    return false;
                }

                D3D11_SUBRESOURCE_DATA data{};
                data.pSysMem =
                    bytes.data() + offset;

                data.SysMemPitch =
                    static_cast<UINT>(
                        rowPitch);

                data.SysMemSlicePitch =
                    static_cast<UINT>(
                        slicePitch);

                subresources.push_back(data);

                offset += slicePitch;

                mipWidth =
                    (std::max)(
                        mipWidth / 2U,
                        1U);

                mipHeight =
                    (std::max)(
                        mipHeight / 2U,
                        1U);
            }

            D3D11_TEXTURE2D_DESC description{};
            description.Width = header.width;
            description.Height = header.height;
            description.MipLevels = mipCount;
            description.ArraySize = 1U;
            description.Format = format;
            description.SampleDesc.Count = 1U;
            description.Usage = D3D11_USAGE_IMMUTABLE;
            description.BindFlags =
                D3D11_BIND_SHADER_RESOURCE;

            ComPtr<ID3D11Texture2D> texture;

            HRESULT result =
                device.CreateTexture2D(
                    &description,
                    subresources.data(),
                    texture.GetAddressOf());

            if (FAILED(result))
            {
                error =
                    "ID3D11Device::CreateTexture2D failed.";

                return false;
            }

            result =
                device.CreateShaderResourceView(
                    texture.Get(),
                    nullptr,
                    output.GetAddressOf());

            if (FAILED(result))
            {
                error =
                    "ID3D11Device::CreateShaderResourceView failed.";

                output.Reset();
                return false;
            }

            return true;
        }

        [[nodiscard]]
        bool CreateSolidTexture(
            ID3D11Device& device,
            const std::array<std::uint8_t, 4U>& color,
            ComPtr<ID3D11ShaderResourceView>& output) noexcept
        {
            D3D11_TEXTURE2D_DESC description{};
            description.Width = 1U;
            description.Height = 1U;
            description.MipLevels = 1U;
            description.ArraySize = 1U;
            description.Format =
                DXGI_FORMAT_R8G8B8A8_UNORM;

            description.SampleDesc.Count = 1U;
            description.Usage = D3D11_USAGE_IMMUTABLE;
            description.BindFlags =
                D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA initialData{};
            initialData.pSysMem = color.data();
            initialData.SysMemPitch = 4U;
            initialData.SysMemSlicePitch = 4U;

            ComPtr<ID3D11Texture2D> texture;

            if (FAILED(
                    device.CreateTexture2D(
                        &description,
                        &initialData,
                        texture.GetAddressOf())))
            {
                return false;
            }

            return SUCCEEDED(
                device.CreateShaderResourceView(
                    texture.Get(),
                    nullptr,
                    output.GetAddressOf()));
        }

        [[nodiscard]]
        DirectX::XMFLOAT3 GetBonePosition(
            const LegacyBone& bone) noexcept
        {
            return
            {
                bone.absoluteBindMatrix[12U],
                bone.absoluteBindMatrix[13U],
                bone.absoluteBindMatrix[14U]
            };
        }

        [[nodiscard]]
        bool ProjectPoint(
            const DirectX::XMFLOAT3& point,
            const DirectX::XMMATRIX& viewProjection,
            const ImVec2& minimum,
            const ImVec2& size,
            ImVec2& output) noexcept
        {
            const DirectX::XMVECTOR position =
                DirectX::XMVectorSet(
                    point.x,
                    point.y,
                    point.z,
                    1.0F);

            const DirectX::XMVECTOR clip =
                DirectX::XMVector4Transform(
                    position,
                    viewProjection);

            const float w =
                DirectX::XMVectorGetW(clip);

            if (!std::isfinite(w) ||
                w <= 0.00001F)
            {
                return false;
            }

            const float inverseW =
                1.0F / w;

            const float x =
                DirectX::XMVectorGetX(clip) *
                inverseW;

            const float y =
                DirectX::XMVectorGetY(clip) *
                inverseW;

            if (!std::isfinite(x) ||
                !std::isfinite(y))
            {
                return false;
            }

            output =
            {
                minimum.x +
                    (x * 0.5F + 0.5F) *
                    size.x,

                minimum.y +
                    (-y * 0.5F + 0.5F) *
                    size.y
            };

            return true;
        }

        [[nodiscard]]
        CameraState BuildCamera(
            const std::array<float, 3U>& target,
            const float yaw,
            const float pitch,
            const float distance,
            const float radius,
            const float aspectRatio) noexcept
        {
            const float cosinePitch =
                std::cos(pitch);

            DirectX::XMVECTOR forward =
                DirectX::XMVectorSet(
                    cosinePitch *
                        std::sin(yaw),

                    -std::sin(pitch),

                    cosinePitch *
                        std::cos(yaw),

                    0.0F);

            forward =
                DirectX::XMVector3Normalize(
                    forward);

            const DirectX::XMVECTOR targetVector =
                DirectX::XMVectorSet(
                    target[0],
                    target[1],
                    target[2],
                    1.0F);

            const DirectX::XMVECTOR camera =
                DirectX::XMVectorSubtract(
                    targetVector,
                    DirectX::XMVectorScale(
                        forward,
                        distance));

            const DirectX::XMVECTOR worldUp =
                DirectX::XMVectorSet(
                    0.0F,
                    1.0F,
                    0.0F,
                    0.0F);

            DirectX::XMVECTOR right =
                DirectX::XMVector3Normalize(
                    DirectX::XMVector3Cross(
                        worldUp,
                        forward));

            DirectX::XMVECTOR up =
                DirectX::XMVector3Normalize(
                    DirectX::XMVector3Cross(
                        forward,
                        right));

            const DirectX::XMMATRIX view =
                DirectX::XMMatrixLookAtLH(
                    camera,
                    targetVector,
                    up);

            const float nearPlane =
                (std::max)(
                    radius * 0.0025F,
                    0.001F);

            const float farPlane =
                (std::max)(
                    distance +
                        radius * 20.0F,
                    nearPlane + 100.0F);

            const DirectX::XMMATRIX projection =
                DirectX::XMMatrixPerspectiveFovLH(
                    PreviewFieldOfView,
                    (std::max)(
                        aspectRatio,
                        0.01F),
                    nearPlane,
                    farPlane);

            CameraState result;
            result.viewProjection =
                view *
                projection;

            DirectX::XMStoreFloat3(
                &result.cameraPosition,
                camera);

            DirectX::XMStoreFloat3(
                &result.right,
                right);

            DirectX::XMStoreFloat3(
                &result.up,
                up);

            return result;
        }
    }

    class LegacyMeshPreview::Impl final
    {
    public:
        void Initialize(
            ID3D11Device* const device,
            ID3D11DeviceContext* const context) noexcept
        {
            Shutdown();

            if (device == nullptr ||
                context == nullptr)
            {
                error_ =
                    "D3D11 preview received an invalid device.";

                return;
            }

            device_ = device;
            context_ = context;

            ComPtr<ID3DBlob> vertexBytecode;
            ComPtr<ID3DBlob> pixelBytecode;

            if (!CompileEditorShaderFile(
                    L"WarZSkeletalPreview.hlsl",
                    "VSMain",
                    "vs_5_0",
                    "LTS.Editor.WarZPreview",
                    vertexBytecode) ||
                !CompileEditorShaderFile(
                    L"WarZSkeletalPreview.hlsl",
                    "PSMain",
                    "ps_5_0",
                    "LTS.Editor.WarZPreview",
                    pixelBytecode))
            {
                error_ =
                    "Failed to compile WarZSkeletalPreview.hlsl.";

                ShutdownResources();
                return;
            }

            if (FAILED(
                    device_->CreateVertexShader(
                        vertexBytecode->
                            GetBufferPointer(),
                        vertexBytecode->
                            GetBufferSize(),
                        nullptr,
                        vertexShader_.
                            GetAddressOf())) ||
                FAILED(
                    device_->CreatePixelShader(
                        pixelBytecode->
                            GetBufferPointer(),
                        pixelBytecode->
                            GetBufferSize(),
                        nullptr,
                        pixelShader_.
                            GetAddressOf())))
            {
                error_ =
                    "Failed to create preview shaders.";

                ShutdownResources();
                return;
            }

            constexpr D3D11_INPUT_ELEMENT_DESC
                inputElements[]
            {
                {
                    "POSITION",
                    0U,
                    DXGI_FORMAT_R32G32B32_FLOAT,
                    0U,
                    0U,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0U
                },
                {
                    "NORMAL",
                    0U,
                    DXGI_FORMAT_R32G32B32_FLOAT,
                    0U,
                    12U,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0U
                },
                {
                    "TANGENT",
                    0U,
                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                    0U,
                    24U,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0U
                },
                {
                    "TEXCOORD",
                    0U,
                    DXGI_FORMAT_R32G32_FLOAT,
                    0U,
                    40U,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0U
                }
            };

            if (FAILED(
                    device_->CreateInputLayout(
                        inputElements,
                        static_cast<UINT>(
                            std::size(
                                inputElements)),
                        vertexBytecode->
                            GetBufferPointer(),
                        vertexBytecode->
                            GetBufferSize(),
                        inputLayout_.
                            GetAddressOf())))
            {
                error_ =
                    "Failed to create preview input layout.";

                ShutdownResources();
                return;
            }

            D3D11_BUFFER_DESC constantDescription{};
            constantDescription.ByteWidth =
                static_cast<UINT>(
                    sizeof(
                        PreviewConstants));

            constantDescription.Usage =
                D3D11_USAGE_DYNAMIC;

            constantDescription.BindFlags =
                D3D11_BIND_CONSTANT_BUFFER;

            constantDescription.CPUAccessFlags =
                D3D11_CPU_ACCESS_WRITE;

            if (FAILED(
                    device_->CreateBuffer(
                        &constantDescription,
                        nullptr,
                        constantBuffer_.
                            GetAddressOf())))
            {
                error_ =
                    "Failed to create preview constant buffer.";

                ShutdownResources();
                return;
            }

            D3D11_RASTERIZER_DESC rasterizer{};
            rasterizer.FillMode =
                D3D11_FILL_SOLID;

            rasterizer.CullMode =
                D3D11_CULL_NONE;

            rasterizer.DepthClipEnable = TRUE;

            if (FAILED(
                    device_->CreateRasterizerState(
                        &rasterizer,
                        solidRasterizer_.
                            GetAddressOf())))
            {
                error_ =
                    "Failed to create preview rasterizer state.";

                ShutdownResources();
                return;
            }

            rasterizer.FillMode =
                D3D11_FILL_WIREFRAME;

            if (FAILED(
                    device_->CreateRasterizerState(
                        &rasterizer,
                        wireRasterizer_.
                            GetAddressOf())))
            {
                error_ =
                    "Failed to create wireframe rasterizer state.";

                ShutdownResources();
                return;
            }

            D3D11_DEPTH_STENCIL_DESC depth{};
            depth.DepthEnable = TRUE;
            depth.DepthWriteMask =
                D3D11_DEPTH_WRITE_MASK_ALL;

            depth.DepthFunc =
                D3D11_COMPARISON_LESS_EQUAL;

            if (FAILED(
                    device_->CreateDepthStencilState(
                        &depth,
                        depthState_.
                            GetAddressOf())))
            {
                error_ =
                    "Failed to create preview depth state.";

                ShutdownResources();
                return;
            }

            D3D11_BLEND_DESC blend{};
            blend.RenderTarget[0].
                RenderTargetWriteMask =
                    D3D11_COLOR_WRITE_ENABLE_ALL;

            if (FAILED(
                    device_->CreateBlendState(
                        &blend,
                        blendState_.
                            GetAddressOf())))
            {
                error_ =
                    "Failed to create preview blend state.";

                ShutdownResources();
                return;
            }

            D3D11_SAMPLER_DESC sampler{};
            sampler.Filter =
                D3D11_FILTER_MIN_MAG_MIP_LINEAR;

            sampler.AddressU =
                D3D11_TEXTURE_ADDRESS_WRAP;

            sampler.AddressV =
                D3D11_TEXTURE_ADDRESS_WRAP;

            sampler.AddressW =
                D3D11_TEXTURE_ADDRESS_WRAP;

            sampler.MaxAnisotropy = 1U;
            sampler.ComparisonFunc =
                D3D11_COMPARISON_NEVER;

            sampler.MinLOD = 0.0F;
            sampler.MaxLOD =
                D3D11_FLOAT32_MAX;

            if (FAILED(
                    device_->CreateSamplerState(
                        &sampler,
                        sampler_.
                            GetAddressOf())) ||
                !CreateSolidTexture(
                    *device_,
                    {255U, 255U, 255U, 255U},
                    whiteTexture_) ||
                !CreateSolidTexture(
                    *device_,
                    {128U, 128U, 255U, 255U},
                    flatNormalTexture_) ||
                !CreateSolidTexture(
                    *device_,
                    {0U, 0U, 0U, 255U},
                    blackTexture_) ||
                !CreateSolidTexture(
                    *device_,
                    {180U, 180U, 180U, 255U},
                    roughnessTexture_))
            {
                error_ =
                    "Failed to create preview textures or sampler.";

                ShutdownResources();
                return;
            }

            initialized_ = true;
            error_.clear();
        }

        void Shutdown() noexcept
        {
            ResetAsset();

            renderTargetView_.Reset();
            renderTargetSrv_.Reset();
            renderTarget_.Reset();

            depthStencilView_.Reset();
            depthStencil_.Reset();

            targetWidth_ = 0U;
            targetHeight_ = 0U;

            ShutdownResources();

            context_.Reset();
            device_.Reset();

            initialized_ = false;
        }

        void ResetAsset() noexcept
        {
            materials_.clear();

            indexBuffer_.Reset();
            vertexBuffer_.Reset();

            cachedSource_.clear();
            cachedVertexCount_ = 0U;
            cachedIndexCount_ = 0U;

            warning_.clear();
        }

        [[nodiscard]]
        bool IsInitialized() const noexcept
        {
            return initialized_;
        }

        [[nodiscard]]
        ID3D11ShaderResourceView*
            GetRenderTargetSrv() const noexcept
        {
            return renderTargetSrv_.Get();
        }

        [[nodiscard]]
        const std::string& GetError() const noexcept
        {
            return error_;
        }

        [[nodiscard]]
        const std::string& GetWarning() const noexcept
        {
            return warning_;
        }

        [[nodiscard]]
        bool Render(
            const LegacyMeshData& mesh,
            const LegacyMaterialSet* materials,
            const DirectX::XMMATRIX& viewProjection,
            const DirectX::XMFLOAT3& cameraPosition,
            const std::uint32_t width,
            const std::uint32_t height,
            const bool wireframe) noexcept
        {
            if (!initialized_ ||
                device_ == nullptr ||
                context_ == nullptr)
            {
                return false;
            }

            if (!EnsureRenderTarget(
                    width,
                    height) ||
                !EnsureAsset(
                    mesh,
                    materials))
            {
                return false;
            }

            ID3D11RenderTargetView* renderTargets[]
            {
                renderTargetView_.Get()
            };

            context_->OMSetRenderTargets(
                1U,
                renderTargets,
                depthStencilView_.Get());

            constexpr float clearColor[]
            {
                0.045F,
                0.055F,
                0.068F,
                1.0F
            };

            context_->ClearRenderTargetView(
                renderTargetView_.Get(),
                clearColor);

            context_->ClearDepthStencilView(
                depthStencilView_.Get(),
                D3D11_CLEAR_DEPTH |
                    D3D11_CLEAR_STENCIL,
                1.0F,
                0U);

            D3D11_VIEWPORT viewport{};
            viewport.Width =
                static_cast<float>(width);

            viewport.Height =
                static_cast<float>(height);

            viewport.MinDepth = 0.0F;
            viewport.MaxDepth = 1.0F;

            context_->RSSetViewports(
                1U,
                &viewport);

            context_->RSSetState(
                wireframe
                    ? wireRasterizer_.Get()
                    : solidRasterizer_.Get());

            context_->OMSetDepthStencilState(
                depthState_.Get(),
                0U);

            constexpr float blendFactor[]
            {
                0.0F,
                0.0F,
                0.0F,
                0.0F
            };

            context_->OMSetBlendState(
                blendState_.Get(),
                blendFactor,
                0xFFFFFFFFU);

            const UINT stride =
                static_cast<UINT>(
                    sizeof(
                        PreviewVertex));

            constexpr UINT offset = 0U;

            ID3D11Buffer* vertexBuffers[]
            {
                vertexBuffer_.Get()
            };

            context_->IASetVertexBuffers(
                0U,
                1U,
                vertexBuffers,
                &stride,
                &offset);

            context_->IASetIndexBuffer(
                indexBuffer_.Get(),
                DXGI_FORMAT_R32_UINT,
                0U);

            context_->IASetInputLayout(
                inputLayout_.Get());

            context_->IASetPrimitiveTopology(
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            context_->VSSetShader(
                vertexShader_.Get(),
                nullptr,
                0U);

            context_->PSSetShader(
                pixelShader_.Get(),
                nullptr,
                0U);

            ID3D11Buffer* constantBuffers[]
            {
                constantBuffer_.Get()
            };

            context_->VSSetConstantBuffers(
                0U,
                1U,
                constantBuffers);

            context_->PSSetConstantBuffers(
                0U,
                1U,
                constantBuffers);

            ID3D11SamplerState* samplers[]
            {
                sampler_.Get()
            };

            context_->PSSetSamplers(
                0U,
                1U,
                samplers);

            bool drewChunk = false;

            if (!mesh.materialChunks.empty())
            {
                for (std::size_t chunkIndex = 0U;
                     chunkIndex <
                         mesh.materialChunks.size();
                     ++chunkIndex)
                {
                    const LegacyMaterialChunk& chunk =
                        mesh.materialChunks[
                            chunkIndex];

                    if (chunk.firstIndex >=
                        mesh.indices.size())
                    {
                        continue;
                    }

                    std::size_t indexCount =
                        (std::min)(
                            static_cast<std::size_t>(
                                chunk.indexCount),
                            mesh.indices.size() -
                                chunk.firstIndex);

                    indexCount -= indexCount % 3U;

                    if (indexCount == 0U)
                    {
                        continue;
                    }

                    const GpuMaterial& material =
                        materials_[
                            (std::min)(
                                chunkIndex,
                                materials_.size() -
                                    1U)];

                    if (!UpdateConstants(
                            viewProjection,
                            cameraPosition,
                            material))
                    {
                        return false;
                    }

                    BindMaterial(material);

                    context_->DrawIndexed(
                        static_cast<UINT>(
                            indexCount),
                        chunk.firstIndex,
                        0);

                    drewChunk = true;
                }
            }

            if (!drewChunk)
            {
                const GpuMaterial& material =
                    materials_.front();

                if (!UpdateConstants(
                        viewProjection,
                        cameraPosition,
                        material))
                {
                    return false;
                }

                BindMaterial(material);

                const std::size_t indexCount =
                    mesh.indices.size() -
                    mesh.indices.size() % 3U;

                context_->DrawIndexed(
                    static_cast<UINT>(
                        indexCount),
                    0U,
                    0);
            }

            ID3D11ShaderResourceView* nullTextures[4]{};

            context_->PSSetShaderResources(
                0U,
                4U,
                nullTextures);

            context_->OMSetRenderTargets(
                0U,
                nullptr,
                nullptr);

            return true;
        }

    private:
        void ShutdownResources() noexcept
        {
            roughnessTexture_.Reset();
            blackTexture_.Reset();
            flatNormalTexture_.Reset();
            whiteTexture_.Reset();

            sampler_.Reset();
            blendState_.Reset();
            depthState_.Reset();
            wireRasterizer_.Reset();
            solidRasterizer_.Reset();

            constantBuffer_.Reset();
            inputLayout_.Reset();
            pixelShader_.Reset();
            vertexShader_.Reset();
        }

        [[nodiscard]]
        bool EnsureRenderTarget(
            const std::uint32_t width,
            const std::uint32_t height) noexcept
        {
            if (renderTargetSrv_ != nullptr &&
                width == targetWidth_ &&
                height == targetHeight_)
            {
                return true;
            }

            renderTargetView_.Reset();
            renderTargetSrv_.Reset();
            renderTarget_.Reset();

            depthStencilView_.Reset();
            depthStencil_.Reset();

            D3D11_TEXTURE2D_DESC color{};
            color.Width = width;
            color.Height = height;
            color.MipLevels = 1U;
            color.ArraySize = 1U;
            color.Format =
                DXGI_FORMAT_R8G8B8A8_UNORM;

            color.SampleDesc.Count = 1U;
            color.Usage = D3D11_USAGE_DEFAULT;
            color.BindFlags =
                D3D11_BIND_RENDER_TARGET |
                D3D11_BIND_SHADER_RESOURCE;

            if (FAILED(
                    device_->CreateTexture2D(
                        &color,
                        nullptr,
                        renderTarget_.
                            GetAddressOf())) ||
                FAILED(
                    device_->CreateRenderTargetView(
                        renderTarget_.Get(),
                        nullptr,
                        renderTargetView_.
                            GetAddressOf())) ||
                FAILED(
                    device_->CreateShaderResourceView(
                        renderTarget_.Get(),
                        nullptr,
                        renderTargetSrv_.
                            GetAddressOf())))
            {
                error_ =
                    "Failed to create preview render target.";

                return false;
            }

            D3D11_TEXTURE2D_DESC depth{};
            depth.Width = width;
            depth.Height = height;
            depth.MipLevels = 1U;
            depth.ArraySize = 1U;
            depth.Format =
                DXGI_FORMAT_D24_UNORM_S8_UINT;

            depth.SampleDesc.Count = 1U;
            depth.Usage = D3D11_USAGE_DEFAULT;
            depth.BindFlags =
                D3D11_BIND_DEPTH_STENCIL;

            if (FAILED(
                    device_->CreateTexture2D(
                        &depth,
                        nullptr,
                        depthStencil_.
                            GetAddressOf())) ||
                FAILED(
                    device_->CreateDepthStencilView(
                        depthStencil_.Get(),
                        nullptr,
                        depthStencilView_.
                            GetAddressOf())))
            {
                error_ =
                    "Failed to create preview depth target.";

                return false;
            }

            targetWidth_ = width;
            targetHeight_ = height;

            return true;
        }

        [[nodiscard]]
        bool EnsureAsset(
            const LegacyMeshData& mesh,
            const LegacyMaterialSet* materialSet) noexcept
        {
            if (vertexBuffer_ != nullptr &&
                indexBuffer_ != nullptr &&
                cachedSource_ ==
                    mesh.sourcePath &&
                cachedVertexCount_ ==
                    mesh.vertices.size() &&
                cachedIndexCount_ ==
                    mesh.indices.size())
            {
                return true;
            }

            ResetAsset();

            if (mesh.vertices.empty() ||
                mesh.indices.empty() ||
                mesh.vertices.size() >
                    static_cast<std::size_t>(
                        (std::numeric_limits<UINT>::max)()) /
                    sizeof(PreviewVertex) ||
                mesh.indices.size() >
                    static_cast<std::size_t>(
                        (std::numeric_limits<UINT>::max)()) /
                    sizeof(std::uint32_t))
            {
                error_ =
                    "Preview mesh is empty or too large.";

                return false;
            }

            std::vector<PreviewVertex> vertices;
            vertices.reserve(
                mesh.vertices.size());

            for (const LegacyMeshVertex& source :
                 mesh.vertices)
            {
                PreviewVertex vertex;

                vertex.position =
                {
                    source.position[0],
                    source.position[1],
                    source.position[2]
                };

                vertex.normal =
                {
                    source.normal[0],
                    source.normal[1],
                    source.normal[2]
                };

                vertex.tangent =
                {
                    source.tangent[0],
                    source.tangent[1],
                    source.tangent[2],
                    source.tangentSign
                };

                vertex.uv =
                {
                    source.uv[0],
                    source.uv[1]
                };

                vertices.push_back(vertex);
            }

            D3D11_BUFFER_DESC vertexDescription{};
            vertexDescription.ByteWidth =
                static_cast<UINT>(
                    vertices.size() *
                    sizeof(PreviewVertex));

            vertexDescription.Usage =
                D3D11_USAGE_IMMUTABLE;

            vertexDescription.BindFlags =
                D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA vertexData{};
            vertexData.pSysMem =
                vertices.data();

            if (FAILED(
                    device_->CreateBuffer(
                        &vertexDescription,
                        &vertexData,
                        vertexBuffer_.
                            GetAddressOf())))
            {
                error_ =
                    "Failed to upload preview vertices.";

                return false;
            }

            D3D11_BUFFER_DESC indexDescription{};
            indexDescription.ByteWidth =
                static_cast<UINT>(
                    mesh.indices.size() *
                    sizeof(std::uint32_t));

            indexDescription.Usage =
                D3D11_USAGE_IMMUTABLE;

            indexDescription.BindFlags =
                D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA indexData{};
            indexData.pSysMem =
                mesh.indices.data();

            if (FAILED(
                    device_->CreateBuffer(
                        &indexDescription,
                        &indexData,
                        indexBuffer_.
                            GetAddressOf())))
            {
                error_ =
                    "Failed to upload preview indices.";

                vertexBuffer_.Reset();
                return false;
            }

            const std::size_t materialCount =
                (std::max)(
                    mesh.materialChunks.size(),
                    static_cast<std::size_t>(1U));

            materials_.resize(materialCount);

            for (std::size_t materialIndex = 0U;
                 materialIndex < materialCount;
                 ++materialIndex)
            {
                const LegacyMaterialData* source = nullptr;

                if (materialSet != nullptr &&
                    materialIndex <
                        mesh.materialChunks.size())
                {
                    source =
                        materialSet->Find(
                            mesh.materialChunks[
                                materialIndex].
                                materialName);
                }

                BuildMaterial(
                    source,
                    materials_[
                        materialIndex]);
            }

            cachedSource_ =
                mesh.sourcePath;

            cachedVertexCount_ =
                mesh.vertices.size();

            cachedIndexCount_ =
                mesh.indices.size();

            error_.clear();
            return true;
        }

        void BuildMaterial(
            const LegacyMaterialData* const source,
            GpuMaterial& output) noexcept
        {
            output = {};

            if (source == nullptr)
            {
                return;
            }

            output.baseColor =
            {
                source->diffuseColor[0],
                source->diffuseColor[1],
                source->diffuseColor[2],
                1.0F
            };

            output.parameters =
            {
                source->specularPower,
                source->specularPower1,
                source->reflectionPower,
                source->selfIlluminationMultiplier
            };

            LoadMaterialTexture(
                *source,
                LegacyTextureSlot::Diffuse,
                true,
                output.diffuse,
                output.textureFlags.x);

            LoadMaterialTexture(
                *source,
                LegacyTextureSlot::Normal,
                false,
                output.normal,
                output.textureFlags.y);

            LoadMaterialTexture(
                *source,
                LegacyTextureSlot::Specular,
                false,
                output.specular,
                output.textureFlags.z);

            LoadMaterialTexture(
                *source,
                LegacyTextureSlot::Roughness,
                false,
                output.roughness,
                output.textureFlags.w);
        }

        void LoadMaterialTexture(
            const LegacyMaterialData& material,
            const LegacyTextureSlot slot,
            const bool srgb,
            ComPtr<ID3D11ShaderResourceView>& output,
            float& loadedFlag) noexcept
        {
            loadedFlag = 0.0F;
            output.Reset();

            const LegacyMaterialTexture* texture =
                material.FindTexture(slot);

            if (texture == nullptr ||
                !texture->dds.valid ||
                texture->dds.path.empty())
            {
                return;
            }

            std::string textureError;

            if (LoadDdsTexture(
                    *device_,
                    texture->dds.path,
                    srgb,
                    output,
                    textureError))
            {
                loadedFlag = 1.0F;
                return;
            }

            if (!textureError.empty())
            {
                if (!warning_.empty())
                {
                    warning_ += '\n';
                }

                warning_ +=
                    material.name +
                    " / " +
                    ToString(slot) +
                    ": " +
                    textureError;
            }
        }

        [[nodiscard]]
        bool UpdateConstants(
            const DirectX::XMMATRIX& viewProjection,
            const DirectX::XMFLOAT3& cameraPosition,
            const GpuMaterial& material) noexcept
        {
            D3D11_MAPPED_SUBRESOURCE mapped{};

            if (FAILED(
                    context_->Map(
                        constantBuffer_.Get(),
                        0U,
                        D3D11_MAP_WRITE_DISCARD,
                        0U,
                        &mapped)))
            {
                error_ =
                    "Failed to update preview constants.";

                return false;
            }

            PreviewConstants constants{};

            DirectX::XMStoreFloat4x4(
                &constants.viewProjection,
                DirectX::XMMatrixTranspose(
                    viewProjection));

            constants.cameraPosition =
            {
                cameraPosition.x,
                cameraPosition.y,
                cameraPosition.z,
                1.0F
            };

            DirectX::XMVECTOR light =
                DirectX::XMVector3Normalize(
                    DirectX::XMVectorSet(
                        -0.45F,
                        0.80F,
                        -0.30F,
                        0.0F));

            DirectX::XMFLOAT3 lightDirection;

            DirectX::XMStoreFloat3(
                &lightDirection,
                light);

            constants.lightDirectionIntensity =
            {
                lightDirection.x,
                lightDirection.y,
                lightDirection.z,
                1.0F
            };

            constants.lightColor =
            {
                1.0F,
                0.97F,
                0.90F,
                1.0F
            };

            constants.ambientColor =
            {
                0.20F,
                0.23F,
                0.28F,
                1.0F
            };

            constants.baseColor =
                material.baseColor;

            constants.materialParameters =
                material.parameters;

            constants.textureFlags =
                material.textureFlags;

            std::memcpy(
                mapped.pData,
                &constants,
                sizeof(constants));

            context_->Unmap(
                constantBuffer_.Get(),
                0U);

            return true;
        }

        void BindMaterial(
            const GpuMaterial& material) noexcept
        {
            ID3D11ShaderResourceView* textures[]
            {
                material.diffuse != nullptr
                    ? material.diffuse.Get()
                    : whiteTexture_.Get(),

                material.normal != nullptr
                    ? material.normal.Get()
                    : flatNormalTexture_.Get(),

                material.specular != nullptr
                    ? material.specular.Get()
                    : blackTexture_.Get(),

                material.roughness != nullptr
                    ? material.roughness.Get()
                    : roughnessTexture_.Get()
            };

            context_->PSSetShaderResources(
                0U,
                static_cast<UINT>(
                    std::size(textures)),
                textures);
        }

        ComPtr<ID3D11Device> device_;
        ComPtr<ID3D11DeviceContext> context_;

        ComPtr<ID3D11VertexShader> vertexShader_;
        ComPtr<ID3D11PixelShader> pixelShader_;
        ComPtr<ID3D11InputLayout> inputLayout_;

        ComPtr<ID3D11Buffer> constantBuffer_;
        ComPtr<ID3D11Buffer> vertexBuffer_;
        ComPtr<ID3D11Buffer> indexBuffer_;

        ComPtr<ID3D11RasterizerState> solidRasterizer_;
        ComPtr<ID3D11RasterizerState> wireRasterizer_;
        ComPtr<ID3D11DepthStencilState> depthState_;
        ComPtr<ID3D11BlendState> blendState_;
        ComPtr<ID3D11SamplerState> sampler_;

        ComPtr<ID3D11ShaderResourceView> whiteTexture_;
        ComPtr<ID3D11ShaderResourceView> flatNormalTexture_;
        ComPtr<ID3D11ShaderResourceView> blackTexture_;
        ComPtr<ID3D11ShaderResourceView> roughnessTexture_;

        ComPtr<ID3D11Texture2D> renderTarget_;
        ComPtr<ID3D11RenderTargetView> renderTargetView_;
        ComPtr<ID3D11ShaderResourceView> renderTargetSrv_;

        ComPtr<ID3D11Texture2D> depthStencil_;
        ComPtr<ID3D11DepthStencilView> depthStencilView_;

        std::vector<GpuMaterial> materials_;

        std::filesystem::path cachedSource_;

        std::size_t cachedVertexCount_ = 0U;
        std::size_t cachedIndexCount_ = 0U;

        std::uint32_t targetWidth_ = 0U;
        std::uint32_t targetHeight_ = 0U;

        std::string error_;
        std::string warning_;

        bool initialized_ = false;
    };

    LegacyMeshPreview::LegacyMeshPreview()
        : impl_(std::make_unique<Impl>())
    {
    }

    LegacyMeshPreview::~LegacyMeshPreview() noexcept
    {
        Shutdown();
    }

    void LegacyMeshPreview::Initialize(
        ID3D11Device* const device,
        ID3D11DeviceContext* const context) noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->Initialize(
                device,
                context);
        }
    }

    void LegacyMeshPreview::Shutdown() noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->Shutdown();
        }
    }

    void LegacyMeshPreview::Reset() noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->ResetAsset();
        }

        framedSource_.clear();
        framedVertexCount_ = 0U;

        target_ =
        {
            0.0F,
            0.0F,
            0.0F
        };

        yaw_ = 0.65F;
        pitch_ = 0.25F;
        distance_ = 3.0F;
        radius_ = 1.0F;

        framed_ = false;
    }

    void LegacyMeshPreview::Frame(
        const LegacyMeshData& mesh) noexcept
    {
        if (mesh.vertices.empty())
        {
            framed_ = false;
            return;
        }

        std::array<float, 3U> minimum
        {
            (std::numeric_limits<float>::max)(),
            (std::numeric_limits<float>::max)(),
            (std::numeric_limits<float>::max)()
        };

        std::array<float, 3U> maximum
        {
            (std::numeric_limits<float>::lowest)(),
            (std::numeric_limits<float>::lowest)(),
            (std::numeric_limits<float>::lowest)()
        };

        bool foundVertex = false;

        for (const LegacyMeshVertex& vertex :
             mesh.vertices)
        {
            if (!std::isfinite(vertex.position[0]) ||
                !std::isfinite(vertex.position[1]) ||
                !std::isfinite(vertex.position[2]))
            {
                continue;
            }

            foundVertex = true;

            for (std::size_t axis = 0U;
                 axis < 3U;
                 ++axis)
            {
                minimum[axis] =
                    (std::min)(
                        minimum[axis],
                        vertex.position[axis]);

                maximum[axis] =
                    (std::max)(
                        maximum[axis],
                        vertex.position[axis]);
            }
        }

        if (!foundVertex)
        {
            framed_ = false;
            return;
        }

        for (std::size_t axis = 0U;
             axis < 3U;
             ++axis)
        {
            target_[axis] =
                (
                    minimum[axis] +
                    maximum[axis]
                ) *
                0.5F;
        }

        float maximumDistanceSquared = 0.0F;

        for (const LegacyMeshVertex& vertex :
             mesh.vertices)
        {
            const float x =
                vertex.position[0] -
                target_[0];

            const float y =
                vertex.position[1] -
                target_[1];

            const float z =
                vertex.position[2] -
                target_[2];

            const float distanceSquared =
                x * x +
                y * y +
                z * z;

            if (std::isfinite(distanceSquared))
            {
                maximumDistanceSquared =
                    (std::max)(
                        maximumDistanceSquared,
                        distanceSquared);
            }
        }

        radius_ =
            (std::max)(
                std::sqrt(
                    maximumDistanceSquared),
                0.05F);

        distance_ =
            (std::max)(
                radius_ * 2.8F,
                0.25F);

        yaw_ = 0.65F;
        pitch_ = 0.25F;

        framedSource_ =
            mesh.sourcePath;

        framedVertexCount_ =
            mesh.vertices.size();

        framed_ = true;
    }

    void LegacyMeshPreview::Draw(
        const LegacyMeshData& mesh,
        const LegacySkeletonData* const skeleton,
        const LegacyMaterialSet* const materials,
        const float requestedWidth,
        const float requestedHeight,
        const bool showSkeleton,
        const bool wireframe) noexcept
    {
        if (mesh.vertices.empty() ||
            mesh.indices.empty())
        {
            ImGui::TextDisabled(
                "Preview geometry is empty.");

            return;
        }

        if (!framed_ ||
            framedSource_ != mesh.sourcePath ||
            framedVertexCount_ !=
                mesh.vertices.size())
        {
            Frame(mesh);
        }

        const float width =
            (std::max)(
                requestedWidth,
                320.0F);

        const float height =
            (std::max)(
                requestedHeight,
                280.0F);

        const ImVec2 canvasSize
        {
            width,
            height
        };

        ImGui::InvisibleButton(
            "##WarZD3D11Preview",
            canvasSize,
            ImGuiButtonFlags_MouseButtonLeft |
                ImGuiButtonFlags_MouseButtonRight);

        const bool hovered =
            ImGui::IsItemHovered();

        const ImVec2 canvasMinimum =
            ImGui::GetItemRectMin();

        const ImVec2 canvasMaximum =
            ImGui::GetItemRectMax();

        ImGuiIO& input =
            ImGui::GetIO();

        CameraState camera =
            BuildCamera(
                target_,
                yaw_,
                pitch_,
                distance_,
                radius_,
                width / height);

        if (hovered)
        {
            if (ImGui::IsMouseDoubleClicked(
                    ImGuiMouseButton_Left))
            {
                Frame(mesh);
            }

            if (ImGui::IsMouseDragging(
                    ImGuiMouseButton_Left,
                    0.0F))
            {
                yaw_ -=
                    input.MouseDelta.x *
                    0.008F;

                pitch_ -=
                    input.MouseDelta.y *
                    0.008F;

                pitch_ =
                    std::clamp(
                        pitch_,
                        -1.45F,
                        1.45F);
            }

            if (ImGui::IsMouseDragging(
                    ImGuiMouseButton_Right,
                    0.0F))
            {
                const float panScale =
                    distance_ /
                    (std::max)(
                        height,
                        1.0F) *
                    1.4F;

                target_[0] +=
                    camera.right.x *
                        -input.MouseDelta.x *
                        panScale +
                    camera.up.x *
                        input.MouseDelta.y *
                        panScale;

                target_[1] +=
                    camera.right.y *
                        -input.MouseDelta.x *
                        panScale +
                    camera.up.y *
                        input.MouseDelta.y *
                        panScale;

                target_[2] +=
                    camera.right.z *
                        -input.MouseDelta.x *
                        panScale +
                    camera.up.z *
                        input.MouseDelta.y *
                        panScale;
            }

            if (input.MouseWheel != 0.0F)
            {
                distance_ *=
                    std::pow(
                        0.84F,
                        input.MouseWheel);

                distance_ =
                    std::clamp(
                        distance_,
                        radius_ * 0.08F,
                        radius_ * 100.0F);
            }

            ImGui::SetTooltip(
                "LMB: Orbit\n"
                "RMB: Pan\n"
                "Mouse Wheel: Zoom\n"
                "Double-click: Frame");
        }

        camera =
            BuildCamera(
                target_,
                yaw_,
                pitch_,
                distance_,
                radius_,
                width / height);

        const std::uint32_t renderWidth =
            std::clamp(
                static_cast<std::uint32_t>(
                    std::ceil(width)),
                1U,
                MaximumPreviewSize);

        const std::uint32_t renderHeight =
            std::clamp(
                static_cast<std::uint32_t>(
                    std::ceil(height)),
                1U,
                MaximumPreviewSize);

        bool rendered = false;

        if (impl_ != nullptr)
        {
            rendered =
                impl_->Render(
                    mesh,
                    materials,
                    camera.viewProjection,
                    camera.cameraPosition,
                    renderWidth,
                    renderHeight,
                    wireframe);
        }

        ImDrawList& drawList =
            *ImGui::GetWindowDrawList();

        drawList.PushClipRect(
            canvasMinimum,
            canvasMaximum,
            true);

        drawList.AddRectFilled(
            canvasMinimum,
            canvasMaximum,
            IM_COL32(
                18,
                22,
                27,
                255));

        if (rendered &&
            impl_->GetRenderTargetSrv() != nullptr)
        {
            drawList.AddImage(
                (ImTextureID)
                    impl_->
                        GetRenderTargetSrv(),
                canvasMinimum,
                canvasMaximum,
                ImVec2(0.0F, 0.0F),
                ImVec2(1.0F, 1.0F));
        }
        else
        {
            const std::string error =
                impl_ != nullptr
                    ? impl_->GetError()
                    : "Preview renderer is unavailable.";

            drawList.AddText(
                ImVec2(
                    canvasMinimum.x + 12.0F,
                    canvasMinimum.y + 12.0F),
                IM_COL32(
                    245,
                    95,
                    75,
                    255),
                error.empty()
                    ? "D3D11 preview rendering failed."
                    : error.c_str());
        }

        if (showSkeleton &&
            skeleton != nullptr)
        {
            for (std::size_t boneIndex = 0U;
                 boneIndex <
                     skeleton->bones.size();
                 ++boneIndex)
            {
                const LegacyBone& bone =
                    skeleton->bones[
                        boneIndex];

                const DirectX::XMFLOAT3 position =
                    GetBonePosition(bone);

                ImVec2 boneScreen;

                if (!ProjectPoint(
                        position,
                        camera.viewProjection,
                        canvasMinimum,
                        canvasSize,
                        boneScreen))
                {
                    continue;
                }

                if (bone.parentIndex >= 0 &&
                    bone.parentIndex <
                        static_cast<std::int32_t>(
                            skeleton->bones.size()))
                {
                    const LegacyBone& parent =
                        skeleton->bones[
                            static_cast<std::size_t>(
                                bone.parentIndex)];

                    ImVec2 parentScreen;

                    if (ProjectPoint(
                            GetBonePosition(parent),
                            camera.viewProjection,
                            canvasMinimum,
                            canvasSize,
                            parentScreen))
                    {
                        drawList.AddLine(
                            parentScreen,
                            boneScreen,
                            IM_COL32(
                                255,
                                205,
                                55,
                                230),
                            2.0F);
                    }
                }

                drawList.AddCircleFilled(
                    boneScreen,
                    bone.parentIndex < 0
                        ? 4.0F
                        : 2.3F,
                    bone.parentIndex < 0
                        ? IM_COL32(
                            255,
                            90,
                            55,
                            255)
                        : IM_COL32(
                            255,
                            225,
                            95,
                            245));
            }
        }

        char statistics[256]{};

        std::snprintf(
            statistics,
            sizeof(statistics),
            "D3D11 | Vertices: %llu | Triangles: %llu | Materials: %llu | Bones: %llu",
            static_cast<unsigned long long>(
                mesh.vertices.size()),
            static_cast<unsigned long long>(
                mesh.indices.size() / 3U),
            static_cast<unsigned long long>(
                mesh.materialChunks.size()),
            static_cast<unsigned long long>(
                skeleton != nullptr
                    ? skeleton->bones.size()
                    : 0U));

        drawList.AddText(
            ImVec2(
                canvasMinimum.x + 9.0F,
                canvasMinimum.y + 8.0F),
            IM_COL32(
                230,
                235,
                240,
                255),
            statistics);

        if (impl_ != nullptr &&
            !impl_->GetWarning().empty())
        {
            drawList.AddText(
                ImVec2(
                    canvasMinimum.x + 9.0F,
                    canvasMinimum.y + 28.0F),
                IM_COL32(
                    245,
                    175,
                    65,
                    255),
                "One or more DDS textures could not be uploaded.");
        }

        drawList.AddRect(
            canvasMinimum,
            canvasMaximum,
            hovered
                ? IM_COL32(
                    90,
                    155,
                    190,
                    255)
                : IM_COL32(
                    70,
                    80,
                    90,
                    255),
            0.0F,
            0,
            hovered
                ? 2.0F
                : 1.0F);

        drawList.PopClipRect();
    }
}