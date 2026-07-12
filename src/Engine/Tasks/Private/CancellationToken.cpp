#include "Tasks/CancellationToken.h"

#include <atomic>
#include <utility>

namespace engine::tasks
{
    namespace detail
    {
        struct CancellationState final
        {
            std::atomic<bool> cancellationRequested{
                false
            };
        };
    }

    CancellationToken::CancellationToken(
        std::shared_ptr<
            detail::CancellationState> state) noexcept
        : state_(std::move(state))
    {
    }

    bool CancellationToken::IsValid() const noexcept
    {
        return state_ != nullptr;
    }

    bool CancellationToken::
        IsCancellationRequested() const noexcept
    {
        return state_ != nullptr &&
            state_->cancellationRequested.load(
                std::memory_order_acquire);
    }

    CancellationSource::CancellationSource()
        : state_(
              std::make_shared<
                  detail::CancellationState>())
    {
    }

    CancellationToken
        CancellationSource::GetToken() const noexcept
    {
        return CancellationToken(state_);
    }

    bool CancellationSource::
        IsCancellationRequested() const noexcept
    {
        return state_ != nullptr &&
            state_->cancellationRequested.load(
                std::memory_order_acquire);
    }

    bool CancellationSource::Cancel() noexcept
    {
        if (state_ == nullptr)
        {
            return false;
        }

        const bool wasCancelled =
            state_->cancellationRequested.exchange(
                true,
                std::memory_order_acq_rel);

        return !wasCancelled;
    }

    void CancellationSource::Reset()
    {
        state_ =
            std::make_shared<
                detail::CancellationState>();
    }
}