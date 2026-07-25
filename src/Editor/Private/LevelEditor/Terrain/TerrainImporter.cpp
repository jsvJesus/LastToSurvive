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
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwctype>
#include <fstream>
#include <limits>
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
            const float tileSize,
            const float heightRange,
            const std::vector<std::int16_t>& heights)
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

            const std::uint32_t splatWidth = (std::min)(width, 2048U);
            const std::uint32_t splatHeight = (std::min)(height, 2048U);
            const float heightOffset = -heightRange * 0.5F;
            const float heightScale = heightRange;
            const std::uint32_t layerCount = 1U;
            const std::uint32_t maskCount = 0U;
            const std::uint64_t heightBytes = sampleCount * sizeof(std::int16_t);

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

            const std::string layerName = "Base";
            const std::string emptyPath;
            const std::string materialType = "Default";
            const float textureScale = 16.0F;
            const float specular = 0.0F;

            if (!WriteString(stream, layerName) ||
                !WriteString(stream, emptyPath) ||
                !WriteString(stream, emptyPath) ||
                !WriteString(stream, materialType) ||
                !WriteValue(stream, textureScale) ||
                !WriteValue(stream, textureScale) ||
                !WriteValue(stream, specular))
            {
                return false;
            }

            const std::vector<std::byte> colorMap = CreateSolidDds(96U, 96U, 96U, 255U);
            const std::vector<std::byte> normalMap = CreateSolidDds(128U, 128U, 255U, 255U);

            if (!WriteBlob(stream, colorMap) || !WriteBlob(stream, normalMap))
            {
                return false;
            }

            stream.flush();
            return static_cast<bool>(stream);
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

        SetTextBuffer(outputName_, ToUtf8(sourcePath_.stem().wstring()));
        DetectResolution();

        return true;
    }

    void TerrainImporter::DetectResolution() noexcept
    {
        width_ = 0;
        height_ = 0;
        importSucceeded_ = false;
        statusIsError_ = false;
        status_.clear();

        std::error_code error;
        const std::uintmax_t fileSize = std::filesystem::file_size(sourcePath_, error);

        if (error || fileSize == 0U || fileSize % sizeof(std::uint16_t) != 0U)
        {
            status_ = "The selected file is not a valid 16-bit RAW heightmap.";
            statusIsError_ = true;
            return;
        }

        const std::uint64_t sampleCount =
            static_cast<std::uint64_t>(fileSize / sizeof(std::uint16_t));

        std::uint64_t side = static_cast<std::uint64_t>(
            std::sqrt(static_cast<long double>(sampleCount)));

        while (side * side < sampleCount)
        {
            ++side;
        }

        while (side > 0U && side * side > sampleCount)
        {
            --side;
        }

        if (side * side == sampleCount &&
            side <= static_cast<std::uint64_t>((std::numeric_limits<int>::max)()))
        {
            width_ = static_cast<int>(side);
            height_ = static_cast<int>(side);
            return;
        }

        status_ = "The heightmap is not square. Enter Width and Height manually.";
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

        if (!std::isfinite(tileSize_) || tileSize_ <= 0.0F)
        {
            status_ = "Tile Size must be greater than zero.";
            return false;
        }

        if (!std::isfinite(heightRange_) || heightRange_ <= 0.0F)
        {
            status_ = "Height Range must be greater than zero.";
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
            status_ = "Enter a valid output name.";
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

        std::ifstream source(sourcePath_, std::ios::binary);

        if (!source)
        {
            status_ = "Could not open the source heightmap.";
            return false;
        }

        std::vector<std::uint16_t> sourceHeights(static_cast<std::size_t>(sampleCount));

        source.read(
            reinterpret_cast<char*>(sourceHeights.data()),
            static_cast<std::streamsize>(expectedFileSize));

        if (!source)
        {
            status_ = "Could not read the complete source heightmap.";
            return false;
        }

        std::vector<std::int16_t> terrainHeights(static_cast<std::size_t>(sampleCount));

        const std::uint32_t width = static_cast<std::uint32_t>(width_);
        const std::uint32_t height = static_cast<std::uint32_t>(height_);

        for (std::uint32_t z = 0U; z < height; ++z)
        {
            const std::uint32_t sourceZ = flipY_ ? height - 1U - z : z;

            for (std::uint32_t x = 0U; x < width; ++x)
            {
                const std::uint32_t sourceX = flipX_ ? width - 1U - x : x;

                const std::size_t sourceIndex =
                    static_cast<std::size_t>(sourceZ) * width + sourceX;

                const std::size_t targetIndex =
                    static_cast<std::size_t>(z) * width + x;

                const double normalized =
                    static_cast<double>(sourceHeights[sourceIndex]) / 65535.0;

                const double signedValue = (normalized * 2.0 - 1.0) * 32767.0;

                terrainHeights[targetIndex] =
                    static_cast<std::int16_t>(std::lround(signedValue));
            }
        }

        const std::filesystem::path gameRoot = FindGameRoot();
        const std::filesystem::path outputDirectory = gameRoot / L"Data" / L"Terrains";
        const std::filesystem::path outputPath =
            (outputDirectory / outputName).replace_extension(L".ltsterrain");

        std::filesystem::create_directories(outputDirectory, error);

        if (error)
        {
            status_ = "Could not create Data/Terrains.";
            return false;
        }

        if (!overwriteExisting_ && std::filesystem::exists(outputPath, error) && !error)
        {
            status_ = "The output terrain already exists. Enable Overwrite Existing.";
            return false;
        }

        if (!WriteTerrainFile(outputPath, width, height, tileSize_, heightRange_, terrainHeights))
        {
            status_ = "Could not write the .ltsterrain file.";
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

        error.clear();
        const std::filesystem::path relativePath =
            std::filesystem::relative(outputPath, gameRoot, error);

        if (error)
        {
            status_ = "Could not create a game-relative terrain path.";
            return false;
        }

        const EditorSceneSnapshot before = context.sceneDocument.CreateSnapshot();

        EditorTransform transform{};
        transform.position[1] = baseHeight_;

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

        status_ = "Terrain imported successfully: " + ToUtf8(outputPath.wstring());
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

        ImGui::SetNextItemWidth(180.0F);
        ImGui::DragFloat("Tile Size", &tileSize_, 0.1F, 0.01F, 10000.0F, "%.2f");

        ImGui::SetNextItemWidth(180.0F);
        ImGui::DragFloat("Height Range", &heightRange_, 1.0F, 1.0F, 100000.0F, "%.1f");

        ImGui::SetNextItemWidth(180.0F);
        ImGui::DragFloat("Base Height", &baseHeight_, 1.0F, -100000.0F, 100000.0F, "%.1f");

        if (width_ > 1 && height_ > 1 && tileSize_ > 0.0F)
        {
            const float worldWidth = static_cast<float>(width_ - 1) * tileSize_;
            const float worldDepth = static_cast<float>(height_ - 1) * tileSize_;

            ImGui::TextDisabled(
                "World size: %.1f x %.1f",
                worldWidth,
                worldDepth);
        }

        ImGui::Checkbox("Flip X", &flipX_);
        ImGui::SameLine();
        ImGui::Checkbox("Flip Y", &flipY_);

        ImGui::Separator();

        ImGui::InputText("Output Name", outputName_.data(), outputName_.size());
        ImGui::Checkbox("Overwrite Existing", &overwriteExisting_);

        const std::wstring cleanOutputName = SanitizeOutputName(outputName_.data());

        if (!cleanOutputName.empty())
        {
            const std::filesystem::path preview =
                FindGameRoot() / L"Data" / L"Terrains" /
                (cleanOutputName + L".ltsterrain");

            const std::string previewText = ToUtf8(preview.wstring());
            ImGui::TextWrapped("Output: %s", previewText.c_str());
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