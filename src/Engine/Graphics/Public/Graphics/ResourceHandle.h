#pragma once

#include <cstdint>
#include <type_traits>

namespace engine::graphics
{
    template<typename Tag>
    class ResourceHandle final
    {
    public:
        using ValueType = std::uint64_t;

        constexpr ResourceHandle() noexcept = default;

        [[nodiscard]] static constexpr ResourceHandle FromParts(
            const std::uint32_t index,
            const std::uint32_t generation) noexcept
        {
            if (index == 0 || generation == 0)
            {
                return ResourceHandle{};
            }

            return ResourceHandle(
                (static_cast<ValueType>(generation) << 32U) |
                static_cast<ValueType>(index));
        }

        [[nodiscard]] static constexpr ResourceHandle FromValue(
            const ValueType value) noexcept
        {
            return ResourceHandle(value);
        }

        [[nodiscard]] constexpr ValueType Value() const noexcept
        {
            return value_;
        }

        [[nodiscard]] constexpr std::uint32_t Index() const noexcept
        {
            return static_cast<std::uint32_t>(
                value_ & 0xFFFFFFFFULL);
        }

        [[nodiscard]] constexpr std::uint32_t Generation() const noexcept
        {
            return static_cast<std::uint32_t>(
                value_ >> 32U);
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return Index() != 0 && Generation() != 0;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return IsValid();
        }

        friend constexpr bool operator==(
            const ResourceHandle left,
            const ResourceHandle right) noexcept
        {
            return left.value_ == right.value_;
        }

        friend constexpr bool operator!=(
            const ResourceHandle left,
            const ResourceHandle right) noexcept
        {
            return !(left == right);
        }

        friend constexpr bool operator<(
            const ResourceHandle left,
            const ResourceHandle right) noexcept
        {
            return left.value_ < right.value_;
        }

    private:
        explicit constexpr ResourceHandle(
            const ValueType value) noexcept
            : value_(value)
        {
        }

        ValueType value_ = 0;
    };

    struct TextureHandleTag final {};
    struct BufferHandleTag final {};
    struct ShaderHandleTag final {};
    struct InputLayoutHandleTag final {};
    struct SamplerHandleTag final {};
    struct PipelineStateHandleTag final {};
    struct RenderTargetHandleTag final {};
    struct DepthStencilHandleTag final {};
    struct SwapChainHandleTag final {};

    using TextureHandle = ResourceHandle<TextureHandleTag>;
    using BufferHandle = ResourceHandle<BufferHandleTag>;
    using ShaderHandle = ResourceHandle<ShaderHandleTag>;
    using InputLayoutHandle = ResourceHandle<InputLayoutHandleTag>;
    using SamplerHandle = ResourceHandle<SamplerHandleTag>;
    using PipelineStateHandle = ResourceHandle<PipelineStateHandleTag>;
    using RenderTargetHandle = ResourceHandle<RenderTargetHandleTag>;
    using DepthStencilHandle = ResourceHandle<DepthStencilHandleTag>;
    using SwapChainHandle = ResourceHandle<SwapChainHandleTag>;

    static_assert(std::is_trivially_copyable_v<TextureHandle>);
    static_assert(std::is_trivially_copyable_v<ShaderHandle>);
    static_assert(std::is_trivially_copyable_v<InputLayoutHandle>);
    static_assert(std::is_trivially_copyable_v<PipelineStateHandle>);

    static_assert(sizeof(TextureHandle) == sizeof(std::uint64_t));
    static_assert(sizeof(ShaderHandle) == sizeof(std::uint64_t));
    static_assert(sizeof(InputLayoutHandle) == sizeof(std::uint64_t));
    static_assert(sizeof(PipelineStateHandle) == sizeof(std::uint64_t));
}
