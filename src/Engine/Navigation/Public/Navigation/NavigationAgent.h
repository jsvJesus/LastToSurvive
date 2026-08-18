#pragma once

#include "Math/Vector3.h"

#include <vector>

namespace engine::navigation
{
    class NavigationMesh;

    class NavigationAgent final
    {
    public:
        enum class Status
        {
            Idle,
            Moving,
            Arrived,
            PathNotFound
        };

        explicit NavigationAgent(NavigationMesh& navigationMesh) noexcept;

        [[nodiscard]] bool MoveTo(const math::Vector3& destination);
        void Stop() noexcept;
        void Update(float elapsedSeconds) noexcept;

        void SetPosition(const math::Vector3& position) noexcept;
        void SetMaximumSpeed(float maximumSpeed) noexcept;
        void SetArrivalRadius(float arrivalRadius) noexcept;

        [[nodiscard]] const math::Vector3& GetPosition() const noexcept;
        [[nodiscard]] Status GetStatus() const noexcept;

    private:
        NavigationMesh* navigationMesh_ = nullptr;
        std::vector<math::Vector3> path_;
        math::Vector3 position_{};
        std::size_t nextPathPoint_ = 0;
        float maximumSpeed_ = 3.5f;
        float arrivalRadius_ = 0.1f;
        Status status_ = Status::Idle;
    };
}
