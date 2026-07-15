#include "Assets/LtsShaderWriter.h"

#include <cstring>
#include <limits>

namespace engine::assets
{
namespace
{
constexpr std::size_t HeaderSize = 128U;
template<class T> void Write(std::byte* bytes, const std::size_t offset, const T value) noexcept
{ std::memcpy(bytes + offset, &value, sizeof(value)); }
}
AssetResult LtsShaderWriter::Encode(const ShaderAsset& shader, AssetData& output) noexcept
{
    output.Clear(); if (!shader.IsValid()) return AssetResult::InvalidArgument;
    const auto& profile=shader.GetTargetProfile();const auto& entry=shader.GetEntryPoint();const auto& name=shader.GetDebugName();
    const std::size_t total=HeaderSize+profile.size()+entry.size()+name.size()+shader.GetBytecodeSize();
    if(total<(HeaderSize)||total>(std::numeric_limits<std::uint32_t>::max)())return AssetResult::FileTooLarge;
    const AssetResult resized=output.Resize(total);if(Failed(resized))return resized;auto* b=output.GetData();std::memset(b,0,total);
    std::memcpy(b,"LTSSHDR\0",8U);Write<std::uint32_t>(b,8U,1U);Write<std::uint32_t>(b,12U,0x01020304U);Write<std::uint32_t>(b,16U,HeaderSize);
    Write<std::uint32_t>(b,20U,static_cast<std::uint32_t>(shader.GetStage()));Write<std::uint32_t>(b,24U,0U);
    std::size_t cursor=HeaderSize;
    const auto region=[&](const std::size_t field,const void* data,const std::size_t size){Write<std::uint64_t>(b,field,static_cast<std::uint64_t>(cursor));Write<std::uint32_t>(b,field+8U,static_cast<std::uint32_t>(size));if(size){std::memcpy(b+cursor,data,size);cursor+=size;}};
    region(32U,profile.data(),profile.size());region(48U,entry.data(),entry.size());region(64U,name.data(),name.size());region(80U,shader.GetBytecode(),shader.GetBytecodeSize());Write<std::uint64_t>(b,96U,shader.GetSourceHash());return AssetResult::Success;
}
}
