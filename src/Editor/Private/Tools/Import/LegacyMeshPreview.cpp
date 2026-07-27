#include "Editor/Tools/Import/LegacyMeshPreview.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

namespace lts::editor
{
    namespace
    {
        constexpr std::size_t MaximumPreviewTriangles =
            200000U;

        constexpr float PreviewFieldOfView =
            45.0F * 3.14159265358979323846F / 180.0F;

        struct Vector3 final
        {
            float x = 0.0F;
            float y = 0.0F;
            float z = 0.0F;
        };

        struct PreviewView final
        {
            Vector3 camera;
            Vector3 forward;
            Vector3 right;
            Vector3 up;

            float focalLength = 1.0F;
            float nearPlane = 0.001F;
        };

        struct ProjectedTriangle final
        {
            ImVec2 points[3];

            float depth = 0.0F;

            ImU32 color = IM_COL32(
                170,
                180,
                190,
                255);
        };

        [[nodiscard]]
        Vector3 ToVector3(
            const std::array<float, 3U>& value) noexcept
        {
            return
            {
                value[0],
                value[1],
                value[2]
            };
        }

        [[nodiscard]]
        Vector3 Add(
            const Vector3& left,
            const Vector3& right) noexcept
        {
            return
            {
                left.x + right.x,
                left.y + right.y,
                left.z + right.z
            };
        }

        [[nodiscard]]
        Vector3 Subtract(
            const Vector3& left,
            const Vector3& right) noexcept
        {
            return
            {
                left.x - right.x,
                left.y - right.y,
                left.z - right.z
            };
        }

        [[nodiscard]]
        Vector3 Multiply(
            const Vector3& value,
            const float scale) noexcept
        {
            return
            {
                value.x * scale,
                value.y * scale,
                value.z * scale
            };
        }

        [[nodiscard]]
        float Dot(
            const Vector3& left,
            const Vector3& right) noexcept
        {
            return
                left.x * right.x +
                left.y * right.y +
                left.z * right.z;
        }

        [[nodiscard]]
        Vector3 Cross(
            const Vector3& left,
            const Vector3& right) noexcept
        {
            return
            {
                left.y * right.z -
                    left.z * right.y,

                left.z * right.x -
                    left.x * right.z,

                left.x * right.y -
                    left.y * right.x
            };
        }

        [[nodiscard]]
        float LengthSquared(
            const Vector3& value) noexcept
        {
            return Dot(value, value);
        }

        [[nodiscard]]
        float Length(
            const Vector3& value) noexcept
        {
            return std::sqrt(
                LengthSquared(value));
        }

        [[nodiscard]]
        Vector3 Normalize(
            const Vector3& value,
            const Vector3& fallback =
                Vector3{0.0F, 1.0F, 0.0F}) noexcept
        {
            const float length = Length(value);

            if (!std::isfinite(length) ||
                length <= 0.000001F)
            {
                return fallback;
            }

            return Multiply(
                value,
                1.0F / length);
        }

        [[nodiscard]]
        bool IsFinite(
            const Vector3& value) noexcept
        {
            return
                std::isfinite(value.x) &&
                std::isfinite(value.y) &&
                std::isfinite(value.z);
        }

        [[nodiscard]]
        PreviewView BuildView(
            const std::array<float, 3U>& targetArray,
            const float yaw,
            const float pitch,
            const float distance,
            const float radius,
            const float viewportHeight) noexcept
        {
            const Vector3 target =
                ToVector3(targetArray);

            const float cosinePitch =
                std::cos(pitch);

            PreviewView view;

            /*
             * Направление от camera к target.
             * Положительный pitch поднимает camera над объектом.
             */
            view.forward = Normalize(
            {
                cosinePitch * std::sin(yaw),
                -std::sin(pitch),
                cosinePitch * std::cos(yaw)
            });

            view.camera = Subtract(
                target,
                Multiply(
                    view.forward,
                    distance));

            const Vector3 worldUp
            {
                0.0F,
                1.0F,
                0.0F
            };

            view.right = Normalize(
                Cross(
                    worldUp,
                    view.forward),
                Vector3{1.0F, 0.0F, 0.0F});

            view.up = Normalize(
                Cross(
                    view.forward,
                    view.right),
                worldUp);

            view.focalLength =
                (std::max)(
                    viewportHeight,
                    1.0F) *
                0.5F /
                std::tan(
                    PreviewFieldOfView *
                    0.5F);

            view.nearPlane =
                (std::max)(
                    radius * 0.0025F,
                    0.001F);

            return view;
        }

        [[nodiscard]]
        bool ProjectPoint(
            const Vector3& point,
            const PreviewView& view,
            const ImVec2 canvasMinimum,
            const ImVec2 canvasSize,
            ImVec2& screenPosition,
            float& depth) noexcept
        {
            const Vector3 relative =
                Subtract(
                    point,
                    view.camera);

            const float viewX =
                Dot(relative, view.right);

            const float viewY =
                Dot(relative, view.up);

            const float viewZ =
                Dot(relative, view.forward);

            if (!std::isfinite(viewZ) ||
                viewZ <= view.nearPlane)
            {
                return false;
            }

            const float inverseDepth =
                1.0F / viewZ;

            screenPosition =
            {
                canvasMinimum.x +
                    canvasSize.x * 0.5F +
                    viewX *
                    view.focalLength *
                    inverseDepth,

                canvasMinimum.y +
                    canvasSize.y * 0.5F -
                    viewY *
                    view.focalLength *
                    inverseDepth
            };

            depth = viewZ;

            return
                std::isfinite(screenPosition.x) &&
                std::isfinite(screenPosition.y);
        }

        [[nodiscard]]
        std::uint32_t HashMaterialName(
            const std::string& materialName,
            const std::size_t materialIndex) noexcept
        {
            std::uint32_t hash = 2166136261U;

            for (const unsigned char character :
                 materialName)
            {
                hash ^= character;
                hash *= 16777619U;
            }

            hash ^=
                static_cast<std::uint32_t>(
                    materialIndex);

            hash *= 16777619U;

            return hash;
        }

        [[nodiscard]]
        ImU32 BuildMaterialColor(
            const LegacyMaterialChunk* materialChunk,
            const LegacyMaterialData* materialData,
            const std::size_t materialIndex,
            const float lighting) noexcept
        {
            float red = 0.72F;
            float green = 0.75F;
            float blue = 0.78F;

            if (materialData != nullptr)
            {
                red = materialData->diffuseColor[0];
                green = materialData->diffuseColor[1];
                blue = materialData->diffuseColor[2];
            }
            else
            {
                const std::string materialName =
                    materialChunk != nullptr
                        ? materialChunk->materialName
                        : std::string{};

                const std::uint32_t hash =
                    HashMaterialName(
                        materialName,
                        materialIndex);

                const float hue =
                    static_cast<float>(
                        hash % 1000U) /
                    1000.0F;

                ImGui::ColorConvertHSVtoRGB(
                    hue,
                    0.32F,
                    0.78F,
                    red,
                    green,
                    blue);
            }

            const float safeLighting =
                std::clamp(
                    lighting,
                    0.18F,
                    1.15F);

            red = std::clamp(
                red * safeLighting,
                0.0F,
                1.0F);

            green = std::clamp(
                green * safeLighting,
                0.0F,
                1.0F);

            blue = std::clamp(
                blue * safeLighting,
                0.0F,
                1.0F);

            return ImGui::ColorConvertFloat4ToU32(
                ImVec4(
                    red,
                    green,
                    blue,
                    1.0F));
        }

        [[nodiscard]]
        const LegacyMaterialChunk* FindMaterialChunk(
            const LegacyMeshData& mesh,
            const std::size_t indexOffset,
            std::size_t& chunkCursor) noexcept
        {
            while (chunkCursor <
                       mesh.materialChunks.size())
            {
                const LegacyMaterialChunk& chunk =
                    mesh.materialChunks[
                        chunkCursor];

                const std::size_t first =
                    chunk.firstIndex;

                const std::size_t end =
                    first +
                    chunk.indexCount;

                if (indexOffset < first)
                {
                    return nullptr;
                }

                if (indexOffset < end)
                {
                    return &chunk;
                }

                ++chunkCursor;
            }

            return nullptr;
        }

        [[nodiscard]]
        Vector3 GetBonePosition(
            const LegacyBone& bone) noexcept
        {
            return
            {
                bone.absoluteBindMatrix[12U],
                bone.absoluteBindMatrix[13U],
                bone.absoluteBindMatrix[14U]
            };
        }

        void DrawAxis(
            ImDrawList& drawList,
            const PreviewView& view,
            const ImVec2 canvasMinimum,
            const ImVec2 canvasSize,
            const Vector3& start,
            const Vector3& end,
            const ImU32 color) noexcept
        {
            ImVec2 startScreen;
            ImVec2 endScreen;

            float startDepth = 0.0F;
            float endDepth = 0.0F;

            if (!ProjectPoint(
                    start,
                    view,
                    canvasMinimum,
                    canvasSize,
                    startScreen,
                    startDepth) ||
                !ProjectPoint(
                    end,
                    view,
                    canvasMinimum,
                    canvasSize,
                    endScreen,
                    endDepth))
            {
                return;
            }

            drawList.AddLine(
                startScreen,
                endScreen,
                color,
                2.0F);
        }
    }

    void LegacyMeshPreview::Reset() noexcept
    {
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
            Reset();
            return;
        }

        Vector3 minimum
        {
            (std::numeric_limits<float>::max)(),
            (std::numeric_limits<float>::max)(),
            (std::numeric_limits<float>::max)()
        };

        Vector3 maximum
        {
            (std::numeric_limits<float>::lowest)(),
            (std::numeric_limits<float>::lowest)(),
            (std::numeric_limits<float>::lowest)()
        };

        bool hasFiniteVertex = false;

        for (const LegacyMeshVertex& vertex :
             mesh.vertices)
        {
            const Vector3 position =
                ToVector3(vertex.position);

            if (!IsFinite(position))
            {
                continue;
            }

            hasFiniteVertex = true;

            minimum.x =
                (std::min)(
                    minimum.x,
                    position.x);

            minimum.y =
                (std::min)(
                    minimum.y,
                    position.y);

            minimum.z =
                (std::min)(
                    minimum.z,
                    position.z);

            maximum.x =
                (std::max)(
                    maximum.x,
                    position.x);

            maximum.y =
                (std::max)(
                    maximum.y,
                    position.y);

            maximum.z =
                (std::max)(
                    maximum.z,
                    position.z);
        }

        if (!hasFiniteVertex)
        {
            Reset();
            return;
        }

        const Vector3 center =
            Multiply(
                Add(
                    minimum,
                    maximum),
                0.5F);

        float radius = 0.0F;

        for (const LegacyMeshVertex& vertex :
             mesh.vertices)
        {
            const Vector3 position =
                ToVector3(vertex.position);

            if (!IsFinite(position))
            {
                continue;
            }

            radius =
                (std::max)(
                    radius,
                    Length(
                        Subtract(
                            position,
                            center)));
        }

        radius_ =
            (std::max)(
                radius,
                0.05F);

        target_ =
        {
            center.x,
            center.y,
            center.z
        };

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
        const LegacySkeletonData* skeleton,
        const LegacyMaterialSet* materials,
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
            "##WarZLegacyMeshPreviewCanvas",
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

            const PreviewView panView =
                BuildView(
                    target_,
                    yaw_,
                    pitch_,
                    distance_,
                    radius_,
                    height);

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

                const Vector3 target =
                    ToVector3(target_);

                const Vector3 horizontal =
                    Multiply(
                        panView.right,
                        -input.MouseDelta.x *
                        panScale);

                const Vector3 vertical =
                    Multiply(
                        panView.up,
                        input.MouseDelta.y *
                        panScale);

                const Vector3 movedTarget =
                    Add(
                        target,
                        Add(
                            horizontal,
                            vertical));

                target_ =
                {
                    movedTarget.x,
                    movedTarget.y,
                    movedTarget.z
                };
            }

            if (input.MouseWheel != 0.0F)
            {
                distance_ *= std::pow(
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

        const PreviewView view =
            BuildView(
                target_,
                yaw_,
                pitch_,
                distance_,
                radius_,
                height);

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

        const std::size_t triangleCount =
            mesh.indices.size() / 3U;

        const std::size_t triangleStep =
            triangleCount >
                MaximumPreviewTriangles
                    ? (
                        triangleCount +
                        MaximumPreviewTriangles -
                        1U
                      ) /
                      MaximumPreviewTriangles
                    : 1U;

        std::vector<ProjectedTriangle>
            projectedTriangles;

        projectedTriangles.reserve(
            (std::min)(
                triangleCount,
                MaximumPreviewTriangles));

        const Vector3 previewLight =
            Normalize(
            {
                -0.45F,
                0.80F,
                -0.30F
            });

        std::size_t chunkCursor = 0U;

        for (std::size_t triangleIndex = 0U;
             triangleIndex < triangleCount;
             triangleIndex += triangleStep)
        {
            const std::size_t indexOffset =
                triangleIndex * 3U;

            const std::uint32_t firstIndex =
                mesh.indices[
                    indexOffset + 0U];

            const std::uint32_t secondIndex =
                mesh.indices[
                    indexOffset + 1U];

            const std::uint32_t thirdIndex =
                mesh.indices[
                    indexOffset + 2U];

            if (firstIndex >=
                    mesh.vertices.size() ||
                secondIndex >=
                    mesh.vertices.size() ||
                thirdIndex >=
                    mesh.vertices.size())
            {
                continue;
            }

            const Vector3 firstPosition =
                ToVector3(
                    mesh.vertices[
                        firstIndex].position);

            const Vector3 secondPosition =
                ToVector3(
                    mesh.vertices[
                        secondIndex].position);

            const Vector3 thirdPosition =
                ToVector3(
                    mesh.vertices[
                        thirdIndex].position);

            if (!IsFinite(firstPosition) ||
                !IsFinite(secondPosition) ||
                !IsFinite(thirdPosition))
            {
                continue;
            }

            ProjectedTriangle triangle;

            float firstDepth = 0.0F;
            float secondDepth = 0.0F;
            float thirdDepth = 0.0F;

            if (!ProjectPoint(
                    firstPosition,
                    view,
                    canvasMinimum,
                    canvasSize,
                    triangle.points[0],
                    firstDepth) ||
                !ProjectPoint(
                    secondPosition,
                    view,
                    canvasMinimum,
                    canvasSize,
                    triangle.points[1],
                    secondDepth) ||
                !ProjectPoint(
                    thirdPosition,
                    view,
                    canvasMinimum,
                    canvasSize,
                    triangle.points[2],
                    thirdDepth))
            {
                continue;
            }

            triangle.depth =
                (
                    firstDepth +
                    secondDepth +
                    thirdDepth
                ) /
                3.0F;

            const Vector3 edgeA =
                Subtract(
                    secondPosition,
                    firstPosition);

            const Vector3 edgeB =
                Subtract(
                    thirdPosition,
                    firstPosition);

            const Vector3 faceNormal =
                Normalize(
                    Cross(
                        edgeA,
                        edgeB));

            /*
             * abs оставляет preview двухсторонним.
             * Иначе меши с обратным winding могут исчезать.
             */
            const float diffuse =
                std::abs(
                    Dot(
                        faceNormal,
                        previewLight));

            const float lighting =
                0.28F +
                diffuse * 0.72F;

            const LegacyMaterialChunk* materialChunk =
                FindMaterialChunk(
                    mesh,
                    indexOffset,
                    chunkCursor);

            const LegacyMaterialData* materialData =
                materialChunk != nullptr &&
                materials != nullptr
                    ? materials->Find(
                        materialChunk->materialName)
                    : nullptr;

            triangle.color =
                BuildMaterialColor(
                    materialChunk,
                    materialData,
                    chunkCursor,
                    lighting);

            projectedTriangles.push_back(
                triangle);
        }

        std::sort(
            projectedTriangles.begin(),
            projectedTriangles.end(),
            [](
                const ProjectedTriangle& left,
                const ProjectedTriangle& right)
            {
                /*
                 * Painter algorithm:
                 * дальние triangles рисуются первыми.
                 */
                return left.depth > right.depth;
            });

        for (const ProjectedTriangle& triangle :
             projectedTriangles)
        {
            if (!wireframe)
            {
                drawList.AddTriangleFilled(
                    triangle.points[0],
                    triangle.points[1],
                    triangle.points[2],
                    triangle.color);
            }
            else
            {
                drawList.AddTriangle(
                    triangle.points[0],
                    triangle.points[1],
                    triangle.points[2],
                    IM_COL32(
                        205,
                        215,
                        225,
                        220),
                    1.0F);
            }
        }

        const Vector3 target =
            ToVector3(target_);

        const float axisLength =
            radius_ * 0.35F;

        DrawAxis(
            drawList,
            view,
            canvasMinimum,
            canvasSize,
            target,
            Add(
                target,
                Vector3{
                    axisLength,
                    0.0F,
                    0.0F
                }),
            IM_COL32(
                220,
                70,
                65,
                255));

        DrawAxis(
            drawList,
            view,
            canvasMinimum,
            canvasSize,
            target,
            Add(
                target,
                Vector3{
                    0.0F,
                    axisLength,
                    0.0F
                }),
            IM_COL32(
                80,
                210,
                105,
                255));

        DrawAxis(
            drawList,
            view,
            canvasMinimum,
            canvasSize,
            target,
            Add(
                target,
                Vector3{
                    0.0F,
                    0.0F,
                    axisLength
                }),
            IM_COL32(
                70,
                125,
                230,
                255));

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

                const Vector3 bonePosition =
                    GetBonePosition(bone);

                ImVec2 boneScreen;
                float boneDepth = 0.0F;

                if (!ProjectPoint(
                        bonePosition,
                        view,
                        canvasMinimum,
                        canvasSize,
                        boneScreen,
                        boneDepth))
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

                    const Vector3 parentPosition =
                        GetBonePosition(parent);

                    ImVec2 parentScreen;
                    float parentDepth = 0.0F;

                    if (ProjectPoint(
                            parentPosition,
                            view,
                            canvasMinimum,
                            canvasSize,
                            parentScreen,
                            parentDepth))
                    {
                        drawList.AddLine(
                            parentScreen,
                            boneScreen,
                            IM_COL32(
                                255,
                                205,
                                55,
                                235),
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
                            95,
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
            "Vertices: %llu  Triangles: %llu  Drawn: %llu  Bones: %llu",
            static_cast<unsigned long long>(
                mesh.vertices.size()),
            static_cast<unsigned long long>(
                triangleCount),
            static_cast<unsigned long long>(
                projectedTriangles.size()),
            static_cast<unsigned long long>(
                skeleton != nullptr
                    ? skeleton->bones.size()
                    : 0U));

        drawList.AddText(
            ImVec2(
                canvasMinimum.x + 9.0F,
                canvasMinimum.y + 8.0F),
            IM_COL32(
                225,
                230,
                235,
                255),
            statistics);

        if (triangleStep > 1U)
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
                "Large mesh: preview triangle sampling is enabled.");
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