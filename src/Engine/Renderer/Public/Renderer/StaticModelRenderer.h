#pragma once
#include "Assets/AssetLoaderRegistry.h"
#include "Assets/AssetManager.h"
#include "Assets/AssetPath.h"
#include "Assets/AssetResult.h"
#include "Assets/MeshAsset.h"
#include "Assets/ShaderAsset.h"
#include "Graphics/CommandContext.h"
#include "Graphics/GraphicsBackend.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/RenderDevice.h"
#include "Math/Matrix4.h"
#include "Math/Vector4.h"
#include "Renderer/RenderView.h"
#include <cstddef>
#include <cstdint>
#include <memory>
namespace engine::renderer {
struct StaticModelRenderHandle final {
  std::uint32_t index = 0U;
  std::uint32_t generation = 0U;

  [[nodiscard]] constexpr bool IsValid() const noexcept {
    return generation != 0U;
  }

  friend constexpr bool
  operator==(const StaticModelRenderHandle &left,
             const StaticModelRenderHandle &right) noexcept {
    return left.index == right.index && left.generation == right.generation;
  }
};
struct StaticModelInstance final {
  StaticModelRenderHandle model;
  engine::math::Matrix4 world;
  engine::math::Vector4 tint{1, 1, 1, 1};
  bool visible = true;
  std::uint32_t objectId = 0U;

  [[nodiscard]] bool IsValid() const noexcept;
};
struct StaticModelRenderStats final {
  std::size_t submittedInstances = 0U;
  std::size_t acceptedInstances = 0U;
  std::size_t rejectedInvalidInstances = 0U;
  std::size_t drawCalls = 0U;
  std::size_t triangles = 0U;
  std::size_t opaqueDraws = 0U;
  std::size_t maskDraws = 0U;
  std::size_t blendDraws = 0U;
  std::size_t pipelineChanges = 0U;
  std::size_t materialChanges = 0U;
  std::size_t meshChanges = 0U;
  std::size_t uniqueModelResources = 0U;
  std::size_t uniqueGpuTextures = 0U;
  std::size_t reusedTextureBindings = 0U;
};

struct StaticModelRendererDiagnostics final {
  std::size_t liveModelResources = 0U;
  std::size_t liveModelHandles = 0U;
  std::size_t liveTextureEntries = 0U;
  std::size_t liveTextureReferences = 0U;
  std::size_t totalTextureAcquisitions = 0U;
  std::size_t reusedTextureAcquisitions = 0U;
  engine::graphics::GraphicsResult lastCleanupResult =
      engine::graphics::GraphicsResult::Success;
  const char *lastFailedOperation = nullptr;
};
class StaticModelRenderer final {
public:
  StaticModelRenderer() noexcept;
  ~StaticModelRenderer() noexcept;
  StaticModelRenderer(const StaticModelRenderer &) = delete;
  StaticModelRenderer &operator=(const StaticModelRenderer &) = delete;

  [[nodiscard]] engine::graphics::GraphicsResult
  Initialize(engine::graphics::RenderDevice &,
             engine::graphics::CommandContext &,
             const engine::assets::ShaderAsset &,
             const engine::assets::ShaderAsset &) noexcept;
  void Shutdown() noexcept;
  [[nodiscard]] bool IsInitialized() const noexcept;
  [[nodiscard]] engine::assets::AssetResult CreateModel(
      engine::assets::AssetManager &, engine::assets::AssetLoaderRegistry &,
      const engine::assets::AssetPath &, StaticModelRenderHandle &) noexcept;
  [[nodiscard]] engine::assets::AssetResult
      DestroyModel(StaticModelRenderHandle) noexcept;
  [[nodiscard]] bool IsModelValid(StaticModelRenderHandle) const noexcept;
  [[nodiscard]] engine::assets::AssetResult
  GetModelBounds(StaticModelRenderHandle,
                 engine::assets::MeshBounds &) const noexcept;
  [[nodiscard]] engine::graphics::GraphicsResult
  Render(const RenderView &, const StaticModelInstance *, std::size_t,
         StaticModelRenderStats &) noexcept;
  void Unbind() noexcept;
  [[nodiscard]] StaticModelRendererDiagnostics GetDiagnostics() const noexcept;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace engine::renderer
