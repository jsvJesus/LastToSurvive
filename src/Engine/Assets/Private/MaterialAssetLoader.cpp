#include "Assets/MaterialAssetLoader.h"

#include <array>
#include <cstring>
#include <limits>
#include <new>

namespace engine::assets
{
namespace
{
constexpr std::size_t V1HeaderSize=160U,V2HeaderSize=192U,EntrySize=24U;
template<typename T>T Read(const std::byte* b,const std::size_t o)noexcept{T v{};std::memcpy(&v,b+o,sizeof(v));return v;}
bool Zero(const std::byte* b,std::size_t first,const std::size_t end)noexcept{for(;first<end;++first)if(b[first]!=std::byte{})return false;return true;}
bool BaseFields(const std::byte* b,MaterialAssetDesc& d)noexcept
{
    const auto alpha=Read<std::uint32_t>(b,20U),filter=Read<std::uint32_t>(b,68U),u=Read<std::uint32_t>(b,72U),v=Read<std::uint32_t>(b,76U),w=Read<std::uint32_t>(b,80U),comparison=Read<std::uint32_t>(b,92U);
    if(alpha>2U||filter>static_cast<std::uint32_t>(engine::graphics::TextureFilter::ComparisonLinear)||u>3U||v>3U||w>3U||comparison>static_cast<std::uint32_t>(engine::graphics::ComparisonFunction::Always))return false;
    for(std::size_t i=0;i<4;++i)d.baseColorFactor[i]=Read<float>(b,28U+i*4U);
    for(std::size_t i=0;i<3;++i)d.emissiveFactor[i]=Read<float>(b,44U+i*4U);
    d.metallicFactor=Read<float>(b,56U);d.roughnessFactor=Read<float>(b,60U);d.alphaCutoff=Read<float>(b,64U);d.alphaMode=static_cast<MaterialAlphaMode>(alpha);
    d.sampler.filter=static_cast<engine::graphics::TextureFilter>(filter);d.sampler.addressU=static_cast<engine::graphics::TextureAddressMode>(u);d.sampler.addressV=static_cast<engine::graphics::TextureAddressMode>(v);d.sampler.addressW=static_cast<engine::graphics::TextureAddressMode>(w);
    d.sampler.mipLodBias=Read<float>(b,84U);d.sampler.maximumAnisotropy=Read<std::uint32_t>(b,88U);d.sampler.comparisonFunction=static_cast<engine::graphics::ComparisonFunction>(comparison);
    for(std::size_t i=0;i<4;++i)d.sampler.borderColor[i]=Read<float>(b,96U+i*4U);d.sampler.minimumLod=Read<float>(b,112U);d.sampler.maximumLod=Read<float>(b,116U);return true;
}
std::optional<AssetPath>* Slot(MaterialAssetDesc& d,const std::uint32_t semantic)noexcept
{
    switch(static_cast<MaterialTextureSemantic>(semantic)){case MaterialTextureSemantic::BaseColor:return &d.baseColorTexture;case MaterialTextureSemantic::Normal:return &d.normalTexture;case MaterialTextureSemantic::SpecularGloss:return &d.specularGlossTexture;case MaterialTextureSemantic::Roughness:return &d.roughnessTexture;case MaterialTextureSemantic::Emissive:return &d.emissiveTexture;case MaterialTextureSemantic::SpecularPower:return &d.specularPowerTexture;default:return nullptr;}
}
AssetResult Finish(const AssetMetadata& metadata,MaterialAssetDesc&& d,std::unique_ptr<LoadedAsset>& out)noexcept
{
    try{d.debugName=metadata.path.String();MaterialAsset material;if(Failed(material.Initialize(std::move(d))))return AssetResult::CorruptData;out=std::make_unique<MaterialLoadedAsset>(std::move(material));}
    catch(const std::bad_alloc&){return AssetResult::OutOfMemory;}catch(...){return AssetResult::InternalError;}return AssetResult::Success;
}
}
AssetResult MaterialAssetLoader::Load(const AssetMetadata& metadata,const AssetData& source,std::unique_ptr<LoadedAsset>& out)noexcept
{
    out.reset();if(!metadata.IsValid())return AssetResult::InvalidMetadata;if(metadata.type!=AssetType::Material)return AssetResult::TypeMismatch;if(source.GetSize()<V1HeaderSize)return AssetResult::CorruptData;
    const auto* b=source.GetData();if(std::memcmp(b,"LTSMAT\0\0",8U)!=0)return AssetResult::UnsupportedFormat;const auto version=Read<std::uint32_t>(b,8U);if(version!=1U&&version!=2U)return AssetResult::UnsupportedFormat;
    if(Read<std::uint32_t>(b,12U)!=0x01020304U)return AssetResult::CorruptData;MaterialAssetDesc d;if(!BaseFields(b,d))return AssetResult::CorruptData;
    const auto flags=Read<std::uint32_t>(b,24U);d.doubleSided=(flags&1U)!=0U;
    if(version==1U)
    {
        if(Read<std::uint32_t>(b,16U)!=V1HeaderSize||(flags&~3U)!=0U||!Zero(b,132U,V1HeaderSize))return AssetResult::CorruptData;
        const auto offset=Read<std::uint64_t>(b,120U);const auto size=Read<std::uint32_t>(b,128U);const bool has=(flags&2U)!=0U;
        if(has){if(size==0U||size>AssetPath::MaximumLength||offset!=V1HeaderSize||size>source.GetSize()-V1HeaderSize||offset+size!=source.GetSize())return AssetResult::CorruptData;AssetPath path;const auto result=AssetPath::TryCreate(std::string_view(reinterpret_cast<const char*>(b+V1HeaderSize),size),path);if(Failed(result))return result;d.baseColorTexture=std::move(path);}
        else if(size!=0U||(offset!=0U&&offset!=V1HeaderSize)||source.GetSize()!=V1HeaderSize)return AssetResult::CorruptData;
        return Finish(metadata,std::move(d),out);
    }
    if(source.GetSize()<V2HeaderSize||Read<std::uint32_t>(b,16U)!=V2HeaderSize||(flags&~1U)!=0U||!Zero(b,152U,V2HeaderSize))return AssetResult::CorruptData;
    d.normalScale=Read<float>(b,120U);d.specularIntensity=Read<float>(b,124U);d.specularPower=Read<float>(b,128U);d.reflectionFactor=Read<float>(b,132U);d.emissiveStrength=Read<float>(b,136U);
    const auto count=Read<std::uint32_t>(b,140U);const auto table=Read<std::uint64_t>(b,144U);if(count>6U||(count==0U?(table!=0U||source.GetSize()!=V2HeaderSize):table!=V2HeaderSize))return AssetResult::CorruptData;
    if(count!=0U&&count>(source.GetSize()-V2HeaderSize)/EntrySize)return AssetResult::CorruptData;const std::size_t pathsBegin=V2HeaderSize+static_cast<std::size_t>(count)*EntrySize;std::size_t expected=pathsBegin;std::uint32_t previous=0U;
    for(std::uint32_t i=0;i<count;++i){const std::size_t e=V2HeaderSize+static_cast<std::size_t>(i)*EntrySize;const auto semantic=Read<std::uint32_t>(b,e);const auto offset=Read<std::uint64_t>(b,e+8U);const auto size=Read<std::uint32_t>(b,e+16U);if((i&&semantic<=previous)||semantic>=6U||!Zero(b,e+4U,e+8U)||!Zero(b,e+20U,e+24U)||size==0U||size>AssetPath::MaximumLength||offset!=expected||size>source.GetSize()-expected)return AssetResult::CorruptData;auto* slot=Slot(d,semantic);if(slot==nullptr||*slot)return AssetResult::CorruptData;AssetPath path;const auto result=AssetPath::TryCreate(std::string_view(reinterpret_cast<const char*>(b+expected),size),path);if(Failed(result))return result;*slot=std::move(path);expected+=size;previous=semantic;}
    if(expected!=source.GetSize())return AssetResult::CorruptData;return Finish(metadata,std::move(d),out);
}
}
