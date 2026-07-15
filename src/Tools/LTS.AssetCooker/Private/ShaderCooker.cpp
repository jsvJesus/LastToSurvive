#include "AssetCooker/ShaderCooker.h"
#include "Assets/LtsShaderWriter.h"
#include "Assets/ShaderAsset.h"
#include "Platform/File.h"

#include <Windows.h>
#include <algorithm>
#include <cstring>
#include <d3dcompiler.h>
#include <limits>
#include <map>
#include <memory>
#include <set>

namespace lts::asset_cooker {
namespace {
using engine::assets::AssetResult;
std::uint64_t HashBytes(std::uint64_t hash, const void *data,
                        const std::size_t size) noexcept {
  const auto *p = static_cast<const unsigned char *>(data);
  for (std::size_t i = 0; i < size; ++i) {
    hash ^= p[i];
    hash *= 1099511628211ULL;
  }
  return hash;
}
std::uint64_t HashValue(std::uint64_t hash,
                        const std::uint64_t value) noexcept {
  return HashBytes(hash, &value, sizeof(value));
}
std::uint64_t HashRegion(std::uint64_t hash, const void *data,
                         const std::size_t size) noexcept {
  hash = HashValue(hash, static_cast<std::uint64_t>(size));
  return HashBytes(hash, data, size);
}
std::uint64_t HashString(std::uint64_t hash,
                         const std::string &value) noexcept {
  return HashRegion(hash, value.data(), value.size());
}
bool Inside(const std::filesystem::path &root,
            const std::filesystem::path &path) noexcept {
  auto r = root.native(), p = path.native();
  if (p.size() < r.size() || _wcsnicmp(p.c_str(), r.c_str(), r.size()) != 0)
    return false;
  return p.size() == r.size() || p[r.size()] == L'\\' || p[r.size()] == L'/';
}
AssetResult Read(const std::filesystem::path &path,
                 std::vector<std::byte> &bytes) noexcept {
  engine::platform::File file(path);
  const auto size = file.GetSize();
  if (!file || !size)
    return AssetResult::IoError;
  if (*size > (std::numeric_limits<std::size_t>::max)())
    return AssetResult::FileTooLarge;
  try {
    bytes.resize(static_cast<std::size_t>(*size));
  } catch (...) {
    return AssetResult::OutOfMemory;
  }
  const auto read = file.Read(bytes.data(), bytes.size());
  return read && read.bytesTransferred == bytes.size() ? AssetResult::Success
                                                       : AssetResult::IoError;
}
class IncludeHandler final : public ID3DInclude {
public:
  IncludeHandler(std::filesystem::path root, std::filesystem::path main,
                 std::uint64_t &hash, std::string &error)
      : root_(std::move(root)), main_(std::move(main)), hash_(hash),
        error_(error) {}
  HRESULT __stdcall Open(D3D_INCLUDE_TYPE, const char *name, LPCVOID parent,
                         LPCVOID *data, UINT *size) override {
    if (!name || !data || !size)
      return E_INVALIDARG;
    *data = nullptr;
    *size = 0U;
    try {
      const std::filesystem::path relative = std::filesystem::u8path(name);
      if (relative.is_absolute()) {
        SetError("absolute include rejected");
        return E_FAIL;
      }
      std::filesystem::path base = main_.parent_path();
      if (parent) {
        const auto owner = blocks_.find(parent);
        if (owner != blocks_.end())
          base = owner->second->path.parent_path();
      }
      std::error_code ec;
      const auto candidate =
          std::filesystem::weakly_canonical(base / relative, ec);
      if (ec || !Inside(root_, candidate)) {
        SetError("include escapes canonical root");
        return E_FAIL;
      }
      for (const auto &open : openPaths_)
        if (_wcsicmp(open.c_str(), candidate.c_str()) == 0) {
          SetError("include cycle");
          return E_FAIL;
        }
      auto block = std::make_unique<Block>();
      block->path = candidate;
      const auto result = Read(candidate, block->bytes);
      if (engine::assets::Failed(result)) {
        SetError("include read failed");
        return E_FAIL;
      }
      if (block->bytes.size() > (std::numeric_limits<UINT>::max)()) {
        SetError("include is too large");
        return E_FAIL;
      }
      const LPCVOID pointer = block->bytes.data();
      const auto relativePath =
          std::filesystem::relative(candidate, root_).generic_u8string();
      openPaths_.push_back(candidate);
      decltype(blocks_)::iterator inserted;
      try {
        inserted = blocks_.emplace(pointer, std::move(block)).first;
      } catch (...) {
        openPaths_.pop_back();
        throw;
      }
      hash_ = HashString(hash_, relativePath);
      hash_ = HashRegion(hash_, pointer, inserted->second->bytes.size());
      *data = pointer;
      *size = static_cast<UINT>(inserted->second->bytes.size());
      return S_OK;
    } catch (const std::bad_alloc &) {
      SetError("out of memory while opening include");
      return E_OUTOFMEMORY;
    } catch (...) {
      SetError("unexpected include handler failure");
      return E_FAIL;
    }
  }
  HRESULT __stdcall Close(LPCVOID data) override {
    try {
      const auto it = blocks_.find(data);
      if (it == blocks_.end())
        return E_FAIL;
      for (auto p = openPaths_.begin(); p != openPaths_.end(); ++p)
        if (*p == it->second->path) {
          openPaths_.erase(p);
          break;
        }
      blocks_.erase(it);
      return S_OK;
    } catch (...) {
      return E_FAIL;
    }
  }

private:
  struct Block {
    std::filesystem::path path;
    std::vector<std::byte> bytes;
  };
  void SetError(const char *message) noexcept {
    try {
      error_ = message;
    } catch (...) {
    }
  }
  std::filesystem::path root_, main_;
  std::uint64_t &hash_;
  std::string &error_;
  std::map<LPCVOID, std::unique_ptr<Block>> blocks_;
  std::vector<std::filesystem::path> openPaths_;
};
bool ProfileMatches(const engine::graphics::ShaderStage stage,
                    const std::string &profile) noexcept {
  return (stage == engine::graphics::ShaderStage::Vertex &&
          profile.rfind("vs_", 0) == 0) ||
         (stage == engine::graphics::ShaderStage::Pixel &&
          profile.rfind("ps_", 0) == 0);
}
} // namespace
AssetResult CookShader(const ShaderCookOptions &raw,
                       engine::assets::AssetData &output,
                       std::string &diagnostics) noexcept {
  output.Clear();
  diagnostics.clear();
  if (raw.input.empty() || raw.includeRoot.empty() || raw.entryPoint.empty() ||
      !ProfileMatches(raw.stage, raw.targetProfile))
    return AssetResult::InvalidArgument;
  std::error_code ec;
  const auto root = std::filesystem::weakly_canonical(raw.includeRoot, ec);
  if (ec)
    return AssetResult::InvalidPath;
  const auto input = std::filesystem::weakly_canonical(raw.input, ec);
  if (ec || !Inside(root, input))
    return AssetResult::InvalidPath;
  std::vector<std::byte> source;
  auto result = Read(input, source);
  if (engine::assets::Failed(result))
    return result;
  std::vector<ShaderDefine> definitions = raw.defines;
  std::sort(definitions.begin(), definitions.end(),
            [](const auto &a, const auto &b) { return a.name < b.name; });
  for (std::size_t i = 0; i < definitions.size(); ++i)
    if (definitions[i].name.empty() ||
        definitions[i].name.find('=') != std::string::npos ||
        (i && definitions[i - 1].name == definitions[i].name))
      return AssetResult::InvalidArgument;
  constexpr std::uint64_t HashContractVersion = 2U;
  const UINT flags = D3DCOMPILE_ENABLE_STRICTNESS |
                     D3DCOMPILE_WARNINGS_ARE_ERRORS |
                     D3DCOMPILE_OPTIMIZATION_LEVEL3;
  std::uint64_t hash = 1469598103934665603ULL;
  const auto relativeInput =
      std::filesystem::relative(input, root, ec).generic_u8string();
  if (ec)
    return AssetResult::InvalidPath;
  hash = HashValue(hash, HashContractVersion);
  hash = HashString(hash, relativeInput);
  hash = HashRegion(hash, source.data(), source.size());
  hash = HashValue(hash, static_cast<std::uint64_t>(raw.stage));
  hash = HashString(hash, raw.entryPoint);
  hash = HashString(hash, raw.targetProfile);
  hash = HashValue(hash, flags);
  std::vector<D3D_SHADER_MACRO> macros;
  try {
    macros.reserve(definitions.size() + 1U);
    for (const auto &d : definitions) {
      hash = HashString(hash, d.name);
      hash = HashString(hash, d.value);
      macros.push_back({d.name.c_str(), d.value.c_str()});
    }
    macros.push_back({nullptr, nullptr});
  } catch (...) {
    return AssetResult::OutOfMemory;
  }
  IncludeHandler include(root, input, hash, diagnostics);
  ID3DBlob *code = nullptr;
  ID3DBlob *errors = nullptr;
  const HRESULT compiled =
      D3DCompile(source.data(), source.size(), nullptr, macros.data(), &include,
                 raw.entryPoint.c_str(), raw.targetProfile.c_str(), flags, 0U,
                 &code, &errors);
  if (errors) {
    diagnostics.assign(static_cast<const char *>(errors->GetBufferPointer()),
                       errors->GetBufferSize());
    errors->Release();
  }
  if (FAILED(compiled)) {
    if (code)
      code->Release();
    return AssetResult::CorruptData;
  }
  engine::assets::ShaderAssetDesc desc;
  desc.stage = raw.stage;
  desc.targetProfile = raw.targetProfile;
  desc.entryPoint = raw.entryPoint;
  desc.sourceHash = hash;
  desc.debugName = input.filename().u8string();
  try {
    const auto *first =
        static_cast<const std::byte *>(code->GetBufferPointer());
    desc.bytecode.assign(first, first + code->GetBufferSize());
  } catch (...) {
    code->Release();
    return AssetResult::OutOfMemory;
  }
  code->Release();
  engine::assets::ShaderAsset asset;
  result = asset.Initialize(std::move(desc));
  if (engine::assets::Failed(result))
    return result;
  return engine::assets::LtsShaderWriter::Encode(asset, output);
}
} // namespace lts::asset_cooker
