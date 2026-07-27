#include "Editor/Tools/Import/LegacyMaterialReader.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cwctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lts::editor
{
    namespace
    {
        constexpr std::uint32_t DdsMagic =
            0x20534444U;

        constexpr std::uint32_t DdsPixelFormatAlphaPixels =
            0x00000001U;

        constexpr std::uint32_t DdsPixelFormatFourCc =
            0x00000004U;

        constexpr std::uint32_t DdsPixelFormatRgb =
            0x00000040U;

        constexpr std::uint32_t DdsCaps2CubeMap =
            0x00000200U;

        [[nodiscard]]
        constexpr std::uint32_t MakeFourCc(
            const char first,
            const char second,
            const char third,
            const char fourth) noexcept
        {
            return
                static_cast<std::uint32_t>(
                    static_cast<unsigned char>(first)) |
                (
                    static_cast<std::uint32_t>(
                        static_cast<unsigned char>(second))
                    << 8U
                ) |
                (
                    static_cast<std::uint32_t>(
                        static_cast<unsigned char>(third))
                    << 16U
                ) |
                (
                    static_cast<std::uint32_t>(
                        static_cast<unsigned char>(fourth))
                    << 24U
                );
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

        [[nodiscard]]
        std::string Trim(std::string value)
        {
            const auto isWhitespace =
                [](const unsigned char character)
            {
                return std::isspace(character) != 0;
            };

            const auto begin =
                std::find_if_not(
                    value.begin(),
                    value.end(),
                    isWhitespace);

            const auto end =
                std::find_if_not(
                    value.rbegin(),
                    value.rend(),
                    isWhitespace).
                    base();

            if (begin >= end)
            {
                return {};
            }

            value =
                std::string(begin, end);

            if (value.size() >= 2U)
            {
                const char first = value.front();
                const char last = value.back();

                if ((first == '"' && last == '"') ||
                    (first == '\'' && last == '\''))
                {
                    value =
                        value.substr(
                            1U,
                            value.size() - 2U);
                }
            }

            return value;
        }

        [[nodiscard]]
        std::string ToLowerAscii(
            std::string value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](const unsigned char character)
                {
                    return static_cast<char>(
                        std::tolower(character));
                });

            return value;
        }

        [[nodiscard]]
        bool EqualsIgnoreCase(
            const std::string_view left,
            const std::string_view right)
        {
            if (left.size() != right.size())
            {
                return false;
            }

            for (std::size_t index = 0U;
                 index < left.size();
                 ++index)
            {
                const unsigned char leftCharacter =
                    static_cast<unsigned char>(
                        left[index]);

                const unsigned char rightCharacter =
                    static_cast<unsigned char>(
                        right[index]);

                if (std::tolower(leftCharacter) !=
                    std::tolower(rightCharacter))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]]
        bool IsRegularFile(
            const std::filesystem::path& path) noexcept
        {
            std::error_code error;

            return
                std::filesystem::is_regular_file(
                    path,
                    error) &&
                !error;
        }

        [[nodiscard]]
        bool IsDirectory(
            const std::filesystem::path& path) noexcept
        {
            std::error_code error;

            return
                std::filesystem::is_directory(
                    path,
                    error) &&
                !error;
        }

        [[nodiscard]]
        std::filesystem::path ConvertLegacyPath(
            std::string value)
        {
            std::replace(
                value.begin(),
                value.end(),
                '\\',
                std::filesystem::path::
                    preferred_separator);

            std::replace(
                value.begin(),
                value.end(),
                '/',
                std::filesystem::path::
                    preferred_separator);

            return
                std::filesystem::u8path(value).
                    lexically_normal();
        }

        [[nodiscard]]
        bool ParseFloat(
            const std::string& text,
            float& output) noexcept
        {
            errno = 0;

            char* end = nullptr;

            const float value =
                std::strtof(
                    text.c_str(),
                    &end);

            if (end == text.c_str() ||
                errno == ERANGE ||
                !std::isfinite(value))
            {
                return false;
            }

            while (*end != '\0' &&
                   std::isspace(
                       static_cast<unsigned char>(
                           *end)) != 0)
            {
                ++end;
            }

            if (*end != '\0')
            {
                return false;
            }

            output = value;
            return true;
        }

        [[nodiscard]]
        bool ParseInteger(
            const std::string& text,
            int& output) noexcept
        {
            errno = 0;

            char* end = nullptr;

            const long value =
                std::strtol(
                    text.c_str(),
                    &end,
                    10);

            if (end == text.c_str() ||
                errno == ERANGE ||
                value <
                    static_cast<long>(
                        (std::numeric_limits<int>::min)()) ||
                value >
                    static_cast<long>(
                        (std::numeric_limits<int>::max)()))
            {
                return false;
            }

            while (*end != '\0' &&
                   std::isspace(
                       static_cast<unsigned char>(
                           *end)) != 0)
            {
                ++end;
            }

            if (*end != '\0')
            {
                return false;
            }

            output = static_cast<int>(value);
            return true;
        }

        [[nodiscard]]
        bool ParseColor(
            const std::string& text,
            std::array<float, 3U>& output)
        {
            std::istringstream parser(text);

            int red = 255;
            int green = 255;
            int blue = 255;

            if (!(parser >> red >> green >> blue))
            {
                return false;
            }

            red = std::clamp(red, 0, 255);
            green = std::clamp(green, 0, 255);
            blue = std::clamp(blue, 0, 255);

            output =
            {
                static_cast<float>(red) / 255.0F,
                static_cast<float>(green) / 255.0F,
                static_cast<float>(blue) / 255.0F
            };

            return true;
        }

        void InitializeTextureSlots(
            LegacyMaterialData& material) noexcept
        {
            for (std::size_t index = 0U;
                 index < material.textures.size();
                 ++index)
            {
                material.textures[index].slot =
                    static_cast<LegacyTextureSlot>(
                        index);
            }
        }

        [[nodiscard]]
        std::string NormalizeTextureName(
            std::string value)
        {
            value = Trim(std::move(value));

            if (EqualsIgnoreCase(value, "NONE"))
            {
                return {};
            }

            return value;
        }

        void SetTextureName(
            LegacyMaterialData& material,
            const LegacyTextureSlot slot,
            std::string value)
        {
            LegacyMaterialTexture* texture =
                material.FindTexture(slot);

            if (texture != nullptr)
            {
                texture->sourceName =
                    NormalizeTextureName(
                        std::move(value));
            }
        }

        [[nodiscard]]
        std::filesystem::path FindFileIgnoreCase(
            const std::filesystem::path& directory,
            const std::filesystem::path& requestedName)
        {
            if (!IsDirectory(directory))
            {
                return {};
            }

            const std::filesystem::path direct =
                directory / requestedName;

            if (IsRegularFile(direct))
            {
                return direct.lexically_normal();
            }

            const std::wstring expected =
                ToLowerAscii(
                    requestedName.
                        filename().
                        generic_u8string()).
                    empty()
                    ? std::wstring{}
                    : requestedName.
                        filename().
                        wstring();

            std::wstring expectedLower =
                expected;

            std::transform(
                expectedLower.begin(),
                expectedLower.end(),
                expectedLower.begin(),
                [](const wchar_t character)
                {
                    return static_cast<wchar_t>(
                        std::towlower(character));
                });

            std::error_code error;

            for (const std::filesystem::directory_entry& entry :
                 std::filesystem::directory_iterator(
                     directory,
                     std::filesystem::directory_options::
                         skip_permission_denied,
                     error))
            {
                if (error)
                {
                    break;
                }

                std::error_code fileError;

                if (!entry.is_regular_file(fileError) ||
                    fileError)
                {
                    continue;
                }

                std::wstring fileName =
                    entry.path().
                        filename().
                        wstring();

                std::transform(
                    fileName.begin(),
                    fileName.end(),
                    fileName.begin(),
                    [](const wchar_t character)
                    {
                        return static_cast<wchar_t>(
                            std::towlower(character));
                    });

                if (fileName == expectedLower)
                {
                    return
                        entry.path().
                            lexically_normal();
                }
            }

            return {};
        }

        [[nodiscard]]
        std::filesystem::path FindRecursiveByName(
            const std::filesystem::path& directory,
            const std::filesystem::path& requestedName)
        {
            if (!IsDirectory(directory))
            {
                return {};
            }

            std::wstring expected =
                requestedName.
                    filename().
                    wstring();

            std::transform(
                expected.begin(),
                expected.end(),
                expected.begin(),
                [](const wchar_t character)
                {
                    return static_cast<wchar_t>(
                        std::towlower(character));
                });

            std::error_code error;

            for (const std::filesystem::directory_entry& entry :
                 std::filesystem::recursive_directory_iterator(
                     directory,
                     std::filesystem::directory_options::
                         skip_permission_denied,
                     error))
            {
                if (error)
                {
                    break;
                }

                std::error_code fileError;

                if (!entry.is_regular_file(fileError) ||
                    fileError)
                {
                    continue;
                }

                std::wstring fileName =
                    entry.path().
                        filename().
                        wstring();

                std::transform(
                    fileName.begin(),
                    fileName.end(),
                    fileName.begin(),
                    [](const wchar_t character)
                    {
                        return static_cast<wchar_t>(
                            std::towlower(character));
                    });

                if (fileName == expected)
                {
                    return
                        entry.path().
                            lexically_normal();
                }
            }

            return {};
        }

        [[nodiscard]]
        std::filesystem::path ResolveImagesDirectory(
            const std::filesystem::path& sourceRoot,
            const std::filesystem::path& packageDirectory,
            const std::filesystem::path& materialPath,
            const std::string& imagesDirectorySource)
        {
            const std::filesystem::path defaultDirectory =
                packageDirectory / L"Textures";

            if (imagesDirectorySource.empty())
            {
                return defaultDirectory.lexically_normal();
            }

            const std::filesystem::path legacyPath =
                ConvertLegacyPath(
                    imagesDirectorySource);

            if (legacyPath.is_absolute())
            {
                return legacyPath.lexically_normal();
            }

            const std::array<
                std::filesystem::path,
                5U> candidates
            {{
                sourceRoot / legacyPath,
                sourceRoot.parent_path() / legacyPath,
                packageDirectory / legacyPath,
                materialPath.parent_path() / legacyPath,
                defaultDirectory
            }};

            for (const std::filesystem::path& candidate :
                 candidates)
            {
                if (IsDirectory(candidate))
                {
                    return candidate.lexically_normal();
                }
            }

            return defaultDirectory.lexically_normal();
        }

        [[nodiscard]]
        std::filesystem::path ResolveTexturePath(
            const std::filesystem::path& sourceRoot,
            const std::filesystem::path& packageDirectory,
            const std::filesystem::path& materialPath,
            const std::filesystem::path& imagesDirectory,
            const std::string& sourceName)
        {
            if (sourceName.empty())
            {
                return {};
            }

            const std::filesystem::path legacyName =
                ConvertLegacyPath(sourceName);

            std::vector<std::filesystem::path> candidates;

            if (legacyName.is_absolute())
            {
                candidates.push_back(legacyName);
            }
            else
            {
                candidates.push_back(
                    imagesDirectory / legacyName);

                candidates.push_back(
                    packageDirectory /
                    L"Textures" /
                    legacyName);

                candidates.push_back(
                    materialPath.parent_path() /
                    legacyName);

                candidates.push_back(
                    sourceRoot / legacyName);

                candidates.push_back(
                    sourceRoot.parent_path() /
                    legacyName);
            }

            for (std::filesystem::path candidate :
                 candidates)
            {
                candidate =
                    candidate.lexically_normal();

                if (IsRegularFile(candidate))
                {
                    return candidate;
                }

                std::filesystem::path ddsCandidate =
                    candidate;

                ddsCandidate.replace_extension(
                    L".dds");

                if (IsRegularFile(ddsCandidate))
                {
                    return
                        ddsCandidate.
                            lexically_normal();
                }

                const std::filesystem::path caseInsensitive =
                    FindFileIgnoreCase(
                        candidate.parent_path(),
                        candidate.filename());

                if (!caseInsensitive.empty())
                {
                    return caseInsensitive;
                }

                const std::filesystem::path caseInsensitiveDds =
                    FindFileIgnoreCase(
                        ddsCandidate.parent_path(),
                        ddsCandidate.filename());

                if (!caseInsensitiveDds.empty())
                {
                    return caseInsensitiveDds;
                }
            }

            std::filesystem::path requestedFile =
                legacyName.filename();

            requestedFile.replace_extension(
                L".dds");

            std::filesystem::path recursive =
                FindRecursiveByName(
                    imagesDirectory,
                    requestedFile);

            if (!recursive.empty())
            {
                return recursive;
            }

            recursive =
                FindRecursiveByName(
                    packageDirectory / L"Textures",
                    requestedFile);

            return recursive;
        }

        [[nodiscard]]
        std::string DescribeDxgiFormat(
            const std::uint32_t format,
            bool& hasAlpha)
        {
            hasAlpha = false;

            switch (format)
            {
                case 28U:
                    hasAlpha = true;
                    return "R8G8B8A8_UNORM";

                case 29U:
                    hasAlpha = true;
                    return "R8G8B8A8_UNORM_SRGB";

                case 71U:
                    return "BC1_UNORM";

                case 72U:
                    return "BC1_UNORM_SRGB";

                case 74U:
                    hasAlpha = true;
                    return "BC2_UNORM";

                case 75U:
                    hasAlpha = true;
                    return "BC2_UNORM_SRGB";

                case 77U:
                    hasAlpha = true;
                    return "BC3_UNORM";

                case 78U:
                    hasAlpha = true;
                    return "BC3_UNORM_SRGB";

                case 80U:
                    return "BC4_UNORM";

                case 81U:
                    return "BC4_SNORM";

                case 83U:
                    return "BC5_UNORM";

                case 84U:
                    return "BC5_SNORM";

                case 87U:
                    hasAlpha = true;
                    return "B8G8R8A8_UNORM";

                case 88U:
                    return "B8G8R8X8_UNORM";

                case 91U:
                    hasAlpha = true;
                    return "B8G8R8A8_UNORM_SRGB";

                case 93U:
                    return "B8G8R8X8_UNORM_SRGB";

                case 95U:
                    return "BC6H_UF16";

                case 96U:
                    return "BC6H_SF16";

                case 98U:
                    hasAlpha = true;
                    return "BC7_UNORM";

                case 99U:
                    hasAlpha = true;
                    return "BC7_UNORM_SRGB";

                default:
                    return
                        "DXGI_FORMAT_" +
                        std::to_string(format);
            }
        }

        [[nodiscard]]
        std::string DescribeLegacyDdsFormat(
            const DdsPixelFormat& format,
            bool& hasAlpha)
        {
            hasAlpha =
                (format.flags &
                 DdsPixelFormatAlphaPixels) != 0U ||
                format.alphaMask != 0U;

            if ((format.flags &
                 DdsPixelFormatFourCc) != 0U)
            {
                switch (format.fourCc)
                {
                    case MakeFourCc('D', 'X', 'T', '1'):
                        return "BC1 / DXT1";

                    case MakeFourCc('D', 'X', 'T', '3'):
                        hasAlpha = true;
                        return "BC2 / DXT3";

                    case MakeFourCc('D', 'X', 'T', '5'):
                        hasAlpha = true;
                        return "BC3 / DXT5";

                    case MakeFourCc('A', 'T', 'I', '1'):
                    case MakeFourCc('B', 'C', '4', 'U'):
                        return "BC4";

                    case MakeFourCc('A', 'T', 'I', '2'):
                    case MakeFourCc('B', 'C', '5', 'U'):
                        return "BC5";

                    default:
                    {
                        char fourCc[5]
                        {
                            static_cast<char>(
                                format.fourCc & 0xFFU),

                            static_cast<char>(
                                (format.fourCc >> 8U) &
                                0xFFU),

                            static_cast<char>(
                                (format.fourCc >> 16U) &
                                0xFFU),

                            static_cast<char>(
                                (format.fourCc >> 24U) &
                                0xFFU),

                            '\0'
                        };

                        return
                            std::string("FourCC ") +
                            fourCc;
                    }
                }
            }

            if ((format.flags &
                 DdsPixelFormatRgb) != 0U)
            {
                if (format.rgbBitCount == 32U)
                {
                    return hasAlpha
                        ? "RGBA8"
                        : "RGBX8";
                }

                if (format.rgbBitCount == 24U)
                {
                    return "RGB8";
                }

                return
                    "RGB " +
                    std::to_string(
                        format.rgbBitCount) +
                    "-bit";
            }

            return "Unknown";
        }

        [[nodiscard]]
        bool ReadDdsInfo(
            const std::filesystem::path& path,
            LegacyDdsInfo& output) noexcept
        {
            output = {};
            output.path = path;
            output.exists = IsRegularFile(path);

            if (!output.exists)
            {
                output.error =
                    "DDS file was not found.";

                return false;
            }

            try
            {
                std::error_code sizeError;

                output.fileSize =
                    std::filesystem::file_size(
                        path,
                        sizeError);

                if (sizeError)
                {
                    output.error =
                        "Failed to query DDS file size.";

                    return false;
                }

                std::ifstream stream(
                    path,
                    std::ios::binary);

                if (!stream)
                {
                    output.error =
                        "Failed to open DDS file.";

                    return false;
                }

                std::uint32_t magic = 0U;
                DdsHeader header;

                stream.read(
                    reinterpret_cast<char*>(&magic),
                    sizeof(magic));

                stream.read(
                    reinterpret_cast<char*>(&header),
                    sizeof(header));

                if (!stream ||
                    magic != DdsMagic ||
                    header.size != sizeof(DdsHeader) ||
                    header.pixelFormat.size !=
                        sizeof(DdsPixelFormat))
                {
                    output.error =
                        "Invalid DDS header.";

                    return false;
                }

                if (header.width == 0U ||
                    header.height == 0U ||
                    header.width > 32768U ||
                    header.height > 32768U)
                {
                    output.error =
                        "Invalid DDS dimensions.";

                    return false;
                }

                output.width = header.width;
                output.height = header.height;

                output.mipCount =
                    (std::max)(
                        header.mipMapCount,
                        1U);

                output.isCubeMap =
                    (header.caps2 &
                     DdsCaps2CubeMap) != 0U;

                if (header.pixelFormat.fourCc ==
                    MakeFourCc(
                        'D',
                        'X',
                        '1',
                        '0'))
                {
                    DdsHeaderDx10 dx10;

                    stream.read(
                        reinterpret_cast<char*>(&dx10),
                        sizeof(dx10));

                    if (!stream ||
                        dx10.arraySize == 0U)
                    {
                        output.error =
                            "Invalid DDS DX10 header.";

                        return false;
                    }

                    output.arraySize =
                        dx10.arraySize;

                    output.format =
                        DescribeDxgiFormat(
                            dx10.dxgiFormat,
                            output.hasAlpha);
                }
                else
                {
                    output.format =
                        DescribeLegacyDdsFormat(
                            header.pixelFormat,
                            output.hasAlpha);
                }

                output.valid = true;
                return true;
            }
            catch (const std::exception& exception)
            {
                output.error =
                    "DDS inspection failed: " +
                    std::string(
                        exception.what());

                return false;
            }
            catch (...)
            {
                output.error =
                    "DDS inspection failed with an unknown error.";

                return false;
            }
        }

        [[nodiscard]]
        std::filesystem::path FindMaterialPath(
            const std::filesystem::path& packageDirectory,
            const std::string& materialName)
        {
            std::filesystem::path fileName =
                ConvertLegacyPath(materialName).
                    filename();

            if (fileName.extension().empty())
            {
                fileName += L".mat";
            }

            std::filesystem::path result =
                FindFileIgnoreCase(
                    packageDirectory / L"Materials",
                    fileName);

            if (!result.empty())
            {
                return result;
            }

            return
                FindFileIgnoreCase(
                    packageDirectory,
                    fileName);
        }

        [[nodiscard]]
        bool ReadMaterialFile(
            const std::filesystem::path& path,
            const std::string& requestedName,
            LegacyMaterialData& output)
        {
            std::ifstream stream(path);

            if (!stream)
            {
                output.error =
                    "Failed to open MAT file.";

                return false;
            }

            std::vector<LegacyMaterialData> parsedMaterials;

            LegacyMaterialData current;
            bool insideMaterial = false;

            std::string line;

            const auto finishCurrent =
                [&]()
            {
                if (!insideMaterial)
                {
                    return;
                }

                if (current.name.empty())
                {
                    ++current.parseWarningCount;
                }

                parsedMaterials.push_back(
                    std::move(current));

                current = {};
                insideMaterial = false;
            };

            while (std::getline(stream, line))
            {
                if (!line.empty() &&
                    line.back() == '\r')
                {
                    line.pop_back();
                }

                const std::string trimmed =
                    Trim(line);

                if (trimmed == "[MaterialBegin]")
                {
                    finishCurrent();

                    current = {};
                    current.sourcePath = path;

                    InitializeTextureSlots(current);

                    insideMaterial = true;
                    continue;
                }

                if (trimmed == "[MaterialEnd]")
                {
                    finishCurrent();
                    continue;
                }

                if (!insideMaterial)
                {
                    continue;
                }

                const std::size_t separator =
                    trimmed.find('=');

                if (separator == std::string::npos)
                {
                    continue;
                }

                const std::string key =
                    ToLowerAscii(
                        Trim(
                            trimmed.substr(
                                0U,
                                separator)));

                const std::string value =
                    Trim(
                        trimmed.substr(
                            separator + 1U));

                if (key == "name")
                {
                    current.name = value;
                }
                else if (key == "type")
                {
                    current.typeName = value;
                }
                else if (key == "imagesdir")
                {
                    current.imagesDirectorySource =
                        NormalizeTextureName(value);
                }
                else if (key == "color24")
                {
                    if (!ParseColor(
                            value,
                            current.diffuseColor))
                    {
                        ++current.parseWarningCount;
                    }
                }
                else if (key == "specularpower")
                {
                    if (!ParseFloat(
                            value,
                            current.specularPower))
                    {
                        ++current.parseWarningCount;
                    }
                }
                else if (key == "specular1power")
                {
                    if (!ParseFloat(
                            value,
                            current.specularPower1))
                    {
                        ++current.parseWarningCount;
                    }
                }
                else if (key == "reflectionpower")
                {
                    if (!ParseFloat(
                            value,
                            current.reflectionPower))
                    {
                        ++current.parseWarningCount;
                    }
                }
                else if (key == "detailscale")
                {
                    if (!ParseFloat(
                            value,
                            current.detailScale))
                    {
                        ++current.parseWarningCount;
                    }
                }
                else if (key == "detailammount")
                {
                    if (!ParseFloat(
                            value,
                            current.detailAmount))
                    {
                        ++current.parseWarningCount;
                    }
                }
                else if (key == "displ_val")
                {
                    if (!ParseFloat(
                            value,
                            current.displacementDepth))
                    {
                        ++current.parseWarningCount;
                    }
                }
                else if (key == "lowqselfillum")
                {
                    if (!ParseFloat(
                            value,
                            current.
                                lowQualitySelfIllumination))
                    {
                        ++current.parseWarningCount;
                    }
                }
                else if (key == "lowqmetallness")
                {
                    if (!ParseFloat(
                            value,
                            current.
                                lowQualityMetalness))
                    {
                        ++current.parseWarningCount;
                    }
                }
                else if (key ==
                         "selfillummultiplier")
                {
                    if (!ParseFloat(
                            value,
                            current.
                                selfIlluminationMultiplier))
                    {
                        ++current.parseWarningCount;
                    }
                }
                else if (key == "displace" ||
                         key == "alphatransparent" ||
                         key == "forcetransparent" ||
                         key == "transparentshadows" ||
                         key == "doublesided" ||
                         key == "camouflage")
                {
                    int integerValue = 0;

                    if (!ParseInteger(
                            value,
                            integerValue))
                    {
                        ++current.parseWarningCount;
                        continue;
                    }

                    const bool enabled =
                        integerValue != 0;

                    if (key == "displace")
                    {
                        current.displacement =
                            enabled;
                    }
                    else if (key ==
                             "alphatransparent")
                    {
                        current.transparent =
                            enabled;
                    }
                    else if (key ==
                             "forcetransparent" ||
                             key ==
                             "transparentshadows")
                    {
                        current.forceAlpha =
                            enabled;
                    }
                    else if (key == "doublesided")
                    {
                        current.doubleSided =
                            enabled;
                    }
                    else
                    {
                        current.camouflage =
                            enabled;
                    }
                }
                else if (key == "texture")
                {
                    SetTextureName(
                        current,
                        LegacyTextureSlot::Diffuse,
                        value);
                }
                else if (key == "normalmap")
                {
                    SetTextureName(
                        current,
                        LegacyTextureSlot::Normal,
                        value);
                }
                else if (key == "specularmap")
                {
                    SetTextureName(
                        current,
                        LegacyTextureSlot::Specular,
                        value);
                }
                else if (key == "envmap")
                {
                    SetTextureName(
                        current,
                        LegacyTextureSlot::Roughness,
                        value);
                }
                else if (key == "glowmap")
                {
                    SetTextureName(
                        current,
                        LegacyTextureSlot::Glow,
                        value);
                }
                else if (key == "detailnmap")
                {
                    SetTextureName(
                        current,
                        LegacyTextureSlot::DetailNormal,
                        value);
                }
                else if (key == "densitymap")
                {
                    SetTextureName(
                        current,
                        LegacyTextureSlot::Density,
                        value);
                }
                else if (key == "camomask")
                {
                    SetTextureName(
                        current,
                        LegacyTextureSlot::CamouflageMask,
                        value);
                }
                else if (key == "distortionmap")
                {
                    SetTextureName(
                        current,
                        LegacyTextureSlot::Distortion,
                        value);
                }
                else if (key == "specpowmap")
                {
                    SetTextureName(
                        current,
                        LegacyTextureSlot::SpecularPower,
                        value);
                }
            }

            finishCurrent();

            if (parsedMaterials.empty())
            {
                output.error =
                    "MAT file contains no MaterialBegin section.";

                return false;
            }

            auto selected =
                parsedMaterials.begin();

            const auto matching =
                std::find_if(
                    parsedMaterials.begin(),
                    parsedMaterials.end(),
                    [&requestedName](
                        const LegacyMaterialData& material)
                    {
                        return EqualsIgnoreCase(
                            material.name,
                            requestedName);
                    });

            if (matching != parsedMaterials.end())
            {
                selected = matching;
            }
            else
            {
                ++selected->parseWarningCount;
            }

            output = std::move(*selected);

            if (output.name.empty())
            {
                output.name = requestedName;
            }

            return true;
        }

        void ResolveMaterialTextures(
            const std::filesystem::path& sourceRoot,
            const std::filesystem::path& packageDirectory,
            LegacyMaterialData& material,
            LegacyMaterialSet& output)
        {
            material.imagesDirectory =
                ResolveImagesDirectory(
                    sourceRoot,
                    packageDirectory,
                    material.sourcePath,
                    material.imagesDirectorySource);

            for (LegacyMaterialTexture& texture :
                 material.textures)
            {
                if (texture.sourceName.empty())
                {
                    continue;
                }

                const std::filesystem::path resolvedPath =
                    ResolveTexturePath(
                        sourceRoot,
                        packageDirectory,
                        material.sourcePath,
                        material.imagesDirectory,
                        texture.sourceName);

                if (resolvedPath.empty())
                {
                    texture.dds.path =
                        material.imagesDirectory /
                        ConvertLegacyPath(
                            texture.sourceName);

                    texture.dds.error =
                        "Referenced texture was not found.";

                    ++output.missingTextureCount;
                    continue;
                }

                if (!ReadDdsInfo(
                        resolvedPath,
                        texture.dds))
                {
                    ++output.invalidDdsCount;
                }
            }
        }
    }

    const char* ToString(
        const LegacyTextureSlot slot) noexcept
    {
        switch (slot)
        {
            case LegacyTextureSlot::Diffuse:
                return "Diffuse";

            case LegacyTextureSlot::Normal:
                return "Normal";

            case LegacyTextureSlot::Specular:
                return "Specular / Metalness";

            case LegacyTextureSlot::Roughness:
                return "Roughness / Env";

            case LegacyTextureSlot::Glow:
                return "Glow";

            case LegacyTextureSlot::DetailNormal:
                return "Detail Normal";

            case LegacyTextureSlot::Density:
                return "Density";

            case LegacyTextureSlot::CamouflageMask:
                return "Camouflage Mask";

            case LegacyTextureSlot::Distortion:
                return "Distortion";

            case LegacyTextureSlot::SpecularPower:
                return "Specular Power";

            case LegacyTextureSlot::Count:
                break;
        }

        return "Unknown";
    }

    const LegacyMaterialTexture*
        LegacyMaterialData::FindTexture(
            const LegacyTextureSlot slot) const noexcept
    {
        const std::size_t index =
            static_cast<std::size_t>(slot);

        return index < textures.size()
            ? &textures[index]
            : nullptr;
    }

    LegacyMaterialTexture*
        LegacyMaterialData::FindTexture(
            const LegacyTextureSlot slot) noexcept
    {
        const std::size_t index =
            static_cast<std::size_t>(slot);

        return index < textures.size()
            ? &textures[index]
            : nullptr;
    }

    const LegacyMaterialData*
        LegacyMaterialSet::Find(
            const std::string_view materialName) const noexcept
    {
        for (const LegacyMaterialData& material :
             materials)
        {
            if (EqualsIgnoreCase(
                    material.name,
                    materialName))
            {
                return &material;
            }
        }

        return nullptr;
    }

    bool LegacyMaterialReader::ReadForMesh(
        const std::filesystem::path& sourceRoot,
        const std::filesystem::path& packageDirectory,
        const std::vector<LegacyMaterialChunk>& chunks,
        LegacyMaterialSet& output) noexcept
    {
        output = {};

        try
        {
            std::unordered_set<std::string>
                processedNames;

            for (const LegacyMaterialChunk& chunk :
                 chunks)
            {
                const std::string materialName =
                    Trim(chunk.materialName);

                if (materialName.empty())
                {
                    continue;
                }

                const std::string normalizedName =
                    ToLowerAscii(materialName);

                if (!processedNames.insert(
                        normalizedName).second)
                {
                    continue;
                }

                const std::filesystem::path materialPath =
                    FindMaterialPath(
                        packageDirectory,
                        materialName);

                if (materialPath.empty())
                {
                    LegacyMaterialData missing;
                    missing.name = materialName;

                    InitializeTextureSlots(missing);

                    missing.error =
                        "Material file was not found.";

                    output.materials.push_back(
                        std::move(missing));

                    output.missingMaterialNames.
                        push_back(materialName);

                    ++output.missingMaterialCount;
                    continue;
                }

                LegacyMaterialData material;

                if (!ReadMaterialFile(
                        materialPath,
                        materialName,
                        material))
                {
                    if (material.name.empty())
                    {
                        material.name =
                            materialName;
                    }

                    output.materials.push_back(
                        std::move(material));

                    ++output.missingMaterialCount;
                    continue;
                }

                ResolveMaterialTextures(
                    sourceRoot,
                    packageDirectory,
                    material,
                    output);

                output.parseWarningCount +=
                    material.parseWarningCount;

                output.materials.push_back(
                    std::move(material));
            }

            return true;
        }
        catch (const std::exception& exception)
        {
            output.error =
                "Material analysis failed: " +
                std::string(
                    exception.what());

            return false;
        }
        catch (...)
        {
            output.error =
                "Material analysis failed with an unknown error.";

            return false;
        }
    }
}