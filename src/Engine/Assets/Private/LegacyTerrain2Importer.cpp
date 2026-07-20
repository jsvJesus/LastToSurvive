#include "Assets/LegacyTerrain2Importer.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace engine::assets
{
namespace
{
constexpr std::uint32_t LegacySignature = 0x32524554U;
constexpr std::uint32_t TerrainSignature = 0x5453544CU;
constexpr std::uint32_t TerrainVersion = 1U;
struct Layer { std::string name, diffuse, normal, materialType; float scaleU=1, scaleV=1, specular=0; };
struct Description
{
    std::uint32_t width=0, height=0, splatWidth=0, splatHeight=0;
    float tileSize=0, heightOffset=0, heightScale=0;
    std::vector<Layer> layers;
};
std::string Trim(std::string value)
{
    const auto ws=[](unsigned char c){return std::isspace(c)!=0;};
    value.erase(value.begin(),std::find_if_not(value.begin(),value.end(),ws));
    value.erase(std::find_if_not(value.rbegin(),value.rend(),ws).base(),value.end());
    if(value.size()>=2&&value.front()=='"'&&value.back()=='"') value=value.substr(1,value.size()-2);
    return value;
}
bool Property(const std::string& line,std::string& key,std::string& value)
{
    const auto p=line.find(':'); if(p==std::string::npos)return false;
    key=Trim(line.substr(0,p)); value=Trim(line.substr(p+1)); return !key.empty();
}
bool Parse(const std::filesystem::path& file,Description& out)
{
    std::ifstream stream(file); if(!stream)return false;
    Layer* layer=nullptr; std::string line;
    while(std::getline(stream,line))
    {
        line=Trim(line);
        if(line=="base_layer"||line=="layer"){out.layers.emplace_back();layer=&out.layers.back();continue;}
        if(line=="{"||line.empty())continue;
        if(line=="}"){layer=nullptr;continue;}
        std::string key,value; if(!Property(line,key,value))continue;
        try
        {
            if(layer)
            {
                if(key=="name")layer->name=value; else if(key=="map_diffuse")layer->diffuse=value;
                else if(key=="map_normal")layer->normal=value; else if(key=="mat_type")layer->materialType=value;
                else if(key=="scale_u")layer->scaleU=std::stof(value); else if(key=="scale_v")layer->scaleV=std::stof(value);
                else if(key=="specular")layer->specular=std::stof(value);
            }
            else if(key=="vert_count_x")out.width=std::stoul(value); else if(key=="vert_count_z")out.height=std::stoul(value);
            else if(key=="splat_res_u")out.splatWidth=std::stoul(value); else if(key=="splat_res_v")out.splatHeight=std::stoul(value);
            else if(key=="tile_unit_size")out.tileSize=std::stof(value); else if(key=="height_offset")out.heightOffset=std::stof(value);
            else if(key=="height_scale")out.heightScale=std::stof(value);
        } catch(...){return false;}
    }
    return out.width>1&&out.height>1&&out.tileSize>0&&out.heightScale>0&&!out.layers.empty();
}
template<class T> bool Read(std::ifstream& s,T& v){return !!s.read(reinterpret_cast<char*>(&v),sizeof(v));}
template<class T> void Write(std::ofstream& s,const T& v){s.write(reinterpret_cast<const char*>(&v),sizeof(v));}
void WriteString(std::ofstream& s,const std::string& v){const auto n=static_cast<std::uint32_t>(v.size());Write(s,n);s.write(v.data(),n);}
bool WriteBlob(std::ofstream& output,const std::filesystem::path& file)
{
    std::ifstream input(file,std::ios::binary|std::ios::ate);if(!input)return false;
    const auto end=input.tellg();if(end<0)return false;const auto size=static_cast<std::uint64_t>(end);Write(output,size);
    input.seekg(0);std::vector<char> buffer(1024U*1024U);std::uint64_t left=size;
    while(left){const auto amount=static_cast<std::streamsize>((std::min)(left,static_cast<std::uint64_t>(buffer.size())));if(!input.read(buffer.data(),amount))return false;output.write(buffer.data(),amount);left-=static_cast<std::uint64_t>(amount);}
    return !!output;
}
}
AssetResult LegacyTerrain2Importer::Import(const std::filesystem::path& source,const std::filesystem::path& destination) noexcept
{
    try
    {
        Description desc;if(!Parse(source/L"terrain2.ini",desc))return AssetResult::CorruptData;
        std::ifstream bin(source/L"terrain2.bin",std::ios::binary);if(!bin)return AssetResult::NotFound;
        std::uint32_t signature=0,version=0,count=0;if(!Read(bin,signature)||!Read(bin,version)||!Read(bin,count))return AssetResult::CorruptData;
        const auto expected=static_cast<std::uint64_t>(desc.width)*desc.height;
        if(signature!=LegacySignature||(version!=101&&version!=103)||count!=expected)return AssetResult::CorruptData;
        std::vector<std::int16_t> heights(count);if(!bin.read(reinterpret_cast<char*>(heights.data()),static_cast<std::streamsize>(heights.size()*sizeof(heights[0]))))return AssetResult::CorruptData;
        std::error_code ec;std::filesystem::create_directories(destination.parent_path(),ec);if(ec)return AssetResult::IoError;
        auto temporary=destination;temporary+=L".tmp";std::ofstream out(temporary,std::ios::binary|std::ios::trunc);if(!out)return AssetResult::IoError;
        Write(out,TerrainSignature);Write(out,TerrainVersion);Write(out,desc.width);Write(out,desc.height);Write(out,desc.splatWidth);Write(out,desc.splatHeight);
        Write(out,desc.tileSize);Write(out,desc.heightOffset);Write(out,desc.heightScale);const auto layers=static_cast<std::uint32_t>(desc.layers.size());const auto masks=(layers+2U)/3U;Write(out,layers);Write(out,masks);
        const auto heightBytes=static_cast<std::uint64_t>(heights.size()*sizeof(heights[0]));Write(out,heightBytes);out.write(reinterpret_cast<const char*>(heights.data()),static_cast<std::streamsize>(heightBytes));
        for(const auto& l:desc.layers){WriteString(out,l.name);WriteString(out,l.diffuse);WriteString(out,l.normal);WriteString(out,l.materialType);Write(out,l.scaleU);Write(out,l.scaleV);Write(out,l.specular);}
        for(std::uint32_t i=0;i<masks;++i)if(!WriteBlob(out,source/(L"Mat-Splat"+std::to_wstring(i)+L".dds")))return AssetResult::NotFound;
        if(!WriteBlob(out,source/L"Color.dds")||!WriteBlob(out,source/L"Normal.dds"))return AssetResult::NotFound;
        out.close();if(!out)return AssetResult::IoError;std::filesystem::remove(destination,ec);ec.clear();std::filesystem::rename(temporary,destination,ec);return ec?AssetResult::IoError:AssetResult::Success;
    }
    catch(const std::bad_alloc&){return AssetResult::OutOfMemory;}catch(...){return AssetResult::IoError;}
}
}
