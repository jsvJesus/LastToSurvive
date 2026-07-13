#pragma once

#include <utility>

namespace engine::graphics::d3d9::detail
{
    template<typename T>
    class ComPtr final
    {
    public:
        ComPtr() noexcept = default;

        ~ComPtr() noexcept
        {
            Reset();
        }

        ComPtr(const ComPtr&) = delete;
        ComPtr& operator=(const ComPtr&) = delete;

        ComPtr(ComPtr&& other) noexcept
            : pointer_(other.Detach())
        {
        }

        ComPtr& operator=(ComPtr&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                pointer_ = other.Detach();
            }

            return *this;
        }

        [[nodiscard]] T* Get() const noexcept
        {
            return pointer_;
        }

        [[nodiscard]] T** Put() noexcept
        {
            Reset();
            return &pointer_;
        }

        void Attach(T* pointer) noexcept
        {
            if (pointer_ != pointer)
            {
                Reset();
                pointer_ = pointer;
            }
        }

        [[nodiscard]] T* Detach() noexcept
        {
            T* pointer = pointer_;
            pointer_ = nullptr;
            return pointer;
        }

        void Reset() noexcept
        {
            if (pointer_ != nullptr)
            {
                pointer_->Release();
                pointer_ = nullptr;
            }
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return pointer_ != nullptr;
        }

    private:
        T* pointer_ = nullptr;
    };
}
