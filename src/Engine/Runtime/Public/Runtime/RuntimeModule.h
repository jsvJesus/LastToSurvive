#pragma once

#include "Runtime/FrameContext.h"

namespace engine::runtime
{
    class Engine;

    class RuntimeModule
    {
    public:
        virtual ~RuntimeModule() = default;

        RuntimeModule(
            const RuntimeModule&) = delete;

        RuntimeModule& operator=(
            const RuntimeModule&) = delete;

        RuntimeModule(
            RuntimeModule&&) = delete;

        RuntimeModule& operator=(
            RuntimeModule&&) = delete;

        [[nodiscard]] virtual const char*
            GetName() const noexcept = 0;

        [[nodiscard]] virtual bool Initialize(
            Engine& engine) = 0;

        virtual void Shutdown(
            Engine& engine) noexcept = 0;

        virtual void BeginFrame(
            Engine& engine,
            const FrameContext& frameContext) noexcept
        {
            (void)engine;
            (void)frameContext;
        }

        virtual void EndFrame(
            Engine& engine,
            const FrameContext& frameContext) noexcept
        {
            (void)engine;
            (void)frameContext;
        }

    protected:
        RuntimeModule() = default;
    };
}