#include "Editor/LevelEditor/Viewport/TransformController.h"

#include "Editor/LevelEditor/Scene/ScenePicker.h"

#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace lts::editor
{
    void TransformController::SetOperation(
        const EditorTransformOperation operation) noexcept
    {
        visualState_.operation = operation;
    }

    void TransformController::ToggleSpace() noexcept
    {
        visualState_.space =
            visualState_.space == EditorTransformSpace::World
                ? EditorTransformSpace::Local
                : EditorTransformSpace::World;
    }

    void TransformController::SetSnappingEnabled(
        const bool enabled) noexcept
    {
        snappingEnabled_ = enabled;
    }

    bool TransformController::IsSnappingEnabled() const noexcept
    {
        return snappingEnabled_;
    }

    void TransformController::SetSnapSteps(
        const float move, const float rotate, const float scale) noexcept
    {
        moveSnap_ = std::max(move, 0.001F);
        rotateSnap_ = std::max(rotate, 0.001F);
        scaleSnap_ = std::max(scale, 0.001F);
    }

    std::array<float, 3U> TransformController::GetSnapSteps() const noexcept
    {
        return {moveSnap_, rotateSnap_, scaleSnap_};
    }

    namespace
    {
        constexpr float GizmoLength = 2.5F;
        constexpr float GizmoDeadZone = 0.40F;
        constexpr float MinimumGizmoPickRadius = 0.30F;
        constexpr float MaximumGizmoPickRadius = 1.35F;
        constexpr float GizmoPickRadiusPerDistance = 0.022F;

        constexpr float RotationRadius = 2.0F;
        constexpr float MinimumRotationPickTolerance = 0.30F;
        constexpr float MaximumRotationPickTolerance = 1.20F;
        constexpr float RotationPickTolerancePerDistance = 0.022F;

        constexpr float Epsilon = 0.000001F;

        [[nodiscard]]
        HWND ToWindowHandle(
            const engine::platform::NativeWindowHandle handle) noexcept
        {
            return reinterpret_cast<HWND>(handle.Value());
        }

        [[nodiscard]]
        bool IsKeyDown(const int virtualKey) noexcept
        {
            return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
        }

        [[nodiscard]]
        DirectX::XMFLOAT3 Add(
            const DirectX::XMFLOAT3& left,
            const DirectX::XMFLOAT3& right) noexcept
        {
            return
            {
                left.x + right.x,
                left.y + right.y,
                left.z + right.z
            };
        }

        [[nodiscard]]
        DirectX::XMFLOAT3 Subtract(
            const DirectX::XMFLOAT3& left,
            const DirectX::XMFLOAT3& right) noexcept
        {
            return
            {
                left.x - right.x,
                left.y - right.y,
                left.z - right.z
            };
        }

        [[nodiscard]]
        DirectX::XMFLOAT3 Multiply(
            const DirectX::XMFLOAT3& value,
            const float scalar) noexcept
        {
            return
            {
                value.x * scalar,
                value.y * scalar,
                value.z * scalar
            };
        }

        [[nodiscard]]
        float Dot(
            const DirectX::XMFLOAT3& left,
            const DirectX::XMFLOAT3& right) noexcept
        {
            return
                left.x * right.x +
                left.y * right.y +
                left.z * right.z;
        }

        [[nodiscard]]
        DirectX::XMFLOAT3 Cross(
            const DirectX::XMFLOAT3& left,
            const DirectX::XMFLOAT3& right) noexcept
        {
            return
            {
                left.y * right.z - left.z * right.y,
                left.z * right.x - left.x * right.z,
                left.x * right.y - left.y * right.x
            };
        }

        [[nodiscard]]
        float LengthSquared(
            const DirectX::XMFLOAT3& value) noexcept
        {
            return Dot(value, value);
        }

        [[nodiscard]]
        float Length(
            const DirectX::XMFLOAT3& value) noexcept
        {
            return std::sqrt(LengthSquared(value));
        }

        [[nodiscard]]
        DirectX::XMFLOAT3 Normalize(
            const DirectX::XMFLOAT3& value) noexcept
        {
            const float length = Length(value);

            if (length <= Epsilon)
            {
                return {};
            }

            return Multiply(value, 1.0F / length);
        }

        [[nodiscard]]
        bool IntersectPlane(
            const EditorPickRay& ray,
            const DirectX::XMFLOAT3& point,
            const DirectX::XMFLOAT3& normal,
            DirectX::XMFLOAT3& hit,
            float& distance) noexcept
        {
            const float denominator = Dot(
                ray.direction,
                normal);

            if (std::abs(denominator) <= Epsilon)
            {
                return false;
            }

            distance = Dot(
                Subtract(point, ray.origin),
                normal) /
                denominator;

            if (
                distance < 0.0F ||
                !std::isfinite(distance))
            {
                return false;
            }

            hit = Add(
                ray.origin,
                Multiply(ray.direction, distance));

            return true;
        }

        [[nodiscard]]
        float DistanceRayToSegment(
            const EditorPickRay& ray,
            const DirectX::XMFLOAT3& segmentStart,
            const DirectX::XMFLOAT3& segmentEnd,
            float& rayParameter) noexcept
        {
            const DirectX::XMFLOAT3 segment = Subtract(
                segmentEnd,
                segmentStart);

            const float segmentLengthSquared =
                LengthSquared(segment);

            if (segmentLengthSquared <= Epsilon)
            {
                rayParameter = std::max(
                    Dot(
                        Subtract(segmentStart, ray.origin),
                        ray.direction),
                    0.0F);

                const DirectX::XMFLOAT3 rayPoint = Add(
                    ray.origin,
                    Multiply(ray.direction, rayParameter));

                return Length(
                    Subtract(rayPoint, segmentStart));
            }

            const DirectX::XMFLOAT3 startToOrigin = Subtract(
                ray.origin,
                segmentStart);

            const float raySegmentDot = Dot(
                ray.direction,
                segment);

            const float rayOriginDot = Dot(
                ray.direction,
                startToOrigin);

            const float segmentOriginDot = Dot(
                segment,
                startToOrigin);

            const float denominator =
                segmentLengthSquared -
                raySegmentDot * raySegmentDot;

            float segmentParameter = 0.0F;

            if (std::abs(denominator) > Epsilon)
            {
                segmentParameter =
                    (raySegmentDot * rayOriginDot -
                        segmentOriginDot) /
                    denominator;
            }

            segmentParameter = std::clamp(
                segmentParameter,
                0.0F,
                1.0F);

            const DirectX::XMFLOAT3 segmentPoint = Add(
                segmentStart,
                Multiply(segment, segmentParameter));

            rayParameter = std::max(
                Dot(
                    Subtract(segmentPoint, ray.origin),
                    ray.direction),
                0.0F);

            const DirectX::XMFLOAT3 rayPoint = Add(
                ray.origin,
                Multiply(ray.direction, rayParameter));

            return Length(
                Subtract(rayPoint, segmentPoint));
        }

        [[nodiscard]]
        bool ClosestParameterOnAxis(
            const EditorPickRay& ray,
            const DirectX::XMFLOAT3& axisOrigin,
            const DirectX::XMFLOAT3& axisDirection,
            float& axisParameter) noexcept
        {
            const DirectX::XMFLOAT3 offset =
                Subtract(ray.origin, axisOrigin);

            const float rayLengthSquared =
                Dot(ray.direction, ray.direction);

            const float axisLengthSquared =
                Dot(axisDirection, axisDirection);

            const float rayAxisDot =
                Dot(ray.direction, axisDirection);

            const float rayOffsetDot =
                Dot(ray.direction, offset);

            const float axisOffsetDot =
                Dot(axisDirection, offset);

            const float denominator =
                rayLengthSquared * axisLengthSquared -
                rayAxisDot * rayAxisDot;

            if (std::abs(denominator) <= Epsilon)
            {
                return false;
            }

            axisParameter =
                (rayLengthSquared * axisOffsetDot -
                    rayAxisDot * rayOffsetDot) /
                denominator;

            return std::isfinite(axisParameter);
        }

        [[nodiscard]]
        std::size_t AxisIndex(
            const EditorTransformAxis axis) noexcept
        {
            switch (axis)
            {
                case EditorTransformAxis::X:
                    return 0U;

                case EditorTransformAxis::Y:
                    return 1U;

                case EditorTransformAxis::Z:
                    return 2U;

                case EditorTransformAxis::None:
                default:
                    return 0U;
            }
        }

        [[nodiscard]]
        float SnapValue(
            const float value,
            const float increment) noexcept
        {
            if (increment <= Epsilon)
            {
                return value;
            }

            return std::round(value / increment) * increment;
        }

        [[nodiscard]]
        const wchar_t* OperationName(
            const EditorTransformOperation operation) noexcept
        {
            switch (operation)
            {
                case EditorTransformOperation::Select:
                    return L"Select";

                case EditorTransformOperation::Move:
                    return L"Move";

                case EditorTransformOperation::Rotate:
                    return L"Rotate";

                case EditorTransformOperation::Scale:
                    return L"Scale";

                default:
                    return L"Unknown";
            }
        }

        [[nodiscard]]
        const wchar_t* SpaceName(
            const EditorTransformSpace space) noexcept
        {
            return space == EditorTransformSpace::World
                ? L"World"
                : L"Local";
        }
    }

    TransformController::~TransformController() noexcept
    {
        const HWND viewport = ToWindowHandle(viewportWindow_);

        if (
            viewport != nullptr &&
            GetCapture() == viewport)
        {
            ReleaseCapture();
        }
    }

    void TransformController::SetViewportWindow(
        const engine::platform::NativeWindowHandle window) noexcept
    {
        const HWND oldViewport = ToWindowHandle(viewportWindow_);

        if (
            oldViewport != nullptr &&
            GetCapture() == oldViewport)
        {
            ReleaseCapture();
        }

        viewportWindow_ = window;
        visualState_.hotAxis = EditorTransformAxis::None;
        visualState_.activeAxis = EditorTransformAxis::None;
        dragChanged_ = false;
    }

    EditorInteractionResult TransformController::Update(
        SceneDocument& document,
        CommandHistory& history,
        CameraController& camera,
        const engine::platform::WindowSize viewportSize,
        const ViewportClick* const viewportClick,
        const StaticMeshRenderer* const meshRenderer) noexcept
    {
        EditorInteractionResult result;

        const HWND viewport = ToWindowHandle(viewportWindow_);

        if (
            viewport == nullptr ||
            !IsWindow(viewport))
        {
            return result;
        }

        if (
            visualState_.activeAxis != EditorTransformAxis::None &&
            UpdateDrag(
                document,
                history,
                camera,
                viewportSize))
        {
            result.documentChanged = true;
        }

        const bool viewportFocused = IsViewportFocused();
        const bool rightMouseDown = IsKeyDown(VK_RBUTTON);

        if (
            viewportFocused &&
            !rightMouseDown &&
            visualState_.activeAxis == EditorTransformAxis::None)
        {
            const bool selectPressed = WasPressed('Q');
            const bool movePressed = WasPressed('W');
            const bool rotatePressed = WasPressed('E');
            const bool scalePressed = WasPressed('R');
            const bool spacePressed = WasPressed('T');
            const bool focusPressed = WasPressed('F');

            if (selectPressed)
            {
                visualState_.operation = EditorTransformOperation::Select;
                result.statusChanged = true;
            }

            if (movePressed)
            {
                visualState_.operation = EditorTransformOperation::Move;
                result.statusChanged = true;
            }

            if (rotatePressed)
            {
                visualState_.operation = EditorTransformOperation::Rotate;
                result.statusChanged = true;
            }

            if (scalePressed)
            {
                visualState_.operation = EditorTransformOperation::Scale;
                result.statusChanged = true;
            }

            if (spacePressed)
            {
                visualState_.space =
                    visualState_.space == EditorTransformSpace::World
                        ? EditorTransformSpace::Local
                        : EditorTransformSpace::World;

                result.statusChanged = true;
            }

            if (focusPressed)
            {
                const EditorSceneEntity* const entity =
                    document.GetSelectedEntity();

                if (entity != nullptr)
                {
                    DirectX::XMFLOAT3 target
                    {
                        entity->transform.position[0],
                        entity->transform.position[1] + 0.75F,
                        entity->transform.position[2]
                    };

                    camera.FocusOn(target, 10.0F);
                }
            }
        }
        else
        {
            static_cast<void>(WasPressed('Q'));
            static_cast<void>(WasPressed('W'));
            static_cast<void>(WasPressed('E'));
            static_cast<void>(WasPressed('R'));
            static_cast<void>(WasPressed('T'));
            static_cast<void>(WasPressed('F'));
        }

        if (
            viewportClick != nullptr &&
            visualState_.activeAxis == EditorTransformAxis::None)
        {
            EditorPickRay ray;

            if (camera.BuildPickRay(
                    viewportClick->x,
                    viewportClick->y,
                    viewportSize.width,
                    viewportSize.height,
                    ray))
            {
                if (!TryBeginDrag(document, ray))
                {
                    std::size_t pickedIndex =
                        InvalidEditorEntityIndex;

                    float distance = 0.0F;

                    if (ScenePicker::Pick(
                            document,
                            ray,
                            pickedIndex,
                            distance,
                            meshRenderer) &&
                        document.SelectEntityByIndex(pickedIndex))
                    {
                        result.selectionChanged = true;
                    }
                    else
                    {
                        document.ClearSelection();
                        result.selectionChanged = true;
                    }
                }
            }
        }

        if (visualState_.activeAxis == EditorTransformAxis::None)
        {
            EditorPickRay currentRay;

            if (
                viewportFocused &&
                BuildCurrentPickRay(
                    camera,
                    viewportSize,
                    currentRay))
            {
                UpdateHotAxis(document, currentRay);
            }
            else
            {
                visualState_.hotAxis = EditorTransformAxis::None;
            }
        }

        return result;
    }

    void TransformController::SetViewportRegion(
        const std::int32_t x,
        const std::int32_t y,
        const std::uint32_t,
        const std::uint32_t) noexcept
    {
        viewportX_ = x;
        viewportY_ = y;
    }

    const EditorTransformVisualState&
        TransformController::GetVisualState() const noexcept
    {
        return visualState_;
    }

    std::wstring TransformController::BuildStatusText() const
    {
        std::wstring status = L"Ready | Mode: Level | Tool: ";
        status += OperationName(visualState_.operation);
        status += L" | Space: ";
        status += SpaceName(visualState_.space);
        status += L" | W/E/R Tool | T World/Local | F Focus";

        return status;
    }

    bool TransformController::IsViewportFocused() const noexcept
    {
        const HWND viewport = ToWindowHandle(viewportWindow_);

        if (
            viewport == nullptr ||
            !IsWindow(viewport))
        {
            return false;
        }

        const HWND root = GetAncestor(
            viewport,
            GA_ROOT);

        return
            root != nullptr &&
            GetForegroundWindow() == root &&
            GetFocus() == viewport;
    }

    bool TransformController::WasPressed(
        const int virtualKey) noexcept
    {
        if (
            virtualKey < 0 ||
            virtualKey >= static_cast<int>(previousKeyDown_.size()))
        {
            return false;
        }

        const bool down = IsKeyDown(virtualKey);
        const std::size_t index = static_cast<std::size_t>(virtualKey);

        const bool pressed =
            down &&
            !previousKeyDown_[index];

        previousKeyDown_[index] = down;
        return pressed;
    }

    bool TransformController::BuildCurrentPickRay(
        CameraController& camera,
        const engine::platform::WindowSize viewportSize,
        EditorPickRay& ray) const noexcept
    {
        const HWND viewport = ToWindowHandle(viewportWindow_);

        if (viewport == nullptr)
        {
            return false;
        }

        POINT cursor{};

        if (!GetCursorPos(&cursor))
        {
            return false;
        }

        if (!ScreenToClient(viewport, &cursor))
        {
            return false;
        }

        if (
            cursor.x < viewportX_ ||
            cursor.y < viewportY_)
        {
            return false;
        }

        return camera.BuildPickRay(
            static_cast<std::uint32_t>(cursor.x - viewportX_),
            static_cast<std::uint32_t>(cursor.y - viewportY_),
            viewportSize.width,
            viewportSize.height,
            ray);
    }

    bool TransformController::TryBeginDrag(
        SceneDocument& document,
        const EditorPickRay& ray) noexcept
    {
        const EditorSceneEntity* const entity =
            document.GetSelectedEntity();

        if (entity == nullptr)
        {
            return false;
        }

        float axisParameter = 0.0F;

        const EditorTransformAxis axis = PickAxis(
            *entity,
            ray,
            axisParameter);

        static_cast<void>(axisParameter);

        if (axis == EditorTransformAxis::None)
        {
            return false;
        }

        dragBefore_ = document.CreateSnapshot();
        dragStartTransform_ = entity->transform;

        dragOrigin_ =
        {
            entity->transform.position[0],
            entity->transform.position[1] + 0.08F,
            entity->transform.position[2]
        };

        dragAxis_ = GetAxisVector(
            entity->transform,
            axis);

        visualState_.activeAxis = axis;
        visualState_.hotAxis = axis;

        dragChanged_ = false;

        DirectX::XMFLOAT3 hit{};
        float distance = 0.0F;

        if (visualState_.operation == EditorTransformOperation::Rotate)
        {
            if (!IntersectPlane(
                    ray,
                    dragOrigin_,
                    dragAxis_,
                    hit,
                    distance))
            {
                visualState_.activeAxis =
                    EditorTransformAxis::None;

                return false;
            }

            dragStartVector_ = Normalize(
                Subtract(hit, dragOrigin_));

            if (LengthSquared(dragStartVector_) <= Epsilon)
            {
                visualState_.activeAxis =
                    EditorTransformAxis::None;

                return false;
            }
        }
        else
        {
            const DirectX::XMFLOAT3 side = Cross(
                ray.direction,
                dragAxis_);

            dragPlaneNormal_ = Normalize(
                Cross(dragAxis_, side));

            if (LengthSquared(dragPlaneNormal_) <= Epsilon)
            {
                const DirectX::XMFLOAT3 fallback =
                    std::abs(dragAxis_.y) < 0.9F
                        ? DirectX::XMFLOAT3
                        {
                            0.0F,
                            1.0F,
                            0.0F
                        }
                        : DirectX::XMFLOAT3
                        {
                            1.0F,
                            0.0F,
                            0.0F
                        };

                dragPlaneNormal_ = Normalize(
                    Cross(
                        dragAxis_,
                        Cross(
                            fallback,
                            dragAxis_)));
            }

            if (!ClosestParameterOnAxis(
                    ray,
                    dragOrigin_,
                    dragAxis_,
                    dragStartParameter_))
            {
                if (!IntersectPlane(
                        ray,
                        dragOrigin_,
                        dragPlaneNormal_,
                        hit,
                        distance))
                {
                    visualState_.activeAxis =
                        EditorTransformAxis::None;

                    return false;
                }

                dragStartParameter_ = Dot(
                    Subtract(
                        hit,
                        dragOrigin_),
                    dragAxis_);
            }
        }

        const HWND viewport =
            ToWindowHandle(viewportWindow_);

        if (viewport != nullptr)
        {
            SetCapture(viewport);
        }

        return true;
    }

    bool TransformController::UpdateDrag(
        SceneDocument& document,
        CommandHistory& history,
        CameraController& camera,
        const engine::platform::WindowSize viewportSize) noexcept
    {
        if (!IsKeyDown(VK_LBUTTON))
        {
            const bool changed = dragChanged_;

            EndDrag(
                document,
                history,
                false);

            return changed;
        }

        if (WasPressed(VK_ESCAPE))
        {
            EndDrag(
                document,
                history,
                true);

            return true;
        }

        EditorPickRay ray;

        if (!BuildCurrentPickRay(
                camera,
                viewportSize,
                ray))
        {
            return false;
        }

        DirectX::XMFLOAT3 hit{};
        float distance = 0.0F;

        EditorTransform transform =
            dragStartTransform_;

        const bool snapping = snappingEnabled_ ||
            IsKeyDown(VK_CONTROL) ||
            IsKeyDown(VK_LCONTROL) ||
            IsKeyDown(VK_RCONTROL);

        if (visualState_.operation == EditorTransformOperation::Rotate)
        {
            if (!IntersectPlane(
                    ray,
                    dragOrigin_,
                    dragAxis_,
                    hit,
                    distance))
            {
                return false;
            }

            const DirectX::XMFLOAT3 currentVector =
                Normalize(
                    Subtract(
                        hit,
                        dragOrigin_));

            if (LengthSquared(currentVector) <= Epsilon)
            {
                return false;
            }

            const float sine = Dot(
                dragAxis_,
                Cross(
                    dragStartVector_,
                    currentVector));

            const float cosine = Dot(
                dragStartVector_,
                currentVector);

            float angleDegrees =
                DirectX::XMConvertToDegrees(
                    std::atan2(
                        sine,
                        cosine));

            if (snapping)
            {
                angleDegrees = SnapValue(
                    angleDegrees,
                    rotateSnap_);
            }

            transform.rotationDegrees[
                AxisIndex(
                    visualState_.activeAxis)] +=
                        angleDegrees;
        }
        else
        {
            float currentParameter = 0.0F;

            if (!ClosestParameterOnAxis(
                    ray,
                    dragOrigin_,
                    dragAxis_,
                    currentParameter))
            {
                if (!IntersectPlane(
                        ray,
                        dragOrigin_,
                        dragPlaneNormal_,
                        hit,
                        distance))
                {
                    return false;
                }

                currentParameter = Dot(
                    Subtract(
                        hit,
                        dragOrigin_),
                    dragAxis_);
            }

            float delta =
                currentParameter -
                dragStartParameter_;

            if (visualState_.operation == EditorTransformOperation::Move)
            {
                if (snapping)
                {
                    delta = SnapValue(
                        delta,
                        moveSnap_);
                }

                transform.position[0] +=
                    dragAxis_.x * delta;

                transform.position[1] +=
                    dragAxis_.y * delta;

                transform.position[2] +=
                    dragAxis_.z * delta;
            }
            else
            {
                delta *= 0.5F;

                if (snapping)
                {
                    delta = SnapValue(
                        delta,
                        scaleSnap_);
                }

                const std::size_t axisIndex =
                    AxisIndex(
                        visualState_.activeAxis);

                transform.scale[axisIndex] =
                    std::max(
                        0.01F,
                        dragStartTransform_.
                            scale[axisIndex] +
                        delta);
            }
        }

        std::array<float, 3U> positionDelta{};
        std::array<float, 3U> rotationDelta{};
        std::array<float, 3U> scaleRatio{1.0F, 1.0F, 1.0F};
        for (std::size_t axis = 0U; axis < 3U; ++axis)
        {
            positionDelta[axis] =
                transform.position[axis] - dragStartTransform_.position[axis];
            rotationDelta[axis] = transform.rotationDegrees[axis] -
                dragStartTransform_.rotationDegrees[axis];
            if (std::abs(dragStartTransform_.scale[axis]) > Epsilon)
                scaleRatio[axis] = transform.scale[axis] /
                    dragStartTransform_.scale[axis];
        }

        document.RestoreSnapshot(dragBefore_, false);
        if (document.ApplySelectionTransformDelta(
                positionDelta, rotationDelta, scaleRatio))
        {
            dragChanged_ = true;
            return true;
        }

        return false;
    }

    void TransformController::EndDrag(
        SceneDocument& document,
        CommandHistory& history,
        const bool cancel) noexcept
    {
        const HWND viewport = ToWindowHandle(viewportWindow_);

        if (
            viewport != nullptr &&
            GetCapture() == viewport)
        {
            ReleaseCapture();
        }

        if (cancel)
        {
            document.RestoreSnapshot(
                dragBefore_,
                false);
        }
        else if (dragChanged_)
        {
            static_cast<void>(
                history.Push(
                    dragBefore_,
                    document.CreateSnapshot()));
        }

        visualState_.activeAxis = EditorTransformAxis::None;
        dragBefore_ = {};
        dragChanged_ = false;
    }

    void TransformController::UpdateHotAxis(
        const SceneDocument& document,
        const EditorPickRay& ray) noexcept
    {
        const EditorSceneEntity* const entity =
            document.GetSelectedEntity();

        if (entity == nullptr)
        {
            visualState_.hotAxis = EditorTransformAxis::None;
            return;
        }

        float parameter = 0.0F;

        visualState_.hotAxis = PickAxis(
            *entity,
            ray,
            parameter);

        static_cast<void>(parameter);
    }

    EditorTransformAxis TransformController::PickAxis(
        const EditorSceneEntity& entity,
        const EditorPickRay& ray,
        float& parameter) const noexcept
    {
        if (visualState_.operation == EditorTransformOperation::Select)
        {
            return EditorTransformAxis::None;
        }
        const DirectX::XMFLOAT3 origin
        {
            entity.transform.position[0],
            entity.transform.position[1] + 0.08F,
            entity.transform.position[2]
        };

        const float cameraDistance = Length(
            Subtract(
                origin,
                ray.origin));

        const float gizmoPickRadius =
            std::clamp(
                cameraDistance *
                    GizmoPickRadiusPerDistance,
                MinimumGizmoPickRadius,
                MaximumGizmoPickRadius);

        const float rotationPickTolerance =
            std::clamp(
                cameraDistance *
                    RotationPickTolerancePerDistance,
                MinimumRotationPickTolerance,
                MaximumRotationPickTolerance);

        constexpr std::array<EditorTransformAxis, 3U> axes
        {
            EditorTransformAxis::X,
            EditorTransformAxis::Y,
            EditorTransformAxis::Z
        };

        EditorTransformAxis bestAxis =
            EditorTransformAxis::None;

        float bestScore =
            std::numeric_limits<float>::max();

        float bestRayParameter =
            std::numeric_limits<float>::max();

        for (const EditorTransformAxis axis : axes)
        {
            const DirectX::XMFLOAT3 axisVector =
                GetAxisVector(
                    entity.transform,
                    axis);

            float rayParameter = 0.0F;

            float score =
                std::numeric_limits<float>::max();

            bool hit = false;

            if (
                visualState_.operation ==
                EditorTransformOperation::Rotate)
            {
                DirectX::XMFLOAT3 planeHit{};

                if (IntersectPlane(
                        ray,
                        origin,
                        axisVector,
                        planeHit,
                        rayParameter))
                {
                    const float radius = Length(
                        Subtract(
                            planeHit,
                            origin));

                    const float ringDistance =
                        std::abs(
                            radius -
                            RotationRadius);

                    hit =
                        ringDistance <=
                        rotationPickTolerance;

                    if (hit)
                    {
                        score =
                            ringDistance /
                            rotationPickTolerance;
                    }
                }
            }
            else
            {
                /*
                 * Не начинаем выбор оси прямо из центра объекта.
                 * Иначе три оси конфликтуют между собой.
                 */
                const DirectX::XMFLOAT3 segmentStart =
                    Add(
                        origin,
                        Multiply(
                            axisVector,
                            GizmoDeadZone));

                const DirectX::XMFLOAT3 segmentEnd =
                    Add(
                        origin,
                        Multiply(
                            axisVector,
                            GizmoLength));

                const float distance =
                    DistanceRayToSegment(
                        ray,
                        segmentStart,
                        segmentEnd,
                        rayParameter);

                hit =
                    distance <=
                    gizmoPickRadius;

                if (hit)
                {
                    /*
                     * Выбираем ось, которая ближе к курсору,
                     * а не просто находится ближе к камере.
                     */
                    score =
                        distance /
                        gizmoPickRadius;
                }
            }

            if (!hit)
            {
                continue;
            }

            const bool betterScore =
                score <
                bestScore - 0.0001F;

            const bool equalScoreButCloser =
                std::abs(
                    score -
                    bestScore) <= 0.0001F &&
                rayParameter <
                    bestRayParameter;

            if (
                betterScore ||
                equalScoreButCloser)
            {
                bestScore = score;
                bestRayParameter = rayParameter;
                bestAxis = axis;
            }
        }

        parameter = bestRayParameter;
        return bestAxis;
    }

    DirectX::XMFLOAT3 TransformController::GetAxisVector(
        const EditorTransform& transform,
        const EditorTransformAxis axis) const noexcept
    {
        DirectX::XMFLOAT3 baseAxis{};

        switch (axis)
        {
            case EditorTransformAxis::X:
                baseAxis = {1.0F, 0.0F, 0.0F};
                break;

            case EditorTransformAxis::Y:
                baseAxis = {0.0F, 1.0F, 0.0F};
                break;

            case EditorTransformAxis::Z:
                baseAxis = {0.0F, 0.0F, 1.0F};
                break;

            case EditorTransformAxis::None:
            default:
                return {};
        }

        if (visualState_.space == EditorTransformSpace::World)
        {
            return baseAxis;
        }

        const DirectX::XMMATRIX rotation =
            DirectX::XMMatrixRotationRollPitchYaw(
                DirectX::XMConvertToRadians(
                    transform.rotationDegrees[0]),
                DirectX::XMConvertToRadians(
                    transform.rotationDegrees[1]),
                DirectX::XMConvertToRadians(
                    transform.rotationDegrees[2]));

        DirectX::XMFLOAT3 result;

        DirectX::XMStoreFloat3(
            &result,
            DirectX::XMVector3Normalize(
                DirectX::XMVector3TransformNormal(
                    DirectX::XMLoadFloat3(&baseAxis),
                    rotation)));

        return result;
    }
}
