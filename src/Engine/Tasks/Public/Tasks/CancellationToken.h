#pragma once

#include <memory>

namespace engine::tasks
{
    namespace detail
    {
        struct CancellationState;
    }

    class CancellationSource;

    class CancellationToken final
    {
    public:
        CancellationToken() noexcept = default;

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] bool
            IsCancellationRequested() const noexcept;

    private:
        explicit CancellationToken(
            std::shared_ptr<
                detail::CancellationState> state) noexcept;

        std::shared_ptr<
            detail::CancellationState> state_;

        friend class CancellationSource;
    };

    class CancellationSource final
    {
    public:
        CancellationSource();

        [[nodiscard]] CancellationToken
            GetToken() const noexcept;

        [[nodiscard]] bool
            IsCancellationRequested() const noexcept;

        /*
         * Возвращает true только при первом
         * успешном переходе в cancelled.
         */
        [[nodiscard]] bool Cancel() noexcept;

        /*
         * Создаёт новое состояние.
         * Старые token продолжают видеть старую отмену.
         */
        void Reset();

    private:
        std::shared_ptr<
            detail::CancellationState> state_;
    };
}