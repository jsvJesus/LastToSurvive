#include "Assets/AssetLoaderRegistry.h"
#include "Assets/AssetManager.h"
#include "Assets/AssetSource.h"
#include "Assets/DdsTextureLoader.h"
#include "Assets/LtsMaterialWriter.h"
#include "Assets/LtsMeshWriter.h"
#include "Assets/LtsStaticModelWriter.h"
#include "Assets/MaterialAssetLoader.h"
#include "Assets/MeshAssetBuilder.h"
#include "Assets/MeshAssetLoader.h"
#include "Assets/StaticModelAssetLoader.h"
#include "Graphics/RenderDevice.h"
#include "GraphicsDX11/D3D11Device.h"
#include "Renderer/RenderView.h"
#include "Renderer/StaticModelRenderer.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <d3dcompiler.h>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
namespace assets = engine::assets;
namespace graphics = engine::graphics;
namespace renderer = engine::renderer;

bool Check(const bool value, const char *message) {
  if (value)
    return true;
  std::fprintf(stderr, "FAILED: %s\n", message);
  return false;
}

template <class Handle> Handle MakeHandle(std::uint32_t &next) noexcept {
  return Handle::FromParts(next++, 1U);
}

class FakeContext final : public graphics::CommandContext {
public:
  bool valid = true;
  std::size_t operation = 0U;
  std::size_t failAt = 0U;
  std::size_t draws = 0U;
  std::vector<std::string> operations;
  std::vector<std::byte> lastObjectConstants;

  [[nodiscard]] bool IsValid() const noexcept override { return valid; }
  void FailOn(const std::size_t value) noexcept {
    operation = 0U;
    failAt = value;
    draws = 0U;
    operations.clear();
  }
  graphics::GraphicsResult Next(const char *name) noexcept {
    try {
      operations.emplace_back(name);
    } catch (...) {
      return graphics::GraphicsResult::OutOfMemory;
    }
    return failAt != 0U && ++operation == failAt
               ? graphics::GraphicsResult::BackendFailure
               : graphics::GraphicsResult::Success;
  }
  [[nodiscard]] graphics::GraphicsResult
  SetViewport(const graphics::Viewport &) noexcept override {
    return Next("SetViewport");
  }
  [[nodiscard]] graphics::GraphicsResult
  SetScissorRect(const graphics::ScissorRect &) noexcept override {
    return Next("SetScissorRect");
  }
  [[nodiscard]] graphics::GraphicsResult
  SetSwapChainRenderTarget(graphics::SwapChain &) noexcept override {
    return Next("SetSwapChainRenderTarget");
  }
  [[nodiscard]] graphics::GraphicsResult
  SetSwapChainRenderTarget(graphics::SwapChain &,
                           graphics::TextureHandle) noexcept override {
    return Next("SetSwapChainRenderTargetDepth");
  }
  [[nodiscard]] graphics::GraphicsResult
  SetRenderTargets(const graphics::TextureHandle *, std::size_t,
                   graphics::TextureHandle) noexcept override {
    return Next("SetRenderTargets");
  }
  void UnbindRenderTargets() noexcept override {}
  [[nodiscard]] graphics::GraphicsResult
  ClearSwapChainColor(graphics::SwapChain &,
                      const graphics::ClearColor &) noexcept override {
    return Next("ClearSwapChainColor");
  }
  [[nodiscard]] graphics::GraphicsResult
  ClearColorTarget(graphics::TextureHandle,
                   const graphics::ClearColor &) noexcept override {
    return Next("ClearColorTarget");
  }
  [[nodiscard]] graphics::GraphicsResult
  ClearDepthStencilTarget(graphics::TextureHandle,
                          graphics::ClearDepthStencilFlags, float,
                          std::uint8_t) noexcept override {
    return Next("ClearDepthStencilTarget");
  }
  [[nodiscard]] graphics::GraphicsResult
  SetGraphicsPipeline(graphics::PipelineStateHandle) noexcept override {
    return Next("SetGraphicsPipeline");
  }
  void UnbindGraphicsPipeline() noexcept override {}
  [[nodiscard]] graphics::GraphicsResult
  SetVertexBuffers(std::uint32_t, const graphics::VertexBufferBinding *,
                   std::size_t) noexcept override {
    return Next("SetVertexBuffers");
  }
  [[nodiscard]] graphics::GraphicsResult
  SetIndexBuffer(const graphics::IndexBufferBinding &) noexcept override {
    return Next("SetIndexBuffer");
  }
  void UnbindIndexBuffer() noexcept override {}
  [[nodiscard]] graphics::GraphicsResult
  SetShaderResources(graphics::ShaderStage, std::uint32_t,
                     const graphics::TextureHandle *,
                     std::size_t) noexcept override {
    return Next("SetShaderResources");
  }
  [[nodiscard]] graphics::GraphicsResult
  UnbindShaderResources(graphics::ShaderStage, std::uint32_t,
                        std::size_t) noexcept override {
    return Next("UnbindShaderResources");
  }
  [[nodiscard]] graphics::GraphicsResult
  SetSamplers(graphics::ShaderStage, std::uint32_t,
              const graphics::SamplerHandle *, std::size_t) noexcept override {
    return Next("SetSamplers");
  }
  [[nodiscard]] graphics::GraphicsResult
  UnbindSamplers(graphics::ShaderStage, std::uint32_t,
                 std::size_t) noexcept override {
    return Next("UnbindSamplers");
  }
  [[nodiscard]] graphics::GraphicsResult
  SetConstantBuffers(graphics::ShaderStage, std::uint32_t,
                     const graphics::BufferHandle *,
                     std::size_t) noexcept override {
    return Next("SetConstantBuffers");
  }
  [[nodiscard]] graphics::GraphicsResult
  UnbindConstantBuffers(graphics::ShaderStage, std::uint32_t,
                        std::size_t) noexcept override {
    return Next("UnbindConstantBuffers");
  }
  [[nodiscard]] graphics::GraphicsResult
  UpdateBuffer(graphics::BufferHandle, const void *data,
               std::size_t size) noexcept override {
    if (size == 160U && data) {
      try {
        const auto *first = static_cast<const std::byte *>(data);
        lastObjectConstants.assign(first, first + size);
      } catch (...) {
        return graphics::GraphicsResult::OutOfMemory;
      }
    }
    return Next("UpdateBuffer");
  }
  [[nodiscard]] graphics::GraphicsResult Draw(std::uint32_t,
                                              std::uint32_t) noexcept override {
    ++draws;
    return Next("Draw");
  }
  [[nodiscard]] graphics::GraphicsResult
  DrawIndexed(std::uint32_t, std::uint32_t, std::int32_t) noexcept override {
    const auto result = Next("DrawIndexed");
    if (graphics::Succeeded(result))
      ++draws;
    return result;
  }
  void ClearState() noexcept override {}
  void Flush() noexcept override {}
};

class FakeDevice final : public graphics::RenderDevice {
public:
  FakeContext context;
  graphics::DeviceState state = graphics::DeviceState::Uninitialized;
  std::size_t operation = 0U;
  std::size_t failAt = 0U;
  std::uint32_t next = 1U;
  std::unordered_set<std::uint64_t> textures, buffers, samplers, shaders,
      layouts, pipelines;

  void FailOn(const std::size_t value) noexcept {
    operation = 0U;
    failAt = value;
  }
  graphics::GraphicsResult Next() noexcept {
    return failAt != 0U && ++operation == failAt
               ? graphics::GraphicsResult::OutOfMemory
               : graphics::GraphicsResult::Success;
  }
  template <class Handle>
  graphics::GraphicsResult Create(std::unordered_set<std::uint64_t> &set,
                                  Handle &output) noexcept {
    output = {};
    const auto result = Next();
    if (graphics::Failed(result))
      return result;
    output = MakeHandle<Handle>(next);
    try {
      set.insert(output.Value());
    } catch (...) {
      output = {};
      return graphics::GraphicsResult::OutOfMemory;
    }
    return graphics::GraphicsResult::Success;
  }
  template <class Handle>
  graphics::GraphicsResult Destroy(std::unordered_set<std::uint64_t> &set,
                                   const Handle value) noexcept {
    return set.erase(value.Value()) != 0U ? graphics::GraphicsResult::Success
                                          : graphics::GraphicsResult::NotFound;
  }
  [[nodiscard]] graphics::GraphicsBackend GetBackend() const noexcept override {
    return graphics::GraphicsBackend::D3D11;
  }
  [[nodiscard]] graphics::DeviceState GetState() const noexcept override {
    return state;
  }
  [[nodiscard]] graphics::GraphicsResult
  Initialize(const graphics::RenderDeviceDesc &) noexcept override {
    state = graphics::DeviceState::Ready;
    return graphics::GraphicsResult::Success;
  }
  void Shutdown() noexcept override { state = graphics::DeviceState::Stopped; }
  [[nodiscard]] graphics::CommandContext *
  GetImmediateCommandContext() noexcept override {
    return &context;
  }
  [[nodiscard]] const graphics::CommandContext *
  GetImmediateCommandContext() const noexcept override {
    return &context;
  }
  [[nodiscard]] graphics::GraphicsResult CreateSwapChain(
      const graphics::SwapChainDesc &,
      std::unique_ptr<graphics::SwapChain> &output) noexcept override {
    output.reset();
    return graphics::GraphicsResult::Unsupported;
  }
  [[nodiscard]] graphics::GraphicsResult
  CreateTexture(const graphics::TextureDesc &,
                const graphics::TextureSubresourceData *, std::size_t,
                graphics::TextureHandle &output) noexcept override {
    return Create(textures, output);
  }
  [[nodiscard]] graphics::GraphicsResult
  DestroyTexture(graphics::TextureHandle value) noexcept override {
    return Destroy(textures, value);
  }
  [[nodiscard]] graphics::GraphicsResult
  CreateBuffer(const graphics::BufferDesc &,
               const graphics::BufferInitialData *,
               graphics::BufferHandle &output) noexcept override {
    return Create(buffers, output);
  }
  [[nodiscard]] graphics::GraphicsResult
  DestroyBuffer(graphics::BufferHandle value) noexcept override {
    return Destroy(buffers, value);
  }
  [[nodiscard]] graphics::GraphicsResult
  CreateSampler(const graphics::SamplerDesc &,
                graphics::SamplerHandle &output) noexcept override {
    return Create(samplers, output);
  }
  [[nodiscard]] graphics::GraphicsResult
  DestroySampler(graphics::SamplerHandle value) noexcept override {
    return Destroy(samplers, value);
  }
  [[nodiscard]] graphics::GraphicsResult
  CreateShader(const graphics::ShaderDesc &,
               graphics::ShaderHandle &output) noexcept override {
    return Create(shaders, output);
  }
  [[nodiscard]] graphics::GraphicsResult
  DestroyShader(graphics::ShaderHandle value) noexcept override {
    return Destroy(shaders, value);
  }
  [[nodiscard]] graphics::GraphicsResult
  CreateInputLayout(const graphics::InputLayoutDesc &,
                    graphics::InputLayoutHandle &output) noexcept override {
    return Create(layouts, output);
  }
  [[nodiscard]] graphics::GraphicsResult
  DestroyInputLayout(graphics::InputLayoutHandle value) noexcept override {
    return Destroy(layouts, value);
  }
  [[nodiscard]] graphics::GraphicsResult CreateGraphicsPipeline(
      const graphics::GraphicsPipelineDesc &,
      graphics::PipelineStateHandle &output) noexcept override {
    return Create(pipelines, output);
  }
  [[nodiscard]] graphics::GraphicsResult DestroyGraphicsPipeline(
      graphics::PipelineStateHandle value) noexcept override {
    return Destroy(pipelines, value);
  }
  [[nodiscard]] std::size_t Live() const noexcept {
    return textures.size() + buffers.size() + samplers.size() + shaders.size() +
           layouts.size() + pipelines.size();
  }
};

class MemorySource final : public assets::AssetSource {
public:
  std::unordered_map<std::string, std::vector<std::byte>> files;
  void Add(const assets::AssetPath &path, const assets::AssetData &data) {
    files[path.String()] = {data.GetData(), data.GetData() + data.GetSize()};
  }
  [[nodiscard]] assets::AssetResult
  Read(const assets::AssetPath &path,
       assets::AssetData &output) noexcept override {
    output.Clear();
    const auto found = files.find(path.String());
    if (found == files.end())
      return assets::AssetResult::NotFound;
    const auto result = output.Resize(found->second.size());
    if (assets::Failed(result))
      return result;
    if (!found->second.empty())
      std::memcpy(output.GetData(), found->second.data(), found->second.size());
    return assets::AssetResult::Success;
  }
  [[nodiscard]] bool
  Exists(const assets::AssetPath &path) const noexcept override {
    return files.find(path.String()) != files.end();
  }
};

void WriteU32(std::byte *bytes, const std::size_t offset,
              const std::uint32_t value) noexcept {
  std::memcpy(bytes + offset, &value, sizeof(value));
}
assets::AssetResult BuildDds(assets::AssetData &output) noexcept {
  const auto result = output.Resize(132U);
  if (assets::Failed(result))
    return result;
  auto *bytes = output.GetData();
  std::memset(bytes, 0, 132U);
  WriteU32(bytes, 0U, 0x20534444U);
  WriteU32(bytes, 4U, 124U);
  WriteU32(bytes, 12U, 1U);
  WriteU32(bytes, 16U, 1U);
  WriteU32(bytes, 20U, 4U);
  WriteU32(bytes, 28U, 1U);
  WriteU32(bytes, 76U, 32U);
  WriteU32(bytes, 80U, 0x41U);
  WriteU32(bytes, 88U, 32U);
  WriteU32(bytes, 92U, 0xFFU);
  WriteU32(bytes, 96U, 0xFF00U);
  WriteU32(bytes, 100U, 0xFF0000U);
  WriteU32(bytes, 104U, 0xFF000000U);
  WriteU32(bytes, 108U, 0x1000U);
  bytes[131U] = std::byte{0xFF};
  return assets::AssetResult::Success;
}

assets::AssetPath Path(const char *text) {
  assets::AssetPath path;
  (void)assets::AssetPath::TryCreate(text, path);
  return path;
}

bool BuildFixture(MemorySource &source, const bool useTexture) {
  assets::StaticMeshVertex vertices[3]{};
  vertices[0].position = {-1, 0, 0};
  vertices[1].position = {1, 0, 0};
  vertices[2].position = {0, 1, 0};
  for (auto &vertex : vertices) {
    vertex.normal = {0, 0, 1};
    vertex.tangent = {1, 0, 0, 1};
  }
  const std::uint16_t indices[] = {0, 1, 2};
  const assets::MeshSubmesh submesh{0, 3, 0, 0};
  assets::MeshAsset mesh;
  if (assets::Failed(assets::MeshAssetBuilder::Build(
          vertices, 3, indices, 3, &submesh, 1, 1, "mesh", mesh)))
    return false;
  assets::AssetData data;
  if (assets::Failed(assets::LtsMeshWriter::Encode(mesh, data)))
    return false;
  source.Add(Path("mesh.mesh"), data);
  assets::MaterialAssetDesc materialDescription;
  materialDescription.debugName = "material";
  if (useTexture) {
    const auto texture = Path("shared.dds");
    materialDescription.baseColorTexture = texture;
    materialDescription.emissiveTexture = texture;
    materialDescription.specularGlossTexture = texture;
  }
  assets::MaterialAsset material;
  if (assets::Failed(material.Initialize(std::move(materialDescription))) ||
      assets::Failed(assets::LtsMaterialWriter::Encode(material, data)))
    return false;
  source.Add(Path("material.ltsmat"), data);
  assets::StaticModelAsset model;
  if (assets::Failed(model.Initialize(Path("mesh.mesh"),
                                      {Path("material.ltsmat")}, "model")) ||
      assets::Failed(assets::LtsStaticModelWriter::Encode(model, data)))
    return false;
  source.Add(Path("model.ltsmodel"), data);
  if (useTexture) {
    if (assets::Failed(BuildDds(data)))
      return false;
    source.Add(Path("shared.dds"), data);
  }
  return true;
}

assets::ShaderAsset Shader(const graphics::ShaderStage stage) {
  assets::ShaderAssetDesc description;
  description.stage = stage;
  description.targetProfile =
      stage == graphics::ShaderStage::Vertex ? "vs_4_0" : "ps_4_0";
  description.entryPoint = "main";
  description.sourceHash = 1U;
  description.bytecode = {std::byte{'D'}, std::byte{'X'}, std::byte{'B'},
                          std::byte{'C'}};
  assets::ShaderAsset shader;
  (void)shader.Initialize(std::move(description));
  return shader;
}

assets::ShaderAsset CompileShader(const graphics::ShaderStage stage) {
  static constexpr const char *Source =
      "cbuffer F:register(b0){float4x4 vp;float4 a,b,c,d;}"
      "cbuffer O:register(b1){float4x4 world;float4x4 normal;float4 "
      "tint;float4 objectData;}"
      "struct I{float3 p:POSITION;float3 n:NORMAL;float4 t:TANGENT;float2 "
      "u:TEXCOORD0;};"
      "struct V{float4 p:SV_Position;};"
      "V VSMain(I i){V o;o.p=mul(mul(float4(i.p,1),world),vp);return o;}"
      "float4 PSMain(V i):SV_Target{return float4(1,1,1,1);}";
  ID3DBlob *code = nullptr;
  ID3DBlob *errors = nullptr;
  const char *entry =
      stage == graphics::ShaderStage::Vertex ? "VSMain" : "PSMain";
  const char *profile =
      stage == graphics::ShaderStage::Vertex ? "vs_4_0" : "ps_4_0";
  const HRESULT result = D3DCompile(
      Source, std::strlen(Source), nullptr, nullptr, nullptr, entry, profile,
      D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0U, &code,
      &errors);
  if (errors)
    errors->Release();
  assets::ShaderAsset shader;
  if (FAILED(result))
    return shader;
  assets::ShaderAssetDesc description;
  description.stage = stage;
  description.targetProfile = profile;
  description.entryPoint = entry;
  description.sourceHash = 2U;
  const auto *begin = static_cast<const std::byte *>(code->GetBufferPointer());
  description.bytecode.assign(begin, begin + code->GetBufferSize());
  code->Release();
  (void)shader.Initialize(std::move(description));
  return shader;
}

renderer::RenderView View() {
  renderer::RenderView view;
  view.viewport.width = 1280;
  view.viewport.height = 720;
  return view;
}
} // namespace

int main() {
  auto view = View();
  if (!Check(view.IsValid(), "valid RenderView"))
    return 1;
  view.lightIntensity = -1;
  if (!Check(!view.IsValid(), "negative light rejected"))
    return 1;
  view = View();
  view.view.m[0][0] = (std::numeric_limits<float>::infinity)();
  if (!Check(!view.IsValid(), "non-finite matrix rejected"))
    return 1;
  const auto vertex = Shader(graphics::ShaderStage::Vertex),
             pixel = Shader(graphics::ShaderStage::Pixel);
  for (std::size_t failure = 1U; failure <= 17U; ++failure) {
    FakeDevice device;
    graphics::RenderDeviceDesc description;
    description.backend = graphics::GraphicsBackend::D3D11;
    (void)device.Initialize(description);
    device.FailOn(failure);
    renderer::StaticModelRenderer tested;
    const auto result =
        tested.Initialize(device, device.context, vertex, pixel);
    if (graphics::Succeeded(result))
      tested.Shutdown();
    if (!Check(device.Live() == 0U,
               "initialization rollback releases every fake resource"))
      return 1;
  }

  FakeDevice device;
  graphics::RenderDeviceDesc description;
  description.backend = graphics::GraphicsBackend::D3D11;
  (void)device.Initialize(description);
  renderer::StaticModelRenderer tested;
  if (!Check(graphics::Succeeded(
                 tested.Initialize(device, device.context, vertex, pixel)),
             "renderer initialization"))
    return 1;
  const std::size_t fallbackResources = device.Live();
  MemorySource source;
  if (!Check(BuildFixture(source, true), "build in-memory model fixture"))
    return 1;
  assets::AssetManager manager;
  if (!Check(assets::Succeeded(manager.Initialize(source)), "asset manager"))
    return 1;
  assets::AssetLoaderRegistry registry;
  assets::StaticModelAssetLoader modelLoader;
  assets::MeshAssetLoader meshLoader;
  assets::MaterialAssetLoader materialLoader;
  assets::DdsTextureLoader textureLoader;
  (void)registry.Register(modelLoader);
  (void)registry.Register(meshLoader);
  (void)registry.Register(materialLoader);
  (void)registry.Register(textureLoader);
  renderer::StaticModelRenderHandle first, second;
  if (!Check(assets::Succeeded(tested.CreateModel(
                 manager, registry, Path("MODEL.LTSMODEL"), first)) &&
                 assets::Succeeded(tested.CreateModel(
                     manager, registry, Path("model.ltsmodel"), second)),
             "duplicate normalized model acquisition"))
    return 1;
  auto diagnostics = tested.GetDiagnostics();
  if (!Check(diagnostics.liveModelResources == 1U &&
                 diagnostics.liveModelHandles == 2U &&
                 diagnostics.liveTextureEntries == 2U &&
                 diagnostics.liveTextureReferences == 3U,
             "shared model and semantic texture cache diagnostics"))
    return 1;
  renderer::StaticModelInstance instances[2];
  instances[0].model = first;
  instances[0].objectId = 2;
  instances[1].model = second;
  instances[1].objectId = 1;
  instances[1].world = engine::math::Matrix4::CreateScale({1.0F, 2.0F, 0.5F});
  renderer::StaticModelRenderStats stats;
  device.context.FailOn(0U);
  if (!Check(graphics::Succeeded(tested.Render(View(), instances, 2U, stats)) &&
                 stats.drawCalls == 2U && stats.acceptedInstances == 2U,
             "multiple handles render one shared model"))
    return 1;
  const auto objectFloat = [&](const std::size_t offset) {
    float value = 0.0F;
    if (device.context.lastObjectConstants.size() >= offset + sizeof(value))
      std::memcpy(&value, device.context.lastObjectConstants.data() + offset,
                  sizeof(value));
    return value;
  };
  instances[0].visible = false;
  device.context.FailOn(0U);
  if (graphics::Failed(tested.Render(View(), instances, 2U, stats)))
    return 1;
  if (!Check(std::abs(objectFloat(64U) - 1.0F) < 0.0001F &&
                 std::abs(objectFloat(84U) - 0.5F) < 0.0001F &&
                 std::abs(objectFloat(104U) - 2.0F) < 0.0001F,
             "inverse-transpose normal matrix for non-uniform scale"))
    return 1;
  instances[0].visible = true;
  instances[0].world = engine::math::Matrix4::CreateScale({-1.0F, 1.0F, 1.0F});
  instances[1].visible = false;
  device.context.FailOn(0U);
  if (!Check(graphics::Succeeded(tested.Render(View(), instances, 2U, stats)) &&
                 objectFloat(148U) == -1.0F,
             "mirrored transform carries tangent handedness"))
    return 1;
  instances[0].world = engine::math::Matrix4::CreateScale({0.0F, 1.0F, 1.0F});
  device.context.FailOn(0U);
  if (!Check(graphics::Succeeded(tested.Render(View(), instances, 2U, stats)) &&
                 stats.rejectedInvalidInstances == 1U && stats.drawCalls == 0U,
             "singular transform rejected"))
    return 1;
  instances[0].world = engine::math::Matrix4::Identity();
  instances[1].visible = true;
  for (std::size_t failure = 1U; failure < 15U; ++failure) {
    device.context.FailOn(failure);
    stats = {};
    const auto result = tested.Render(View(), instances, 2U, stats);
    if (graphics::Failed(result) &&
        !Check(device.context.draws == 0U || failure > 12U,
               "failed mandatory bind prevents draw"))
      return 1;
  }
  device.context.FailOn(0U);
  if (!Check(assets::Succeeded(tested.DestroyModel(first)) &&
                 tested.IsModelValid(second) &&
                 tested.GetDiagnostics().liveTextureEntries == 2U,
             "first handle keeps shared GPU resource"))
    return 1;
  if (!Check(tested.DestroyModel(first) == assets::AssetResult::NotFound,
             "repeated destroy rejected"))
    return 1;
  if (!Check(assets::Succeeded(tested.DestroyModel(second)),
             "last handle destroy"))
    return 1;
  diagnostics = tested.GetDiagnostics();
  if (!Check(diagnostics.liveModelResources == 0U &&
                 diagnostics.liveTextureEntries == 0U &&
                 device.Live() == fallbackResources,
             "last handle releases model textures"))
    return 1;
  for (int cycle = 0; cycle < 50; ++cycle) {
    renderer::StaticModelRenderHandle handle;
    if (assets::Failed(tested.CreateModel(manager, registry,
                                          Path("model.ltsmodel"), handle)))
      return 1;
    renderer::StaticModelInstance instance;
    instance.model = handle;
    device.context.FailOn(0U);
    if (graphics::Failed(tested.Render(View(), &instance, 1U, stats)) ||
        assets::Failed(tested.DestroyModel(handle)))
      return 1;
    if (tested.GetDiagnostics().liveModelResources != 0U ||
        tested.GetDiagnostics().liveTextureEntries != 0U)
      return 1;
  }
  {
    engine::graphics::d3d11::D3D11Device warpDevice;
    graphics::RenderDeviceDesc warpDescription;
    warpDescription.backend = graphics::GraphicsBackend::D3D11;
    warpDescription.enableValidation = true;
    warpDescription.forceSoftwareAdapter = true;
    if (!Check(graphics::Succeeded(warpDevice.Initialize(warpDescription)),
               "initialize WARP DX11 device"))
      return 1;
    auto *context = warpDevice.GetImmediateCommandContext();
    const auto warpVertex = CompileShader(graphics::ShaderStage::Vertex),
               warpPixel = CompileShader(graphics::ShaderStage::Pixel);
    renderer::StaticModelRenderer warpRenderer;
    if (!Check(context && graphics::Succeeded(warpRenderer.Initialize(
                              warpDevice, *context, warpVertex, warpPixel)),
               "initialize renderer on WARP"))
      return 1;
    graphics::TextureDesc targetDescription;
    targetDescription.width = 64U;
    targetDescription.height = 64U;
    targetDescription.format = graphics::Format::R8G8B8A8UNorm;
    targetDescription.bindFlags = graphics::TextureBindFlags::RenderTarget;
    graphics::TextureHandle target;
    if (!Check(
            graphics::Succeeded(warpDevice.CreateTexture(
                targetDescription, nullptr, 0U, target)) &&
                graphics::Succeeded(context->SetRenderTargets(&target, 1U, {})),
            "bind WARP render target"))
      return 1;
    renderer::StaticModelRenderHandle warpFirst, warpSecond;
    if (!Check(assets::Succeeded(warpRenderer.CreateModel(
                   manager, registry, Path("model.ltsmodel"), warpFirst)) &&
                   assets::Succeeded(warpRenderer.CreateModel(
                       manager, registry, Path("MODEL.LTSMODEL"), warpSecond)),
               "WARP duplicate model handles"))
      return 1;
    renderer::StaticModelInstance warpInstances[2];
    warpInstances[0].model = warpFirst;
    warpInstances[1].model = warpSecond;
    warpInstances[1].world =
        engine::math::Matrix4::CreateTranslation({0.25F, 0.0F, 0.0F});
    if (!Check(graphics::Succeeded(
                   warpRenderer.Render(View(), warpInstances, 2U, stats)),
               "WARP render multiple instances"))
      return 1;
    if (!Check(assets::Succeeded(warpRenderer.DestroyModel(warpFirst)) &&
                   graphics::Succeeded(warpRenderer.Render(
                       View(), &warpInstances[1], 1U, stats)) &&
                   assets::Succeeded(warpRenderer.DestroyModel(warpSecond)),
               "WARP shared resource survives first handle"))
      return 1;
    warpRenderer.Unbind();
    context->UnbindRenderTargets();
    if (!Check(graphics::Succeeded(warpDevice.DestroyTexture(target)),
               "destroy WARP target"))
      return 1;
    warpRenderer.Shutdown();
    context->ClearState();
    context->Flush();
    warpDevice.Shutdown();
  }
  tested.Shutdown();
  tested.Shutdown();
  if (!Check(device.Live() == 0U,
             "repeated shutdown releases all fake resources"))
    return 1;
  std::puts("LTS.Renderer resource lifetime tests passed");
  return 0;
}
