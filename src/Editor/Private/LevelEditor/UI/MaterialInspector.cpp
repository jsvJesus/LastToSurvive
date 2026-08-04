#include "Editor/LevelEditor/UI/MaterialInspector.h"

#include "Editor/LevelEditor/Rendering/StaticMeshRenderer.h"

#include <Assets/AssetData.h>
#include <Assets/AssetMetadata.h>
#include <Assets/AssetPath.h>
#include <Assets/AssetResult.h>
#include <Assets/AssetType.h>
#include <Assets/LtsMaterialWriter.h>
#include <Assets/MaterialAssetLoader.h>
#include <Assets/SkeletalMeshAsset.h>
#include <Assets/SkeletalMeshAssetLoader.h>

#include <Windows.h>
#include <ShObjIdl.h>
#include <imgui.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lts::editor
{
    namespace
    {
        constexpr std::uintmax_t MaximumMaterialFileSize =
            16U * 1024U * 1024U;

        constexpr std::uintmax_t MaximumSkeletalMeshFileSize =
            512U * 1024U * 1024U;

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
        bool ContainsParentTraversal(
            const std::filesystem::path& path)
        {
            for (const auto& component : path)
            {
                if (component == L"..")
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]]
        std::filesystem::path FindGameRoot() noexcept
        {
            try
            {
                std::error_code error;

                std::filesystem::path current =
                    std::filesystem::current_path(error);

                if (error)
                {
                    return {};
                }

                current =
                    std::filesystem::absolute(
                        current,
                        error).lexically_normal();

                if (error)
                {
                    return {};
                }

                while (!current.empty())
                {
                    error.clear();

                    if (std::filesystem::is_directory(
                            current / L"Data",
                            error) &&
                        !error)
                    {
                        return current;
                    }

                    error.clear();

                    const std::filesystem::path nestedGame =
                        current / L"game";

                    if (std::filesystem::is_directory(
                            nestedGame / L"Data",
                            error) &&
                        !error)
                    {
                        return nestedGame.lexically_normal();
                    }

                    const std::filesystem::path parent =
                        current.parent_path();

                    if (parent.empty() || parent == current)
                    {
                        break;
                    }

                    current = parent;
                }
            }
            catch (...)
            {
            }

            return {};
        }

        [[nodiscard]]
        engine::assets::AssetResult ReadAssetData(
            const std::filesystem::path& file,
            const std::uintmax_t maximumFileSize,
            engine::assets::AssetData& output) noexcept
        {
            output.Clear();

            try
            {
                std::error_code error;

                const std::uintmax_t fileSize =
                    std::filesystem::file_size(
                        file,
                        error);

                if (error)
                {
                    return engine::assets::AssetResult::IoError;
                }

                if (fileSize == 0U ||
                    fileSize > maximumFileSize ||
                    fileSize >
                        static_cast<std::uintmax_t>(
                            (std::numeric_limits<
                                std::streamsize>::max)()))
                {
                    return engine::assets::AssetResult::FileTooLarge;
                }

                const auto resizeResult =
                    output.Resize(
                        static_cast<std::size_t>(
                            fileSize));

                if (engine::assets::Failed(resizeResult))
                {
                    return resizeResult;
                }

                std::ifstream input(
                    file,
                    std::ios::binary);

                if (!input)
                {
                    output.Clear();

                    return engine::assets::AssetResult::IoError;
                }

                input.read(
                    reinterpret_cast<char*>(
                        output.GetData()),
                    static_cast<std::streamsize>(
                        fileSize));

                if (!input)
                {
                    output.Clear();

                    return engine::assets::AssetResult::IoError;
                }

                return engine::assets::AssetResult::Success;
            }
            catch (const std::bad_alloc&)
            {
                output.Clear();

                return engine::assets::AssetResult::OutOfMemory;
            }
            catch (...)
            {
                output.Clear();

                return engine::assets::AssetResult::InternalError;
            }
        }

        [[nodiscard]]
        engine::assets::AssetResult CreateMetadata(
            const std::filesystem::path& file,
            const std::filesystem::path& gameRoot,
            const engine::assets::AssetType type,
            const std::size_t fileSize,
            engine::assets::AssetMetadata& output) noexcept
        {
            try
            {
                std::error_code error;

                const std::filesystem::path relative =
                    std::filesystem::relative(
                        file,
                        gameRoot,
                        error);

                if (error ||
                    relative.empty() ||
                    ContainsParentTraversal(relative))
                {
                    return engine::assets::AssetResult::InvalidPath;
                }

                engine::assets::AssetPath assetPath;

                const auto pathResult =
                    engine::assets::AssetPath::TryCreate(
                        relative.generic_u8string(),
                        assetPath);

                if (engine::assets::Failed(pathResult))
                {
                    return pathResult;
                }

                output = {};
                output.path = std::move(assetPath);
                output.id = output.path.GetId();
                output.type = type;
                output.schemaVersion =
                    type == engine::assets::AssetType::Material
                        ? 2U
                        : 1U;

                output.sourceSize =
                    static_cast<std::uint64_t>(
                        fileSize);

                return engine::assets::AssetResult::Success;
            }
            catch (const std::bad_alloc&)
            {
                return engine::assets::AssetResult::OutOfMemory;
            }
            catch (...)
            {
                return engine::assets::AssetResult::InternalError;
            }
        }

        [[nodiscard]]
        bool LoadMaterialFile(
            const std::filesystem::path& file,
            const std::filesystem::path& gameRoot,
            engine::assets::MaterialAssetDesc& output,
            std::string& errorText) noexcept
        {
            output = {};
            errorText.clear();

            engine::assets::AssetData source;

            auto result =
                ReadAssetData(
                    file,
                    MaximumMaterialFileSize,
                    source);

            if (engine::assets::Failed(result))
            {
                errorText =
                    "Failed to read material: ";

                errorText +=
                    engine::assets::ToString(result);

                return false;
            }

            engine::assets::AssetMetadata metadata;

            result =
                CreateMetadata(
                    file,
                    gameRoot,
                    engine::assets::AssetType::Material,
                    source.GetSize(),
                    metadata);

            if (engine::assets::Failed(result))
            {
                errorText =
                    "Failed to create material metadata: ";

                errorText +=
                    engine::assets::ToString(result);

                return false;
            }

            engine::assets::MaterialAssetLoader loader;

            std::unique_ptr<engine::assets::LoadedAsset>
                loadedAsset;

            result =
                loader.Load(
                    metadata,
                    source,
                    loadedAsset);

            if (engine::assets::Failed(result) ||
                loadedAsset == nullptr ||
                loadedAsset->GetType() !=
                    engine::assets::AssetType::Material)
            {
                errorText =
                    "Failed to decode material: ";

                errorText +=
                    engine::assets::ToString(result);

                return false;
            }

            const auto* materialAsset =
                static_cast<
                    const engine::assets::
                        MaterialLoadedAsset*>(
                            loadedAsset.get());

            output =
                materialAsset->GetMaterial().GetDesc();

            return true;
        }

        [[nodiscard]]
        bool WriteAssetData(
            const std::filesystem::path& file,
            const engine::assets::AssetData& data,
            std::string& errorText) noexcept
        {
            errorText.clear();

            try
            {
                if (data.GetData() == nullptr ||
                    data.GetSize() == 0U)
                {
                    errorText =
                        "Material writer returned empty data.";

                    return false;
                }

                std::error_code error;

                std::filesystem::create_directories(
                    file.parent_path(),
                    error);

                if (error)
                {
                    errorText =
                        "Failed to create material directory.";

                    return false;
                }

                std::filesystem::path temporary =
                    file;

                temporary += L".tmp";

                error.clear();

                std::filesystem::remove(
                    temporary,
                    error);

                std::ofstream output(
                    temporary,
                    std::ios::binary |
                    std::ios::trunc);

                if (!output)
                {
                    errorText =
                        "Failed to create temporary material file.";

                    return false;
                }

                output.write(
                    reinterpret_cast<const char*>(
                        data.GetData()),
                    static_cast<std::streamsize>(
                        data.GetSize()));

                output.flush();
                output.close();

                if (!output)
                {
                    error.clear();

                    std::filesystem::remove(
                        temporary,
                        error);

                    errorText =
                        "Failed to write complete material data.";

                    return false;
                }

                error.clear();

                std::filesystem::remove(
                    file,
                    error);

                error.clear();

                std::filesystem::rename(
                    temporary,
                    file,
                    error);

                if (error)
                {
                    std::error_code cleanupError;

                    std::filesystem::remove(
                        temporary,
                        cleanupError);

                    errorText =
                        "Failed to replace material file.";

                    return false;
                }

                return true;
            }
            catch (const std::bad_alloc&)
            {
                errorText =
                    "Not enough memory to save material.";

                return false;
            }
            catch (...)
            {
                errorText =
                    "Unexpected material save failure.";

                return false;
            }
        }

        [[nodiscard]]
        bool SaveMaterialFile(
            const std::filesystem::path& file,
            const engine::assets::MaterialAssetDesc& desc,
            std::string& errorText) noexcept
        {
            engine::assets::MaterialAsset material;

            const auto initializeResult =
                material.Initialize(desc);

            if (engine::assets::Failed(
                    initializeResult))
            {
                errorText =
                    "Material validation failed: ";

                errorText +=
                    engine::assets::ToString(
                        initializeResult);

                return false;
            }

            engine::assets::AssetData encoded;

            const auto encodeResult =
                engine::assets::LtsMaterialWriter::Encode(
                    material,
                    encoded);

            if (engine::assets::Failed(encodeResult))
            {
                errorText =
                    "Material encoding failed: ";

                errorText +=
                    engine::assets::ToString(
                        encodeResult);

                return false;
            }

            return WriteAssetData(
                file,
                encoded,
                errorText);
        }

        [[nodiscard]]
        bool FindMeshesRoot(
            const std::filesystem::path& meshFile,
            std::filesystem::path& output)
        {
            output.clear();

            std::filesystem::path cursor =
                meshFile.parent_path();

            while (!cursor.empty())
            {
                if (Lowercase(
                        cursor.filename().wstring()) ==
                    L"meshes")
                {
                    output = cursor;

                    return true;
                }

                const std::filesystem::path parent =
                    cursor.parent_path();

                if (parent.empty() || parent == cursor)
                {
                    break;
                }

                cursor = parent;
            }

            return false;
        }

        [[nodiscard]]
        std::vector<std::filesystem::path>
            FindMirroredMaterialFiles(
                const std::filesystem::path& meshFile)
        {
            std::vector<std::filesystem::path> matching;
            std::vector<std::filesystem::path> legacy;

            try
            {
                std::filesystem::path meshesRoot;

                if (!FindMeshesRoot(
                        meshFile,
                        meshesRoot))
                {
                    return {};
                }

                std::error_code error;

                const std::filesystem::path package =
                    std::filesystem::relative(
                        meshFile.parent_path(),
                        meshesRoot,
                        error);

                if (error)
                {
                    return {};
                }

                const std::filesystem::path directory =
                    meshesRoot.parent_path() /
                    L"Materials" /
                    package;

                if (!std::filesystem::is_directory(
                        directory,
                        error) ||
                    error)
                {
                    return {};
                }

                const std::wstring prefix =
                    Lowercase(
                        meshFile.stem().wstring() +
                        L"_");

                std::filesystem::directory_iterator iterator(
                    directory,
                    error);

                const std::filesystem::directory_iterator end;

                while (!error && iterator != end)
                {
                    const bool regularFile =
                        iterator->is_regular_file(error);

                    if (error)
                    {
                        break;
                    }

                    if (regularFile &&
                        Lowercase(
                            iterator->path().
                                extension().
                                wstring()) ==
                            L".material")
                    {
                        const std::filesystem::path file =
                            iterator->path().
                                lexically_normal();

                        legacy.push_back(file);

                        const std::wstring filename =
                            Lowercase(
                                file.filename().wstring());

                        if (filename.rfind(prefix, 0U) == 0U)
                        {
                            matching.push_back(file);
                        }
                    }

                    iterator.increment(error);
                }
            }
            catch (...)
            {
                return {};
            }

            std::vector<std::filesystem::path> result =
                !matching.empty()
                    ? std::move(matching)
                    : std::move(legacy);

            std::sort(
                result.begin(),
                result.end(),
                [](const auto& left,
                   const auto& right)
                {
                    return Lowercase(
                               left.filename().wstring()) <
                           Lowercase(
                               right.filename().wstring());
                });

            return result;
        }

        [[nodiscard]]
        std::vector<std::filesystem::path>
            FindSkeletalMaterialFiles(
                const std::filesystem::path& meshFile,
                const std::filesystem::path& gameRoot)
        {
            engine::assets::AssetData source;

            auto result =
                ReadAssetData(
                    meshFile,
                    MaximumSkeletalMeshFileSize,
                    source);

            if (engine::assets::Failed(result))
            {
                return {};
            }

            engine::assets::AssetMetadata metadata;

            result =
                CreateMetadata(
                    meshFile,
                    gameRoot,
                    engine::assets::AssetType::SkeletalMesh,
                    source.GetSize(),
                    metadata);

            if (engine::assets::Failed(result))
            {
                return {};
            }

            engine::assets::SkeletalMeshAssetLoader loader;

            std::unique_ptr<engine::assets::LoadedAsset>
                loadedAsset;

            result =
                loader.Load(
                    metadata,
                    source,
                    loadedAsset);

            if (engine::assets::Failed(result) ||
                loadedAsset == nullptr ||
                loadedAsset->GetType() !=
                    engine::assets::AssetType::SkeletalMesh)
            {
                return {};
            }

            const auto* loadedMesh =
                static_cast<
                    const engine::assets::
                        SkeletalMeshLoadedAsset*>(
                            loadedAsset.get());

            const engine::assets::SkeletalMeshAsset&
                skeletalMesh =
                    loadedMesh->GetSkeletalMesh();

            std::vector<std::filesystem::path> files;
            std::unordered_set<std::wstring> uniqueFiles;

            for (std::size_t sectionIndex = 0U;
                 sectionIndex <
                     skeletalMesh.GetSectionCount();
                 ++sectionIndex)
            {
                const engine::assets::SkeletalMeshSection*
                    section =
                        skeletalMesh.GetSection(
                            sectionIndex);

                if (section == nullptr ||
                    section->materialAssetPath.empty())
                {
                    continue;
                }

                std::filesystem::path materialFile =
                    std::filesystem::u8path(
                        section->materialAssetPath);

                if (materialFile.is_relative())
                {
                    const auto firstComponent =
                        materialFile.begin();

                    if (firstComponent != materialFile.end() &&
                        Lowercase(
                            firstComponent->wstring()) ==
                            L"data")
                    {
                        materialFile =
                            gameRoot /
                            materialFile;
                    }
                    else
                    {
                        materialFile =
                            gameRoot /
                            L"Data" /
                            materialFile;
                    }
                }

                materialFile =
                    materialFile.lexically_normal();

                const std::wstring key =
                    Lowercase(
                        materialFile.wstring());

                if (uniqueFiles.insert(key).second)
                {
                    files.push_back(
                        std::move(materialFile));
                }
            }

            std::sort(
                files.begin(),
                files.end(),
                [](const auto& left,
                   const auto& right)
                {
                    return Lowercase(
                               left.filename().wstring()) <
                           Lowercase(
                               right.filename().wstring());
                });

            return files;
        }

        [[nodiscard]]
        std::filesystem::path ResolveTextureDirectory(
            const std::filesystem::path& meshFile)
        {
            std::filesystem::path meshesRoot;

            if (!FindMeshesRoot(
                    meshFile,
                    meshesRoot))
            {
                return {};
            }

            std::error_code error;

            const std::filesystem::path package =
                std::filesystem::relative(
                    meshFile.parent_path(),
                    meshesRoot,
                    error);

            if (error)
            {
                return {};
            }

            return (
                meshesRoot.parent_path() /
                L"Textures" /
                package).lexically_normal();
        }

        class ComScope final
        {
        public:
            ComScope() noexcept
            {
                const HRESULT result =
                    CoInitializeEx(
                        nullptr,
                        COINIT_APARTMENTTHREADED |
                        COINIT_DISABLE_OLE1DDE);

                uninitialize_ =
                    result == S_OK ||
                    result == S_FALSE;
            }

            ~ComScope()
            {
                if (uninitialize_)
                {
                    CoUninitialize();
                }
            }

        private:
            bool uninitialize_ = false;
        };

        [[nodiscard]]
        bool SelectTextureFile(
            std::filesystem::path& output)
        {
            output.clear();

            ComScope comScope;

            Microsoft::WRL::ComPtr<IFileOpenDialog>
                dialog;

            if (FAILED(
                    CoCreateInstance(
                        CLSID_FileOpenDialog,
                        nullptr,
                        CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(&dialog))))
            {
                return false;
            }

            constexpr COMDLG_FILTERSPEC filters[]
            {
                {
                    L"Texture files",
                    L"*.dds;*.png;*.jpg;*.jpeg;*.bmp;*.tif;*.tiff"
                },
                {
                    L"DirectDraw Surface (*.dds)",
                    L"*.dds"
                },
                {
                    L"Common image files",
                    L"*.png;*.jpg;*.jpeg;*.bmp;*.tif;*.tiff"
                },
                {
                    L"All files (*.*)",
                    L"*.*"
                }
            };

            if (FAILED(
                    dialog->SetFileTypes(
                        static_cast<UINT>(
                            std::size(filters)),
                        filters)))
            {
                return false;
            }

            static_cast<void>(
                dialog->SetTitle(
                    L"Select material texture"));

            if (FAILED(
                    dialog->Show(
                        GetActiveWindow())))
            {
                return false;
            }

            Microsoft::WRL::ComPtr<IShellItem>
                item;

            if (FAILED(
                    dialog->GetResult(&item)))
            {
                return false;
            }

            PWSTR selectedPath = nullptr;

            if (FAILED(
                    item->GetDisplayName(
                        SIGDN_FILESYSPATH,
                        &selectedPath)))
            {
                return false;
            }

            output =
                std::filesystem::path(
                    selectedPath).
                    lexically_normal();

            CoTaskMemFree(selectedPath);

            return !output.empty();
        }

        [[nodiscard]]
        bool IsInsideGameData(
            const std::filesystem::path& file,
            const std::filesystem::path& gameRoot,
            std::filesystem::path& relative)
        {
            relative.clear();

            std::error_code error;

            relative =
                std::filesystem::relative(
                    file,
                    gameRoot,
                    error);

            if (error ||
                relative.empty() ||
                ContainsParentTraversal(relative))
            {
                relative.clear();

                return false;
            }

            const auto firstComponent =
                relative.begin();

            return firstComponent != relative.end() &&
                Lowercase(
                    firstComponent->wstring()) ==
                    L"data";
        }

        [[nodiscard]]
        bool CopyTextureIntoProject(
            const std::filesystem::path& source,
            const std::filesystem::path& destinationDirectory,
            std::filesystem::path& output,
            std::string& errorText)
        {
            output.clear();
            errorText.clear();

            try
            {
                std::error_code error;

                std::filesystem::create_directories(
                    destinationDirectory,
                    error);

                if (error)
                {
                    errorText =
                        "Failed to create texture directory.";

                    return false;
                }

                output =
                    destinationDirectory /
                    source.filename();

                bool sameFile = false;

                error.clear();

                if (std::filesystem::exists(output, error) &&
                    !error)
                {
                    error.clear();

                    sameFile =
                        std::filesystem::equivalent(
                            source,
                            output,
                            error) &&
                        !error;
                }

                if (!sameFile)
                {
                    error.clear();

                    std::filesystem::copy_file(
                        source,
                        output,
                        std::filesystem::
                            copy_options::
                                overwrite_existing,
                        error);

                    if (error)
                    {
                        output.clear();

                        errorText =
                            "Failed to copy texture into Data/Textures.";

                        return false;
                    }
                }

                output = output.lexically_normal();

                return true;
            }
            catch (const std::bad_alloc&)
            {
                errorText =
                    "Not enough memory to copy texture.";

                return false;
            }
            catch (...)
            {
                errorText =
                    "Unexpected texture copy failure.";

                return false;
            }
        }
    }

    void MaterialInspector::Reset() noexcept
    {
        meshAssetPath_.clear();
        gameRoot_.clear();
        meshFile_.clear();
        textureDirectory_.clear();

        slots_.clear();

        status_.clear();
        statusIsError_ = false;
    }

    bool MaterialInspector::Reload(
        const std::wstring& meshAssetPath) noexcept
    {
        Reset();

        meshAssetPath_ = meshAssetPath;

        try
        {
            gameRoot_ = FindGameRoot();

            if (gameRoot_.empty())
            {
                status_ =
                    "Could not find the game directory.";

                statusIsError_ = true;

                return false;
            }

            meshFile_ =
                std::filesystem::path(
                    meshAssetPath_);

            if (meshFile_.is_relative())
            {
                meshFile_ =
                    gameRoot_ /
                    meshFile_;
            }

            meshFile_ =
                meshFile_.lexically_normal();

            std::error_code error;

            if (!std::filesystem::is_regular_file(
                    meshFile_,
                    error) ||
                error)
            {
                status_ =
                    "Selected mesh file does not exist.";

                statusIsError_ = true;

                return false;
            }

            textureDirectory_ =
                ResolveTextureDirectory(
                    meshFile_);

            const std::wstring extension =
                Lowercase(
                    meshFile_.
                        extension().
                        wstring());

            std::vector<std::filesystem::path>
                materialFiles;

            if (extension == L".skm")
            {
                materialFiles =
                    FindSkeletalMaterialFiles(
                        meshFile_,
                        gameRoot_);

                if (materialFiles.empty())
                {
                    materialFiles =
                        FindMirroredMaterialFiles(
                            meshFile_);
                }
            }
            else if (extension == L".sm")
            {
                materialFiles =
                    FindMirroredMaterialFiles(
                        meshFile_);
            }
            else
            {
                status_ =
                    "Material Inspector supports only .sm and .skm.";

                statusIsError_ = true;

                return false;
            }

            for (const auto& materialFile : materialFiles)
            {
                MaterialSlot slot;
                slot.file = materialFile;

                std::string loadError;

                if (!LoadMaterialFile(
                        materialFile,
                        gameRoot_,
                        slot.original,
                        loadError))
                {
                    slot.message =
                        std::move(loadError);

                    slot.error = true;
                }
                else
                {
                    slot.edited =
                        slot.original;
                }

                slots_.push_back(
                    std::move(slot));
            }

            if (slots_.empty())
            {
                status_ =
                    "No .material files were found for this mesh.";

                statusIsError_ = true;

                return false;
            }

            status_ =
                "Material slots loaded: " +
                std::to_string(
                    slots_.size());

            statusIsError_ = false;

            return true;
        }
        catch (const std::bad_alloc&)
        {
            status_ =
                "Not enough memory to load materials.";

            statusIsError_ = true;

            return false;
        }
        catch (...)
        {
            status_ =
                "Unexpected Material Inspector failure.";

            statusIsError_ = true;

            return false;
        }
    }

    bool MaterialInspector::SaveMaterial(
    const std::size_t slotIndex,
    StaticMeshRenderer& renderer) noexcept
    {
        if (slotIndex >= slots_.size())
        {
            return false;
        }

        MaterialSlot& slot = slots_[slotIndex];

        slot.message.clear();
        slot.error = false;

        if (!SaveMaterialFile(
                slot.file,
                slot.edited,
                slot.message))
        {
            slot.error = true;
            return false;
        }

        /*
         * Файл уже сохранён, поэтому это становится
         * новым исходным состоянием даже при ошибке
         * обновления GPU-кэша.
         */
        slot.original = slot.edited;
        slot.dirty = false;

        if (!renderer.ReloadMaterials(meshAssetPath_))
        {
            slot.message =
                "Material saved, but renderer reload failed.";

            slot.error = true;
            return false;
        }

        slot.message =
            "Material saved and applied to the scene.";

        return true;
    }

    bool MaterialInspector::SelectTexture(
        const std::size_t slotIndex,
        std::optional<engine::assets::AssetPath>&
            texture) noexcept
    {
        if (slotIndex >= slots_.size())
        {
            return false;
        }

        MaterialSlot& slot =
            slots_[slotIndex];

        std::filesystem::path selectedFile;

        if (!SelectTextureFile(
                selectedFile))
        {
            return false;
        }

        try
        {
            std::filesystem::path projectTextureFile;
            std::filesystem::path gameRelativePath;

            if (IsInsideGameData(
                    selectedFile,
                    gameRoot_,
                    gameRelativePath))
            {
                projectTextureFile =
                    selectedFile;
            }
            else
            {
                if (textureDirectory_.empty())
                {
                    slot.message =
                        "Could not resolve Data/Textures directory.";

                    slot.error = true;

                    return false;
                }

                if (!CopyTextureIntoProject(
                        selectedFile,
                        textureDirectory_,
                        projectTextureFile,
                        slot.message))
                {
                    slot.error = true;

                    return false;
                }

                std::error_code error;

                gameRelativePath =
                    std::filesystem::relative(
                        projectTextureFile,
                        gameRoot_,
                        error);

                if (error ||
                    gameRelativePath.empty() ||
                    ContainsParentTraversal(
                        gameRelativePath))
                {
                    slot.message =
                        "Could not create a game-relative texture path.";

                    slot.error = true;

                    return false;
                }
            }

            engine::assets::AssetPath assetPath;

            const auto result =
                engine::assets::AssetPath::TryCreate(
                    gameRelativePath.
                        generic_u8string(),
                    assetPath);

            if (engine::assets::Failed(result))
            {
                slot.message =
                    "Invalid texture asset path: ";

                slot.message +=
                    engine::assets::ToString(result);

                slot.error = true;

                return false;
            }

            texture =
                std::move(assetPath);

            slot.dirty = true;
            slot.error = false;

            slot.message =
                "Texture selected. Press Save Material.";

            return true;
        }
        catch (const std::bad_alloc&)
        {
            slot.message =
                "Not enough memory to select texture.";

            slot.error = true;

            return false;
        }
        catch (...)
        {
            slot.message =
                "Unexpected texture selection failure.";

            slot.error = true;

            return false;
        }
    }

    void MaterialInspector::DrawTexture(
    const char* const label,
    const int controlId,
    const std::size_t slotIndex,
    std::optional<engine::assets::AssetPath>& texture,
    StaticMeshRenderer& renderer) noexcept
    {
        if (slotIndex >= slots_.size())
        {
            return;
        }

        ImGui::PushID(controlId);

        ImGui::TextUnformatted(label);

        if (texture.has_value())
        {
            ImGui::TextWrapped(
                "%s",
                texture->String().c_str());
        }
        else
        {
            ImGui::TextDisabled("<none>");
        }

        if (ImGui::Button("Select..."))
        {
            if (SelectTexture(
                    slotIndex,
                    texture))
            {
                /*
                 * Новая текстура сразу записывается
                 * в .material и появляется во Viewport.
                 */
                static_cast<void>(
                    SaveMaterial(
                        slotIndex,
                        renderer));
            }
        }

        ImGui::SameLine();

        ImGui::BeginDisabled(
            !texture.has_value());

        if (ImGui::Button("Clear"))
        {
            texture.reset();

            MaterialSlot& slot =
                slots_[slotIndex];

            slot.dirty = true;
            slot.error = false;

            /*
             * Очистка сразу записывается в .material
             * и сбрасывает GPU-текстуру.
             */
            static_cast<void>(
                SaveMaterial(
                    slotIndex,
                    renderer));
        }

        ImGui::EndDisabled();

        ImGui::PopID();
    }

    void MaterialInspector::Draw(
        const std::wstring& meshAssetPath,
        StaticMeshRenderer& renderer) noexcept
    {
        if (meshAssetPath.empty())
        {
            Reset();

            return;
        }

        if (meshAssetPath_ != meshAssetPath)
        {
            static_cast<void>(
                Reload(meshAssetPath));
        }

        ImGui::SeparatorText("Materials");

        ImGui::TextWrapped(
            "Mesh: %s",
            meshFile_.empty()
                ? "<invalid>"
                : meshFile_.
                    filename().
                    u8string().
                    c_str());

        if (ImGui::Button("Reload Materials"))
        {
            static_cast<void>(
                Reload(meshAssetPath));
        }

        if (!status_.empty())
        {
            if (statusIsError_)
            {
                ImGui::TextWrapped(
                    "Error: %s",
                    status_.c_str());
            }
            else
            {
                ImGui::TextDisabled(
                    "%s",
                    status_.c_str());
            }
        }

        for (std::size_t slotIndex = 0U;
             slotIndex < slots_.size();
             ++slotIndex)
        {
            MaterialSlot& slot =
                slots_[slotIndex];

            ImGui::PushID(
                static_cast<int>(
                    slotIndex));

            const std::string header =
                "[" +
                std::to_string(slotIndex) +
                "] " +
                slot.file.
                    filename().
                    u8string();

            const ImGuiTreeNodeFlags flags =
                slotIndex == 0U
                    ? ImGuiTreeNodeFlags_DefaultOpen
                    : ImGuiTreeNodeFlags_None;

            if (ImGui::TreeNodeEx(
                    header.c_str(),
                    flags))
            {
                if (slot.error)
                {
                    ImGui::TextWrapped(
                        "Error: %s",
                        slot.message.c_str());

                    if (ImGui::Button("Retry Load"))
                    {
                        engine::assets::MaterialAssetDesc
                            loaded;

                        std::string loadError;

                        if (LoadMaterialFile(
                                slot.file,
                                gameRoot_,
                                loaded,
                                loadError))
                        {
                            slot.original =
                                loaded;

                            slot.edited =
                                std::move(loaded);

                            slot.dirty = false;
                            slot.error = false;
                            slot.message.clear();
                        }
                        else
                        {
                            slot.message =
                                std::move(loadError);
                        }
                    }

                    ImGui::TreePop();
                    ImGui::PopID();

                    continue;
                }

                engine::assets::MaterialAssetDesc& material =
                    slot.edited;

                if (ImGui::ColorEdit3("Base Color", material.baseColorFactor.data(), ImGuiColorEditFlags_Float))
                {
                    slot.dirty = true;
                    slot.error = false;

                    if (renderer.PreviewMaterial(
                            meshAssetPath_,
                            slotIndex,
                            material))
                    {
                        slot.message =
                            "Live preview. Release to save.";
                    }
                    else
                    {
                        slot.message =
                            "Failed to update material preview.";

                        slot.error = true;
                    }
                }

                /*
                 * Вызывается сразу после ColorEdit3.
                 * Пока цвет двигается — работает PreviewMaterial().
                 * Когда редактирование закончено — сохраняем файл.
                 */
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    static_cast<void>(
                        SaveMaterial(
                            slotIndex,
                            renderer));
                }

                if (ImGui::SliderFloat("Opacity", &material.baseColorFactor[3U], 0.0F, 1.0F, "%.3f"))
                {
                    slot.dirty = true;
                    slot.error = false;

                    if (renderer.PreviewMaterial(
                            meshAssetPath_,
                            slotIndex,
                            material))
                    {
                        slot.message =
                            "Live preview. Release to save.";
                    }
                    else
                    {
                        slot.message =
                            "Failed to update opacity preview.";

                        slot.error = true;
                    }
                }

                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    static_cast<void>(
                        SaveMaterial(
                            slotIndex,
                            renderer));
                }

                static constexpr const char*
                    AlphaModes[]
                {
                    "Opaque",
                    "Mask",
                    "Blend"
                };

                int alphaMode =
                    static_cast<int>(
                        material.alphaMode);

                if (ImGui::Combo(
                        "Alpha Mode",
                        &alphaMode,
                        AlphaModes,
                        static_cast<int>(
                            std::size(
                                AlphaModes))))
                {
                    material.alphaMode =
                        static_cast<
                            engine::assets::
                                MaterialAlphaMode>(
                                    alphaMode);

                    slot.dirty = true;
                }

                if (material.alphaMode ==
                    engine::assets::
                        MaterialAlphaMode::Mask)
                {
                    if (ImGui::SliderFloat(
                            "Alpha Cutoff",
                            &material.alphaCutoff,
                            0.0F,
                            1.0F,
                            "%.3f"))
                    {
                        slot.dirty = true;
                    }
                }

                if (ImGui::Checkbox(
                        "Double Sided",
                        &material.doubleSided))
                {
                    slot.dirty = true;
                }

                if (ImGui::SliderFloat(
                        "Metallic",
                        &material.metallicFactor,
                        0.0F,
                        1.0F,
                        "%.3f"))
                {
                    slot.dirty = true;
                }

                if (ImGui::SliderFloat(
                        "Roughness",
                        &material.roughnessFactor,
                        0.0F,
                        1.0F,
                        "%.3f"))
                {
                    slot.dirty = true;
                }

                if (ImGui::DragFloat(
                        "Normal Scale",
                        &material.normalScale,
                        0.01F,
                        0.0F,
                        4.0F,
                        "%.3f"))
                {
                    slot.dirty = true;
                }

                if (ImGui::DragFloat(
                        "Specular Intensity",
                        &material.specularIntensity,
                        0.01F,
                        0.0F,
                        16.0F,
                        "%.3f"))
                {
                    slot.dirty = true;
                }

                if (ImGui::DragFloat(
                        "Specular Power",
                        &material.specularPower,
                        1.0F,
                        1.0F,
                        8192.0F,
                        "%.1f"))
                {
                    slot.dirty = true;
                }

                if (ImGui::DragFloat(
                        "Reflection",
                        &material.reflectionFactor,
                        0.01F,
                        0.0F,
                        16.0F,
                        "%.3f"))
                {
                    slot.dirty = true;
                }

                if (ImGui::ColorEdit3("Emissive Color", material.emissiveFactor.data(), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR))
                {
                    slot.dirty = true;
                    slot.error = false;

                    static_cast<void>(
                        renderer.PreviewMaterial(
                            meshAssetPath_,
                            slotIndex,
                            material));

                    slot.message =
                        "Live preview. Release to save.";
                }

                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    static_cast<void>(
                        SaveMaterial(
                            slotIndex,
                            renderer));
                }

                if (ImGui::DragFloat(
                        "Emissive Strength",
                        &material.emissiveStrength,
                        0.05F,
                        0.0F,
                        64.0F,
                        "%.3f"))
                {
                    slot.dirty = true;
                }

                ImGui::SeparatorText("Textures");

                DrawTexture("Base Color", 0, slotIndex, material.baseColorTexture, renderer);
                DrawTexture("Normal", 1, slotIndex, material.normalTexture, renderer);
                DrawTexture("Specular / Gloss", 2, slotIndex, material.specularGlossTexture, renderer);
                DrawTexture("Roughness", 3, slotIndex, material.roughnessTexture, renderer);
                DrawTexture("Emissive", 4, slotIndex, material.emissiveTexture, renderer);
                DrawTexture("Specular Power", 5, slotIndex, material.specularPowerTexture, renderer);

                ImGui::Separator();

                ImGui::BeginDisabled(
                    !slot.dirty);

                if (ImGui::Button(
                        "Save Material"))
                {
                    static_cast<void>(
                        SaveMaterial(
                            slotIndex,
                            renderer));
                }

                ImGui::EndDisabled();

                ImGui::SameLine();

                ImGui::BeginDisabled(
                    !slot.dirty);

                if (ImGui::Button("Revert"))
                {
                    slot.edited =
                        slot.original;

                    slot.dirty = false;
                    slot.error = false;

                    slot.message =
                        "Unsaved changes reverted.";
                }

                ImGui::EndDisabled();

                if (slot.dirty)
                {
                    ImGui::SameLine();
                    ImGui::TextDisabled("* modified");
                }

                if (!slot.message.empty())
                {
                    if (slot.error)
                    {
                        ImGui::TextWrapped(
                            "Error: %s",
                            slot.message.c_str());
                    }
                    else
                    {
                        ImGui::TextDisabled(
                            "%s",
                            slot.message.c_str());
                    }
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }
}