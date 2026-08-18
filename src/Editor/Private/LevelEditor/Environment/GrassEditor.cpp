#include "Editor/LevelEditor/Environment/GrassEditor.h"
#include "Editor/LevelEditor/Terrain/TerrainRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cwctype>
#include <fstream>
#include <system_error>
#include <unordered_set>

namespace lts::editor
{
    namespace
    {
        constexpr std::array<char, 8U> GrassMagic
        {
            'L', 'T', 'S', 'G', 'R', 'S', '2', '\0'
        };

        constexpr std::uint32_t GrassVersion = 1U;
        constexpr std::uint32_t MaximumInstanceCount = 2000000U;
        constexpr std::uint32_t MaximumPathLength = 4096U;

        template<typename Value>
        [[nodiscard]] bool WriteValue(
            std::ofstream& stream,
            const Value& value)
        {
            stream.write(
                reinterpret_cast<const char*>(&value),
                static_cast<std::streamsize>(sizeof(Value)));

            return static_cast<bool>(stream);
        }

        template<typename Value>
        [[nodiscard]] bool ReadValue(
            std::ifstream& stream,
            Value& value)
        {
            stream.read(
                reinterpret_cast<char*>(&value),
                static_cast<std::streamsize>(sizeof(Value)));

            return static_cast<bool>(stream);
        }

        [[nodiscard]] std::wstring Lowercase(
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

        [[nodiscard]] bool IsSrt(
            const std::filesystem::path& path)
        {
            return Lowercase(path.extension().wstring()) == L".srt";
        }

        [[nodiscard]] std::wstring BuildLogicalMeshPath(
            const std::filesystem::path& relativeSrtPath)
        {
            std::filesystem::path relativeMeshPath =
                relativeSrtPath;

            relativeMeshPath.replace_extension(L".mesh");

            return (
                std::filesystem::path(L"Data") /
                L"StaticMeshes" /
                L"SpeedTree" /
                L"Grass" /
                relativeMeshPath
            ).generic_wstring();
        }

        [[nodiscard]] bool SamePath(
            const std::filesystem::path& left,
            const std::filesystem::path& right)
        {
            return Lowercase(left.generic_wstring()) ==
                Lowercase(right.generic_wstring());
        }
    }

    GrassEditor::GrassEditor() noexcept
        : random_(0x47524153U)
    {
    }

    bool GrassEditor::RefreshAssets(
        const std::filesystem::path& workspaceRoot,
        std::string& status) noexcept
    {
        try
        {
            const std::filesystem::path grassRoot =
                workspaceRoot /
                L"bin" /
                L"Data" /
                L"SpeedTree" /
                L"Grass";

            std::error_code error;

            std::vector<GrassAssetEntry> found;

            if (!std::filesystem::is_directory(
                    grassRoot,
                    error) ||
                error)
            {
                assets_.clear();
                selectedAssetIndex_ = 0U;

                status =
                    "Create bin/Data/SpeedTree/Grass "
                    "and put .srt/.dds there.";

                return false;
            }

            std::filesystem::recursive_directory_iterator iterator(
                grassRoot,
                error);

            const std::filesystem::recursive_directory_iterator end;

            while (!error && iterator != end)
            {
                if (!iterator->is_regular_file(error) || error)
                {
                    error.clear();
                    iterator.increment(error);
                    continue;
                }

                if (IsSrt(iterator->path()))
                {
                    GrassAssetEntry entry;

                    entry.sourcePath =
                        iterator->path().lexically_normal();

                    entry.relativePath =
                        std::filesystem::relative(
                            entry.sourcePath,
                            grassRoot,
                            error);

                    if (!error)
                    {
                        entry.displayName =
                            entry.relativePath.generic_wstring();

                        entry.logicalMeshPath =
                            BuildLogicalMeshPath(
                                entry.relativePath);

                        found.push_back(std::move(entry));
                    }
                    else
                    {
                        error.clear();
                    }
                }

                iterator.increment(error);
            }

            std::sort(
                found.begin(),
                found.end(),
                [](const GrassAssetEntry& left,
                   const GrassAssetEntry& right)
                {
                    return Lowercase(left.displayName) <
                        Lowercase(right.displayName);
                });

            assets_ = std::move(found);

            if (assets_.empty())
            {
                selectedAssetIndex_ = 0U;
            }
            else
            {
                selectedAssetIndex_ = (std::min)(
                    selectedAssetIndex_,
                    assets_.size() - 1U);
            }

            status =
                "Grass assets found: " +
                std::to_string(assets_.size()) +
                ".";

            return !assets_.empty();
        }
        catch (...)
        {
            assets_.clear();
            selectedAssetIndex_ = 0U;

            status =
                "Cannot scan SpeedTree Grass directory.";

            return false;
        }
    }

    bool GrassEditor::Load(
        const std::filesystem::path& workspaceRoot,
        const std::filesystem::path& levelRoot,
        std::string& status) noexcept
    {
        Reset();

        static_cast<void>(
            RefreshAssets(workspaceRoot, status));

        const std::filesystem::path dataPath =
            levelRoot /
            L"Grass" /
            L"grass.dat";

        std::error_code error;

        if (!std::filesystem::is_regular_file(
                dataPath,
                error) ||
            error)
        {
            status =
                "Grass map is empty. "
                "Paint creates Grass/grass.dat.";

            return true;
        }

        try
        {
            std::ifstream stream(
                dataPath,
                std::ios::binary);

            std::array<char, 8U> magic{};

            std::uint32_t version = 0U;
            std::uint32_t count = 0U;

            stream.read(
                magic.data(),
                static_cast<std::streamsize>(magic.size()));

            if (!stream ||
                magic != GrassMagic ||
                !ReadValue(stream, version) ||
                version != GrassVersion ||
                !ReadValue(stream, viewDistance_) ||
                !ReadValue(stream, count) ||
                count > MaximumInstanceCount)
            {
                status =
                    "Grass/grass.dat has invalid format.";

                return false;
            }

            instances_.reserve(count);

            for (std::uint32_t index = 0U;
                 index < count;
                 ++index)
            {
                std::uint32_t pathLength = 0U;

                if (!ReadValue(stream, pathLength) ||
                    pathLength == 0U ||
                    pathLength > MaximumPathLength)
                {
                    status =
                        "Grass/grass.dat contains "
                        "an invalid asset path.";

                    Reset();
                    return false;
                }

                std::string path(pathLength, '\0');

                stream.read(
                    path.data(),
                    static_cast<std::streamsize>(
                        path.size()));

                Instance instance;

                instance.relativeSrtPath =
                    std::filesystem::u8path(path);

                instance.logicalMeshPath =
                    BuildLogicalMeshPath(
                        instance.relativeSrtPath);

                if (!stream ||
                    !ReadValue(stream, instance.x) ||
                    !ReadValue(stream, instance.y) ||
                    !ReadValue(stream, instance.z) ||
                    !ReadValue(stream, instance.yawDegrees) ||
                    !ReadValue(stream, instance.scale) ||
                    !ReadValue(stream, instance.modulation))
                {
                    status =
                        "Grass/grass.dat ended unexpectedly.";

                    Reset();
                    return false;
                }

                instances_.push_back(
                    std::move(instance));
            }

            dirty_ = false;
            renderDocumentDirty_ = true;

            status =
                "Grass loaded: " +
                std::to_string(instances_.size()) +
                " instances.";

            return true;
        }
        catch (...)
        {
            Reset();

            status =
                "Cannot load Grass/grass.dat.";

            return false;
        }
    }

    bool GrassEditor::Save(
        const std::filesystem::path& levelRoot,
        std::string& status) noexcept
    {
        if (levelRoot.empty())
        {
            status =
                "Load a level before saving Grass.";

            return false;
        }

        try
        {
            const std::filesystem::path directory =
                levelRoot /
                L"Grass";

            const std::filesystem::path dataPath =
                directory /
                L"grass.dat";

            const std::filesystem::path temporaryPath =
                directory /
                L"grass.dat.tmp";

            std::error_code error;

            std::filesystem::create_directories(
                directory,
                error);

            if (error)
            {
                status =
                    "Cannot create level Grass directory.";

                return false;
            }

            std::ofstream stream(
                temporaryPath,
                std::ios::binary |
                std::ios::trunc);

            const std::uint32_t count =
                static_cast<std::uint32_t>(
                    instances_.size());

            stream.write(
                GrassMagic.data(),
                static_cast<std::streamsize>(
                    GrassMagic.size()));

            if (!stream ||
                !WriteValue(stream, GrassVersion) ||
                !WriteValue(stream, viewDistance_) ||
                !WriteValue(stream, count))
            {
                status =
                    "Cannot write Grass/grass.dat header.";

                return false;
            }

            for (const Instance& instance : instances_)
            {
                const std::string path =
                    instance.relativeSrtPath
                        .generic_u8string();

                const std::uint32_t pathLength =
                    static_cast<std::uint32_t>(
                        path.size());

                if (path.empty() ||
                    path.size() > MaximumPathLength ||
                    !WriteValue(stream, pathLength))
                {
                    status =
                        "Cannot write Grass asset path.";

                    return false;
                }

                stream.write(
                    path.data(),
                    static_cast<std::streamsize>(
                        path.size()));

                if (!stream ||
                    !WriteValue(stream, instance.x) ||
                    !WriteValue(stream, instance.y) ||
                    !WriteValue(stream, instance.z) ||
                    !WriteValue(stream, instance.yawDegrees) ||
                    !WriteValue(stream, instance.scale) ||
                    !WriteValue(stream, instance.modulation))
                {
                    status =
                        "Cannot write Grass instance.";

                    return false;
                }
            }

            stream.close();

            if (!stream)
            {
                status =
                    "Cannot finish Grass/grass.dat.";

                return false;
            }

            std::filesystem::remove(
                dataPath,
                error);

            error.clear();

            std::filesystem::rename(
                temporaryPath,
                dataPath,
                error);

            if (error)
            {
                status =
                    "Cannot replace Grass/grass.dat.";

                return false;
            }

            dirty_ = false;

            status =
                "Grass saved: " +
                std::to_string(instances_.size()) +
                " instances.";

            return true;
        }
        catch (...)
        {
            status =
                "Cannot save Grass/grass.dat.";

            return false;
        }
    }

    void GrassEditor::Reset() noexcept
    {
        instances_.clear();
        renderDocument_.Clear();

        strokeActive_ = false;
        strokeChanged_ = false;

        hasLastStamp_ = false;
        hasLastCamera_ = false;

        dirty_ = false;
        renderDocumentDirty_ = true;
    }

    void GrassEditor::BeginStroke() noexcept
    {
        strokeActive_ = true;
        strokeChanged_ = false;
        hasLastStamp_ = false;
    }

    bool GrassEditor::EnsureSelectedAssetCooked(
    const std::filesystem::path&,
    std::string& status) noexcept
    {
        if (assets_.empty() ||
            selectedAssetIndex_ >= assets_.size())
        {
            status = "Select a Grass .srt asset.";
            return false;
        }

        const GrassAssetEntry& asset =
            assets_[selectedAssetIndex_];

        std::error_code error;

        if (!IsSrt(asset.sourcePath) ||
            !std::filesystem::is_regular_file(
                asset.sourcePath,
                error) ||
            error)
        {
            status =
                "Grass .srt file does not exist: " +
                asset.sourcePath.generic_u8string();

            return false;
        }

        status =
            "Native SpeedTree asset selected: " +
            asset.relativePath.generic_u8string();

        return true;
    }

    bool GrassEditor::Stamp(
        const std::filesystem::path& workspaceRoot,
        const TerrainRenderer& terrainRenderer,
        const SceneDocument& levelDocument,
        const float worldX,
        const float worldZ,
        const float radius,
        const bool erase,
        std::string& status) noexcept
    {
        if (!strokeActive_ ||
            radius <= 0.0F ||
            assets_.empty() ||
            selectedAssetIndex_ >= assets_.size())
        {
            return false;
        }

        const float minimumStampDistance =
            (std::max)(1.0F, radius * 0.18F);

        if (hasLastStamp_)
        {
            const float dx =
                worldX - lastStampX_;

            const float dz =
                worldZ - lastStampZ_;

            if (dx * dx + dz * dz <
                minimumStampDistance *
                minimumStampDistance)
            {
                return false;
            }
        }

        lastStampX_ = worldX;
        lastStampZ_ = worldZ;
        hasLastStamp_ = true;

        const GrassAssetEntry& asset =
            assets_[selectedAssetIndex_];

        bool changed = false;

        if (erase)
        {
            const float radiusSquared =
                radius * radius;

            const std::size_t oldSize =
                instances_.size();

            instances_.erase(
                std::remove_if(
                    instances_.begin(),
                    instances_.end(),
                    [&](const Instance& instance)
                    {
                        const float dx =
                            instance.x - worldX;

                        const float dz =
                            instance.z - worldZ;

                        return
                            SamePath(
                                instance.relativeSrtPath,
                                asset.relativePath) &&
                            dx * dx + dz * dz <=
                                radiusSquared;
                    }),
                instances_.end());

            changed =
                instances_.size() != oldSize;
        }
        else
        {
            if (!EnsureSelectedAssetCooked(
                    workspaceRoot,
                    status))
            {
                return false;
            }

            std::uniform_real_distribution<float>
                unit(0.0F, 1.0F);

            std::uniform_real_distribution<float>
                angle(0.0F, 6.283185307F);

            std::uniform_real_distribution<float>
                yaw(0.0F, 360.0F);

            std::uniform_real_distribution<float>
                scale(0.85F, 1.15F);

            const int count =
                std::clamp(
                    static_cast<int>(
                        radius * radius * 0.02F),
                    1,
                    128);

            for (int index = 0;
                 index < count;
                 ++index)
            {
                const float distance =
                    std::sqrt(unit(random_)) *
                    radius;

                const float direction =
                    angle(random_);

                const float x =
                    worldX +
                    std::cos(direction) *
                    distance;

                const float z =
                    worldZ +
                    std::sin(direction) *
                    distance;

                float y = 0.0F;

                if (!terrainRenderer.TryGetSurfaceHeight(
                        levelDocument,
                        x,
                        z,
                        y))
                {
                    continue;
                }

                const bool occupied =
                    std::any_of(
                        instances_.begin(),
                        instances_.end(),
                        [&](const Instance& instance)
                        {
                            const float dx =
                                instance.x - x;

                            const float dz =
                                instance.z - z;

                            return
                                SamePath(
                                    instance.relativeSrtPath,
                                    asset.relativePath) &&
                                dx * dx + dz * dz <
                                    0.5625F;
                        });

                if (occupied)
                {
                    continue;
                }

                Instance instance;

                instance.relativeSrtPath =
                    asset.relativePath;

                instance.x = x;
                instance.y = y + 0.02F;
                instance.z = z;

                instance.yawDegrees =
                    yaw(random_);

                instance.scale =
                    scale(random_);

                instance.modulation =
                    scale(random_);

                instances_.push_back(
                    std::move(instance));

                changed = true;
            }
        }

        if (changed)
        {
            strokeChanged_ = true;
            MarkChanged();
        }

        return changed;
    }

    bool GrassEditor::EndStroke() noexcept
    {
        const bool changed =
            strokeActive_ &&
            strokeChanged_;

        strokeActive_ = false;
        strokeChanged_ = false;
        hasLastStamp_ = false;

        return changed;
    }

    bool GrassEditor::RecalculateHeights(
        const TerrainRenderer& terrainRenderer,
        const SceneDocument& levelDocument,
        std::string& status) noexcept
    {
        std::size_t updated = 0U;

        for (Instance& instance : instances_)
        {
            float height = 0.0F;

            if (terrainRenderer.TryGetSurfaceHeight(
                    levelDocument,
                    instance.x,
                    instance.z,
                    height))
            {
                instance.y =
                    height + 0.02F;

                ++updated;
            }
        }

        if (updated > 0U)
        {
            MarkChanged();
        }

        status =
            "Grass heights recalculated: " +
            std::to_string(updated) +
            ".";

        return updated > 0U;
    }

    bool GrassEditor::UpdateModulation(
        std::string& status) noexcept
    {
        std::uniform_real_distribution<float>
            modulation(0.85F, 1.15F);

        std::uniform_real_distribution<float>
            yaw(0.0F, 360.0F);

        for (Instance& instance : instances_)
        {
            instance.modulation =
                modulation(random_);

            instance.yawDegrees =
                yaw(random_);
        }

        if (!instances_.empty())
        {
            MarkChanged();
        }

        status =
            "Grass modulation updated.";

        return !instances_.empty();
    }

    bool GrassEditor::ClearAll(
        std::string& status) noexcept
    {
        if (instances_.empty())
        {
            status =
                "Grass is already empty.";

            return false;
        }

        instances_.clear();
        MarkChanged();

        status =
            "All Grass cleared.";

        return true;
    }

    bool GrassEditor::Optimize(
        std::string& status) noexcept
    {
        std::unordered_set<std::wstring> occupied;

        std::vector<Instance> optimized;
        optimized.reserve(instances_.size());

        for (Instance& instance : instances_)
        {
            if (!std::isfinite(instance.x) ||
                !std::isfinite(instance.y) ||
                !std::isfinite(instance.z) ||
                instance.relativeSrtPath.empty())
            {
                continue;
            }

            const long long cellX =
                static_cast<long long>(
                    std::floor(instance.x * 2.0F));

            const long long cellZ =
                static_cast<long long>(
                    std::floor(instance.z * 2.0F));

            const std::wstring key =
                Lowercase(
                    instance.relativeSrtPath
                        .generic_wstring()) +
                L"#" +
                std::to_wstring(cellX) +
                L"#" +
                std::to_wstring(cellZ);

            if (occupied.insert(key).second)
            {
                optimized.push_back(
                    std::move(instance));
            }
        }

        const std::size_t removed =
            instances_.size() -
            optimized.size();

        instances_ =
            std::move(optimized);

        if (removed > 0U)
        {
            MarkChanged();
        }

        status =
            "Grass optimized. Removed: " +
            std::to_string(removed) +
            ".";

        return removed > 0U;
    }

    void GrassEditor::PrepareRenderDocument(
        const SceneDocument& levelDocument,
        const DirectX::XMFLOAT3& cameraPosition) noexcept
    {
        const std::uint64_t revision =
            levelDocument
                .GetSceneWorld()
                .GetRevision();

        const float cameraDx =
            cameraPosition.x -
            lastCameraX_;

        const float cameraDz =
            cameraPosition.z -
            lastCameraZ_;

        if (!renderDocumentDirty_ &&
            hasLastCamera_ &&
            revision == lastLevelRevision_ &&
            cameraDx * cameraDx +
                cameraDz * cameraDz <
                64.0F)
        {
            return;
        }

        try
        {
            renderDocument_.Clear();

            for (const EditorSceneEntity& source :
                 levelDocument.GetEntities())
            {
                if (!source.environment.has_value() &&
                    !source.directionalLight.has_value())
                {
                    continue;
                }

                const EditorEntityId id =
                    renderDocument_.CreateEntity(
                        source.name,
                        source.kind,
                        source.transform);

                EditorSceneEntity* const target =
                    renderDocument_
                        .FindEntityMutable(id);

                if (target != nullptr)
                {
                    target->environment =
                        source.environment;

                    target->directionalLight =
                        source.directionalLight;
                }
            }

            const float viewDistanceSquared =
                viewDistance_ *
                viewDistance_;

            std::size_t visibleIndex = 0U;

            for (const Instance& instance :
                 instances_)
            {
                const float dx =
                    instance.x -
                    cameraPosition.x;

                const float dz =
                    instance.z -
                    cameraPosition.z;

                if (dx * dx + dz * dz >
                    viewDistanceSquared)
                {
                    continue;
                }

                EditorTransform transform;

                transform.position =
                {
                    instance.x,
                    instance.y,
                    instance.z
                };

                transform.rotationDegrees =
                {
                    0.0F,
                    instance.yawDegrees,
                    0.0F
                };

                const float finalScale =
                    instance.scale *
                    instance.modulation;

                transform.scale =
                {
                    finalScale,
                    finalScale,
                    finalScale
                };

                if (renderDocument_.CreateStaticMeshEntity(
                        L"Grass " +
                            std::to_wstring(
                                ++visibleIndex),
                        instance.logicalMeshPath,
                        transform))
                {
                    EditorSceneEntity* const entity =
                        renderDocument_
                            .GetSelectedEntityMutable();

                    if (entity != nullptr &&
                        entity->staticMesh.has_value())
                    {
                        entity->staticMesh->castShadows =
                            false;

                        entity->staticMesh->renderOrder =
                            2;
                    }
                }
            }

            renderDocument_.ClearSelection();
            renderDocument_.MarkSaved();

            lastCameraX_ =
                cameraPosition.x;

            lastCameraZ_ =
                cameraPosition.z;

            lastLevelRevision_ =
                revision;

            hasLastCamera_ = true;
            renderDocumentDirty_ = false;
        }
        catch (...)
        {
            renderDocument_.Clear();
            renderDocumentDirty_ = true;
        }
    }

    const SceneDocument&
        GrassEditor::GetRenderDocument() const noexcept
    {
        return renderDocument_;
    }

    const std::vector<GrassAssetEntry>&
        GrassEditor::GetAssets() const noexcept
    {
        return assets_;
    }

    std::size_t
        GrassEditor::GetSelectedAssetIndex() const noexcept
    {
        return selectedAssetIndex_;
    }

    void GrassEditor::SetSelectedAssetIndex(
        const std::size_t index) noexcept
    {
        if (index < assets_.size())
        {
            selectedAssetIndex_ = index;
        }
    }

    std::size_t
        GrassEditor::GetInstanceCount() const noexcept
    {
        return instances_.size();
    }

    const std::vector<GrassRenderInstance>&
        GrassEditor::GetRenderInstances() const noexcept
    {
        return instances_;
    }

    bool GrassEditor::IsDirty() const noexcept
    {
        return dirty_;
    }

    float GrassEditor::GetViewDistance() const noexcept
    {
        return viewDistance_;
    }

    void GrassEditor::SetViewDistance(
        const float value) noexcept
    {
        const float clamped =
            std::clamp(
                value,
                25.0F,
                1024.0F);

        if (std::abs(
                clamped -
                viewDistance_) <
            0.001F)
        {
            return;
        }

        viewDistance_ = clamped;
        dirty_ = true;
        renderDocumentDirty_ = true;
    }

    void GrassEditor::MarkChanged() noexcept
    {
        dirty_ = true;
        renderDocumentDirty_ = true;
    }
}