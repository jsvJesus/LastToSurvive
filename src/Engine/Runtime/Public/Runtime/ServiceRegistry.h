#pragma once

#include <Platform/Synchronization.h>

#include <cstddef>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

namespace engine::runtime
{
    class ServiceRegistry final
    {
    public:
        ServiceRegistry() = default;

        ~ServiceRegistry() noexcept;

        ServiceRegistry(
            const ServiceRegistry&) = delete;

        ServiceRegistry& operator=(
            const ServiceRegistry&) = delete;

        ServiceRegistry(
            ServiceRegistry&&) = delete;

        ServiceRegistry& operator=(
            ServiceRegistry&&) = delete;

        template<typename ServiceType>
        [[nodiscard]] bool Register(
            ServiceType& service)
        {
            using CleanServiceType =
                std::remove_cv_t<
                    std::remove_reference_t<
                        ServiceType>>;

            return RegisterRaw(
                std::type_index(
                    typeid(CleanServiceType)),
                static_cast<void*>(
                    &service));
        }

        template<typename ServiceType>
        [[nodiscard]] ServiceType*
            TryGet() noexcept
        {
            using CleanServiceType =
                std::remove_cv_t<
                    std::remove_reference_t<
                        ServiceType>>;

            return static_cast<ServiceType*>(
                TryGetRaw(
                    std::type_index(
                        typeid(CleanServiceType))));
        }

        template<typename ServiceType>
        [[nodiscard]] const ServiceType*
            TryGet() const noexcept
        {
            using CleanServiceType =
                std::remove_cv_t<
                    std::remove_reference_t<
                        ServiceType>>;

            return static_cast<const ServiceType*>(
                TryGetRaw(
                    std::type_index(
                        typeid(CleanServiceType))));
        }

        template<typename ServiceType>
        [[nodiscard]] bool Contains() const noexcept
        {
            return TryGet<ServiceType>() != nullptr;
        }

        template<typename ServiceType>
        [[nodiscard]] bool Unregister(
            ServiceType* expectedService = nullptr) noexcept
        {
            using CleanServiceType =
                std::remove_cv_t<
                    std::remove_reference_t<
                        ServiceType>>;

            return UnregisterRaw(
                std::type_index(
                    typeid(CleanServiceType)),
                static_cast<void*>(
                    expectedService));
        }

        void Clear() noexcept;

        [[nodiscard]] std::size_t
            GetServiceCount() const noexcept;

    private:
        [[nodiscard]] bool RegisterRaw(
            std::type_index type,
            void* service);

        [[nodiscard]] void* TryGetRaw(
            std::type_index type) const noexcept;

        [[nodiscard]] bool UnregisterRaw(
            std::type_index type,
            void* expectedService) noexcept;

        mutable engine::platform::Mutex mutex_;

        std::unordered_map<
            std::type_index,
            void*>
                services_;
    };
}