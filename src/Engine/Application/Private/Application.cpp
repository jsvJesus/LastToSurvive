#include "Application/Application.h"

#include <Core/Log.h>
#include <Platform/Clock.h>
#include <Runtime/RuntimeModule.h>
#include <Runtime/TaskRuntimeModule.h>

#include <Windows.h>

#include <algorithm>
#include <fstream>
#include <new>
#include <string>
#include <string_view>
#include <utility>

namespace lts::application
{
    namespace
    {
        [[nodiscard]]
        const char* GetLogLevelName(
            const engine::core::LogLevel level) noexcept
        {
            switch (level)
            {
                case engine::core::LogLevel::Trace:
                    return "Trace";

                case engine::core::LogLevel::Debug:
                    return "Debug";

                case engine::core::LogLevel::Information:
                    return "Info";

                case engine::core::LogLevel::Warning:
                    return "Warning";

                case engine::core::LogLevel::Error:
                    return "Error";

                case engine::core::LogLevel::Critical:
                    return "Critical";

                default:
                    return "Unknown";
            }
        }

        [[nodiscard]]
        DWORD BuildWindowStyle(
            const ApplicationDesc& description) noexcept
        {
            DWORD style = WS_OVERLAPPEDWINDOW;

            if (!description.resizable)
            {
                style &= ~static_cast<DWORD>(
                    WS_THICKFRAME |
                    WS_MAXIMIZEBOX);
            }

            return style;
        }

        void EnableProcessDpiAwareness() noexcept
        {
            HMODULE user32Module =
                GetModuleHandleW(L"user32.dll");

            if (user32Module != nullptr)
            {
                using SetDpiAwarenessContextFunction =
                    BOOL(WINAPI*)(HANDLE);

                const auto setDpiAwarenessContext =
                    reinterpret_cast<
                        SetDpiAwarenessContextFunction>(
                            GetProcAddress(
                                user32Module,
                                "SetProcessDpiAwarenessContext"));

                if (
                    setDpiAwarenessContext != nullptr &&
                    setDpiAwarenessContext(
                        reinterpret_cast<HANDLE>(-4))
                )
                {
                    return;
                }
            }

            SetProcessDPIAware();
        }

        void CalculateWindowRectangle(
            RECT& rectangle,
            const DWORD style) noexcept
        {
            HMODULE user32Module =
                GetModuleHandleW(L"user32.dll");

            if (user32Module != nullptr)
            {
                using GetDpiForSystemFunction =
                    UINT(WINAPI*)();

                using AdjustWindowRectForDpiFunction =
                    BOOL(WINAPI*)(
                        LPRECT,
                        DWORD,
                        BOOL,
                        DWORD,
                        UINT);

                const auto getDpiForSystem =
                    reinterpret_cast<
                        GetDpiForSystemFunction>(
                            GetProcAddress(
                                user32Module,
                                "GetDpiForSystem"));

                const auto adjustWindowRectForDpi =
                    reinterpret_cast<
                        AdjustWindowRectForDpiFunction>(
                            GetProcAddress(
                                user32Module,
                                "AdjustWindowRectExForDpi"));

                if (adjustWindowRectForDpi != nullptr)
                {
                    const UINT dpi =
                        getDpiForSystem != nullptr
                            ? getDpiForSystem()
                            : 96U;

                    adjustWindowRectForDpi(
                        &rectangle,
                        style,
                        FALSE,
                        0,
                        dpi);

                    return;
                }
            }

            AdjustWindowRectEx(
                &rectangle,
                style,
                FALSE,
                0);
        }
    }

    class Application::Impl final
    {
    public:
        explicit Impl(
            Application& owner) noexcept
            : owner_(owner)
        {
        }

        ~Impl() noexcept
        {
            SetEventDispatchEnabled(false);
            DestroyNativeWindow();
            CloseLogFile();
        }

        void OpenLogFile(
            const std::string& fileName) noexcept
        {
            if (fileName.empty())
            {
                return;
            }

            try
            {
                logFile_.open(
                    fileName,
                    std::ios::out |
                    std::ios::trunc);

                if (logFile_.is_open())
                {
                    logFile_ <<
                        "[Application] Log started.\n";

                    logFile_.flush();
                }
            }
            catch (...)
            {
                OutputDebugStringA(
                    "[Application] Failed to open log file.\n");
            }
        }

        void CloseLogFile() noexcept
        {
            try
            {
                if (logFile_.is_open())
                {
                    logFile_ <<
                        "[Application] Log finished.\n";

                    logFile_.flush();
                    logFile_.close();
                }
            }
            catch (...)
            {
            }
        }

        static void LogSink(
            const engine::core::LogMessage& message,
            void* userData) noexcept
        {
            auto* const self =
                static_cast<Impl*>(userData);

            if (self == nullptr)
            {
                return;
            }

            try
            {
                std::string line;

                line.reserve(
                    message.category.size() +
                    message.text.size() +
                    32U);

                line += '[';
                line += GetLogLevelName(message.level);
                line += "] [";

                line.append(
                    message.category.data(),
                    message.category.size());

                line += "] ";

                line.append(
                    message.text.data(),
                    message.text.size());

                line += '\n';

                OutputDebugStringA(line.c_str());

                if (self->logFile_.is_open())
                {
                    self->logFile_.write(
                        line.data(),
                        static_cast<std::streamsize>(
                            line.size()));

                    self->logFile_.flush();
                }
            }
            catch (...)
            {
                OutputDebugStringA(
                    "[Application] Logging failure.\n");
            }
        }

        [[nodiscard]]
        bool CreateNativeWindow() noexcept
        {
            instance_ =
                GetModuleHandleW(nullptr);

            if (instance_ == nullptr)
            {
                return false;
            }

            className_ =
                L"LTS.Application.Window." +
                std::to_wstring(
                    GetCurrentProcessId());

            WNDCLASSEXW windowClass{};

            windowClass.cbSize =
                sizeof(WNDCLASSEXW);

            windowClass.style =
                CS_HREDRAW |
                CS_VREDRAW |
                CS_OWNDC;

            windowClass.lpfnWndProc =
                &Impl::WindowProcedure;

            windowClass.hInstance =
                instance_;

            windowClass.hCursor =
                LoadCursorW(
                    nullptr,
                    IDC_ARROW);

            windowClass.hbrBackground =
                nullptr;

            windowClass.lpszClassName =
                className_.c_str();

            classAtom_ =
                RegisterClassExW(
                    &windowClass);

            if (classAtom_ == 0)
            {
                return false;
            }

            const DWORD windowStyle =
                BuildWindowStyle(
                    owner_.description_);

            RECT windowRectangle
            {
                0,
                0,
                static_cast<LONG>(
                    owner_.description_.width),
                static_cast<LONG>(
                    owner_.description_.height)
            };

            CalculateWindowRectangle(
                windowRectangle,
                windowStyle);

            const int windowWidth =
                windowRectangle.right -
                windowRectangle.left;

            const int windowHeight =
                windowRectangle.bottom -
                windowRectangle.top;

            windowHandle_ =
                CreateWindowExW(
                    0,
                    className_.c_str(),
                    owner_.description_.title.c_str(),
                    windowStyle,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    windowWidth,
                    windowHeight,
                    nullptr,
                    nullptr,
                    instance_,
                    this);

            return windowHandle_ != nullptr;
        }

        void ShowNativeWindow() noexcept
        {
            if (windowHandle_ == nullptr)
            {
                return;
            }

            const int showCommand =
                owner_.description_.startMaximized
                    ? SW_SHOWMAXIMIZED
                    : SW_SHOW;

            ShowWindow(
                windowHandle_,
                showCommand);

            UpdateWindow(
                windowHandle_);
        }

        void DestroyNativeWindow() noexcept
        {
            if (windowHandle_ != nullptr)
            {
                HWND nativeWindow =
                    windowHandle_;

                windowHandle_ = nullptr;

                DestroyWindow(
                    nativeWindow);
            }

            if (
                classAtom_ != 0 &&
                instance_ != nullptr &&
                !className_.empty()
            )
            {
                UnregisterClassW(
                    className_.c_str(),
                    instance_);

                classAtom_ = 0;
            }

            instance_ = nullptr;
            className_.clear();
        }

        [[nodiscard]]
        std::uintptr_t
            GetNativeWindowValue() const noexcept
        {
            return reinterpret_cast<
                std::uintptr_t>(
                    windowHandle_);
        }

        void SetEventDispatchEnabled(
            const bool enabled) noexcept
        {
            dispatchEvents_ = enabled;
        }

        void WakeMessageLoop() noexcept
        {
            if (windowHandle_ != nullptr)
            {
                PostMessageW(
                    windowHandle_,
                    WM_NULL,
                    0,
                    0);
            }
        }

        static LRESULT CALLBACK WindowProcedure(
            HWND window,
            UINT message,
            WPARAM wParam,
            LPARAM lParam) noexcept
        {
            Impl* self =
                reinterpret_cast<Impl*>(
                    GetWindowLongPtrW(
                        window,
                        GWLP_USERDATA));

            if (message == WM_NCCREATE)
            {
                const auto* const createInfo =
                    reinterpret_cast<
                        const CREATESTRUCTW*>(
                            lParam);

                self =
                    static_cast<Impl*>(
                        createInfo->lpCreateParams);

                if (self != nullptr)
                {
                    self->windowHandle_ =
                        window;

                    SetWindowLongPtrW(
                        window,
                        GWLP_USERDATA,
                        reinterpret_cast<LONG_PTR>(
                            self));
                }
            }

            if (self == nullptr)
            {
                return DefWindowProcW(
                    window,
                    message,
                    wParam,
                    lParam);
            }

            static_cast<void>(
                self->owner_.inputSystem_.HandleNativeMessage(
                    message,
                    static_cast<std::uintptr_t>(wParam),
                    static_cast<std::intptr_t>(lParam)));

            switch (message)
            {
                case WM_CLOSE:
                {
                    if (self->dispatchEvents_)
                    {
                        ApplicationEvent event;

                        event.type =
                            ApplicationEventType::
                                CloseRequested;

                        self->owner_.DispatchEvent(
                            event);
                    }

                    self->owner_.RequestExit();
                    return 0;
                }

                case WM_DESTROY:
                {
                    self->windowHandle_ = nullptr;

                    PostQuitMessage(0);
                    return 0;
                }

                case WM_NCDESTROY:
                {
                    SetWindowLongPtrW(
                        window,
                        GWLP_USERDATA,
                        0);

                    return DefWindowProcW(
                        window,
                        message,
                        wParam,
                        lParam);
                }

                case WM_SIZE:
                {
                    const bool wasMinimized =
                        self->owner_.minimized_;

                    if (wParam == SIZE_MINIMIZED)
                    {
                        self->owner_.minimized_ = true;

                        if (self->dispatchEvents_)
                        {
                            ApplicationEvent event;

                            event.type =
                                ApplicationEventType::
                                    Minimized;

                            self->owner_.DispatchEvent(
                                event);
                        }

                        return 0;
                    }

                    self->owner_.minimized_ = false;

                    if (
                        wasMinimized &&
                        self->dispatchEvents_
                    )
                    {
                        ApplicationEvent restoredEvent;

                        restoredEvent.type =
                            ApplicationEventType::
                                Restored;

                        self->owner_.DispatchEvent(
                            restoredEvent);
                    }

                    const std::uint32_t width =
                        static_cast<std::uint32_t>(
                            LOWORD(lParam));

                    const std::uint32_t height =
                        static_cast<std::uint32_t>(
                            HIWORD(lParam));

                    if (
                        self->dispatchEvents_ &&
                        width > 0 &&
                        height > 0
                    )
                    {
                        ApplicationEvent resizeEvent;

                        resizeEvent.type =
                            ApplicationEventType::
                                Resize;

                        resizeEvent.width = width;
                        resizeEvent.height = height;

                        self->owner_.DispatchEvent(
                            resizeEvent);
                    }

                    return 0;
                }

                case WM_ACTIVATEAPP:
                {
                    if (self->dispatchEvents_)
                    {
                        ApplicationEvent event;

                        event.type =
                            wParam != 0
                                ? ApplicationEventType::
                                    Activated
                                : ApplicationEventType::
                                    Deactivated;

                        self->owner_.DispatchEvent(
                            event);
                    }

                    return 0;
                }

                case WM_DPICHANGED:
                {
                    const auto* const suggestedRectangle =
                        reinterpret_cast<
                            const RECT*>(lParam);

                    if (suggestedRectangle != nullptr)
                    {
                        SetWindowPos(
                            window,
                            nullptr,
                            suggestedRectangle->left,
                            suggestedRectangle->top,
                            suggestedRectangle->right -
                                suggestedRectangle->left,
                            suggestedRectangle->bottom -
                                suggestedRectangle->top,
                            SWP_NOACTIVATE |
                            SWP_NOZORDER);
                    }

                    if (self->dispatchEvents_)
                    {
                        ApplicationEvent event;

                        event.type =
                            ApplicationEventType::
                                DpiChanged;

                        event.dpi =
                            static_cast<std::uint32_t>(
                                HIWORD(wParam));

                        self->owner_.DispatchEvent(
                            event);
                    }

                    return 0;
                }

                case WM_ERASEBKGND:
                    return 1;

                default:
                    break;
            }

            return DefWindowProcW(
                window,
                message,
                wParam,
                lParam);
        }

    private:
        Application& owner_;

        HINSTANCE instance_ = nullptr;
        HWND windowHandle_ = nullptr;
        ATOM classAtom_ = 0;

        std::wstring className_;
        std::ofstream logFile_;

        bool dispatchEvents_ = false;
    };

    Application::Application(
        ApplicationDesc description)
        : description_(
            std::move(description)),
          impl_(
            std::make_unique<Impl>(*this))
    {
    }

    Application::~Application() noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->SetEventDispatchEnabled(false);
        }

        engine_.Shutdown();

        if (window_.IsValid())
        {
            const auto detachedWindow =
                window_.Detach();

            static_cast<void>(
                detachedWindow);
        }

        if (impl_ != nullptr)
        {
            impl_->DestroyNativeWindow();
        }
    }

    ApplicationResult Application::Run() noexcept
    {
        if (hasRun_)
        {
            return ApplicationResult::
                AlreadyExecuted;
        }

        hasRun_ = true;

        if (!description_.IsValid())
        {
            return ApplicationResult::
                InvalidDescription;
        }

        running_ = true;

        ApplicationResult runResult =
            ApplicationResult::Success;

        if (
            description_.enableDpiAwareness
        )
        {
            EnableProcessDpiAwareness();
        }

        impl_->OpenLogFile(
            description_.logFileName);

        engine::core::GetLogger().SetSink(
            &Impl::LogSink,
            impl_.get());

        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "Application",
            "Starting application.");

        do
        {
            if (!impl_->CreateNativeWindow())
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Critical,
                    "Application",
                    "Failed to create Win32 window.");

                runResult =
                    ApplicationResult::
                        WindowCreationFailed;

                break;
            }

            const auto nativeWindow =
                engine::platform::
                    NativeWindowHandle::FromValue(
                        impl_->
                            GetNativeWindowValue());

            window_.Attach(
                nativeWindow);

            std::unique_ptr<
                engine::runtime::RuntimeModule>
                    taskRuntimeModule(
                        new (std::nothrow)
                            engine::runtime::
                                TaskRuntimeModule());

            if (
                taskRuntimeModule == nullptr ||
                !engine_.AddModule(
                    std::move(
                        taskRuntimeModule))
            )
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Critical,
                    "Application",
                    "Failed to add task runtime module.");

                runResult =
                    ApplicationResult::
                        RuntimeInitializationFailed;

                break;
            }

            if (
                !engine_.Initialize(
                    description_.engineConfig)
            )
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Critical,
                    "Application",
                    "Runtime initialization failed.");

                runResult =
                    ApplicationResult::
                        RuntimeInitializationFailed;

                break;
            }

            clientLifecycleStarted_ = true;

            runResult =
                OnInitialize();

            if (
                runResult !=
                ApplicationResult::Success
            )
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Critical,
                    "Application",
                    "Client initialization failed.");

                break;
            }

            impl_->SetEventDispatchEnabled(
                true);

            impl_->ShowNativeWindow();

            using Clock =
                engine::platform::Clock;

            Clock::Tick previousTick =
                Clock::Now();

            while (
                !engine_.IsExitRequested()
            )
            {
                inputSystem_.BeginFrame();

                MSG message{};

                while (
                    PeekMessageW(
                        &message,
                        nullptr,
                        0,
                        0,
                        PM_REMOVE)
                )
                {
                    if (message.message == WM_QUIT)
                    {
                        engine_.RequestExit();
                        break;
                    }

                    TranslateMessage(
                        &message);

                    DispatchMessageW(
                        &message);
                }

                if (engine_.IsExitRequested())
                {
                    break;
                }

                if (minimized_)
                {
                    WaitMessage();

                    previousTick =
                        Clock::Now();

                    continue;
                }

                const Clock::Tick currentTick =
                    Clock::Now();

                double deltaSeconds =
                    Clock::ElapsedSeconds(
                        previousTick,
                        currentTick);

                previousTick = currentTick;

                deltaSeconds =
                    std::clamp(
                        deltaSeconds,
                        0.0,
                        0.25);

                if (
                    !engine_.BeginFrame(
                        deltaSeconds)
                )
                {
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Critical,
                        "Application",
                        "Runtime BeginFrame failed.");

                    runResult =
                        ApplicationResult::
                            RuntimeFrameFailed;

                    break;
                }

                OnUpdate(
                    deltaSeconds);

                OnRender();

                if (!engine_.EndFrame())
                {
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Critical,
                        "Application",
                        "Runtime EndFrame failed.");

                    runResult =
                        ApplicationResult::
                            RuntimeFrameFailed;

                    break;
                }
            }
        }
        while (false);

        impl_->SetEventDispatchEnabled(
            false);

        if (clientLifecycleStarted_)
        {
            OnShutdown();
            clientLifecycleStarted_ = false;
        }

        engine_.Shutdown();

        if (window_.IsValid())
        {
            const auto detachedWindow =
                window_.Detach();

            static_cast<void>(
                detachedWindow);
        }

        impl_->DestroyNativeWindow();

        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "Application",
            "Application stopped.");

        engine::core::GetLogger().ClearSink();

        impl_->CloseLogFile();

        running_ = false;
        minimized_ = false;

        return runResult;
    }

    void Application::RequestExit() noexcept
    {
        engine_.RequestExit();

        if (impl_ != nullptr)
        {
            impl_->WakeMessageLoop();
        }
    }

    bool Application::IsRunning() const noexcept
    {
        return running_;
    }

    bool Application::IsMinimized() const noexcept
    {
        return minimized_;
    }

    const ApplicationDesc&
        Application::GetDescription() const noexcept
    {
        return description_;
    }

    engine::platform::Window&
        Application::GetWindow() noexcept
    {
        return window_;
    }

    const engine::platform::Window&
        Application::GetWindow() const noexcept
    {
        return window_;
    }

    engine::runtime::Engine&
        Application::GetEngine() noexcept
    {
        return engine_;
    }

    const engine::runtime::Engine&
        Application::GetEngine() const noexcept
    {
        return engine_;
    }

    ApplicationContext
        Application::GetContext() noexcept
    {
        return ApplicationContext
        {
            &window_,
            &engine_
        };
    }

    engine::platform::InputSystem&
        Application::GetInputSystem() noexcept
    {
        return inputSystem_;
    }

    const engine::platform::InputSystem&
        Application::GetInputSystem() const noexcept
    {
        return inputSystem_;
    }

    void Application::OnEvent(
        const ApplicationEvent&) noexcept
    {
    }

    void Application::DispatchEvent(
        const ApplicationEvent& event) noexcept
    {
        OnEvent(event);
    }
}
