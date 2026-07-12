#include "Runtime/ServiceRegistry.h"

namespace engine::runtime
{
    ServiceRegistry::~ServiceRegistry() noexcept
    {
        Clear();
    }

    bool ServiceRegistry::RegisterRaw(
        const std::type_index type,
        void* const service)
    {
        if (service == nullptr)
        {
            return false;
        }

        engine::platform::MutexLockGuard lock(
            mutex_);

        const auto iterator =
            services_.find(type);

        if (iterator != services_.end())
        {
            /*
             * Повторная регистрация того же объекта
             * считается успешной и идемпотентной.
             */
            return iterator->second == service;
        }

        services_.emplace(
            type,
            service);

        return true;
    }

    void* ServiceRegistry::TryGetRaw(
        const std::type_index type) const noexcept
    {
        engine::platform::MutexLockGuard lock(
            mutex_);

        const auto iterator =
            services_.find(type);

        if (iterator == services_.end())
        {
            return nullptr;
        }

        return iterator->second;
    }

    bool ServiceRegistry::UnregisterRaw(
        const std::type_index type,
        void* const expectedService) noexcept
    {
        engine::platform::MutexLockGuard lock(
            mutex_);

        const auto iterator =
            services_.find(type);

        if (iterator == services_.end())
        {
            return false;
        }

        if (
            expectedService != nullptr &&
            iterator->second != expectedService
        )
        {
            return false;
        }

        services_.erase(iterator);

        return true;
    }

    void ServiceRegistry::Clear() noexcept
    {
        engine::platform::MutexLockGuard lock(
            mutex_);

        services_.clear();
    }

    std::size_t ServiceRegistry::
        GetServiceCount() const noexcept
    {
        engine::platform::MutexLockGuard lock(
            mutex_);

        return services_.size();
    }
}