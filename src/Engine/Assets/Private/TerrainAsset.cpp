#include "Assets/TerrainAsset.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
namespace engine::assets
{
namespace
{
constexpr std::uint32_t Signature=0x5453544CU,Version=1U;
template<class T>bool Read(std::ifstream& s,T& v){return !!s.read(reinterpret_cast<char*>(&v),sizeof(v));}
bool ReadString(std::ifstream& s,std::string& v)
{
    std::uint32_t n=0;if(!Read(s,n)||n>4096U)return false;v.resize(n);return n==0U||!!s.read(v.data(),n);
}
bool ReadTexture(std::ifstream& s,TerrainEmbeddedTexture& texture)
{
    if(!Read(s,texture.size)||texture.size==0U||texture.size>512ULL*1024ULL*1024ULL)return false;
    const auto p=s.tellg();if(p<0)return false;texture.offset=static_cast<std::uint64_t>(p);
    s.seekg(static_cast<std::streamoff>(texture.size),std::ios::cur);return !!s;
}
}
AssetResult TerrainAsset::Load(const std::filesystem::path& path,TerrainAsset& output) noexcept
{
    try
    {
        TerrainAsset result;std::ifstream s(path,std::ios::binary);if(!s)return AssetResult::NotFound;
        std::uint32_t signature=0,version=0,layerCount=0,maskCount=0;std::uint64_t heightBytes=0;
        if(!Read(s,signature)||!Read(s,version)||signature!=Signature||version!=Version||
            !Read(s,result.width)||!Read(s,result.height)||!Read(s,result.splatWidth)||!Read(s,result.splatHeight)||
            !Read(s,result.tileSize)||!Read(s,result.heightOffset)||!Read(s,result.heightScale)||!Read(s,layerCount)||!Read(s,maskCount)||!Read(s,heightBytes))return AssetResult::CorruptData;
        const auto samples=static_cast<std::uint64_t>(result.width)*result.height;
        if(result.width<2U||result.height<2U||samples>268435456ULL||heightBytes!=samples*sizeof(std::int16_t)||layerCount==0U||layerCount>64U||maskCount!=(layerCount+2U)/3U)return AssetResult::CorruptData;
        result.heights.resize(static_cast<std::size_t>(samples));if(!s.read(reinterpret_cast<char*>(result.heights.data()),static_cast<std::streamsize>(heightBytes)))return AssetResult::CorruptData;
        result.layers.resize(layerCount);for(auto& l:result.layers)if(!ReadString(s,l.name)||!ReadString(s,l.diffusePath)||!ReadString(s,l.normalPath)||!ReadString(s,l.materialType)||!Read(s,l.scaleU)||!Read(s,l.scaleV)||!Read(s,l.specular))return AssetResult::CorruptData;
        result.masks.resize(maskCount);for(auto& m:result.masks)if(!ReadTexture(s,m))return AssetResult::CorruptData;
        if(!ReadTexture(s,result.colorMap)||!ReadTexture(s,result.normalMap))return AssetResult::CorruptData;
        result.sourcePath=path;output=std::move(result);return AssetResult::Success;
    }catch(const std::bad_alloc&){return AssetResult::OutOfMemory;}catch(...){return AssetResult::IoError;}
}
float TerrainAsset::GetHeight(const std::uint32_t x,const std::uint32_t z)const noexcept
{
    if(x>=width||z>=height||heights.empty())return 0.0F;
    const float maximum=(std::max)(std::fabs(heightOffset),std::fabs(heightOffset+heightScale));
    return maximum>0.0F?static_cast<float>(heights[static_cast<std::size_t>(x)*height+z])*(maximum/32767.0F):0.0F;
}
bool TerrainAsset::IsValid()const noexcept{return width>1U&&height>1U&&heights.size()==static_cast<std::size_t>(width)*height&&!layers.empty();}
}
