#include "Assets/MeshAssetLoader.h"
#include <cmath>
#include <cstring>
#include <new>
namespace engine::assets
{
    namespace
    {
        constexpr std::size_t HeaderSize=160U; constexpr std::uint32_t Endian=0x01020304U;
        std::uint32_t U32(const std::byte* p,std::size_t o) noexcept {std::uint32_t v;std::memcpy(&v,p+o,4);return v;}
        std::uint64_t U64(const std::byte* p,std::size_t o) noexcept {std::uint64_t v;std::memcpy(&v,p+o,8);return v;}
        float F32(const std::byte* p,std::size_t o) noexcept {float v;std::memcpy(&v,p+o,4);return v;}
        bool Region(std::uint64_t o,std::uint64_t s,std::size_t total) noexcept{return o<=total&&s<=total-o;}
    }
    AssetResult MeshAssetLoader::Load(const AssetMetadata& md,const AssetData& src,std::unique_ptr<LoadedAsset>& out) noexcept
    {
        out.reset(); if(!md.IsValid())return AssetResult::InvalidMetadata;if(md.type!=AssetType::Mesh)return AssetResult::TypeMismatch;
        if(src.GetSize()<HeaderSize)return AssetResult::CorruptData;const std::byte* b=src.GetData();const char magic[8]={'L','T','S','M','E','S','H','\0'};if(std::memcmp(b,magic,8)!=0)return AssetResult::UnsupportedFormat;
        if(U32(b,8)!=1U)return AssetResult::UnsupportedFormat;if(U32(b,12)!=Endian||U32(b,16)<HeaderSize||U32(b,16)>src.GetSize())return AssetResult::CorruptData;
        const auto stride=U32(b,20),fmt=U32(b,24),vc=U32(b,28),ic=U32(b,32),sc=U32(b,36),mc=U32(b,40);
        if(stride!=48U||(fmt!=1U&&fmt!=2U)||vc==0U||ic==0U||sc==0U||mc==0U||vc>10000000U||ic>30000000U||sc>65536U||mc>65536U)return AssetResult::CorruptData;
        const std::uint64_t vo=U64(b,48),vs=U64(b,56),io=U64(b,64),is=U64(b,72),so=U64(b,80),ss=U64(b,88);const std::uint64_t ies=fmt==1U?2U:4U;
        if(vs!=std::uint64_t(vc)*48U||is!=std::uint64_t(ic)*ies||ss!=std::uint64_t(sc)*16U||vo<HeaderSize||vo%4U||io%ies||so%4U||io<vo+vs||so<io+is||!Region(vo,vs,src.GetSize())||!Region(io,is,src.GetSize())||!Region(so,ss,src.GetSize()))return AssetResult::CorruptData;
        const std::uint64_t end=(std::max)(vo+vs,(std::max)(io+is,so+ss));if(end!=src.GetSize())return AssetResult::CorruptData;
        MeshAsset m;try{m.vertices_.resize(vc);m.indices_.resize(static_cast<std::size_t>(is));m.submeshes_.resize(sc);m.debugName_=md.path.String();}catch(const std::bad_alloc&){return AssetResult::OutOfMemory;}catch(...){return AssetResult::InternalError;}
        std::memcpy(m.vertices_.data(),b+vo,static_cast<std::size_t>(vs));std::memcpy(m.indices_.data(),b+io,static_cast<std::size_t>(is));m.indexFormat_=fmt==1U?engine::graphics::IndexFormat::UInt16:engine::graphics::IndexFormat::UInt32;m.materialSlotCount_=mc;
        for(std::size_t i=0;i<3;++i){m.bounds_.minimum[i]=F32(b,96+i*4);m.bounds_.maximum[i]=F32(b,108+i*4);m.bounds_.sphereCenter[i]=F32(b,120+i*4);}m.bounds_.sphereRadius=F32(b,132);
        for(std::size_t i=0;i<sc;++i){const std::size_t o=static_cast<std::size_t>(so)+i*16;m.submeshes_[i]={U32(b,o),U32(b,o+4),static_cast<std::int32_t>(U32(b,o+8)),U32(b,o+12)};}
        if(!m.IsValid())return AssetResult::CorruptData;try{out=std::make_unique<MeshLoadedAsset>(std::move(m));}catch(const std::bad_alloc&){return AssetResult::OutOfMemory;}catch(...){return AssetResult::InternalError;}return AssetResult::Success;
    }
}
