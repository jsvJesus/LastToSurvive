#pragma once

#include <Platform/Window.h>
#include <Runtime/Engine.h>
#include <Platform/Input.h>
#include <Runtime/EngineConfig.h>

#include <cstdint>
#include <memory>
#include <string>

namespace lts::application
{
    enum class ApplicationResult : int
    {
        Success = 0,
        InvalidDescription = 1,
        AlreadyExecuted = 2,
        WindowClassRegistrationFailed = 3,
        WindowCreationFailed = 4,
        RuntimeInitializationFailed = 5,
        ClientInitializationFailed = 6,
        RuntimeFrameFailed = 7
    };

    enum class ApplicationEventType : std::uint8_t
    {
        Resize = 0,
        Minimized,
        Restored,
        Activated,
        Deactivated,
        DpiChanged,
        CloseRequested
    };

    struct ApplicationEvent final
    {
        ApplicationEventType type =
            ApplicationEventType::Resize;

        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t dpi = 96;
    };

    struct ApplicationDesc final
    {
        std::wstring title =
            L"LastToSurvive";

        std::string logFileName =
            "LTS.Application.log";

        std::uint32_t width = 1600;
        std::uint32_t height = 900;

        bool resizable = true;
        bool startMaximized = false;
        bool enableDpiAwareness = true;

        engine::runtime::EngineConfig engineConfig;

        [[nodiscard]]
        bool IsValid() const noexcept
        {
            return
                !title.empty() &&
                width > 0 &&
                height > 0 &&
                !engineConfig.applicationName.empty();
        }
    };

    struct ApplicationContext final
    {
        engine::platform::Window* window = nullptr;
        engine::runtime::Engine* engine = nullptr;
    };

    class Application
    {
    public:
        explicit Application(
            ApplicationDesc description);

        virtual ~Application() noexcept;

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        [[nodiscard]]
        ApplicationResult Run() noexcept;

        void RequestExit() noexcept;

        [[nodiscard]]
        bool IsRunning() const noexcept;

        [[nodiscard]]
        bool IsMinimized() const noexcept;

        [[nodiscard]]
        const ApplicationDesc&
            GetDescription() const noexcept;

        [[nodiscard]]
        engine::platform::Window&
            GetWindow() noexcept;

        [[nodiscard]]
        const engine::platform::Window&
            GetWindow() const noexcept;

        [[nodiscard]]
        engine::runtime::Engine&
            GetEngine() noexcept;

        [[nodiscard]]
        const engine::runtime::Engine&
            GetEngine() const noexcept;

        [[nodiscard]]
        ApplicationContext
            GetContext() noexcept;

        [[nodiscard]]
        engine::platform::InputSystem&
            GetInputSystem() noexcept;

        [[nodiscard]]
        const engine::platform::InputSystem&
            GetInputSystem() const noexcept;

    protected:
        [[nodiscard]]
        virtual ApplicationResult
            OnInitialize() noexcept = 0;

        virtual void OnShutdown() noexcept = 0;

        virtual void OnUpdate(
            double deltaSeconds) noexcept = 0;

        virtual void OnRender() noexcept = 0;

        virtual void OnEvent(
            const ApplicationEvent& event) noexcept;

    private:
        class Impl;

        void DispatchEvent(
            const ApplicationEvent& event) noexcept;

        ApplicationDesc description_;

        engine::platform::Window window_;
        engine::platform::InputSystem inputSystem_;
        engine::runtime::Engine engine_;

        std::unique_ptr<Impl> impl_;

        bool running_ = false;
        bool minimized_ = false;
        bool hasRun_ = false;
        bool clientLifecycleStarted_ = false;
    };
}
