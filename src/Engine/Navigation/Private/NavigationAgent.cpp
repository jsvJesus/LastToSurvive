#include "Navigation/NavigationAgent.h"

#include "Navigation/NavigationMesh.h"

#include <algorithm>

namespace engine::navigation
{
    NavigationAgent::NavigationAgent(
        NavigationMesh& navigationMesh) noexcept
        : navigationMesh_(&navigationMesh)
    {
    }

    bool NavigationAgent::MoveTo(
        const math::Vector3& destination)
    {
        path_.clear();
        nextPathPoint_ = 0;

        if (!navigationMesh_ ||
            !navigationMesh_->FindPath(position_, destination, path_) ||
            path_.empty())
        {
            status_ = Status::PathNotFound;
            return false;
        }

        nextPathPoint_ = path_.size() > 1U ? 1U : 0U;
        status_ = Status::Moving;
        return true;
    }

    void NavigationAgent::Stop() noexcept
    {
        path_.clear();
        nextPathPoint_ = 0;
        status_ = Status::Idle;
    }

    void NavigationAgent::Update(
        const float elapsedSeconds) noexcept
    {
        if (status_ != Status::Moving ||
            nextPathPoint_ >= path_.size() ||
            elapsedSeconds <= 0.0f)
        {
            return;
        }

        const math::Vector3 delta = path_[nextPathPoint_] - position_;
        const float distance = delta.Length();
        const float movement = maximumSpeed_ * elapsedSeconds;

        if (distance <= std::max(arrivalRadius_, movement))
        {
            position_ = path_[nextPathPoint_];
            ++nextPathPoint_;

            if (nextPathPoint_ >= path_.size())
            {
                status_ = Status::Arrived;
            }
            return;
        }

        position_ += delta * (movement / distance);
    }

    void NavigationAgent::SetPosition(
        const math::Vector3& position) noexcept
    {
        position_ = position;
    }

    void NavigationAgent::SetMaximumSpeed(
        const float maximumSpeed) noexcept
    {
        maximumSpeed_ = std::max(0.0f, maximumSpeed);
    }

    void NavigationAgent::SetArrivalRadius(
        const float arrivalRadius) noexcept
    {
        arrivalRadius_ = std::max(0.0f, arrivalRadius);
    }

    const math::Vector3& NavigationAgent::GetPosition() const noexcept
    {
        return position_;
    }

    NavigationAgent::Status NavigationAgent::GetStatus() const noexcept
    {
        return status_;
    }
}
