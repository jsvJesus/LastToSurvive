#include "Renderer/StaticModelRenderer.h"

#include "Assets/DdsTextureLoader.h"
#include "Assets/GpuMesh.h"
#include "Assets/GpuTexture.h"
#include "Assets/MaterialAssetLoader.h"
#include "Assets/MeshAssetLoader.h"
#include "Assets/StaticModelAssetLoader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace engine::renderer {
namespace {
using engine::assets::AssetResult;
using engine::graphics::GraphicsResult;

constexpr std::uint32_t NoFreeSlot =
    (std::numeric_limits<std::uint32_t>::max)();

bool Finite(const engine::math::Matrix4 &matrix) noexcept {
  for (const auto &row : matrix.m) {
    for (const float value : row) {
      if (!std::isfinite(value))
        return false;
    }
  }
  return true;
}

bool Finite(const engine::math::Vector4 &value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z) && std::isfinite(value.w);
}

float LinearDeterminant(const engine::math::Matrix4 &matrix) noexcept {
  return matrix.m[0][0] * (matrix.m[1][1] * matrix.m[2][2] -
                           matrix.m[1][2] * matrix.m[2][1]) -
         matrix.m[0][1] * (matrix.m[1][0] * matrix.m[2][2] -
                           matrix.m[1][2] * matrix.m[2][0]) +
         matrix.m[0][2] * (matrix.m[1][0] * matrix.m[2][1] -
                           matrix.m[1][1] * matrix.m[2][0]);
}

struct alignas(16) FrameConstants final {
  float viewProjection[16];
  float cameraTime[4];
  float lightDirectionIntensity[4];
  float lightColor[4];
  float ambientDebug[4];
};

struct alignas(16) ObjectConstants final {
  float world[16];
  float normal[16];
  float tint[4];
  float objectData[4];
};

struct alignas(16) MaterialConstants final {
  float base[4];
  float emissive[4];
  float surface[4];
  float specular[4];
};

static_assert(sizeof(FrameConstants) % 16U == 0U);
static_assert(sizeof(ObjectConstants) % 16U == 0U);
static_assert(sizeof(MaterialConstants) % 16U == 0U);
} // namespace

bool StaticModelInstance::IsValid() const noexcept {
  return model.IsValid() && Finite(world) && Finite(tint);
}

class StaticModelRenderer::Impl final {
public:
  struct TextureKey final {
    engine::assets::AssetPath path;
    engine::assets::RequestedColorSpace colorSpace =
        engine::assets::RequestedColorSpace::Preserve;

    friend bool operator==(const TextureKey &left,
                           const TextureKey &right) noexcept {
      return left.path == right.path && left.colorSpace == right.colorSpace;
    }
  };

  struct TextureKeyHash final {
    std::size_t operator()(const TextureKey &key) const noexcept {
      const std::size_t pathHash = std::hash<std::string>{}(key.path.String());
      return pathHash ^ (static_cast<std::size_t>(key.colorSpace) +
                         0x9e3779b9U + (pathHash << 6U) + (pathHash >> 2U));
    }
  };

  struct TextureEntry final {
    std::unique_ptr<engine::assets::GpuTexture> gpu;
    std::size_t references = 0U;
    std::size_t acquisitions = 0U;
    std::string debugKey;
  };

  struct Material final {
    engine::assets::MaterialAsset asset;
    std::array<engine::graphics::TextureHandle, 6U> textures{};
    std::vector<TextureKey> acquiredTextures;
    engine::graphics::SamplerHandle sampler;
    engine::graphics::BufferHandle constants;
  };

  struct Resource final {
    engine::assets::GpuMesh mesh;
    std::vector<Material> materials;
    engine::assets::MeshBounds bounds;
    engine::graphics::GraphicsBackend backend =
        engine::graphics::GraphicsBackend::None;
    std::string name;
    std::size_t references = 0U;
  };

  struct Slot final {
    Resource *resource = nullptr;
    std::uint32_t generation = 1U;
    std::uint32_t nextFree = NoFreeSlot;
    bool active = false;
  };

  struct Draw final {
    const StaticModelInstance *instance = nullptr;
    Resource *resource = nullptr;
    const engine::assets::MeshSubmesh *submesh = nullptr;
    Material *material = nullptr;
    std::size_t alpha = 0U;
    float distance = 0.0F;
    std::size_t sequence = 0U;
  };

  engine::graphics::RenderDevice *device = nullptr;
  engine::graphics::CommandContext *context = nullptr;
  engine::graphics::ShaderHandle vertexShader;
  engine::graphics::ShaderHandle pixelShader;
  engine::graphics::InputLayoutHandle inputLayout;
  std::array<engine::graphics::PipelineStateHandle, 6U> pipelines{};
  std::array<engine::graphics::TextureHandle, 6U> fallbacks{};
  engine::graphics::BufferHandle frameBuffer;
  engine::graphics::BufferHandle objectBuffer;
  std::unordered_map<TextureKey, TextureEntry, TextureKeyHash> textureCache;
  std::unordered_map<std::string, std::unique_ptr<Resource>> modelCache;
  std::vector<Slot> slots;
  std::uint32_t firstFreeSlot = NoFreeSlot;
  std::size_t liveHandles = 0U;
  std::size_t totalTextureAcquisitions = 0U;
  std::size_t reusedTextureAcquisitions = 0U;
  GraphicsResult lastCleanupResult = GraphicsResult::Success;
  const char *lastFailedOperation = nullptr;

  bool Ready() const noexcept {
    return device != nullptr && context != nullptr && vertexShader.IsValid() &&
           pixelShader.IsValid();
  }

  void RecordCleanupResult(const char *operation,
                           const GraphicsResult result) noexcept {
    if (engine::graphics::Failed(result) &&
        engine::graphics::Succeeded(lastCleanupResult)) {
      lastCleanupResult = result;
      lastFailedOperation = operation;
    }
  }

  AssetResult LoadTypedAsset(
      engine::assets::AssetManager &manager,
      engine::assets::AssetLoaderRegistry &registry,
      const engine::assets::AssetPath &path,
      const engine::assets::AssetType type,
      std::unique_ptr<engine::assets::LoadedAsset> &output) noexcept {
    output.reset();
    engine::assets::AssetHandle handle;
    AssetResult result = manager.FindByPath(path, handle);
    if (result == AssetResult::NotFound) {
      engine::assets::AssetMetadata metadata;
      metadata.path = path;
      metadata.id = path.GetId();
      metadata.type = type;
      result = manager.Register(metadata, handle);
    }
    if (engine::assets::Failed(result))
      return result;
    if (engine::assets::Failed(result = manager.Load(handle)))
      return result;

    const engine::assets::AssetData *data = nullptr;
    result = manager.GetData(handle, data);
    if (engine::assets::Failed(result))
      return result;
    if (data == nullptr)
      return AssetResult::IoError;

    engine::assets::AssetMetadata metadata;
    result = manager.GetMetadata(handle, metadata);
    if (engine::assets::Failed(result))
      return result;
    if (metadata.type != type)
      return AssetResult::TypeMismatch;

    result = registry.Load(metadata, *data, output);
    if (engine::assets::Failed(result))
      return result;
    if (!output || output->GetType() != type) {
      output.reset();
      return AssetResult::TypeMismatch;
    }
    return AssetResult::Success;
  }

  AssetResult AcquireTexture(engine::assets::AssetManager &manager,
                             engine::assets::AssetLoaderRegistry &registry,
                             const TextureKey &key,
                             engine::graphics::TextureHandle &output) {
    output = {};
    auto existing = textureCache.find(key);
    if (existing != textureCache.end()) {
      ++existing->second.references;
      ++existing->second.acquisitions;
      ++totalTextureAcquisitions;
      ++reusedTextureAcquisitions;
      output = existing->second.gpu->GetHandle();
      return AssetResult::Success;
    }

    std::unique_ptr<engine::assets::LoadedAsset> loaded;
    AssetResult result =
        LoadTypedAsset(manager, registry, key.path,
                       engine::assets::AssetType::Texture, loaded);
    if (engine::assets::Failed(result))
      return result;

    auto *typed =
        static_cast<engine::assets::TextureLoadedAsset *>(loaded.get());
    if (!typed->GetTexture().IsValid())
      return AssetResult::TypeMismatch;
    engine::assets::TextureAsset texture = typed->ReleaseTexture();

    TextureEntry entry;
    entry.gpu = std::make_unique<engine::assets::GpuTexture>();
    engine::assets::GpuTextureUploadOptions options;
    options.requestedColorSpace = key.colorSpace;
    const GraphicsResult upload = entry.gpu->Upload(*device, texture, options);
    if (engine::graphics::Failed(upload))
      return AssetResult::InternalError;
    entry.references = 1U;
    entry.acquisitions = 1U;
    entry.debugKey = key.path.String();
    output = entry.gpu->GetHandle();

    try {
      const auto inserted = textureCache.emplace(key, std::move(entry));
      if (!inserted.second) {
        RecordCleanupResult("GpuTexture::Release after duplicate",
                            entry.gpu->Release(*device));
        return AssetResult::AlreadyExists;
      }
    } catch (...) {
      RecordCleanupResult("GpuTexture::Release after cache failure",
                          entry.gpu->Release(*device));
      throw;
    }
    ++totalTextureAcquisitions;
    return AssetResult::Success;
  }

  void ReleaseTexture(const TextureKey &key) noexcept {
    const auto found = textureCache.find(key);
    if (found == textureCache.end() || found->second.references == 0U)
      return;
    --found->second.references;
    if (found->second.references != 0U)
      return;
    RecordCleanupResult("GpuTexture::Release",
                        found->second.gpu->Release(*device));
    textureCache.erase(found);
  }

  void ReleaseMaterial(Material &material) noexcept {
    if (material.constants.IsValid())
      RecordCleanupResult("DestroyBuffer(material)",
                          device->DestroyBuffer(material.constants));
    if (material.sampler.IsValid())
      RecordCleanupResult("DestroySampler",
                          device->DestroySampler(material.sampler));
    for (auto iterator = material.acquiredTextures.rbegin();
         iterator != material.acquiredTextures.rend(); ++iterator)
      ReleaseTexture(*iterator);
    material.acquiredTextures.clear();
    material.constants = {};
    material.sampler = {};
  }

  AssetResult CreateMaterial(engine::assets::AssetManager &manager,
                             engine::assets::AssetLoaderRegistry &registry,
                             const engine::assets::AssetPath &path,
                             Material &output) {
    output.textures = fallbacks;
    std::unique_ptr<engine::assets::LoadedAsset> loaded;
    AssetResult result = LoadTypedAsset(
        manager, registry, path, engine::assets::AssetType::Material, loaded);
    if (engine::assets::Failed(result))
      return result;
    auto *typed =
        static_cast<engine::assets::MaterialLoadedAsset *>(loaded.get());
    if (!typed->GetMaterial().IsValid())
      return AssetResult::TypeMismatch;
    output.asset = typed->ReleaseMaterial();

    const auto &description = output.asset.GetDesc();
    const std::array<const std::optional<engine::assets::AssetPath> *, 6U>
        paths = {&description.baseColorTexture,
                 &description.normalTexture,
                 &description.specularGlossTexture,
                 &description.roughnessTexture,
                 &description.emissiveTexture,
                 &description.specularPowerTexture};
    for (std::size_t slot = 0U; slot < paths.size(); ++slot) {
      if (!*paths[slot])
        continue;
      TextureKey key;
      key.path = **paths[slot];
      key.colorSpace = slot == 0U || slot == 4U
                           ? engine::assets::RequestedColorSpace::Srgb
                           : engine::assets::RequestedColorSpace::Linear;
      result = AcquireTexture(manager, registry, key, output.textures[slot]);
      if (engine::assets::Failed(result)) {
        ReleaseMaterial(output);
        return result;
      }
      try {
        output.acquiredTextures.push_back(key);
      } catch (...) {
        ReleaseTexture(key);
        ReleaseMaterial(output);
        throw;
      }
    }

    GraphicsResult graphicsResult =
        device->CreateSampler(description.sampler, output.sampler);
    if (engine::graphics::Failed(graphicsResult)) {
      ReleaseMaterial(output);
      return AssetResult::InternalError;
    }

    MaterialConstants constants{};
    std::memcpy(constants.base, description.baseColorFactor.data(),
                sizeof(constants.base));
    std::memcpy(constants.emissive, description.emissiveFactor.data(),
                3U * sizeof(float));
    constants.emissive[3] = description.emissiveStrength;
    constants.surface[0] = description.roughnessFactor;
    constants.surface[1] = description.alphaCutoff;
    constants.surface[2] = static_cast<float>(description.alphaMode);
    constants.surface[3] = description.normalScale;
    constants.specular[0] = description.specularIntensity;
    constants.specular[1] = description.specularPower;
    constants.specular[2] = description.reflectionFactor;

    engine::graphics::BufferDesc bufferDescription;
    bufferDescription.byteSize = sizeof(constants);
    bufferDescription.usage = engine::graphics::ResourceUsage::Dynamic;
    bufferDescription.bindFlags = engine::graphics::BufferBindFlags::Constant;
    bufferDescription.cpuAccess = engine::graphics::CpuAccessFlags::Write;
    graphicsResult =
        device->CreateBuffer(bufferDescription, nullptr, output.constants);
    if (engine::graphics::Failed(graphicsResult)) {
      ReleaseMaterial(output);
      return AssetResult::InternalError;
    }
    graphicsResult =
        context->UpdateBuffer(output.constants, &constants, sizeof(constants));
    if (engine::graphics::Failed(graphicsResult)) {
      ReleaseMaterial(output);
      return AssetResult::InternalError;
    }
    return AssetResult::Success;
  }

  void ReleaseModelResource(Resource &resource) noexcept {
    for (auto &material : resource.materials)
      ReleaseMaterial(material);
    resource.materials.clear();
    if (resource.mesh.IsValid())
      RecordCleanupResult("GpuMesh::Release", resource.mesh.Release(*device));
  }

  AssetResult BuildModelResource(engine::assets::AssetManager &manager,
                                 engine::assets::AssetLoaderRegistry &registry,
                                 const engine::assets::AssetPath &path,
                                 Resource &output) {
    std::unique_ptr<engine::assets::LoadedAsset> loaded;
    AssetResult result =
        LoadTypedAsset(manager, registry, path,
                       engine::assets::AssetType::StaticModel, loaded);
    if (engine::assets::Failed(result))
      return result;
    auto *typedModel =
        static_cast<engine::assets::StaticModelLoadedAsset *>(loaded.get());
    if (!typedModel->GetModel().IsValid())
      return AssetResult::TypeMismatch;
    engine::assets::StaticModelAsset model = typedModel->ReleaseModel();

    result = LoadTypedAsset(manager, registry, model.GetMeshPath(),
                            engine::assets::AssetType::Mesh, loaded);
    if (engine::assets::Failed(result))
      return result;
    auto *typedMesh =
        static_cast<engine::assets::MeshLoadedAsset *>(loaded.get());
    if (!typedMesh->GetMesh().IsValid())
      return AssetResult::TypeMismatch;
    engine::assets::MeshAsset mesh = typedMesh->ReleaseMesh();
    if (mesh.GetMaterialSlotCount() != model.GetMaterialCount())
      return AssetResult::TypeMismatch;

    const GraphicsResult upload = output.mesh.Upload(*device, mesh);
    if (engine::graphics::Failed(upload))
      return AssetResult::InternalError;
    output.bounds = mesh.GetBounds();
    output.backend = device->GetBackend();
    output.name = path.String();
    output.materials.reserve(model.GetMaterialCount());
    for (std::size_t index = 0U; index < model.GetMaterialCount(); ++index) {
      Material material;
      result = CreateMaterial(manager, registry, model.GetMaterialPath(index),
                              material);
      if (engine::assets::Failed(result)) {
        ReleaseModelResource(output);
        return result;
      }
      try {
        output.materials.push_back(std::move(material));
      } catch (...) {
        ReleaseMaterial(material);
        ReleaseModelResource(output);
        throw;
      }
    }
    return AssetResult::Success;
  }

  AssetResult AllocateHandle(Resource &resource,
                             StaticModelRenderHandle &output) {
    std::uint32_t index = NoFreeSlot;
    if (firstFreeSlot != NoFreeSlot) {
      index = firstFreeSlot;
      firstFreeSlot = slots[index].nextFree;
    } else {
      if (slots.size() >= NoFreeSlot)
        return AssetResult::OutOfMemory;
      index = static_cast<std::uint32_t>(slots.size());
      slots.emplace_back();
    }
    Slot &slot = slots[index];
    slot.resource = &resource;
    slot.active = true;
    slot.nextFree = NoFreeSlot;
    output = {index, slot.generation};
    ++liveHandles;
    return AssetResult::Success;
  }

  void FreeHandle(const std::uint32_t index) noexcept {
    Slot &slot = slots[index];
    slot.resource = nullptr;
    slot.active = false;
    ++slot.generation;
    if (slot.generation == 0U)
      ++slot.generation;
    slot.nextFree = firstFreeSlot;
    firstFreeSlot = index;
    --liveHandles;
  }

  GraphicsResult BindFrame(const RenderView &view) {
    FrameConstants frame{};
    std::memcpy(frame.viewProjection, view.viewProjection.m,
                sizeof(frame.viewProjection));
    frame.cameraTime[0] = view.cameraPosition.x;
    frame.cameraTime[1] = view.cameraPosition.y;
    frame.cameraTime[2] = view.cameraPosition.z;
    frame.cameraTime[3] = view.elapsedTime;
    const auto light = view.lightDirection.Normalized();
    frame.lightDirectionIntensity[0] = light.x;
    frame.lightDirectionIntensity[1] = light.y;
    frame.lightDirectionIntensity[2] = light.z;
    frame.lightDirectionIntensity[3] = view.lightIntensity;
    frame.lightColor[0] = view.lightColor.x;
    frame.lightColor[1] = view.lightColor.y;
    frame.lightColor[2] = view.lightColor.z;
    frame.ambientDebug[0] = view.ambientColor.x;
    frame.ambientDebug[1] = view.ambientColor.y;
    frame.ambientDebug[2] = view.ambientColor.z;
    frame.ambientDebug[3] = static_cast<float>(view.debugMode);
    GraphicsResult result =
        context->UpdateBuffer(frameBuffer, &frame, sizeof(frame));
    if (engine::graphics::Failed(result))
      return result;
    result = context->SetConstantBuffers(engine::graphics::ShaderStage::Vertex,
                                         0U, &frameBuffer, 1U);
    if (engine::graphics::Failed(result))
      return result;
    return context->SetConstantBuffers(engine::graphics::ShaderStage::Pixel, 0U,
                                       &frameBuffer, 1U);
  }

  GraphicsResult BindObject(const StaticModelInstance &instance) {
    engine::math::Matrix4 inverse;
    if (!instance.world.TryInverse(inverse))
      return GraphicsResult::InvalidArgument;
    ObjectConstants object{};
    std::memcpy(object.world, instance.world.m, sizeof(object.world));
    engine::math::Matrix4 normal = inverse.Transposed();
    normal.m[3][0] = 0.0F;
    normal.m[3][1] = 0.0F;
    normal.m[3][2] = 0.0F;
    std::memcpy(object.normal, normal.m, sizeof(object.normal));
    object.tint[0] = instance.tint.x;
    object.tint[1] = instance.tint.y;
    object.tint[2] = instance.tint.z;
    object.tint[3] = instance.tint.w;
    object.objectData[0] = static_cast<float>(instance.objectId);
    object.objectData[1] =
        LinearDeterminant(instance.world) < 0.0F ? -1.0F : 1.0F;
    GraphicsResult result =
        context->UpdateBuffer(objectBuffer, &object, sizeof(object));
    if (engine::graphics::Failed(result))
      return result;
    result = context->SetConstantBuffers(engine::graphics::ShaderStage::Vertex,
                                         1U, &objectBuffer, 1U);
    if (engine::graphics::Failed(result))
      return result;
    return context->SetConstantBuffers(engine::graphics::ShaderStage::Pixel, 1U,
                                       &objectBuffer, 1U);
  }

  GraphicsResult BindMaterial(const Material &material) {
    GraphicsResult result = context->SetConstantBuffers(
        engine::graphics::ShaderStage::Pixel, 2U, &material.constants, 1U);
    if (engine::graphics::Failed(result))
      return result;
    result = context->SetShaderResources(engine::graphics::ShaderStage::Pixel,
                                         0U, material.textures.data(),
                                         material.textures.size());
    if (engine::graphics::Failed(result))
      return result;
    return context->SetSamplers(engine::graphics::ShaderStage::Pixel, 0U,
                                &material.sampler, 1U);
  }

  GraphicsResult BuildDrawQueue(const RenderView &view,
                                const StaticModelInstance *instances,
                                const std::size_t count,
                                std::vector<Draw> &draws,
                                std::unordered_set<const Resource *> &unique,
                                StaticModelRenderStats &stats) {
    for (std::size_t instanceIndex = 0U; instanceIndex < count;
         ++instanceIndex) {
      const StaticModelInstance &instance = instances[instanceIndex];
      if (!instance.visible)
        continue;
      if (!instance.IsValid() || instance.model.index >= slots.size()) {
        ++stats.rejectedInvalidInstances;
        continue;
      }
      const Slot &slot = slots[instance.model.index];
      engine::math::Matrix4 inverse;
      const float determinant = LinearDeterminant(instance.world);
      if (!slot.active || slot.generation != instance.model.generation ||
          slot.resource == nullptr || !instance.world.TryInverse(inverse) ||
          std::abs(determinant) <= 0.000001F) {
        ++stats.rejectedInvalidInstances;
        continue;
      }
      Resource *resource = slot.resource;
      unique.insert(resource);
      ++stats.acceptedInstances;
      const auto center = instance.world.TransformPoint(
          {resource->bounds.sphereCenter[0], resource->bounds.sphereCenter[1],
           resource->bounds.sphereCenter[2]});
      const float x = center.x - view.cameraPosition.x;
      const float y = center.y - view.cameraPosition.y;
      const float z = center.z - view.cameraPosition.z;
      const float distance = x * x + y * y + z * z;
      for (std::size_t submeshIndex = 0U;
           submeshIndex < resource->mesh.GetSubmeshCount(); ++submeshIndex) {
        const auto *submesh = resource->mesh.GetSubmesh(submeshIndex);
        if (!submesh || submesh->indexCount == 0U ||
            submesh->materialSlot >= resource->materials.size())
          continue;
        Material *material = &resource->materials[submesh->materialSlot];
        const std::size_t alpha =
            static_cast<std::size_t>(material->asset.GetDesc().alphaMode);
        if (alpha >= 3U)
          continue;
        draws.push_back({&instance, resource, submesh, material, alpha,
                         distance, draws.size()});
      }
    }
    return GraphicsResult::Success;
  }

  GraphicsResult DrawQueue(const std::vector<Draw> &draws,
                           StaticModelRenderStats &stats) {
    const Resource *lastResource = nullptr;
    const Material *lastMaterial = nullptr;
    const StaticModelInstance *lastInstance = nullptr;
    engine::graphics::PipelineStateHandle lastPipeline;
    for (const Draw &draw : draws) {
      const auto pipeline =
          pipelines[draw.alpha * 2U +
                    (draw.material->asset.GetDesc().doubleSided ? 1U : 0U)];
      GraphicsResult result = GraphicsResult::Success;
      if (pipeline != lastPipeline) {
        result = context->SetGraphicsPipeline(pipeline);
        if (engine::graphics::Failed(result))
          return result;
        lastPipeline = pipeline;
        ++stats.pipelineChanges;
      }
      if (draw.resource != lastResource) {
        engine::graphics::VertexBufferBinding vertexBinding{
            draw.resource->mesh.GetVertexBuffer(),
            draw.resource->mesh.GetVertexStride(), 0U};
        engine::graphics::IndexBufferBinding indexBinding{
            draw.resource->mesh.GetIndexBuffer(), 0U};
        result = context->SetVertexBuffers(0U, &vertexBinding, 1U);
        if (engine::graphics::Failed(result))
          return result;
        result = context->SetIndexBuffer(indexBinding);
        if (engine::graphics::Failed(result))
          return result;
        lastResource = draw.resource;
        ++stats.meshChanges;
      }
      if (draw.instance != lastInstance) {
        result = BindObject(*draw.instance);
        if (engine::graphics::Failed(result))
          return result;
        lastInstance = draw.instance;
      }
      if (draw.material != lastMaterial) {
        result = BindMaterial(*draw.material);
        if (engine::graphics::Failed(result))
          return result;
        lastMaterial = draw.material;
        ++stats.materialChanges;
      }
      result = context->DrawIndexed(draw.submesh->indexCount,
                                    draw.submesh->firstIndex,
                                    draw.submesh->baseVertex);
      if (engine::graphics::Failed(result))
        return result;
      const std::size_t triangles = draw.submesh->indexCount / 3U;
      if (stats.triangles >
          (std::numeric_limits<std::size_t>::max)() - triangles)
        return GraphicsResult::InvalidState;
      ++stats.drawCalls;
      stats.triangles += triangles;
      if (draw.alpha == 0U)
        ++stats.opaqueDraws;
      else if (draw.alpha == 1U)
        ++stats.maskDraws;
      else
        ++stats.blendDraws;
    }
    return GraphicsResult::Success;
  }

  void DestroyRendererResources() noexcept {
    for (auto &model : modelCache)
      ReleaseModelResource(*model.second);
    modelCache.clear();
    textureCache.clear();
    slots.clear();
    firstFreeSlot = NoFreeSlot;
    liveHandles = 0U;
    if (!device)
      return;
    for (auto &fallback : fallbacks)
      if (fallback.IsValid())
        RecordCleanupResult("DestroyTexture(fallback)",
                            device->DestroyTexture(fallback));
    if (frameBuffer.IsValid())
      RecordCleanupResult("DestroyBuffer(frame)",
                          device->DestroyBuffer(frameBuffer));
    if (objectBuffer.IsValid())
      RecordCleanupResult("DestroyBuffer(object)",
                          device->DestroyBuffer(objectBuffer));
    for (auto &pipeline : pipelines)
      if (pipeline.IsValid())
        RecordCleanupResult("DestroyGraphicsPipeline",
                            device->DestroyGraphicsPipeline(pipeline));
    if (inputLayout.IsValid())
      RecordCleanupResult("DestroyInputLayout",
                          device->DestroyInputLayout(inputLayout));
    if (pixelShader.IsValid())
      RecordCleanupResult("DestroyShader(pixel)",
                          device->DestroyShader(pixelShader));
    if (vertexShader.IsValid())
      RecordCleanupResult("DestroyShader(vertex)",
                          device->DestroyShader(vertexShader));
    fallbacks = {};
    frameBuffer = {};
    objectBuffer = {};
    pipelines = {};
    inputLayout = {};
    pixelShader = {};
    vertexShader = {};
  }
};

StaticModelRenderer::StaticModelRenderer() noexcept {
  try {
    impl_ = std::make_unique<Impl>();
  } catch (...) {
    impl_.reset();
  }
}

StaticModelRenderer::~StaticModelRenderer() noexcept { Shutdown(); }

bool StaticModelRenderer::IsInitialized() const noexcept {
  return impl_ && impl_->Ready();
}

GraphicsResult StaticModelRenderer::Initialize(
    engine::graphics::RenderDevice &device,
    engine::graphics::CommandContext &context,
    const engine::assets::ShaderAsset &vertex,
    const engine::assets::ShaderAsset &pixel) noexcept {
  if (!impl_ || IsInitialized() || !device.IsReady() || !context.IsValid() ||
      !vertex.IsValid() || !pixel.IsValid() ||
      vertex.GetStage() != engine::graphics::ShaderStage::Vertex ||
      pixel.GetStage() != engine::graphics::ShaderStage::Pixel)
    return GraphicsResult::InvalidArgument;
  impl_->device = &device;
  impl_->context = &context;
  impl_->lastCleanupResult = GraphicsResult::Success;
  impl_->lastFailedOperation = nullptr;

  engine::graphics::ShaderDesc shaderDescription;
  shaderDescription.stage = vertex.GetStage();
  shaderDescription.bytecode = {vertex.GetBytecode(), vertex.GetBytecodeSize()};
  shaderDescription.debugName = vertex.GetDebugName().c_str();
  GraphicsResult result =
      device.CreateShader(shaderDescription, impl_->vertexShader);
  if (engine::graphics::Failed(result)) {
    Shutdown();
    return result;
  }
  shaderDescription.stage = pixel.GetStage();
  shaderDescription.bytecode = {pixel.GetBytecode(), pixel.GetBytecodeSize()};
  shaderDescription.debugName = pixel.GetDebugName().c_str();
  result = device.CreateShader(shaderDescription, impl_->pixelShader);
  if (engine::graphics::Failed(result)) {
    Shutdown();
    return result;
  }

  const engine::graphics::VertexElementDesc elements[] = {
      {"POSITION", 0U, engine::graphics::Format::R32G32B32Float, 0U, 0U,
       engine::graphics::VertexInputRate::PerVertex, 0U},
      {"NORMAL", 0U, engine::graphics::Format::R32G32B32Float, 0U, 12U,
       engine::graphics::VertexInputRate::PerVertex, 0U},
      {"TANGENT", 0U, engine::graphics::Format::R32G32B32A32Float, 0U, 24U,
       engine::graphics::VertexInputRate::PerVertex, 0U},
      {"TEXCOORD", 0U, engine::graphics::Format::R32G32Float, 0U, 40U,
       engine::graphics::VertexInputRate::PerVertex, 0U}};
  const engine::graphics::InputLayoutDesc inputDescription{
      impl_->vertexShader, elements, 4U, "StaticModel layout"};
  result = device.CreateInputLayout(inputDescription, impl_->inputLayout);
  if (engine::graphics::Failed(result)) {
    Shutdown();
    return result;
  }

  for (std::size_t alpha = 0U; alpha < 3U; ++alpha) {
    for (std::size_t sided = 0U; sided < 2U; ++sided) {
      engine::graphics::GraphicsPipelineDesc description;
      description.vertexShader = impl_->vertexShader;
      description.pixelShader = impl_->pixelShader;
      description.inputLayout = impl_->inputLayout;
      description.topology = engine::graphics::PrimitiveTopology::TriangleList;
      description.rasterizer.cullMode = sided
                                            ? engine::graphics::CullMode::None
                                            : engine::graphics::CullMode::Back;
      description.rasterizer.scissorEnable = true;
      description.depthStencil.depthEnable = true;
      description.depthStencil.depthWriteEnable = alpha != 2U;
      if (alpha == 2U) {
        auto &blend = description.blend.renderTargets[0];
        blend.blendEnable = true;
        blend.sourceColor = engine::graphics::BlendFactor::SourceAlpha;
        blend.destinationColor =
            engine::graphics::BlendFactor::InverseSourceAlpha;
        blend.sourceAlpha = engine::graphics::BlendFactor::One;
        blend.destinationAlpha =
            engine::graphics::BlendFactor::InverseSourceAlpha;
      }
      result = device.CreateGraphicsPipeline(
          description, impl_->pipelines[alpha * 2U + sided]);
      if (engine::graphics::Failed(result)) {
        Shutdown();
        return result;
      }
    }
  }

  constexpr std::array<std::uint32_t, 6U> pixels = {0xFFFFFFFFU, 0xFFFF8080U,
                                                    0xFF000000U, 0xFFFFFFFFU,
                                                    0xFF000000U, 0xFFFFFFFFU};
  engine::graphics::TextureDesc textureDescription;
  textureDescription.width = 1U;
  textureDescription.height = 1U;
  textureDescription.format = engine::graphics::Format::R8G8B8A8UNorm;
  textureDescription.usage = engine::graphics::ResourceUsage::Immutable;
  textureDescription.bindFlags =
      engine::graphics::TextureBindFlags::ShaderResource;
  engine::graphics::TextureSubresourceData initialData;
  initialData.dataSize = 4U;
  initialData.rowPitch = 4U;
  initialData.slicePitch = 4U;
  for (std::size_t index = 0U; index < impl_->fallbacks.size(); ++index) {
    initialData.data = reinterpret_cast<const std::byte *>(&pixels[index]);
    result = device.CreateTexture(textureDescription, &initialData, 1U,
                                  impl_->fallbacks[index]);
    if (engine::graphics::Failed(result)) {
      Shutdown();
      return result;
    }
  }

  engine::graphics::BufferDesc bufferDescription;
  bufferDescription.usage = engine::graphics::ResourceUsage::Dynamic;
  bufferDescription.bindFlags = engine::graphics::BufferBindFlags::Constant;
  bufferDescription.cpuAccess = engine::graphics::CpuAccessFlags::Write;
  bufferDescription.byteSize = sizeof(FrameConstants);
  result = device.CreateBuffer(bufferDescription, nullptr, impl_->frameBuffer);
  if (engine::graphics::Failed(result)) {
    Shutdown();
    return result;
  }
  bufferDescription.byteSize = sizeof(ObjectConstants);
  result = device.CreateBuffer(bufferDescription, nullptr, impl_->objectBuffer);
  if (engine::graphics::Failed(result)) {
    Shutdown();
    return result;
  }
  return GraphicsResult::Success;
}

AssetResult
StaticModelRenderer::CreateModel(engine::assets::AssetManager &manager,
                                 engine::assets::AssetLoaderRegistry &registry,
                                 const engine::assets::AssetPath &path,
                                 StaticModelRenderHandle &output) noexcept {
  output = {};
  if (!IsInitialized() || !path.IsValid())
    return AssetResult::InvalidArgument;
  try {
    const std::string key = path.String();
    const auto existing = impl_->modelCache.find(key);
    if (existing != impl_->modelCache.end()) {
      const AssetResult allocated =
          impl_->AllocateHandle(*existing->second, output);
      if (engine::assets::Failed(allocated))
        return allocated;
      ++existing->second->references;
      return AssetResult::Success;
    }

    auto resource = std::make_unique<Impl::Resource>();
    AssetResult result = AssetResult::InternalError;
    try {
      result = impl_->BuildModelResource(manager, registry, path, *resource);
    } catch (...) {
      impl_->ReleaseModelResource(*resource);
      throw;
    }
    if (engine::assets::Failed(result))
      return result;
    resource->references = 1U;
    decltype(impl_->modelCache)::iterator insertedIterator;
    bool insertedNew = false;
    try {
      const auto inserted = impl_->modelCache.emplace(key, std::move(resource));
      insertedIterator = inserted.first;
      insertedNew = inserted.second;
    } catch (...) {
      if (resource)
        impl_->ReleaseModelResource(*resource);
      throw;
    }
    if (!insertedNew)
      return AssetResult::AlreadyExists;
    result = impl_->AllocateHandle(*insertedIterator->second, output);
    if (engine::assets::Failed(result)) {
      impl_->ReleaseModelResource(*insertedIterator->second);
      impl_->modelCache.erase(insertedIterator);
      return result;
    }
    return AssetResult::Success;
  } catch (const std::bad_alloc &) {
    return AssetResult::OutOfMemory;
  } catch (...) {
    return AssetResult::InternalError;
  }
}

bool StaticModelRenderer::IsModelValid(
    const StaticModelRenderHandle handle) const noexcept {
  if (!IsInitialized() || !handle.IsValid() ||
      handle.index >= impl_->slots.size())
    return false;
  const Impl::Slot &slot = impl_->slots[handle.index];
  return slot.active && slot.resource != nullptr &&
         slot.generation == handle.generation;
}

AssetResult StaticModelRenderer::GetModelBounds(
    const StaticModelRenderHandle handle,
    engine::assets::MeshBounds &output) const noexcept {
  output = {};
  if (!handle.IsValid())
    return AssetResult::InvalidArgument;
  if (!IsModelValid(handle))
    return AssetResult::NotFound;
  output = impl_->slots[handle.index].resource->bounds;
  return AssetResult::Success;
}

AssetResult StaticModelRenderer::DestroyModel(
    const StaticModelRenderHandle handle) noexcept {
  if (!handle.IsValid())
    return AssetResult::InvalidArgument;
  if (!IsModelValid(handle))
    return AssetResult::NotFound;
  Impl::Resource *resource = impl_->slots[handle.index].resource;
  impl_->FreeHandle(handle.index);
  if (--resource->references != 0U)
    return AssetResult::Success;
  const std::string key = resource->name;
  impl_->ReleaseModelResource(*resource);
  impl_->modelCache.erase(key);
  return AssetResult::Success;
}

GraphicsResult StaticModelRenderer::Render(
    const RenderView &view, const StaticModelInstance *instances,
    const std::size_t count, StaticModelRenderStats &stats) noexcept {
  stats = {};
  stats.submittedInstances = count;
  if (!IsInitialized() || !view.IsValid() ||
      (count != 0U && instances == nullptr))
    return GraphicsResult::InvalidArgument;
  GraphicsResult result = impl_->BindFrame(view);
  if (engine::graphics::Failed(result))
    return result;
  try {
    std::vector<Impl::Draw> draws;
    std::unordered_set<const Impl::Resource *> uniqueResources;
    result = impl_->BuildDrawQueue(view, instances, count, draws,
                                   uniqueResources, stats);
    if (engine::graphics::Failed(result))
      return result;
    std::unordered_set<const Impl::Material *> frameMaterials;
    std::unordered_set<engine::graphics::TextureHandle::ValueType>
        frameTextures;
    for (const Impl::Draw &draw : draws) {
      if (!frameMaterials.insert(draw.material).second)
        continue;
      for (const auto texture : draw.material->textures) {
        bool fallback = false;
        for (const auto candidate : impl_->fallbacks)
          if (texture == candidate) {
            fallback = true;
            break;
          }
        if (fallback)
          continue;
        if (!frameTextures.insert(texture.Value()).second)
          ++stats.reusedTextureBindings;
      }
    }
    std::stable_sort(draws.begin(), draws.end(),
                     [](const Impl::Draw &left, const Impl::Draw &right) {
                       if (left.alpha != right.alpha)
                         return left.alpha < right.alpha;
                       if (left.alpha == 2U && left.distance != right.distance)
                         return left.distance > right.distance;
                       if (left.resource != right.resource)
                         return left.resource->name < right.resource->name;
                       if (left.material != right.material)
                         return left.material < right.material;
                       if (left.instance->objectId != right.instance->objectId)
                         return left.instance->objectId <
                                right.instance->objectId;
                       return left.sequence < right.sequence;
                     });
    result = impl_->DrawQueue(draws, stats);
    if (engine::graphics::Failed(result))
      return result;
    stats.uniqueModelResources = uniqueResources.size();
    stats.uniqueGpuTextures = frameTextures.size();
    return GraphicsResult::Success;
  } catch (const std::bad_alloc &) {
    return GraphicsResult::OutOfMemory;
  } catch (...) {
    return GraphicsResult::BackendFailure;
  }
}

void StaticModelRenderer::Unbind() noexcept {
  if (!impl_ || !impl_->context)
    return;
  impl_->RecordCleanupResult(
      "UnbindConstantBuffers(VS)",
      impl_->context->UnbindConstantBuffers(
          engine::graphics::ShaderStage::Vertex, 0U, 3U));
  impl_->RecordCleanupResult("UnbindConstantBuffers(PS)",
                             impl_->context->UnbindConstantBuffers(
                                 engine::graphics::ShaderStage::Pixel, 0U, 3U));
  impl_->RecordCleanupResult("UnbindShaderResources",
                             impl_->context->UnbindShaderResources(
                                 engine::graphics::ShaderStage::Pixel, 0U, 6U));
  impl_->RecordCleanupResult("UnbindSamplers",
                             impl_->context->UnbindSamplers(
                                 engine::graphics::ShaderStage::Pixel, 0U, 1U));
  impl_->context->UnbindGraphicsPipeline();
}

StaticModelRendererDiagnostics
StaticModelRenderer::GetDiagnostics() const noexcept {
  StaticModelRendererDiagnostics diagnostics;
  if (!impl_)
    return diagnostics;
  diagnostics.liveModelResources = impl_->modelCache.size();
  diagnostics.liveModelHandles = impl_->liveHandles;
  diagnostics.liveTextureEntries = impl_->textureCache.size();
  for (const auto &texture : impl_->textureCache)
    diagnostics.liveTextureReferences += texture.second.references;
  diagnostics.totalTextureAcquisitions = impl_->totalTextureAcquisitions;
  diagnostics.reusedTextureAcquisitions = impl_->reusedTextureAcquisitions;
  diagnostics.lastCleanupResult = impl_->lastCleanupResult;
  diagnostics.lastFailedOperation = impl_->lastFailedOperation;
  return diagnostics;
}

void StaticModelRenderer::Shutdown() noexcept {
  if (!impl_)
    return;
  Unbind();
  impl_->DestroyRendererResources();
  impl_->device = nullptr;
  impl_->context = nullptr;
  impl_->totalTextureAcquisitions = 0U;
  impl_->reusedTextureAcquisitions = 0U;
}
} // namespace engine::renderer
