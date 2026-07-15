#include "Assets/ShaderAssetLoader.h"

#include <cstring>
#include <new>

namespace engine::assets
{
namespace
{
constexpr std::size_t HeaderSize=128U;
template<class T>T Read(const std::byte* b,const std::size_t o)noexcept{T v{};std::memcpy(&v,b+o,sizeof(v));return v;}
bool Zero(const std::byte* b,std::size_t p,const std::size_t e)noexcept{for(;p<e;++p)if(b[p]!=std::byte{})return false;return true;}
}
AssetResult ShaderAssetLoader::Load(const AssetMetadata& metadata,const AssetData& source,std::unique_ptr<LoadedAsset>& out)noexcept
{
    out.reset();if(!metadata.IsValid())return AssetResult::InvalidMetadata;if(metadata.type!=AssetType::Shader)return AssetResult::TypeMismatch;
    if(source.GetSize()<HeaderSize)return AssetResult::CorruptData;const auto* b=source.GetData();if(std::memcmp(b,"LTSSHDR\0",8U)!=0)return AssetResult::UnsupportedFormat;
    if(Read<std::uint32_t>(b,8U)!=1U)return AssetResult::UnsupportedFormat;if(Read<std::uint32_t>(b,12U)!=0x01020304U||Read<std::uint32_t>(b,16U)!=HeaderSize||Read<std::uint32_t>(b,24U)!=0U||!Zero(b,28U,32U)||!Zero(b,108U,HeaderSize))return AssetResult::CorruptData;
    const auto stage=Read<std::uint32_t>(b,20U);if(stage==0U||stage>static_cast<std::uint32_t>(engine::graphics::ShaderStage::Compute))return AssetResult::CorruptData;
    std::size_t expected=HeaderSize;ShaderAssetDesc desc;desc.stage=static_cast<engine::graphics::ShaderStage>(stage);
    const auto stringRegion=[&](const std::size_t field,std::string& value,const bool required)->bool{const auto off=Read<std::uint64_t>(b,field);const auto len=Read<std::uint32_t>(b,field+8U);if(!Zero(b,field+12U,field+16U)||off!=expected||len>ShaderAsset::MaximumMetadataLength||(required&&len==0U)||len>source.GetSize()-expected)return false;value.assign(reinterpret_cast<const char*>(b+expected),len);expected+=len;return true;};
    try
    {
        if(!stringRegion(32U,desc.targetProfile,true)||!stringRegion(48U,desc.entryPoint,true)||!stringRegion(64U,desc.debugName,false))return AssetResult::CorruptData;
        const auto off=Read<std::uint64_t>(b,80U);const auto len=Read<std::uint32_t>(b,88U);if(!Zero(b,92U,96U)||off!=expected||len<4U||len>ShaderAsset::MaximumBytecodeSize||len>source.GetSize()-expected||expected+len!=source.GetSize())return AssetResult::CorruptData;
        desc.bytecode.assign(b+expected,b+expected+len);desc.sourceHash=Read<std::uint64_t>(b,96U);if(desc.sourceHash==0U)return AssetResult::CorruptData;
        if(desc.bytecode.size()<4U||std::memcmp(desc.bytecode.data(),"DXBC",4U)!=0)return AssetResult::CorruptData;
        ShaderAsset shader;if(Failed(shader.Initialize(std::move(desc))))return AssetResult::CorruptData;out=std::make_unique<ShaderLoadedAsset>(std::move(shader));return AssetResult::Success;
    }catch(const std::bad_alloc&){return AssetResult::OutOfMemory;}catch(...){return AssetResult::InternalError;}
}
}
