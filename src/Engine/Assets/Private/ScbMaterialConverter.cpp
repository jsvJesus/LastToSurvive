#include "Assets/ScbMaterialConverter.h"

#include "Assets/AssetData.h"
#include "Assets/AssetPath.h"
#include "Assets/MaterialAsset.h"
#include "Assets/MaterialAssetWriter.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <locale>
#include <new>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

namespace engine::assets
{
    namespace
    {
        constexpr float DefaultAlphaCutoff =
            0.15F;

        struct SourceMaterial final
        {
            MaterialAssetDesc description;

            std::string baseColorTexture;
            std::string normalTexture;
            std::string specularTexture;
            std::string specularPowerTexture;
            std::string emissiveTexture;
            std::string imagesDirectory;
        };

        [[nodiscard]]
        std::wstring Lowercase(
            std::wstring value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](const wchar_t character)
                {
                    return static_cast<wchar_t>(
                        std::towlower(character));
                });

            return value;
        }

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
                    return static_cast<char>(
                        std::tolower(character));
                });

            return value;
        }

        [[nodiscard]]
        std::string TrimAscii(
            std::string value)
        {
            const auto isSpace =
                [](const unsigned char character)
                {
                    return
                        std::isspace(character) != 0;
                };

            value.erase(
                value.begin(),
                std::find_if(
                    value.begin(),
                    value.end(),
                    [&](const char character)
                    {
                        return !isSpace(
                            static_cast<unsigned char>(
                                character));
                    }));

            value.erase(
                std::find_if(
                    value.rbegin(),
                    value.rend(),
                    [&](const char character)
                    {
                        return !isSpace(
                            static_cast<unsigned char>(
                                character));
                    }).base(),
                value.end());

            return value;
        }

        [[nodiscard]]
        float ParseFloat(
            const std::string& value,
            const float fallback) noexcept
        {
            char* end = nullptr;

            const float parsed =
                std::strtof(
                    value.c_str(),
                    &end);

            return
                end != value.c_str() &&
                std::isfinite(parsed)
                    ? parsed
                    : fallback;
        }

        [[nodiscard]]
        bool ParseBool(
            const std::string& value)
        {
            const std::string lowered =
                LowercaseAscii(
                    TrimAscii(value));

            return
                lowered == "1" ||
                lowered == "true" ||
                lowered == "yes";
        }

        void InitializeSampler(
            MaterialAssetDesc& description) noexcept
        {
            description.sampler.filter =
                engine::graphics::
                    TextureFilter::Anisotropic;

            description.sampler.addressU =
                engine::graphics::
                    TextureAddressMode::Wrap;

            description.sampler.addressV =
                engine::graphics::
                    TextureAddressMode::Wrap;

            description.sampler.addressW =
                engine::graphics::
                    TextureAddressMode::Wrap;

            description.sampler.maximumAnisotropy =
                16U;
        }

        [[nodiscard]]
        bool ParseMaterial(
            const std::filesystem::path& path,
            SourceMaterial& output) noexcept
        {
            try
            {
                std::ifstream input(path);

                if (!input)
                {
                    return false;
                }

                input.imbue(
                    std::locale::classic());

                SourceMaterial material;

                bool forceAlphaTest = false;
                bool alphaTransparent = false;
                bool glows = false;

                float alphaReference = 0.0F;
                float selfIllumination = 0.0F;

                std::string line;

                while (std::getline(input, line))
                {
                    const std::size_t separator =
                        line.find('=');

                    if (separator == std::string::npos)
                    {
                        continue;
                    }

                    const std::string key =
                        LowercaseAscii(
                            TrimAscii(
                                line.substr(
                                    0U,
                                    separator)));

                    const std::string value =
                        TrimAscii(
                            line.substr(
                                separator + 1U));

                    if (key == "name")
                    {
                        material.description.debugName =
                            value;
                    }
                    else if (key == "texture")
                    {
                        material.baseColorTexture =
                            value;
                    }
                    else if (key == "imagesdir")
                    {
                        material.imagesDirectory =
                            value;
                    }
                    else if (key == "normalmap")
                    {
                        material.normalTexture =
                            value;
                    }
                    else if (key == "specularmap")
                    {
                        material.specularTexture =
                            value;
                    }
                    else if (key == "specpowmap")
                    {
                        material.specularPowerTexture =
                            value;
                    }
                    else if (key == "glowmap")
                    {
                        material.emissiveTexture =
                            value;
                    }
                    else if (key == "specularpower")
                    {
                        material.description.specularIntensity =
                            (std::clamp)(
                                ParseFloat(
                                    value,
                                    0.0F),
                                0.0F,
                                1.0F);
                    }
                    else if (key == "specular1power")
                    {
                        const float gloss =
                            (std::clamp)(
                                ParseFloat(
                                    value,
                                    0.0F),
                                0.0F,
                                1.0F);

                        material.description.specularPower =
                            std::exp2(
                                1.0F +
                                gloss * 10.0F);
                    }
                    else if (key == "reflectionpower")
                    {
                        material.description.reflectionFactor =
                            (std::clamp)(
                                ParseFloat(
                                    value,
                                    0.0F),
                                0.0F,
                                1.0F);
                    }
                    else if (key == "normalscale")
                    {
                        material.description.normalScale =
                            (std::clamp)(
                                ParseFloat(
                                    value,
                                    1.0F),
                                0.0F,
                                4.0F);
                    }
                    else if (key == "doublesided")
                    {
                        material.description.doubleSided =
                            ParseBool(value);
                    }
                    else if (
                        key == "forcetransparent" ||
                        key == "transparentshadows")
                    {
                        forceAlphaTest =
                            ParseBool(value);
                    }
                    else if (key == "alphatransparent")
                    {
                        alphaTransparent =
                            ParseBool(value);
                    }
                    else if (key == "glows")
                    {
                        glows =
                            ParseBool(value);
                    }
                    else if (key == "alpharef")
                    {
                        alphaReference =
                            ParseFloat(
                                value,
                                0.0F);
                    }
                    else if (key == "selfillummultiplier")
                    {
                        selfIllumination =
                            (std::max)(
                                ParseFloat(
                                    value,
                                    0.0F),
                                0.0F);
                    }
                    else if (key == "color24")
                    {
                        std::istringstream colors(value);

                        colors.imbue(
                            std::locale::classic());

                        int red = 255;
                        int green = 255;
                        int blue = 255;

                        if (colors >> red >> green >> blue)
                        {
                            material.description.baseColorFactor =
                            {
                                (std::clamp)(
                                    red,
                                    0,
                                    255) / 255.0F,

                                (std::clamp)(
                                    green,
                                    0,
                                    255) / 255.0F,

                                (std::clamp)(
                                    blue,
                                    0,
                                    255) / 255.0F,

                                1.0F
                            };
                        }
                    }
                }

                material.description.emissiveStrength =
                    selfIllumination;

                if (
                    glows &&
                    material.description.emissiveStrength <=
                        0.0F)
                {
                    material.description.emissiveStrength =
                        1.0F;
                }

                if (alphaTransparent)
                {
                    material.description.alphaMode =
                        MaterialAlphaMode::Blend;
                }
                else if (
                    forceAlphaTest ||
                    alphaReference > 0.0F)
                {
                    material.description.alphaMode =
                        MaterialAlphaMode::Mask;

                    material.description.alphaCutoff =
                        alphaReference > 0.0F
                            ? (std::clamp)(
                                alphaReference > 1.0F
                                    ? alphaReference / 255.0F
                                    : alphaReference,
                                0.0F,
                                1.0F)
                            : DefaultAlphaCutoff;
                }

                InitializeSampler(
                    material.description);

                output =
                    std::move(material);

                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]]
        bool IsSafeRelativePath(
            const std::filesystem::path& path)
        {
            if (
                path.empty() ||
                path.is_absolute() ||
                path.has_root_path())
            {
                return false;
            }

            for (const auto& component : path)
            {
                if (component == L"..")
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
        std::filesystem::path TryTexturePath(
            const std::filesystem::path& path)
        {
            if (IsRegularFile(path))
            {
                return path.lexically_normal();
            }

            std::filesystem::path ddsPath =
                path;

            ddsPath.replace_extension(
                L".dds");

            if (IsRegularFile(ddsPath))
            {
                return
                    ddsPath.lexically_normal();
            }

            return {};
        }

        [[nodiscard]]
        std::filesystem::path FindDataRoot(
            std::filesystem::path path)
        {
            path =
                path.lexically_normal();

            while (!path.empty())
            {
                if (
                    Lowercase(
                        path.filename().wstring()) ==
                    L"data")
                {
                    return path;
                }

                const std::filesystem::path parent =
                    path.parent_path();

                if (
                    parent.empty() ||
                    parent == path)
                {
                    break;
                }

                path = parent;
            }

            return {};
        }

        [[nodiscard]]
        std::filesystem::path FindPackageRoot(
            const std::filesystem::path& sourceMeshPath)
        {
            std::filesystem::path cursor =
                sourceMeshPath.parent_path();

            std::filesystem::path result =
                cursor;

            while (!cursor.empty())
            {
                if (
                    Lowercase(
                        cursor.filename().wstring()) ==
                    L"objectsdepot")
                {
                    break;
                }

                std::error_code error;

                if (
                    std::filesystem::is_directory(
                        cursor / L"Materials",
                        error) &&
                    !error)
                {
                    return cursor;
                }

                error.clear();

                if (
                    std::filesystem::is_directory(
                        cursor / L"Textures",
                        error) &&
                    !error)
                {
                    result = cursor;
                }

                const std::filesystem::path parent =
                    cursor.parent_path();

                if (
                    parent.empty() ||
                    parent == cursor)
                {
                    break;
                }

                cursor = parent;
            }

            return result;
        }

        [[nodiscard]]
        std::filesystem::path FindMaterialFile(
            const std::filesystem::path& sourceMeshPath,
            const std::string& materialName)
        {
            std::filesystem::path fileName =
                materialName.empty() ||
                materialName == "__default"
                    ? std::filesystem::path(
                        L"_DEFAULT_.mat")
                    : std::filesystem::u8path(
                        materialName).filename();

            if (
                Lowercase(
                    fileName.extension().wstring()) !=
                L".mat")
            {
                fileName += L".mat";
            }

            std::filesystem::path cursor =
                sourceMeshPath.parent_path();

            while (!cursor.empty())
            {
                for (const auto& candidate :
                    {
                        cursor /
                            L"Materials" /
                            fileName,

                        cursor /
                            fileName
                    })
                {
                    if (IsRegularFile(candidate))
                    {
                        return
                            candidate.lexically_normal();
                    }
                }

                if (
                    Lowercase(
                        cursor.filename().wstring()) ==
                    L"objectsdepot")
                {
                    break;
                }

                const std::filesystem::path parent =
                    cursor.parent_path();

                if (
                    parent.empty() ||
                    parent == cursor)
                {
                    break;
                }

                cursor = parent;
            }

            return {};
        }

        [[nodiscard]]
        std::filesystem::path FindTextureFile(
            const std::string& value,
            const std::string& imagesDirectory,
            const std::filesystem::path& materialPath,
            const std::filesystem::path& sourceMeshPath,
            const std::filesystem::path& packageRoot)
        {
            if (value.empty())
            {
                return {};
            }

            const std::filesystem::path requested =
                std::filesystem::u8path(value).
                    lexically_normal();

            if (requested.is_absolute())
            {
                return
                    TryTexturePath(
                        requested);
            }

            std::vector<std::filesystem::path>
                directories;

            const std::filesystem::path dataRoot =
                FindDataRoot(
                    sourceMeshPath);

            if (!imagesDirectory.empty())
            {
                std::filesystem::path imagesPath =
                    std::filesystem::u8path(
                        imagesDirectory).
                        lexically_normal();

                if (imagesPath.is_absolute())
                {
                    directories.push_back(
                        imagesPath);
                }
                else
                {
                    if (!dataRoot.empty())
                    {
                        auto component =
                            imagesPath.begin();

                        if (
                            component != imagesPath.end() &&
                            Lowercase(
                                component->wstring()) ==
                            L"data")
                        {
                            std::filesystem::path relative;

                            for (
                                ++component;
                                component != imagesPath.end();
                                ++component)
                            {
                                relative /= *component;
                            }

                            directories.push_back(
                                dataRoot /
                                relative);
                        }
                        else
                        {
                            directories.push_back(
                                dataRoot /
                                imagesPath);
                        }
                    }

                    directories.push_back(
                        materialPath.parent_path() /
                        imagesPath);

                    directories.push_back(
                        packageRoot /
                        imagesPath);
                }
            }

            directories.push_back(
                materialPath.parent_path().
                    parent_path() /
                L"Textures");

            directories.push_back(
                packageRoot /
                L"Textures");

            directories.push_back(
                sourceMeshPath.parent_path() /
                L"Textures");

            directories.push_back(
                materialPath.parent_path());

            directories.push_back(
                packageRoot);

            for (const auto& directory : directories)
            {
                std::filesystem::path result =
                    TryTexturePath(
                        directory /
                        requested);

                if (!result.empty())
                {
                    return result;
                }

                if (requested.has_parent_path())
                {
                    result =
                        TryTexturePath(
                            directory /
                            requested.filename());

                    if (!result.empty())
                    {
                        return result;
                    }
                }
            }

            return {};
        }

        [[nodiscard]]
        std::filesystem::path BuildTextureRelativePath(
            const std::filesystem::path& sourceTexture)
        {
            std::filesystem::path cursor =
                sourceTexture.parent_path();

            while (!cursor.empty())
            {
                if (
                    Lowercase(
                        cursor.filename().wstring()) ==
                    L"textures")
                {
                    std::error_code error;

                    const std::filesystem::path relative =
                        std::filesystem::relative(
                            sourceTexture,
                            cursor,
                            error);

                    if (
                        !error &&
                        IsSafeRelativePath(relative))
                    {
                        return relative;
                    }

                    break;
                }

                const std::filesystem::path parent =
                    cursor.parent_path();

                if (
                    parent.empty() ||
                    parent == cursor)
                {
                    break;
                }

                cursor = parent;
            }

            return
                sourceTexture.filename();
        }

        [[nodiscard]]
        AssetResult BuildTextureAssetPath(
            const std::filesystem::path& texturePath,
            AssetPath& output)
        {
            const std::filesystem::path dataRoot =
                FindDataRoot(
                    texturePath);

            if (dataRoot.empty())
            {
                return AssetResult::InvalidPath;
            }

            std::error_code error;

            const std::filesystem::path relative =
                std::filesystem::relative(
                    texturePath,
                    dataRoot,
                    error);

            if (
                error ||
                !IsSafeRelativePath(relative))
            {
                return AssetResult::InvalidPath;
            }

            const std::filesystem::path logical =
                std::filesystem::path(
                    L"Data") /
                relative;

            return AssetPath::TryCreate(
                logical.generic_u8string(),
                output);
        }

        [[nodiscard]]
        AssetResult CopyTexture(
            const std::filesystem::path& sourceTexture,
            const std::filesystem::path& destinationMeshPath,
            AssetPath& output,
            std::wstring& error)
        {
            const std::filesystem::path relative =
                BuildTextureRelativePath(
                    sourceTexture);

            if (!IsSafeRelativePath(relative))
            {
                error =
                    L"Texture path contains parent traversal.";

                return AssetResult::InvalidPath;
            }

            const std::filesystem::path destination =
                destinationMeshPath.
                    parent_path() /
                L"Textures" /
                relative;

            std::error_code filesystemError;

            std::filesystem::create_directories(
                destination.parent_path(),
                filesystemError);

            if (filesystemError)
            {
                error =
                    L"Failed to create the texture directory.";

                return AssetResult::IoError;
            }

            if (
                sourceTexture.lexically_normal() !=
                destination.lexically_normal())
            {
                filesystemError.clear();

                std::filesystem::copy_file(
                    sourceTexture,
                    destination,
                    std::filesystem::
                        copy_options::overwrite_existing,
                    filesystemError);

                if (filesystemError)
                {
                    error =
                        L"Failed to copy a material texture.";

                    return AssetResult::IoError;
                }
            }

            const AssetResult pathResult =
                BuildTextureAssetPath(
                    destination,
                    output);

            if (Failed(pathResult))
            {
                error =
                    L"Failed to create a logical texture path.";
            }

            return pathResult;
        }

        [[nodiscard]]
        AssetResult ConvertTexture(
            const std::string& textureName,
            const std::string& imagesDirectory,
            const std::filesystem::path& materialPath,
            const std::filesystem::path& sourceMeshPath,
            const std::filesystem::path& packageRoot,
            const std::filesystem::path& destinationMeshPath,
            std::optional<AssetPath>& output,
            std::wstring& error)
        {
            output.reset();

            const std::filesystem::path sourceTexture =
                FindTextureFile(
                    textureName,
                    imagesDirectory,
                    materialPath,
                    sourceMeshPath,
                    packageRoot);

            if (sourceTexture.empty())
            {
                return AssetResult::Success;
            }

            AssetPath path;

            const AssetResult result =
                CopyTexture(
                    sourceTexture,
                    destinationMeshPath,
                    path,
                    error);

            if (Failed(result))
            {
                return result;
            }

            output =
                std::move(path);

            return AssetResult::Success;
        }

        [[nodiscard]]
        std::wstring SafeFileName(
            const std::string& value)
        {
            std::wstring result;

            try
            {
                result =
                    std::filesystem::u8path(value).
                        filename().
                        wstring();
            }
            catch (...)
            {
                result.clear();
            }

            if (result.empty())
            {
                result =
                    L"default";
            }

            for (wchar_t& character : result)
            {
                if (
                    character < 32 ||
                    character == L'<' ||
                    character == L'>' ||
                    character == L':' ||
                    character == L'"' ||
                    character == L'/' ||
                    character == L'\\' ||
                    character == L'|' ||
                    character == L'?' ||
                    character == L'*')
                {
                    character =
                        L'_';
                }
            }

            return result;
        }

        [[nodiscard]]
        std::wstring SlotNumber(
            const std::size_t slot)
        {
            std::wstring value =
                std::to_wstring(slot);

            if (value.size() < 4U)
            {
                value.insert(
                    value.begin(),
                    4U - value.size(),
                    L'0');
            }

            return value;
        }

        [[nodiscard]]
        std::filesystem::path BuildMaterialPath(
            const std::filesystem::path& destinationMeshPath,
            const std::size_t slot,
            const std::string& materialName)
        {
            std::wstring filename =
                destinationMeshPath.
                    stem().
                    wstring();

            filename += L"_";
            filename += SlotNumber(slot);
            filename += L"_";
            filename += SafeFileName(materialName);
            filename += L".material";

            return
                destinationMeshPath.
                    parent_path() /
                L"Materials" /
                filename;
        }

        [[nodiscard]]
        AssetResult WriteMaterial(
            const std::filesystem::path& path,
            const MaterialAsset& material,
            std::wstring& error)
        {
            AssetData encoded;

            const AssetResult encodeResult =
                MaterialAssetWriter::Encode(
                    material,
                    encoded);

            if (Failed(encodeResult))
            {
                error =
                    L"Material encoding failed.";

                return encodeResult;
            }

            std::error_code filesystemError;

            std::filesystem::create_directories(
                path.parent_path(),
                filesystemError);

            if (filesystemError)
            {
                error =
                    L"Failed to create the material directory.";

                return AssetResult::IoError;
            }

            std::filesystem::path temporary =
                path;

            temporary += L".tmp";

            std::filesystem::remove(
                temporary,
                filesystemError);

            {
                std::ofstream output(
                    temporary,
                    std::ios::binary |
                    std::ios::trunc);

                if (!output)
                {
                    error =
                        L"Failed to create the temporary material.";

                    return AssetResult::IoError;
                }

                output.write(
                    reinterpret_cast<const char*>(
                        encoded.GetData()),
                    static_cast<std::streamsize>(
                        encoded.GetSize()));

                output.flush();

                if (!output.good())
                {
                    output.close();

                    std::filesystem::remove(
                        temporary,
                        filesystemError);

                    error =
                        L"Failed to write the complete material.";

                    return AssetResult::IoError;
                }
            }

            if (!MoveFileExW(
                    temporary.c_str(),
                    path.c_str(),
                    MOVEFILE_REPLACE_EXISTING |
                        MOVEFILE_WRITE_THROUGH))
            {
                std::filesystem::remove(
                    temporary,
                    filesystemError);

                error =
                    L"Failed to replace the material file.";

                return AssetResult::IoError;
            }

            return AssetResult::Success;
        }

        void RemoveStaleMaterials(
            const std::filesystem::path& destinationMeshPath,
            const std::unordered_set<std::wstring>&
                generatedFiles) noexcept
        {
            try
            {
                const std::filesystem::path directory =
                    destinationMeshPath.
                        parent_path() /
                    L"Materials";

                std::error_code error;

                if (
                    !std::filesystem::is_directory(
                        directory,
                        error) ||
                    error)
                {
                    return;
                }

                const std::wstring prefix =
                    Lowercase(
                        destinationMeshPath.
                            stem().
                            wstring() +
                        L"_");

                for (
                    std::filesystem::directory_iterator
                        iterator(directory, error),
                        end;

                    !error &&
                    iterator != end;

                    iterator.increment(error))
                {
                    if (
                        !iterator->is_regular_file(error) ||
                        error)
                    {
                        error.clear();
                        continue;
                    }

                    const std::filesystem::path file =
                        iterator->path();

                    if (
                        Lowercase(
                            file.extension().wstring()) !=
                            L".material")
                    {
                        continue;
                    }

                    const std::wstring filename =
                        Lowercase(
                            file.filename().wstring());

                    if (
                        filename.rfind(
                            prefix,
                            0U) != 0U ||
                        generatedFiles.find(filename) !=
                            generatedFiles.end())
                    {
                        continue;
                    }

                    std::filesystem::remove(
                        file,
                        error);

                    error.clear();
                }
            }
            catch (...)
            {
            }
        }
    }

    AssetResult ScbMaterialConverter::Convert(
        const std::filesystem::path& sourceMeshPath,
        const std::filesystem::path& destinationMeshPath,
        const std::vector<std::string>& materialNames,
        std::wstring& error) noexcept
    {
        error.clear();

        try
        {
            if (
                sourceMeshPath.empty() ||
                destinationMeshPath.empty() ||
                materialNames.empty())
            {
                error =
                    L"Material conversion arguments are invalid.";

                return AssetResult::InvalidArgument;
            }

            const std::filesystem::path packageRoot =
                FindPackageRoot(
                    sourceMeshPath);

            std::unordered_set<std::wstring>
                generatedFiles;

            generatedFiles.reserve(
                materialNames.size());

            for (
                std::size_t slot = 0U;
                slot < materialNames.size();
                ++slot)
            {
                const std::string materialName =
                    materialNames[slot].empty()
                        ? "__default"
                        : materialNames[slot];

                const std::filesystem::path sourceMaterialPath =
                    FindMaterialFile(
                        sourceMeshPath,
                        materialName);

                SourceMaterial sourceMaterial;

                if (
                    sourceMaterialPath.empty() ||
                    !ParseMaterial(
                        sourceMaterialPath,
                        sourceMaterial))
                {
                    sourceMaterial.description.debugName =
                        materialName;

                    InitializeSampler(
                        sourceMaterial.description);
                }

                if (
                    sourceMaterial.description.debugName.empty())
                {
                    sourceMaterial.description.debugName =
                        materialName;
                }

                AssetResult result =
                    ConvertTexture(
                        sourceMaterial.baseColorTexture,
                        sourceMaterial.imagesDirectory,
                        sourceMaterialPath,
                        sourceMeshPath,
                        packageRoot,
                        destinationMeshPath,
                        sourceMaterial.description.
                            baseColorTexture,
                        error);

                if (Failed(result))
                {
                    return result;
                }

                result =
                    ConvertTexture(
                        sourceMaterial.normalTexture,
                        sourceMaterial.imagesDirectory,
                        sourceMaterialPath,
                        sourceMeshPath,
                        packageRoot,
                        destinationMeshPath,
                        sourceMaterial.description.
                            normalTexture,
                        error);

                if (Failed(result))
                {
                    return result;
                }

                result =
                    ConvertTexture(
                        sourceMaterial.specularTexture,
                        sourceMaterial.imagesDirectory,
                        sourceMaterialPath,
                        sourceMeshPath,
                        packageRoot,
                        destinationMeshPath,
                        sourceMaterial.description.
                            specularGlossTexture,
                        error);

                if (Failed(result))
                {
                    return result;
                }

                result =
                    ConvertTexture(
                        sourceMaterial.specularPowerTexture,
                        sourceMaterial.imagesDirectory,
                        sourceMaterialPath,
                        sourceMeshPath,
                        packageRoot,
                        destinationMeshPath,
                        sourceMaterial.description.
                            specularPowerTexture,
                        error);

                if (Failed(result))
                {
                    return result;
                }

                result =
                    ConvertTexture(
                        sourceMaterial.emissiveTexture,
                        sourceMaterial.imagesDirectory,
                        sourceMaterialPath,
                        sourceMeshPath,
                        packageRoot,
                        destinationMeshPath,
                        sourceMaterial.description.
                            emissiveTexture,
                        error);

                if (Failed(result))
                {
                    return result;
                }

                MaterialAsset material;

                result =
                    material.Initialize(
                        std::move(
                            sourceMaterial.description));

                if (Failed(result))
                {
                    error =
                        L"Converted material validation failed.";

                    return result;
                }

                const std::filesystem::path destination =
                    BuildMaterialPath(
                        destinationMeshPath,
                        slot,
                        materialName);

                result =
                    WriteMaterial(
                        destination,
                        material,
                        error);

                if (Failed(result))
                {
                    return result;
                }

                generatedFiles.insert(
                    Lowercase(
                        destination.
                            filename().
                            wstring()));
            }

            RemoveStaleMaterials(
                destinationMeshPath,
                generatedFiles);

            return AssetResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            error =
                L"Not enough memory to convert SCB materials.";

            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            error =
                L"Unexpected SCB material conversion failure.";

            return AssetResult::InternalError;
        }
    }
}