#include "Editor/LevelEditor/Terrain/TerrainImporter.h"

#include "Editor/Commands/CommandHistory.h"
#include "Editor/LevelEditor/Scene/SceneDocument.h"
#include "Editor/LevelEditor/Terrain/TerrainRenderer.h"
#include "Editor/LevelEditor/Viewport/CameraController.h"

#include <Assets/AssetResult.h>
#include <Assets/TerrainAsset.h>
#include <Graphics/RenderDevice.h>

#include <imgui.h>

#include <Windows.h>
#include <ShObjIdl.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace lts::editor
{
    namespace
    {
        constexpr std::uint32_t TerrainSignature = 0x5453544CU;
        constexpr std::uint32_t TerrainVersion = 1U;
        constexpr std::uint64_t MaximumSampleCount = 268435456ULL;
        constexpr std::uint32_t MaximumLayerCount = 18U;
        constexpr std::uint32_t MaximumMaskCount = 6U;
        constexpr std::uint64_t MaximumEmbeddedTextureSize =
            512ULL * 1024ULL * 1024ULL;

        struct SourceLayer final
        {
            std::string name;
            std::string diffusePath;
            std::string normalPath;
            std::string materialType;
            float scaleU = 16.0F;
            float scaleV = 16.0F;
            float specular = 0.0F;
        };

        struct SourceTerrainDescription final
        {
            std::uint32_t width = 0U;
            std::uint32_t height = 0U;
            std::uint32_t splatWidth = 0U;
            std::uint32_t splatHeight = 0U;
            float tileSize = 0.0F;
            float heightOffset = 0.0F;
            float heightScale = 0.0F;
            std::vector<SourceLayer> layers;
        };

        struct DdsPixelFormat final
        {
            std::uint32_t size = 32U;
            std::uint32_t flags = 0x41U;
            std::uint32_t fourCC = 0U;
            std::uint32_t rgbBitCount = 32U;
            std::uint32_t redMask = 0x000000FFU;
            std::uint32_t greenMask = 0x0000FF00U;
            std::uint32_t blueMask = 0x00FF0000U;
            std::uint32_t alphaMask = 0xFF000000U;
        };

        struct DdsHeader final
        {
            std::uint32_t size = 124U;
            std::uint32_t flags = 0x100FU;
            std::uint32_t height = 1U;
            std::uint32_t width = 1U;
            std::uint32_t pitch = 4U;
            std::uint32_t depth = 0U;
            std::uint32_t mipMapCount = 0U;
            std::array<std::uint32_t, 11> reserved{};
            DdsPixelFormat pixelFormat{};
            std::uint32_t caps = 0x1000U;
            std::uint32_t caps2 = 0U;
            std::uint32_t caps3 = 0U;
            std::uint32_t caps4 = 0U;
            std::uint32_t reserved2 = 0U;
        };

        static_assert(sizeof(DdsPixelFormat) == 32U);
        static_assert(sizeof(DdsHeader) == 124U);

        template<typename T>
        bool WriteValue(std::ofstream& stream, const T& value)
        {
            stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
            return static_cast<bool>(stream);
        }

        bool WriteString(std::ofstream& stream, const std::string& value)
        {
            const auto length = static_cast<std::uint32_t>(value.size());

            if (!WriteValue(stream, length))
            {
                return false;
            }

            if (!value.empty())
            {
                stream.write(value.data(), static_cast<std::streamsize>(value.size()));
            }

            return static_cast<bool>(stream);
        }

        bool WriteBlob(std::ofstream& stream, const std::vector<std::byte>& data)
        {
            const auto size = static_cast<std::uint64_t>(data.size());

            if (!WriteValue(stream, size))
            {
                return false;
            }

            if (!data.empty())
            {
                stream.write(
                    reinterpret_cast<const char*>(data.data()),
                    static_cast<std::streamsize>(data.size()));
            }

            return static_cast<bool>(stream);
        }

        std::vector<std::byte> CreateSolidDds(
            const std::uint8_t red,
            const std::uint8_t green,
            const std::uint8_t blue,
            const std::uint8_t alpha)
        {
            constexpr std::uint32_t magic = 0x20534444U;

            DdsHeader header{};
            const std::array<std::uint8_t, 4> pixel{red, green, blue, alpha};

            std::vector<std::byte> data(sizeof(magic) + sizeof(header) + pixel.size());

            std::memcpy(data.data(), &magic, sizeof(magic));
            std::memcpy(data.data() + sizeof(magic), &header, sizeof(header));
            std::memcpy(data.data() + sizeof(magic) + sizeof(header), pixel.data(), pixel.size());

            return data;
        }

        std::vector<std::byte> CreateRgbaDds(
            const std::uint32_t width,
            const std::uint32_t height,
            const std::vector<std::byte>& pixels)
        {
            constexpr std::uint32_t magic = 0x20534444U;
            const std::uint64_t expectedSize =
                static_cast<std::uint64_t>(width) * height * 4ULL;

            if (width == 0U || height == 0U || pixels.size() != expectedSize)
            {
                return {};
            }

            DdsHeader header{};
            header.width = width;
            header.height = height;
            header.pitch = width * 4U;

            std::vector<std::byte> data(
                sizeof(magic) + sizeof(header) + pixels.size());

            std::memcpy(data.data(), &magic, sizeof(magic));
            std::memcpy(data.data() + sizeof(magic), &header, sizeof(header));
            std::memcpy(
                data.data() + sizeof(magic) + sizeof(header),
                pixels.data(),
                pixels.size());

            return data;
        }

        bool ReadFileBlob(
            const std::filesystem::path& path,
            std::vector<std::byte>& output)
        {
            std::error_code error;
            const std::uintmax_t fileSize = std::filesystem::file_size(path, error);

            if (error || fileSize == 0U || fileSize > MaximumEmbeddedTextureSize)
            {
                return false;
            }

            std::ifstream stream(path, std::ios::binary);

            if (!stream)
            {
                return false;
            }

            output.resize(static_cast<std::size_t>(fileSize));
            return static_cast<bool>(stream.read(
                reinterpret_cast<char*>(output.data()),
                static_cast<std::streamsize>(output.size())));
        }

        bool DecodeRgbaImage(
            const std::filesystem::path& path,
            std::uint32_t& width,
            std::uint32_t& height,
            std::vector<std::byte>& pixels)
        {
            const HRESULT initializeResult =
                CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            const bool uninitialize = SUCCEEDED(initializeResult);

            Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
            Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
            Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
            Microsoft::WRL::ComPtr<IWICFormatConverter> converter;

            HRESULT result = CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&factory));

            if (SUCCEEDED(result))
            {
                result = factory->CreateDecoderFromFilename(
                    path.c_str(),
                    nullptr,
                    GENERIC_READ,
                    WICDecodeMetadataCacheOnDemand,
                    &decoder);
            }

            if (SUCCEEDED(result))
            {
                result = decoder->GetFrame(0U, &frame);
            }

            if (SUCCEEDED(result))
            {
                result = factory->CreateFormatConverter(&converter);
            }

            if (SUCCEEDED(result))
            {
                result = converter->Initialize(
                    frame.Get(),
                    GUID_WICPixelFormat32bppRGBA,
                    WICBitmapDitherTypeNone,
                    nullptr,
                    0.0,
                    WICBitmapPaletteTypeCustom);
            }

            UINT decodedWidth = 0U;
            UINT decodedHeight = 0U;

            if (SUCCEEDED(result))
            {
                result = converter->GetSize(&decodedWidth, &decodedHeight);
            }

            const std::uint64_t byteCount =
                static_cast<std::uint64_t>(decodedWidth) * decodedHeight * 4ULL;

            if (SUCCEEDED(result) &&
                byteCount > 0U &&
                byteCount <= MaximumEmbeddedTextureSize &&
                byteCount <= static_cast<std::uint64_t>(UINT_MAX))
            {
                pixels.resize(static_cast<std::size_t>(byteCount));
                result = converter->CopyPixels(
                    nullptr,
                    decodedWidth * 4U,
                    static_cast<UINT>(byteCount),
                    reinterpret_cast<BYTE*>(pixels.data()));
            }
            else if (SUCCEEDED(result))
            {
                result = E_INVALIDARG;
            }

            converter.Reset();
            frame.Reset();
            decoder.Reset();
            factory.Reset();

            if (uninitialize)
            {
                CoUninitialize();
            }

            if (FAILED(result))
            {
                pixels.clear();
                width = 0U;
                height = 0U;
                return false;
            }

            width = decodedWidth;
            height = decodedHeight;
            return true;
        }

        bool TransformRgbaPixels(
            std::vector<std::byte>& pixels,
            const std::uint32_t targetWidth,
            const std::uint32_t targetHeight,
            const bool transposeAxes,
            const bool flipX,
            const bool flipY)
        {
            const std::uint32_t sourceWidth = transposeAxes
                ? targetHeight
                : targetWidth;
            const std::uint32_t sourceHeight = transposeAxes
                ? targetWidth
                : targetHeight;
            const std::uint64_t byteCount =
                static_cast<std::uint64_t>(sourceWidth) * sourceHeight * 4ULL;

            if (pixels.size() != byteCount)
            {
                return false;
            }

            if (!transposeAxes && !flipX && !flipY)
            {
                return true;
            }

            std::vector<std::byte> source = pixels;

            for (std::uint32_t z = 0U; z < targetHeight; ++z)
            {
                for (std::uint32_t x = 0U; x < targetWidth; ++x)
                {
                    const std::uint32_t transformedX =
                        flipX ? targetWidth - 1U - x : x;
                    const std::uint32_t transformedZ =
                        flipY ? targetHeight - 1U - z : z;
                    const std::uint32_t sourceX =
                        transposeAxes ? transformedZ : transformedX;
                    const std::uint32_t sourceZ =
                        transposeAxes ? transformedX : transformedZ;
                    const std::size_t sourceOffset =
                        (static_cast<std::size_t>(sourceZ) * sourceWidth + sourceX) * 4U;
                    const std::size_t targetOffset =
                        (static_cast<std::size_t>(z) * targetWidth + x) * 4U;

                    std::memcpy(
                        pixels.data() + targetOffset,
                        source.data() + sourceOffset,
                        4U);
                }
            }

            return true;
        }

        std::string ToUtf8(const std::wstring& value)
        {
            if (value.empty())
            {
                return {};
            }

            const int size = WideCharToMultiByte(
                CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                nullptr, 0, nullptr, nullptr);

            if (size <= 0)
            {
                return {};
            }

            std::string result(static_cast<std::size_t>(size), '\0');

            WideCharToMultiByte(
                CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                result.data(), size, nullptr, nullptr);

            return result;
        }

        std::string Trim(std::string value)
        {
            const auto isWhitespace = [](const unsigned char character)
            {
                return std::isspace(character) != 0;
            };

            value.erase(
                value.begin(),
                std::find_if_not(value.begin(), value.end(), isWhitespace));
            value.erase(
                std::find_if_not(value.rbegin(), value.rend(), isWhitespace).base(),
                value.end());

            if (value.size() >= 2U && value.front() == '"' && value.back() == '"')
            {
                value = value.substr(1U, value.size() - 2U);
            }

            return value;
        }

        bool ParseFloat(const std::string& text, float& output)
        {
            try
            {
                std::size_t parsed = 0U;
                const float value = std::stof(text, &parsed);

                if (parsed != text.size() || !std::isfinite(value))
                {
                    return false;
                }

                output = value;
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        bool ParseUnsigned(const std::string& text, std::uint32_t& output)
        {
            try
            {
                std::size_t parsed = 0U;
                const unsigned long value = std::stoul(text, &parsed, 10);

                if (parsed != text.size() ||
                    value > static_cast<unsigned long>(UINT32_MAX))
                {
                    return false;
                }

                output = static_cast<std::uint32_t>(value);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        std::string NormalizeLogicalPath(std::string path)
        {
            std::replace(path.begin(), path.end(), '\\', '/');
            return path;
        }

        bool ParseTerrainDescription(
            const std::filesystem::path& path,
            SourceTerrainDescription& description)
        {
            std::ifstream stream(path);

            if (!stream)
            {
                return false;
            }

            SourceTerrainDescription result;
            SourceLayer* current = nullptr;
            bool baseFound = false;
            std::string line;

            while (std::getline(stream, line))
            {
                line = Trim(std::move(line));

                if (line.empty() || line.rfind("//", 0U) == 0U ||
                    line.rfind("#", 0U) == 0U)
                {
                    continue;
                }

                if (line == "base_layer" || line == "layer")
                {
                    if ((line == "base_layer" && (baseFound || !result.layers.empty())) ||
                        (line == "layer" && !baseFound) ||
                        result.layers.size() >= MaximumLayerCount)
                    {
                        return false;
                    }

                    result.layers.emplace_back();
                    current = &result.layers.back();
                    baseFound = true;
                    current->name = result.layers.size() == 1U
                        ? "Base Layer"
                        : "Layer " + std::to_string(result.layers.size() - 1U);
                    continue;
                }

                if (line == "{")
                {
                    continue;
                }

                if (line == "}")
                {
                    current = nullptr;
                    continue;
                }

                const std::size_t separator = line.find(':');

                if (separator == std::string::npos)
                {
                    continue;
                }

                const std::string key = Trim(line.substr(0U, separator));
                const std::string value = Trim(line.substr(separator + 1U));

                if (current == nullptr)
                {
                    if (key == "vert_count_x" && !ParseUnsigned(value, result.width))
                    {
                        return false;
                    }
                    if (key == "vert_count_z" && !ParseUnsigned(value, result.height))
                    {
                        return false;
                    }
                    if (key == "splat_res_u" && !ParseUnsigned(value, result.splatWidth))
                    {
                        return false;
                    }
                    if (key == "splat_res_v" && !ParseUnsigned(value, result.splatHeight))
                    {
                        return false;
                    }
                    if (key == "tile_unit_size" && !ParseFloat(value, result.tileSize))
                    {
                        return false;
                    }
                    if (key == "height_offset" && !ParseFloat(value, result.heightOffset))
                    {
                        return false;
                    }
                    if (key == "height_scale" && !ParseFloat(value, result.heightScale))
                    {
                        return false;
                    }
                    continue;
                }

                if (key == "name")
                {
                    current->name = value;
                }
                else if (key == "map_diffuse")
                {
                    current->diffusePath = NormalizeLogicalPath(value);
                }
                else if (key == "map_normal")
                {
                    current->normalPath = NormalizeLogicalPath(value);
                }
                else if (key == "mat_type")
                {
                    current->materialType = value;
                }
                else if (key == "scale_u" && !ParseFloat(value, current->scaleU))
                {
                    return false;
                }
                else if (key == "scale_v" && !ParseFloat(value, current->scaleV))
                {
                    return false;
                }
                else if (key == "specular" && !ParseFloat(value, current->specular))
                {
                    return false;
                }
            }

            if (result.splatWidth == 0U)
            {
                result.splatWidth = result.width;
            }

            if (result.splatHeight == 0U)
            {
                result.splatHeight = result.height;
            }

            if (!baseFound || result.width < 2U || result.height < 2U ||
                result.splatWidth == 0U || result.splatHeight == 0U ||
                !std::isfinite(result.tileSize) || result.tileSize <= 0.0F ||
                !std::isfinite(result.heightOffset) ||
                !std::isfinite(result.heightScale) || result.heightScale <= 0.0F ||
                result.layers.empty() || result.layers.size() > MaximumLayerCount)
            {
                return false;
            }

            description = std::move(result);
            return true;
        }

        bool ConvertTerrain2LayerScalesToWorldTileSizes(
            SourceTerrainDescription& description)
        {
            // Terrain2 stores scale_u/scale_v as the number of texture repeats
            // across the full X extent of the terrain.  The DX11 terrain shader
            // stores the inverse representation: the size of one repeat in
            // world units.  Terrain2 intentionally used the X extent for both
            // axes to keep the layer texels square on non-square terrains.
            const float terrainWidth =
                static_cast<float>(description.width) * description.tileSize;

            if (!std::isfinite(terrainWidth) || terrainWidth <= 0.0F)
            {
                return false;
            }

            for (SourceLayer& layer : description.layers)
            {
                if (!std::isfinite(layer.scaleU) || layer.scaleU <= 0.0F ||
                    !std::isfinite(layer.scaleV) || layer.scaleV <= 0.0F)
                {
                    return false;
                }

                layer.scaleU = terrainWidth / layer.scaleU;
                layer.scaleV = terrainWidth / layer.scaleV;
            }

            return true;
        }

        bool ParseExportMetadata(
            const std::filesystem::path& path,
            std::uint32_t& width,
            std::uint32_t& height,
            float& tileSize,
            float& minimumHeight,
            float& maximumHeight)
        {
            std::ifstream stream(path);

            if (!stream)
            {
                return false;
            }

            float exportedWidth = 0.0F;
            bool hasWidth = false;
            bool hasHeight = false;
            bool hasTileSize = false;
            bool hasMinimum = false;
            bool hasMaximum = false;
            std::string line;

            while (std::getline(stream, line))
            {
                line = Trim(std::move(line));
                const std::size_t separator = line.find('=');

                if (separator == std::string::npos)
                {
                    continue;
                }

                const std::string key = Trim(line.substr(0U, separator));
                const std::string value = Trim(line.substr(separator + 1U));

                if (key == "ExportWidthSamples")
                {
                    hasWidth = ParseUnsigned(value, width);
                }
                else if (key == "ExportLengthSamples")
                {
                    hasHeight = ParseUnsigned(value, height);
                }
                else if (key == "CellSize")
                {
                    hasTileSize = ParseFloat(value, tileSize);
                }
                else if (key == "ExportWidthWorld")
                {
                    static_cast<void>(ParseFloat(value, exportedWidth));
                }
                else if (key == "TerrainMinHeight" || key == "ExportMinHeight")
                {
                    hasMinimum = ParseFloat(value, minimumHeight);
                }
                else if (key == "TerrainMaxHeight" || key == "ExportMaxHeight")
                {
                    hasMaximum = ParseFloat(value, maximumHeight);
                }
            }

            if (!hasTileSize && exportedWidth > 0.0F && width > 1U)
            {
                tileSize = exportedWidth / static_cast<float>(width - 1U);
                hasTileSize = true;
            }

            return hasWidth && hasHeight && hasTileSize && hasMinimum && hasMaximum &&
                width > 1U && height > 1U && tileSize > 0.0F && maximumHeight > minimumHeight;
        }

        bool ParseLayerMaskRange(
            const std::filesystem::path& path,
            std::uint32_t& firstLayer,
            std::uint32_t& lastLayer)
        {
            std::wstring name = path.filename().wstring();
            std::transform(name.begin(), name.end(), name.begin(), [](const wchar_t value)
            {
                return static_cast<wchar_t>(std::towlower(value));
            });

            const std::wstring marker = L"_layers_";
            const std::size_t begin = name.find(marker);

            if (begin == std::wstring::npos)
            {
                return false;
            }

            const std::size_t numberBegin = begin + marker.size();
            const std::size_t dash = name.find(L'-', numberBegin);
            const std::size_t suffix = name.find(L"_rgba", dash);

            if (dash == std::wstring::npos || suffix == std::wstring::npos)
            {
                return false;
            }

            try
            {
                firstLayer = static_cast<std::uint32_t>(
                    std::stoul(name.substr(numberBegin, dash - numberBegin)));
                lastLayer = static_cast<std::uint32_t>(
                    std::stoul(name.substr(dash + 1U, suffix - dash - 1U)));
            }
            catch (...)
            {
                return false;
            }

            return firstLayer > 0U && lastLayer >= firstLayer &&
                lastLayer <= MaximumLayerCount - 1U;
        }

        void SetTextBuffer(std::array<char, 128>& buffer, const std::string& value)
        {
            buffer.fill('\0');

            const std::size_t length = (std::min)(value.size(), buffer.size() - 1U);

            if (length > 0U)
            {
                std::memcpy(buffer.data(), value.data(), length);
            }
        }

        bool HasR16Extension(const std::filesystem::path& path)
        {
            std::wstring extension = path.extension().wstring();

            std::transform(
                extension.begin(), extension.end(), extension.begin(),
                [](const wchar_t character)
                {
                    return static_cast<wchar_t>(std::towlower(character));
                });

            return extension == L".r16";
        }

        std::filesystem::path FindGameRoot()
        {
            std::error_code error;
            std::filesystem::path current = std::filesystem::current_path(error);

            if (error)
            {
                return L"game";
            }

            for (;;)
            {
                if (current.filename() == L"game" &&
                    std::filesystem::is_directory(current / L"Data", error))
                {
                    return current.lexically_normal();
                }

                error.clear();

                const std::filesystem::path nestedGame = current / L"game";

                if (std::filesystem::is_directory(nestedGame / L"Data", error))
                {
                    return nestedGame.lexically_normal();
                }

                const std::filesystem::path parent = current.parent_path();

                if (parent.empty() || parent == current)
                {
                    break;
                }

                current = parent;
            }

            return (std::filesystem::current_path() / L"game").lexically_normal();
        }

        std::filesystem::path FindBinRoot()
        {
            return (FindGameRoot().parent_path() / L"bin").lexically_normal();
        }

        std::wstring SanitizeOutputName(const char* value)
        {
            if (value == nullptr || *value == '\0')
            {
                return {};
            }

            std::wstring result = std::filesystem::u8path(value).filename().wstring();
            constexpr wchar_t invalidCharacters[] = L"<>:\"/\\|?*";

            for (wchar_t& character : result)
            {
                if (character < 32 || std::wcschr(invalidCharacters, character) != nullptr)
                {
                    character = L'_';
                }
            }

            while (!result.empty() && (result.back() == L'.' || result.back() == L' '))
            {
                result.pop_back();
            }

            if (result == L"." || result == L"..")
            {
                result.clear();
            }

            return result;
        }

        std::string SanitizeIniValue(std::string value)
        {
            std::replace(value.begin(), value.end(), '\r', ' ');
            std::replace(value.begin(), value.end(), '\n', ' ');
            return value;
        }

        bool WriteTerrainIni(
            const std::filesystem::path& iniPath,
            const std::wstring& levelName,
            const R16TerrainImportSettings& settings,
            const engine::assets::TerrainAsset& terrain,
            const float centerHeight,
            const std::array<float, 3U>& actorScale,
            std::string& status)
        {
            std::error_code error;
            std::filesystem::path temporaryPath = iniPath;
            temporaryPath += L".tmp";
            std::filesystem::remove(temporaryPath, error);
            error.clear();

            std::ofstream stream(
                temporaryPath,
                std::ios::binary | std::ios::trunc);

            if (!stream)
            {
                status = "Could not create Terrain.ini.";
                return false;
            }

            const auto pathValue = [](const std::filesystem::path& path)
            {
                return SanitizeIniValue(ToUtf8(path.generic_wstring()));
            };

            stream << std::setprecision(9);

            if (!settings.layerDescriptionPath.empty())
            {
                std::ifstream description(
                    settings.layerDescriptionPath,
                    std::ios::binary);

                if (!description)
                {
                    stream.close();
                    std::filesystem::remove(temporaryPath, error);
                    status = "Could not read the Terrain2 description for Terrain.ini.";
                    return false;
                }

                stream << description.rdbuf();
                stream << "\n\n// Studio R16 import metadata\n";
                stream << "source_r16:\t\"" << pathValue(settings.sourcePath) << "\"\n";
                stream << "asset:\t\"Terrain.terrain\"\n";
                stream << "level_name:\t\""
                       << SanitizeIniValue(ToUtf8(levelName)) << "\"\n";
                stream << "transpose_axes:\t" << (settings.transposeAxes ? 1 : 0) << "\n";
                stream << "flip_x:\t" << (settings.flipX ? 1 : 0) << "\n";
                stream << "flip_y:\t" << (settings.flipY ? 1 : 0) << "\n";
                stream << "center_height:\t" << centerHeight << "\n";
                stream << "actor_scale_x:\t" << actorScale[0] << "\n";
                stream << "actor_scale_y:\t" << actorScale[1] << "\n";
                stream << "actor_scale_z:\t" << actorScale[2] << "\n";
            }
            else
            {
                stream << "[Terrain]\n";
                stream << "Version=1\n";
                stream << "LevelName=" << SanitizeIniValue(ToUtf8(levelName)) << "\n";
                stream << "Asset=Terrain.terrain\n";
                stream << "SourceR16=" << pathValue(settings.sourcePath) << "\n";
                stream << "Width=" << terrain.width << "\n";
                stream << "Height=" << terrain.height << "\n";
                stream << "SampleSpacing=" << terrain.tileSize << "\n";
                stream << "HeightOffset=" << terrain.heightOffset << "\n";
                stream << "HeightRange=" << terrain.heightScale << "\n";
                stream << "CenterHeight=" << centerHeight << "\n";
                stream << "ScaleX=" << actorScale[0] << "\n";
                stream << "ScaleY=" << actorScale[1] << "\n";
                stream << "ScaleZ=" << actorScale[2] << "\n";
                stream << "TransposeAxes=" << (settings.transposeAxes ? 1 : 0) << "\n";
                stream << "FlipX=" << (settings.flipX ? 1 : 0) << "\n";
                stream << "FlipY=" << (settings.flipY ? 1 : 0) << "\n";
                stream << "SplatWidth=" << terrain.splatWidth << "\n";
                stream << "SplatHeight=" << terrain.splatHeight << "\n";
                stream << "LayerCount=" << terrain.layers.size() << "\n";
                stream << "MaskCount=" << terrain.masks.size() << "\n";
                stream << "ColorMap=" << pathValue(settings.colorMapPath) << "\n";
                stream << "NormalMap=" << pathValue(settings.normalMapPath) << "\n";
                stream << "MaterialsRoot="
                       << pathValue(FindBinRoot() / L"Data" / L"TerrainData" / L"Materials")
                       << "\n";

                for (std::size_t index = 0U; index < terrain.layers.size(); ++index)
                {
                    const engine::assets::TerrainLayer& layer = terrain.layers[index];
                    stream << "\n[Layer" << index << "]\n";
                    stream << "Name=" << SanitizeIniValue(layer.name) << "\n";
                    stream << "Diffuse=" << SanitizeIniValue(layer.diffusePath) << "\n";
                    stream << "Normal=" << SanitizeIniValue(layer.normalPath) << "\n";
                    stream << "MaterialType=" << SanitizeIniValue(layer.materialType) << "\n";
                    stream << "ScaleU=" << layer.scaleU << "\n";
                    stream << "ScaleV=" << layer.scaleV << "\n";
                    stream << "Specular=" << layer.specular << "\n";
                }
            }

            stream.flush();

            if (!stream)
            {
                stream.close();
                std::filesystem::remove(temporaryPath, error);
                status = "Could not write Terrain.ini.";
                return false;
            }

            stream.close();

            if (!MoveFileExW(
                    temporaryPath.c_str(),
                    iniPath.c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            {
                std::filesystem::remove(temporaryPath, error);
                status = "Could not commit Terrain.ini.";
                return false;
            }

            return true;
        }

        std::vector<engine::scene::TerrainComponent::LayerOverride>
        BuildLayerOverrides(const engine::assets::TerrainAsset& terrain)
        {
            std::vector<engine::scene::TerrainComponent::LayerOverride> result;
            result.reserve(terrain.layers.size());

            for (const engine::assets::TerrainLayer& source : terrain.layers)
            {
                engine::scene::TerrainComponent::LayerOverride layer;
                layer.name = source.name;
                layer.diffusePath = source.diffusePath;
                layer.normalPath = source.normalPath;
                layer.scaleU = source.scaleU;
                layer.scaleV = source.scaleV;
                result.push_back(std::move(layer));
            }

            return result;
        }

        void FocusCamera(
            CameraController& camera,
            const engine::assets::TerrainAsset& terrain,
            const EditorTransform& transform)
        {
            const float width = static_cast<float>(terrain.width - 1U) * terrain.tileSize;
            const float depth = static_cast<float>(terrain.height - 1U) * terrain.tileSize;
            const float worldWidth = width * std::fabs(transform.scale[0]);
            const float worldDepth = depth * std::fabs(transform.scale[2]);

            const float amplitude = (std::max)(
                std::fabs(terrain.heightOffset),
                std::fabs(terrain.heightOffset + terrain.heightScale));

            const DirectX::XMFLOAT3 target
            {
                transform.position[0] + width * transform.scale[0] * 0.5F,
                transform.position[1] + amplitude * std::fabs(transform.scale[1]) * 0.35F,
                transform.position[2] + depth * transform.scale[2] * 0.5F
            };

            camera.FocusOn(target, (std::max)(worldWidth, worldDepth) * 0.64F);
        }

        bool WriteTerrainFile(
            const std::filesystem::path& outputPath,
            const std::uint32_t width,
            const std::uint32_t height,
            const std::uint32_t splatWidth,
            const std::uint32_t splatHeight,
            const float tileSize,
            const float heightOffset,
            const float heightRange,
            const std::vector<std::int16_t>& heights,
            const std::vector<SourceLayer>& layers,
            const std::vector<std::vector<std::byte>>& masks,
            const std::vector<std::byte>& colorMap,
            const std::vector<std::byte>& normalMap)
        {
            const std::uint64_t sampleCount =
                static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);

            if (sampleCount == 0U || heights.size() != sampleCount)
            {
                return false;
            }

            std::ofstream stream(outputPath, std::ios::binary | std::ios::trunc);

            if (!stream)
            {
                return false;
            }

            const float heightScale = heightRange;
            const std::uint32_t layerCount = static_cast<std::uint32_t>(layers.size());
            const std::uint32_t maskCount = static_cast<std::uint32_t>(masks.size());
            const std::uint64_t heightBytes = sampleCount * sizeof(std::int16_t);

            if (splatWidth == 0U || splatHeight == 0U ||
                layerCount == 0U || layerCount > MaximumLayerCount ||
                maskCount != (layerCount - 1U + 2U) / 3U ||
                maskCount > MaximumMaskCount || colorMap.empty() || normalMap.empty())
            {
                return false;
            }

            if (!WriteValue(stream, TerrainSignature) ||
                !WriteValue(stream, TerrainVersion) ||
                !WriteValue(stream, width) ||
                !WriteValue(stream, height) ||
                !WriteValue(stream, splatWidth) ||
                !WriteValue(stream, splatHeight) ||
                !WriteValue(stream, tileSize) ||
                !WriteValue(stream, heightOffset) ||
                !WriteValue(stream, heightScale) ||
                !WriteValue(stream, layerCount) ||
                !WriteValue(stream, maskCount) ||
                !WriteValue(stream, heightBytes))
            {
                return false;
            }

            stream.write(
                reinterpret_cast<const char*>(heights.data()),
                static_cast<std::streamsize>(heightBytes));

            if (!stream)
            {
                return false;
            }

            for (const SourceLayer& layer : layers)
            {
                if (!WriteString(stream, layer.name) ||
                    !WriteString(stream, layer.diffusePath) ||
                    !WriteString(stream, layer.normalPath) ||
                    !WriteString(stream, layer.materialType) ||
                    !WriteValue(stream, layer.scaleU) ||
                    !WriteValue(stream, layer.scaleV) ||
                    !WriteValue(stream, layer.specular))
                {
                    return false;
                }
            }

            for (const std::vector<std::byte>& mask : masks)
            {
                if (!WriteBlob(stream, mask))
                {
                    return false;
                }
            }

            if (!WriteBlob(stream, colorMap) || !WriteBlob(stream, normalMap))
            {
                return false;
            }

            stream.flush();
            return static_cast<bool>(stream);
        }
    }

    bool DetectR16TerrainImportSettings(
        const std::filesystem::path& sourcePath,
        R16TerrainImportSettings& settings,
        float& terrainCenterHeight,
        std::string& status) noexcept
    {
        try
        {
            R16TerrainImportSettings result;
            result.sourcePath = sourcePath.lexically_normal();
            terrainCenterHeight = 0.0F;

            if (sourcePath.empty() || !HasR16Extension(sourcePath))
            {
                status = "Select a valid .r16 heightmap.";
                return false;
            }

            std::error_code error;
            const std::uintmax_t fileSize = std::filesystem::file_size(sourcePath, error);

            if (error || fileSize == 0U || fileSize % sizeof(std::uint16_t) != 0U)
            {
                status = "The selected file is not a valid 16-bit RAW heightmap.";
                return false;
            }

            const std::uint64_t sampleCount = fileSize / sizeof(std::uint16_t);
            const std::uint64_t side = static_cast<std::uint64_t>(
                std::sqrt(static_cast<long double>(sampleCount)));

            if (side * side == sampleCount && side <= UINT32_MAX)
            {
                result.width = static_cast<std::uint32_t>(side);
                result.height = static_cast<std::uint32_t>(side);
            }

            std::filesystem::path metadataPath = sourcePath;
            metadataPath.replace_extension(L".txt");

            float minimumHeight = -256.0F;
            float maximumHeight = 256.0F;
            std::uint32_t metadataWidth = result.width;
            std::uint32_t metadataHeight = result.height;

            if (std::filesystem::is_regular_file(metadataPath, error) && !error &&
                ParseExportMetadata(
                    metadataPath,
                    metadataWidth,
                    metadataHeight,
                    result.tileSize,
                    minimumHeight,
                    maximumHeight))
            {
                result.width = metadataWidth;
                result.height = metadataHeight;
                result.heightRange = maximumHeight - minimumHeight;
                result.heightOffset = -result.heightRange * 0.5F;
                terrainCenterHeight = (minimumHeight + maximumHeight) * 0.5F;
            }

            result.splatWidth = result.width;
            result.splatHeight = result.height;

            std::map<std::uint32_t, std::filesystem::path> orderedMasks;
            std::uint32_t highestPaintedLayer = 0U;

            for (const auto& entry : std::filesystem::directory_iterator(
                     sourcePath.parent_path(), error))
            {
                if (error || !entry.is_regular_file())
                {
                    continue;
                }

                std::uint32_t firstLayer = 0U;
                std::uint32_t lastLayer = 0U;

                if (ParseLayerMaskRange(entry.path(), firstLayer, lastLayer))
                {
                    orderedMasks[firstLayer] = entry.path().lexically_normal();
                    highestPaintedLayer = (std::max)(highestPaintedLayer, lastLayer);
                }
            }

            std::uint32_t expectedFirstLayer = 1U;

            for (const auto& [firstLayer, path] : orderedMasks)
            {
                if (firstLayer != expectedFirstLayer || result.layerMaskPaths.size() >= MaximumMaskCount)
                {
                    status = "Layer masks must form consecutive groups: 1-3, 4-6, ...";
                    return false;
                }

                result.layerMaskPaths.push_back(path);
                expectedFirstLayer += 3U;
            }

            std::wstring lowerSource = sourcePath.wstring();
            std::transform(
                lowerSource.begin(), lowerSource.end(), lowerSource.begin(),
                [](const wchar_t value)
                {
                    return static_cast<wchar_t>(std::towlower(value));
                });

            const std::filesystem::path gameRoot = FindGameRoot();
            const std::filesystem::path workspaceRoot = gameRoot.parent_path();

            if (lowerSource.find(L"colorado") != std::wstring::npos)
            {
                const std::filesystem::path terrain2 =
                    workspaceRoot / L"bin" / L"Levels" / L"WZ_Colorado" / L"Terrain2";
                const std::filesystem::path description = terrain2 / L"terrain2.ini";

                error.clear();
                if (std::filesystem::is_regular_file(description, error) && !error)
                {
                    SourceTerrainDescription terrain2Description;

                    if (!ParseTerrainDescription(description, terrain2Description))
                    {
                        status = "Could not parse the matching Terrain2.ini.";
                        return false;
                    }

                    const std::uint64_t terrain2SampleCount =
                        static_cast<std::uint64_t>(terrain2Description.width) *
                        terrain2Description.height;

                    if (terrain2SampleCount != sampleCount)
                    {
                        status = "The R16 resolution does not match Terrain2.ini.";
                        return false;
                    }

                    result.width = terrain2Description.width;
                    result.height = terrain2Description.height;
                    result.splatWidth = terrain2Description.splatWidth;
                    result.splatHeight = terrain2Description.splatHeight;
                    result.tileSize = terrain2Description.tileSize;
                    result.heightOffset = terrain2Description.heightOffset;
                    result.heightRange = terrain2Description.heightScale;
                    result.layerDescriptionPath = description;
                    result.colorMapPath = terrain2 / L"Color.dds";
                    result.normalMapPath = terrain2 / L"Normal.dds";
                    result.layerMaskPaths.clear();

                    const std::size_t terrain2MaskCount =
                        (terrain2Description.layers.size() - 1U + 2U) / 3U;

                    for (std::size_t index = 0U; index < terrain2MaskCount; ++index)
                    {
                        result.layerMaskPaths.push_back(
                            terrain2 / (L"Mat-Splat" + std::to_wstring(index) + L".dds"));
                    }

                    /*
                     * UE R16 export orientation -> WarZ world X/Z.
                     *
                     * LevelData coordinates prove that the exported rows map
                     * directly to X and must only be mirrored along Z.  The
                     * previous transpose + X flip reproduced terrain2.bin's
                     * storage order, not Terrain2's world-space sampling.
                     */
                    result.transposeAxes = false;
                    result.flipX = false;
                    result.flipY = true;
                    result.masksArePreorientedDds = true;
                    terrainCenterHeight = 0.0F;
                }
            }

            if (result.width < 2U || result.height < 2U)
            {
                status = "The heightmap is not square and its metadata has no resolution.";
                return false;
            }

            if (!result.layerMaskPaths.empty() && highestPaintedLayer == 0U)
            {
                status = "Layer mask filenames do not contain valid layer ranges.";
                return false;
            }

            settings = std::move(result);
            status = "Detected " + std::to_string(settings.width) + "x" +
                std::to_string(settings.height) + ", " +
                std::to_string(highestPaintedLayer) + " painted layers in " +
                std::to_string(settings.layerMaskPaths.size()) + " RGBA masks.";
            return true;
        }
        catch (...)
        {
            status = "Could not inspect the .r16 terrain source.";
            return false;
        }
    }

    bool WriteR16TerrainAsset(
        const R16TerrainImportSettings& settings,
        std::string& status) noexcept
    {
        try
        {
            const std::uint64_t sampleCount =
                static_cast<std::uint64_t>(settings.width) * settings.height;

            if (settings.sourcePath.empty() || settings.destinationPath.empty() ||
                settings.width < 2U || settings.height < 2U ||
                sampleCount > MaximumSampleCount ||
                !std::isfinite(settings.tileSize) || settings.tileSize <= 0.0F ||
                !std::isfinite(settings.heightOffset) ||
                !std::isfinite(settings.heightRange) || settings.heightRange <= 0.0F)
            {
                status = "Invalid R16 terrain import settings.";
                return false;
            }

            std::error_code error;
            const std::uint64_t expectedFileSize = sampleCount * sizeof(std::uint16_t);

            if (std::filesystem::file_size(settings.sourcePath, error) != expectedFileSize || error)
            {
                status = "File size does not match Width x Height x 2 bytes.";
                return false;
            }

            std::vector<SourceLayer> layers;

            if (!settings.layerDescriptionPath.empty())
            {
                SourceTerrainDescription description;

                if (!ParseTerrainDescription(settings.layerDescriptionPath, description))
                {
                    status = "Could not parse the terrain layer description.";
                    return false;
                }

                if (!ConvertTerrain2LayerScalesToWorldTileSizes(description))
                {
                    status = "The Terrain2 layer scales are invalid.";
                    return false;
                }

                layers = std::move(description.layers);
            }
            else
            {
                std::uint32_t highestLayer = 0U;

                for (const std::filesystem::path& maskPath : settings.layerMaskPaths)
                {
                    std::uint32_t firstLayer = 0U;
                    std::uint32_t lastLayer = 0U;

                    if (!ParseLayerMaskRange(maskPath, firstLayer, lastLayer))
                    {
                        status = "A layer mask filename has no valid layer range.";
                        return false;
                    }

                    highestLayer = (std::max)(highestLayer, lastLayer);
                }

                layers.resize(static_cast<std::size_t>(highestLayer) + 1U);

                for (std::size_t index = 0U; index < layers.size(); ++index)
                {
                    layers[index].name = index == 0U
                        ? "Base"
                        : "Layer " + std::to_string(index);
                }
            }

            if (layers.empty())
            {
                layers.push_back(SourceLayer{"Base"});
            }

            const std::size_t expectedMaskCount = (layers.size() - 1U + 2U) / 3U;

            if (layers.size() > MaximumLayerCount ||
                expectedMaskCount != settings.layerMaskPaths.size() ||
                expectedMaskCount > MaximumMaskCount)
            {
                status = "Layer count does not match the imported RGBA masks.";
                return false;
            }

            const std::filesystem::path gameRoot = FindGameRoot();
            const std::filesystem::path workspaceRoot = gameRoot.parent_path();

            for (const SourceLayer& layer : layers)
            {
                for (const std::string* logicalPath : {&layer.diffusePath, &layer.normalPath})
                {
                    if (logicalPath->empty())
                    {
                        continue;
                    }

                    const std::filesystem::path path = std::filesystem::u8path(*logicalPath);
                    error.clear();
                    const bool existsInGame =
                        std::filesystem::is_regular_file(gameRoot / path, error) && !error;
                    error.clear();
                    const bool existsInBin =
                        std::filesystem::is_regular_file(workspaceRoot / L"bin" / path, error) && !error;

                    if (!existsInGame && !existsInBin)
                    {
                        status = "Terrain material is missing: " + *logicalPath;
                        return false;
                    }
                }
            }

            std::ifstream source(settings.sourcePath, std::ios::binary);
            std::vector<std::uint16_t> sourceHeights(static_cast<std::size_t>(sampleCount));

            if (!source || !source.read(
                    reinterpret_cast<char*>(sourceHeights.data()),
                    static_cast<std::streamsize>(expectedFileSize)))
            {
                status = "Could not read the complete source heightmap.";
                return false;
            }

            std::vector<std::int16_t> terrainHeights(static_cast<std::size_t>(sampleCount));
            const std::uint32_t sourceWidth = settings.transposeAxes
                ? settings.height
                : settings.width;
            const float maximumHeight = settings.heightOffset + settings.heightRange;
            const float amplitude = (std::max)(
                std::fabs(settings.heightOffset),
                std::fabs(maximumHeight));

            if (!std::isfinite(amplitude) || amplitude <= 0.0F)
            {
                status = "The terrain height interval is invalid.";
                return false;
            }

            for (std::uint32_t z = 0U; z < settings.height; ++z)
            {
                for (std::uint32_t x = 0U; x < settings.width; ++x)
                {
                    const std::uint32_t transformedX =
                        settings.flipX ? settings.width - 1U - x : x;
                    const std::uint32_t transformedZ =
                        settings.flipY ? settings.height - 1U - z : z;
                    const std::uint32_t sourceX = settings.transposeAxes
                        ? transformedZ
                        : transformedX;
                    const std::uint32_t sourceZ = settings.transposeAxes
                        ? transformedX
                        : transformedZ;
                    const std::size_t sourceIndex =
                        static_cast<std::size_t>(sourceZ) * sourceWidth + sourceX;
                    const std::size_t targetIndex =
                        static_cast<std::size_t>(z) * settings.width + x;
                    const double normalized =
                        static_cast<double>(sourceHeights[sourceIndex]) / 65535.0;

                    const double worldHeight =
                        static_cast<double>(settings.heightOffset) +
                        normalized * static_cast<double>(settings.heightRange);
                    const long encodedHeight = std::lround(
                        worldHeight / static_cast<double>(amplitude) * 32767.0);

                    terrainHeights[targetIndex] = static_cast<std::int16_t>(
                        (std::clamp)(encodedHeight, -32767L, 32767L));
                }
            }

            std::vector<std::vector<std::byte>> masks;
            masks.reserve(settings.layerMaskPaths.size());

            for (const std::filesystem::path& maskPath : settings.layerMaskPaths)
            {
                if (settings.masksArePreorientedDds)
                {
                    std::vector<std::byte> mask;

                    if (!ReadFileBlob(maskPath, mask))
                    {
                        status = "Could not read the Terrain2 layer mask: " +
                            ToUtf8(maskPath.wstring());
                        return false;
                    }

                    masks.push_back(std::move(mask));
                    continue;
                }

                std::uint32_t maskWidth = 0U;
                std::uint32_t maskHeight = 0U;
                std::vector<std::byte> pixels;
                const std::uint32_t expectedMaskWidth = settings.transposeAxes
                    ? settings.height
                    : settings.width;
                const std::uint32_t expectedMaskHeight = settings.transposeAxes
                    ? settings.width
                    : settings.height;

                if (!DecodeRgbaImage(maskPath, maskWidth, maskHeight, pixels) ||
                    maskWidth != expectedMaskWidth || maskHeight != expectedMaskHeight)
                {
                    status = "Layer mask must match the heightmap resolution: " +
                        ToUtf8(maskPath.wstring());
                    return false;
                }

                if (!TransformRgbaPixels(
                    pixels,
                    settings.width,
                    settings.height,
                    settings.transposeAxes,
                    settings.flipX,
                    settings.flipY))
                {
                    status = "Could not transform a terrain layer mask.";
                    return false;
                }

                masks.push_back(CreateRgbaDds(settings.width, settings.height, pixels));

                if (masks.back().empty())
                {
                    status = "Could not encode a layer mask.";
                    return false;
                }
            }

            std::vector<std::byte> colorMap;
            std::vector<std::byte> normalMap;

            if (settings.colorMapPath.empty() || !ReadFileBlob(settings.colorMapPath, colorMap))
            {
                colorMap = CreateSolidDds(255U, 255U, 255U, 255U);
            }

            if (settings.normalMapPath.empty() || !ReadFileBlob(settings.normalMapPath, normalMap))
            {
                normalMap = CreateSolidDds(128U, 128U, 255U, 255U);
            }

            std::filesystem::create_directories(settings.destinationPath.parent_path(), error);

            if (error)
            {
                status = "Could not create the terrain output directory.";
                return false;
            }

            std::filesystem::path temporary = settings.destinationPath;
            temporary += L".tmp";
            std::filesystem::remove(temporary, error);
            error.clear();

            if (!WriteTerrainFile(
                    temporary,
                    settings.width,
                    settings.height,
                    settings.splatWidth == 0U ? settings.width : settings.splatWidth,
                    settings.splatHeight == 0U ? settings.height : settings.splatHeight,
                    settings.tileSize,
                    settings.heightOffset,
                    settings.heightRange,
                    terrainHeights,
                    layers,
                    masks,
                    colorMap,
                    normalMap))
            {
                status = "Could not write the .terrain file.";
                return false;
            }

            engine::assets::TerrainAsset validation;

            if (engine::assets::Failed(
                    engine::assets::TerrainAsset::Load(temporary, validation)) ||
                !validation.IsValid())
            {
                std::filesystem::remove(temporary, error);
                status = "The generated terrain failed validation.";
                return false;
            }

            if (!MoveFileExW(
                    temporary.c_str(),
                    settings.destinationPath.c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            {
                std::filesystem::remove(temporary, error);
                status = "Could not commit the generated terrain file.";
                return false;
            }

            status = "Terrain asset written: " + ToUtf8(settings.destinationPath.wstring());
            return true;
        }
        catch (const std::bad_alloc&)
        {
            status = "Not enough memory to import the terrain.";
            return false;
        }
        catch (...)
        {
            status = "Unexpected error while importing the terrain.";
            return false;
        }
    }

    void TerrainImporter::Open() noexcept
    {
        openRequested_ = true;
        importSucceeded_ = false;
        statusIsError_ = false;
        status_.clear();
    }

    bool TerrainImporter::SelectSourceFile(void* ownerWindow) noexcept
    {
        Microsoft::WRL::ComPtr<IFileOpenDialog> dialog;

        if (FAILED(CoCreateInstance(
                CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&dialog))))
        {
            status_ = "Could not create the file selection dialog.";
            statusIsError_ = true;
            return false;
        }

        DWORD options = 0U;

        if (FAILED(dialog->GetOptions(&options)) ||
            FAILED(dialog->SetOptions(
                options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST)))
        {
            status_ = "Could not configure the file selection dialog.";
            statusIsError_ = true;
            return false;
        }

        constexpr COMDLG_FILTERSPEC filters[]
        {
            {L"UE5 16-bit RAW Heightmap (*.r16)", L"*.r16"},
            {L"All files (*.*)", L"*.*"}
        };

        if (FAILED(dialog->SetFileTypes(2U, filters)) ||
            FAILED(dialog->SetDefaultExtension(L"r16")) ||
            FAILED(dialog->SetTitle(L"Select UE5 R16 Heightmap")))
        {
            status_ = "Could not configure the heightmap filter.";
            statusIsError_ = true;
            return false;
        }

        const HWND owner = reinterpret_cast<HWND>(ownerWindow);
        const HRESULT showResult = dialog->Show(owner);

        if (showResult == HRESULT_FROM_WIN32(ERROR_CANCELLED))
        {
            return false;
        }

        if (FAILED(showResult))
        {
            status_ = "The file selection dialog failed.";
            statusIsError_ = true;
            return false;
        }

        Microsoft::WRL::ComPtr<IShellItem> item;

        if (FAILED(dialog->GetResult(&item)))
        {
            status_ = "Could not read the selected file.";
            statusIsError_ = true;
            return false;
        }

        PWSTR rawPath = nullptr;

        if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &rawPath)))
        {
            status_ = "Could not read the selected file path.";
            statusIsError_ = true;
            return false;
        }

        sourcePath_ = std::filesystem::path(rawPath).lexically_normal();
        CoTaskMemFree(rawPath);

        DetectResolution();

        if (!layerDescriptionPath_.empty())
        {
            const std::filesystem::path detectedLevelDirectory =
                layerDescriptionPath_.parent_path().parent_path();
            SetTextBuffer(
                outputName_,
                ToUtf8(detectedLevelDirectory.filename().wstring()));
        }
        else
        {
            SetTextBuffer(outputName_, ToUtf8(sourcePath_.parent_path().filename().wstring()));
        }

        return true;
    }

    void TerrainImporter::DetectResolution() noexcept
    {
        width_ = 0;
        height_ = 0;
        importSucceeded_ = false;
        statusIsError_ = false;
        status_.clear();

        R16TerrainImportSettings settings;
        float terrainCenterHeight = 0.0F;

        if (!DetectR16TerrainImportSettings(
                sourcePath_, settings, terrainCenterHeight, status_))
        {
            statusIsError_ = true;
            return;
        }

        width_ = static_cast<int>(settings.width);
        height_ = static_cast<int>(settings.height);
        tileSize_ = settings.tileSize;
        heightOffset_ = settings.heightOffset;
        heightRange_ = settings.heightRange;
        splatWidth_ = settings.splatWidth;
        splatHeight_ = settings.splatHeight;
        baseHeight_ = terrainCenterHeight;
        layerDescriptionPath_ = std::move(settings.layerDescriptionPath);
        colorMapPath_ = std::move(settings.colorMapPath);
        normalMapPath_ = std::move(settings.normalMapPath);
        layerMaskPaths_ = std::move(settings.layerMaskPaths);
        transposeAxes_ = settings.transposeAxes;
        flipX_ = settings.flipX;
        flipY_ = settings.flipY;
        masksArePreorientedDds_ = settings.masksArePreorientedDds;
    }

    bool TerrainImporter::Import(TerrainImportContext& context) noexcept
    {
        importSucceeded_ = false;
        statusIsError_ = true;
        status_.clear();

        if (sourcePath_.empty() || !HasR16Extension(sourcePath_))
        {
            status_ = "Select a valid .r16 heightmap.";
            return false;
        }

        if (width_ < 2 || height_ < 2)
        {
            status_ = "Width and Height must be at least 2.";
            return false;
        }

        for (const float scale : resultScale_)
        {
            if (!std::isfinite(scale) || scale <= 0.0F)
            {
                status_ = "Result Scale X, Y and Z must be greater than zero.";
                return false;
            }
        }

        if (!std::isfinite(tileSize_) || tileSize_ <= 0.0F ||
            !std::isfinite(heightRange_) || heightRange_ <= 0.0F)
        {
            status_ = "Sample Spacing and Height Range must be greater than zero.";
            return false;
        }

        if (!std::isfinite(baseHeight_))
        {
            status_ = "Base Height is invalid.";
            return false;
        }

        const std::wstring outputName = SanitizeOutputName(outputName_.data());

        if (outputName.empty())
        {
            status_ = "Enter a valid level name.";
            return false;
        }

        const std::uint64_t sampleCount =
            static_cast<std::uint64_t>(width_) * static_cast<std::uint64_t>(height_);

        if (sampleCount == 0U || sampleCount > MaximumSampleCount)
        {
            status_ = "The heightmap resolution is too large.";
            return false;
        }

        const std::uint64_t expectedFileSize = sampleCount * sizeof(std::uint16_t);

        std::error_code error;
        const std::uintmax_t actualFileSize = std::filesystem::file_size(sourcePath_, error);

        if (error || actualFileSize != expectedFileSize)
        {
            status_ = "File size does not match Width x Height x 2 bytes.";
            return false;
        }

        const std::uint32_t width = static_cast<std::uint32_t>(width_);
        const std::uint32_t height = static_cast<std::uint32_t>(height_);

        const std::filesystem::path binRoot = FindBinRoot();
        const std::filesystem::path outputDirectory =
            binRoot / L"Levels" / outputName / L"Terrain";
        const std::filesystem::path outputPath = outputDirectory / L"Terrain.terrain";
        const std::filesystem::path iniPath = outputDirectory / L"Terrain.ini";

        std::filesystem::create_directories(outputDirectory, error);

        if (error)
        {
            status_ = "Could not create the level Terrain directory.";
            return false;
        }

        const bool terrainExists = std::filesystem::exists(outputPath, error) && !error;
        error.clear();
        const bool iniExists = std::filesystem::exists(iniPath, error) && !error;

        if (!overwriteExisting_ && (terrainExists || iniExists))
        {
            status_ = "This level already has Terrain data. Enable Overwrite Existing.";
            return false;
        }

        /*
          * Внутри asset храним нормализованный terrain:
          *
          * X/Z: один sample = 1 unit.
          * Y: высота в диапазоне -1..+1.
          *
          * Реальный масштаб карты задаётся Transform Terrain Actor.
          */
        R16TerrainImportSettings settings;
        settings.sourcePath = sourcePath_;
        settings.destinationPath = outputPath;
        settings.layerDescriptionPath = layerDescriptionPath_;
        settings.colorMapPath = colorMapPath_;
        settings.normalMapPath = normalMapPath_;
        settings.layerMaskPaths = layerMaskPaths_;
        settings.width = width;
        settings.height = height;
        settings.splatWidth = splatWidth_ == 0U ? width : splatWidth_;
        settings.splatHeight = splatHeight_ == 0U ? height : splatHeight_;
        settings.tileSize = tileSize_;
        settings.heightOffset = heightOffset_;
        settings.heightRange = heightRange_;
        settings.transposeAxes = transposeAxes_;
        settings.flipX = flipX_;
        settings.flipY = flipY_;
        settings.masksArePreorientedDds = masksArePreorientedDds_;

        if (!WriteR16TerrainAsset(settings, status_))
        {
            return false;
        }

        engine::assets::TerrainAsset terrainAsset;
        const engine::assets::AssetResult loadResult =
            engine::assets::TerrainAsset::Load(outputPath, terrainAsset);

        if (engine::assets::Failed(loadResult) || !terrainAsset.IsValid())
        {
            status_ = "The generated terrain failed validation.";
            return false;
        }

        if (!WriteTerrainIni(
                iniPath,
                outputName,
                settings,
                terrainAsset,
                baseHeight_,
                resultScale_,
                status_))
        {
            return false;
        }

        error.clear();
        const std::filesystem::path relativePath =
            std::filesystem::relative(outputPath, binRoot, error);

        if (error)
        {
            status_ = "Could not create a bin-relative terrain path.";
            return false;
        }

        const EditorSceneSnapshot before = context.sceneDocument.CreateSnapshot();

        EditorTransform transform{};

        transform.position[1] = baseHeight_;

        /*
         * Старый terrain-конвертер:
         * X/Y = горизонтальная плоскость карты.
         * Z   = вертикальная высота.
         *
         * Наш движок:
         * X/Z = горизонтальная плоскость.
         * Y   = вертикальная высота.
         */
        transform.scale =
        {
            resultScale_[0],
            resultScale_[1],
            resultScale_[2]
        };

        if (!context.sceneDocument.CreateTerrainEntity(
                outputName, relativePath.generic_wstring(), transform))
        {
            status_ = "The terrain file was created, but the Terrain Actor could not be created.";
            return false;
        }

        if (!context.sceneDocument.SetSelectedTerrainLayers(BuildLayerOverrides(terrainAsset)))
        {
            context.sceneDocument.RestoreSnapshot(before, false);
            status_ = "Could not initialize the base terrain layer.";
            return false;
        }

        const std::filesystem::path previousTerrainPath = context.loadedTerrainPath;

        if (!context.terrainRenderer.LoadTerrain(context.graphicsDevice, outputPath))
        {
            context.sceneDocument.RestoreSnapshot(before, false);
            static_cast<void>(
                context.terrainRenderer.LoadTerrain(context.graphicsDevice, previousTerrainPath));

            status_ = "The terrain file is valid, but GPU resources could not be created.";
            return false;
        }

        if (!context.commandHistory.Push(before, context.sceneDocument.CreateSnapshot()))
        {
            context.sceneDocument.RestoreSnapshot(before, false);
            static_cast<void>(
                context.terrainRenderer.LoadTerrain(context.graphicsDevice, previousTerrainPath));

            status_ = "Could not add the terrain creation operation to Undo/Redo.";
            return false;
        }

        context.loadedTerrainPath = outputPath.lexically_normal();
        FocusCamera(context.cameraController, terrainAsset, transform);

        status_ = "Terrain and Terrain.ini saved: " + ToUtf8(outputDirectory.wstring());
        statusIsError_ = false;
        importSucceeded_ = true;

        return true;
    }

    void TerrainImporter::Draw(TerrainImportContext& context) noexcept
    {
        if (openRequested_)
        {
            ImGui::OpenPopup("Import Terrain (.r16)");
            openRequested_ = false;
        }

        ImGui::SetNextWindowSize(ImVec2(620.0F, 0.0F), ImGuiCond_Appearing);
        ImGui::SetNextWindowBgAlpha(1.0F);

        bool windowOpen = true;

        if (!ImGui::BeginPopupModal(
                "Import Terrain (.r16)",
                &windowOpen,
                ImGuiWindowFlags_AlwaysAutoResize))
        {
            return;
        }

        if (ImGui::Button("Browse..."))
        {
            static_cast<void>(SelectSourceFile(context.ownerWindow));
        }

        ImGui::SameLine();

        const std::string sourcePath = ToUtf8(sourcePath_.wstring());

        if (sourcePath.empty())
        {
            ImGui::TextDisabled("No .r16 file selected");
        }
        else
        {
            ImGui::TextWrapped("%s", sourcePath.c_str());
        }

        ImGui::Separator();

        ImGui::SetNextItemWidth(140.0F);
        ImGui::InputInt("Width", &width_);

        ImGui::SameLine();

        ImGui::SetNextItemWidth(140.0F);
        ImGui::InputInt("Height", &height_);

        ImGui::SeparatorText("Terrain Dimensions");

        ImGui::SetNextItemWidth(180.0F);
        ImGui::DragFloat(
            "Sample Spacing",
            &tileSize_,
            0.01F,
            0.001F,
            10000.0F,
            "%.6f");

        ImGui::SetNextItemWidth(180.0F);
        ImGui::DragFloat(
            "Height Range",
            &heightRange_,
            0.1F,
            0.001F,
            1000000.0F,
            "%.3f");

        ImGui::SetNextItemWidth(180.0F);

        ImGui::DragFloat(
            "Terrain Center Height",
            &baseHeight_,
            1.0F,
            -1000000.0F,
            1000000.0F,
            "%.3f");

        ImGui::SeparatorText("Actor Scale");

        ImGui::SetNextItemWidth(360.0F);

        ImGui::DragFloat3(
            "##ResultScale",
            resultScale_.data(),
            0.01F,
            0.000001F,
            1000000.0F,
            "%.6f");

        ImGui::TextDisabled("Engine X/Y/Z multiplier (normally 1, 1, 1)");

        if (width_ > 1 &&
            height_ > 1 &&
            resultScale_[0] > 0.0F &&
            resultScale_[1] > 0.0F)
        {
            const double worldWidth =
                static_cast<double>(width_ - 1) *
                static_cast<double>(tileSize_) *
                static_cast<double>(resultScale_[0]);

            const double worldDepth =
                static_cast<double>(height_ - 1) *
                static_cast<double>(tileSize_) *
                static_cast<double>(resultScale_[2]);

            ImGui::TextDisabled(
                "Map size: %.3f x %.3f",
                worldWidth,
                worldDepth);
        }

        ImGui::Text("Layer masks: %zu", layerMaskPaths_.size());

        if (!layerDescriptionPath_.empty())
        {
            const std::string description = ToUtf8(layerDescriptionPath_.wstring());
            ImGui::TextWrapped("Layers: %s", description.c_str());
        }

        ImGui::TextWrapped(
            "Materials root: %s",
            ToUtf8((FindBinRoot() / L"Data" /
                L"TerrainData" / L"Materials").wstring()).c_str());

        ImGui::TextDisabled(
            "R16 transform: transpose=%d, flip X=%d, flip Y=%d",
            transposeAxes_ ? 1 : 0,
            flipX_ ? 1 : 0,
            flipY_ ? 1 : 0);

        ImGui::BeginDisabled(masksArePreorientedDds_);
        ImGui::Checkbox("Flip X", &flipX_);
        ImGui::SameLine();
        ImGui::Checkbox("Flip Y", &flipY_);
        ImGui::EndDisabled();

        ImGui::Separator();

        ImGui::InputText("Level Name", outputName_.data(), outputName_.size());
        ImGui::Checkbox("Overwrite Existing", &overwriteExisting_);

        const std::wstring cleanOutputName = SanitizeOutputName(outputName_.data());

        if (!cleanOutputName.empty())
        {
            const std::filesystem::path preview =
                FindBinRoot() / L"Levels" / cleanOutputName / L"Terrain";

            const std::string previewText = ToUtf8(preview.wstring());
            ImGui::TextWrapped("Output folder: %s", previewText.c_str());
            ImGui::TextDisabled("Files: Terrain.terrain, Terrain.ini");
        }

        if (!status_.empty())
        {
            const ImVec4 color =
                importSucceeded_
                    ? ImVec4(0.35F, 0.85F, 0.40F, 1.0F)
                    : statusIsError_
                        ? ImVec4(0.95F, 0.35F, 0.30F, 1.0F)
                        : ImVec4(0.95F, 0.75F, 0.25F, 1.0F);

            ImGui::Separator();
            ImGui::TextColored(color, "%s", status_.c_str());
        }

        ImGui::Separator();

        const bool hasSource = !sourcePath_.empty();

        if (!hasSource)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Import", ImVec2(120.0F, 0.0F)))
        {
            static_cast<void>(Import(context));
        }

        if (!hasSource)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (ImGui::Button(importSucceeded_ ? "Close" : "Cancel", ImVec2(120.0F, 0.0F)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
