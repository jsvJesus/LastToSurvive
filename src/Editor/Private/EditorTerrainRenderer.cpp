#include "Editor/EditorTerrainRenderer.h"
#include "Editor/EditorShaderCompiler.h"
#include <Assets/TerrainAsset.h>
#include <Assets/AssetData.h>
#include <Assets/DdsTextureDecoder.h>
#include <Assets/TextureAsset.h>
#include <Core/Log.h>
#include <Graphics/Buffer.h>
#include <Graphics/CommandContext.h>
#include <Graphics/Format.h>
#include <Graphics/InputLayout.h>
#include <Graphics/PipelineState.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/Shader.h>
#include <Graphics/Sampler.h>
#include <Graphics/Texture.h>
#include <GraphicsDX11/D3D11Device.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>
namespace lts::editor
{
namespace {
    struct Vertex{DirectX::XMFLOAT3 position;DirectX::XMFLOAT3 normal;DirectX::XMFLOAT2 uv;};
    
    struct alignas(16) Constants final
    {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 viewProjection;

        std::array<
            DirectX::XMFLOAT4,
            18U> placement;

        std::array<
            DirectX::XMFLOAT4,
            18U> layerParameters;

        /*
         * x = terrain local width.
         * y = terrain local depth.
         * z = active layer count.
         */
        DirectX::XMFLOAT4 terrainInfo;
    };

    static_assert(
        sizeof(Constants) % 16U == 0U);

    struct BrushVertex{DirectX::XMFLOAT3 position;DirectX::XMFLOAT4 color;};
    struct alignas(16) BrushConstants{DirectX::XMFLOAT4X4 viewProjection;};
    struct Chunk
    {
        DirectX::XMFLOAT3 minimum{};
        DirectX::XMFLOAT3 maximum{};
        DirectX::XMFLOAT3 center{};
        std::array<std::uint32_t, 3> first{};
        std::array<std::uint32_t, 3> count{};
    };

    struct PaintChange
    {
        std::uint32_t pixel=0U;
        std::array<std::uint8_t,17> before{};
        std::array<std::uint8_t,17> after{};
    };

    using PaintCommand=std::vector<PaintChange>;

    bool IsChunkVisible(
        const Chunk& chunk,
        DirectX::FXMMATRIX worldViewProjection) noexcept
    {
        std::array<DirectX::XMVECTOR, 8> clipPoints{};
        std::size_t point = 0;
        for (std::uint32_t z = 0; z < 2; ++z)
        {
            for (std::uint32_t y = 0; y < 2; ++y)
            {
                for (std::uint32_t x = 0; x < 2; ++x)
                {
                    const auto position = DirectX::XMVectorSet(
                        x == 0 ? chunk.minimum.x : chunk.maximum.x,
                        y == 0 ? chunk.minimum.y : chunk.maximum.y,
                        z == 0 ? chunk.minimum.z : chunk.maximum.z,
                        1.0F);
                    clipPoints[point++] = DirectX::XMVector4Transform(position, worldViewProjection);
                }
            }
        }

        const auto allOutside = [&clipPoints](auto predicate)
        {
            for (const auto& clip : clipPoints)
            {
                DirectX::XMFLOAT4 value{};
                DirectX::XMStoreFloat4(&value, clip);
                if (!predicate(value))
                    return false;
            }
            return true;
        };

        return !allOutside([](const DirectX::XMFLOAT4& p) { return p.x < -p.w; }) &&
               !allOutside([](const DirectX::XMFLOAT4& p) { return p.x > p.w; }) &&
               !allOutside([](const DirectX::XMFLOAT4& p) { return p.y < -p.w; }) &&
               !allOutside([](const DirectX::XMFLOAT4& p) { return p.y > p.w; }) &&
               !allOutside([](const DirectX::XMFLOAT4& p) { return p.z < 0.0F; }) &&
               !allOutside([](const DirectX::XMFLOAT4& p) { return p.z > p.w; });
    }
    bool CreateEmbeddedTexture(engine::graphics::RenderDevice& device,const engine::assets::TerrainAsset& terrain,const engine::assets::TerrainEmbeddedTexture& embedded,engine::graphics::TextureHandle& output)
    {
        std::ifstream stream(terrain.sourcePath,std::ios::binary);if(!stream)return false;stream.seekg(static_cast<std::streamoff>(embedded.offset));engine::assets::AssetData data;if(engine::assets::Failed(data.Resize(static_cast<std::size_t>(embedded.size))))return false;if(!stream.read(reinterpret_cast<char*>(data.GetData()),static_cast<std::streamsize>(embedded.size)))return false;engine::assets::TextureAsset decoded;if(engine::assets::Failed(engine::assets::DdsTextureDecoder::Decode(data,decoded)))return false;std::vector<engine::graphics::TextureSubresourceData> initial(decoded.GetSubresourceCount());for(std::size_t i=0;i<initial.size();++i)if(engine::assets::Failed(decoded.GetSubresourceData(i,initial[i])))return false;return engine::graphics::Succeeded(device.CreateTexture(decoded.GetDesc(),initial.data(),initial.size(),output));
    }

    bool DecodeEmbeddedMask(
        const engine::assets::TerrainAsset& terrain,
        const engine::assets::TerrainEmbeddedTexture& embedded,
        std::vector<std::byte>& rgba,
        std::uint32_t& width,
        std::uint32_t& height)
    {
        std::ifstream stream(terrain.sourcePath,std::ios::binary);
        if(!stream)return false;
        stream.seekg(static_cast<std::streamoff>(embedded.offset));
        engine::assets::AssetData data;
        if(engine::assets::Failed(data.Resize(static_cast<std::size_t>(embedded.size))))return false;
        if(!stream.read(reinterpret_cast<char*>(data.GetData()),static_cast<std::streamsize>(embedded.size)))return false;
        engine::assets::TextureAsset decoded;
        if(engine::assets::Failed(engine::assets::DdsTextureDecoder::Decode(data,decoded)))return false;
        const auto& desc=decoded.GetDesc();
        width=desc.width;height=desc.height;
        engine::graphics::TextureSubresourceData source{};
        if(engine::assets::Failed(decoded.GetSubresourceData(0,source)))return false;
        rgba.assign(static_cast<std::size_t>(width)*height*4U,std::byte{0});
        if(desc.format==engine::graphics::Format::R8G8B8A8UNorm ||
           desc.format==engine::graphics::Format::R8G8B8A8UNormSrgb)
        {
            for(std::uint32_t y=0;y<height;++y)
                std::memcpy(rgba.data()+static_cast<std::size_t>(y)*width*4U,
                    source.data+static_cast<std::size_t>(y)*source.rowPitch,
                    static_cast<std::size_t>(width)*4U);
            return true;
        }
        if(desc.format!=engine::graphics::Format::BC1UNorm &&
           desc.format!=engine::graphics::Format::BC1UNormSrgb)return false;

        const auto expand5=[](const std::uint16_t value){return static_cast<std::uint8_t>((value<<3U)|(value>>2U));};
        const auto expand6=[](const std::uint16_t value){return static_cast<std::uint8_t>((value<<2U)|(value>>4U));};
        for(std::uint32_t blockY=0;blockY<(height+3U)/4U;++blockY)
        {
            const auto* row=reinterpret_cast<const std::uint8_t*>(source.data+static_cast<std::size_t>(blockY)*source.rowPitch);
            for(std::uint32_t blockX=0;blockX<(width+3U)/4U;++blockX)
            {
                const auto* block=row+blockX*8U;
                const std::uint16_t color0=static_cast<std::uint16_t>(block[0]|(block[1]<<8U));
                const std::uint16_t color1=static_cast<std::uint16_t>(block[2]|(block[3]<<8U));
                std::array<std::array<std::uint8_t,4>,4> colors{};
                colors[0]={expand5((color0>>11U)&31U),expand6((color0>>5U)&63U),expand5(color0&31U),255U};
                colors[1]={expand5((color1>>11U)&31U),expand6((color1>>5U)&63U),expand5(color1&31U),255U};
                for(std::size_t channel=0;channel<3U;++channel)
                {
                    if(color0>color1)
                    {
                        colors[2][channel]=static_cast<std::uint8_t>((2U*colors[0][channel]+colors[1][channel])/3U);
                        colors[3][channel]=static_cast<std::uint8_t>((colors[0][channel]+2U*colors[1][channel])/3U);
                    }
                    else
                    {
                        colors[2][channel]=static_cast<std::uint8_t>((colors[0][channel]+colors[1][channel])/2U);
                        colors[3][channel]=0U;
                    }
                }
                colors[2][3]=255U;colors[3][3]=color0>color1?255U:0U;
                const std::uint32_t selectors=static_cast<std::uint32_t>(block[4])|
                    (static_cast<std::uint32_t>(block[5])<<8U)|
                    (static_cast<std::uint32_t>(block[6])<<16U)|
                    (static_cast<std::uint32_t>(block[7])<<24U);
                for(std::uint32_t py=0;py<4U;++py)for(std::uint32_t px=0;px<4U;++px)
                {
                    const std::uint32_t x=blockX*4U+px,y=blockY*4U+py;
                    if(x>=width||y>=height)continue;
                    const auto& color=colors[(selectors>>(2U*(py*4U+px)))&3U];
                    std::memcpy(rgba.data()+(static_cast<std::size_t>(y)*width+x)*4U,color.data(),4U);
                }
            }
        }
        return true;
    }
    bool CreateFallbackMask(engine::graphics::RenderDevice& device,engine::graphics::TextureHandle& output)
    {
        const std::array<std::byte,4> pixel{};engine::graphics::TextureDesc desc{};engine::graphics::TextureSubresourceData data{pixel.data(),pixel.size(),4U,4U};return engine::graphics::Succeeded(device.CreateTexture(desc,&data,1U,output));
    }
    bool CreateEditableMaskTexture(
        engine::graphics::RenderDevice& device,
        const std::vector<std::byte>& rgba,
        const std::uint32_t width,
        const std::uint32_t height,
        engine::graphics::TextureHandle& output)
    {
        if(rgba.size()!=static_cast<std::size_t>(width)*height*4U)return false;
        engine::graphics::TextureDesc desc{};
        desc.width=width;desc.height=height;
        desc.format=engine::graphics::Format::R8G8B8A8UNorm;
        desc.bindFlags=engine::graphics::TextureBindFlags::ShaderResource;
        engine::graphics::TextureSubresourceData data{
            rgba.data(),rgba.size(),static_cast<std::size_t>(width)*4U,rgba.size()};
        return engine::graphics::Succeeded(device.CreateTexture(desc,&data,1U,output));
    }
    bool CreateDdsFileTexture(engine::graphics::RenderDevice& device,const std::filesystem::path& path,engine::graphics::TextureHandle& output)
    {
        std::error_code ec;const auto size=std::filesystem::file_size(path,ec);if(ec||size==0U||size>512U*1024U*1024U)return false;engine::assets::AssetData data;if(engine::assets::Failed(data.Resize(static_cast<std::size_t>(size))))return false;std::ifstream stream(path,std::ios::binary);if(!stream||!stream.read(reinterpret_cast<char*>(data.GetData()),static_cast<std::streamsize>(size)))return false;engine::assets::TextureAsset decoded;engine::assets::DdsTextureDecodeOptions options{};options.forceSrgb=true;if(engine::assets::Failed(engine::assets::DdsTextureDecoder::Decode(data,options,decoded)))return false;std::vector<engine::graphics::TextureSubresourceData> initial(decoded.GetSubresourceCount());for(std::size_t i=0;i<initial.size();++i)if(engine::assets::Failed(decoded.GetSubresourceData(i,initial[i])))return false;return engine::graphics::Succeeded(device.CreateTexture(decoded.GetDesc(),initial.data(),initial.size(),output));
    }}
    class EditorTerrainRenderer::Impl
    {
    public:
        bool Initialize(engine::graphics::RenderDevice& d) noexcept
        {
            device_=&d;
            if(terrainPath_.empty())return true;engine::assets::TerrainAsset terrain;
            if(engine::assets::Failed(engine::assets::TerrainAsset::Load(terrainPath_,terrain)))return false;
            // Keep every source height sample. LOD is selected per chunk through the
            // index buffer; the source geometry itself is never destructively reduced.
            constexpr std::uint32_t step=1U;
            const std::uint32_t vx=terrain.width;
            const std::uint32_t vz=terrain.height;
            std::vector<Vertex> vertices(static_cast<std::size_t>(vx)*vz);
            for(std::uint32_t z=0;z<vz;++z)for(std::uint32_t x=0;x<vx;++x)
            {
                const auto sx=(std::min)(x*step,terrain.width-1U),sz=(std::min)(z*step,terrain.height-1U);
                const float hl=terrain.GetHeight(sx>step?sx-step:sx,sz),hr=terrain.GetHeight((std::min)(sx+step,terrain.width-1U),sz);
                const float hd=terrain.GetHeight(sx,sz>step?sz-step:sz),hu=terrain.GetHeight(sx,(std::min)(sz+step,terrain.height-1U));
                DirectX::XMVECTOR n=DirectX::XMVector3Normalize(DirectX::XMVectorSet(hl-hr,2.0F*step*terrain.tileSize,hd-hu,0));
                auto& v=vertices[static_cast<std::size_t>(z)*vx+x];v.position={sx*terrain.tileSize,terrain.GetHeight(sx,sz),sz*terrain.tileSize};DirectX::XMStoreFloat3(&v.normal,n);v.uv={static_cast<float>(sx)/static_cast<float>(terrain.width-1U),static_cast<float>(sz)/static_cast<float>(terrain.height-1U)};
            }
            std::vector<std::uint32_t> indices;
            constexpr std::uint32_t chunkCells=64U;
            chunks_.clear();
            for(std::uint32_t cz=0;cz<vz-1U;cz+=chunkCells)for(std::uint32_t cx=0;cx<vx-1U;cx+=chunkCells)
            {
                Chunk chunk;
                const std::uint32_t endX=(std::min)(cx+chunkCells,vx-1U),endZ=(std::min)(cz+chunkCells,vz-1U);
                float minimumHeight=terrain.GetHeight(cx,cz);
                float maximumHeight=minimumHeight;
                for(std::uint32_t z=cz;z<=endZ;++z)for(std::uint32_t x=cx;x<=endX;++x)
                {
                    const float height=terrain.GetHeight(x,z);
                    minimumHeight=(std::min)(minimumHeight,height);
                    maximumHeight=(std::max)(maximumHeight,height);
                }
                chunk.minimum={cx*terrain.tileSize,minimumHeight,cz*terrain.tileSize};
                chunk.maximum={endX*terrain.tileSize,maximumHeight,endZ*terrain.tileSize};
                chunk.center={(chunk.minimum.x+chunk.maximum.x)*0.5F,(minimumHeight+maximumHeight)*0.5F,(chunk.minimum.z+chunk.maximum.z)*0.5F};

                const float skirtDepth=(std::max)(20.0F,terrain.heightScale*0.025F);
                std::array<std::vector<std::uint32_t>,4> skirtBottom{};
                const auto appendSkirtVertex=[&](std::uint32_t sourceIndex)
                {
                    Vertex skirt=vertices[sourceIndex];
                    skirt.position.y-=skirtDepth;
                    const auto index=static_cast<std::uint32_t>(vertices.size());
                    vertices.push_back(skirt);
                    return index;
                };
                for(std::uint32_t x=cx;x<=endX;++x)
                {
                    skirtBottom[0].push_back(appendSkirtVertex(cz*vx+x));
                    skirtBottom[1].push_back(appendSkirtVertex(endZ*vx+x));
                }
                for(std::uint32_t z=cz;z<=endZ;++z)
                {
                    skirtBottom[2].push_back(appendSkirtVertex(z*vx+cx));
                    skirtBottom[3].push_back(appendSkirtVertex(z*vx+endX));
                }
                for(std::uint32_t lod=0;lod<3U;++lod)
                {
                    const std::uint32_t stride=1U<<lod;
                    chunk.first[lod]=static_cast<std::uint32_t>(indices.size());
                    for(std::uint32_t z=cz;z<endZ;z+=stride)
                    {
                        const std::uint32_t nextZ=(std::min)(z+stride,endZ);
                        for(std::uint32_t x=cx;x<endX;x+=stride)
                        {
                            const std::uint32_t nextX=(std::min)(x+stride,endX);
                            const auto a=z*vx+x;
                            const auto b=z*vx+nextX;
                            const auto c=nextZ*vx+x;
                            const auto diagonal=nextZ*vx+nextX;
                            indices.insert(indices.end(),{a,c,b,b,c,diagonal});
                        }
                    }

                    const auto addSkirt=[&](std::uint32_t length,const auto topIndex,const std::vector<std::uint32_t>& bottom)
                    {
                        for(std::uint32_t edge=0;edge<length;edge+=stride)
                        {
                            const std::uint32_t next=(std::min)(edge+stride,length);
                            const std::uint32_t topA=topIndex(edge);
                            const std::uint32_t topB=topIndex(next);
                            indices.insert(indices.end(),{topA,bottom[edge],topB,topB,bottom[edge],bottom[next]});
                        }
                    };
                    addSkirt(endX-cx,[&](std::uint32_t edge){return cz*vx+cx+edge;},skirtBottom[0]);
                    addSkirt(endX-cx,[&](std::uint32_t edge){return endZ*vx+cx+edge;},skirtBottom[1]);
                    addSkirt(endZ-cz,[&](std::uint32_t edge){return (cz+edge)*vx+cx;},skirtBottom[2]);
                    addSkirt(endZ-cz,[&](std::uint32_t edge){return (cz+edge)*vx+endX;},skirtBottom[3]);
                    chunk.count[lod]=static_cast<std::uint32_t>(indices.size())-chunk.first[lod];
                }
                chunks_.push_back(chunk);
            }
            indexCount_=static_cast<std::uint32_t>(indices.size());
            engine::graphics::BufferDesc bd{};bd.byteSize=vertices.size()*sizeof(Vertex);bd.stride=sizeof(Vertex);bd.bindFlags=engine::graphics::BufferBindFlags::Vertex;engine::graphics::BufferInitialData vi{reinterpret_cast<const std::byte*>(vertices.data()),bd.byteSize};
            if(engine::graphics::Failed(d.CreateBuffer(bd,&vi,vertex_)))return false;
            bd.byteSize=indices.size()*sizeof(std::uint32_t);bd.stride=sizeof(std::uint32_t);bd.bindFlags=engine::graphics::BufferBindFlags::Index;bd.indexFormat=engine::graphics::IndexFormat::UInt32;engine::graphics::BufferInitialData ii{reinterpret_cast<const std::byte*>(indices.data()),bd.byteSize};
            if(engine::graphics::Failed(d.CreateBuffer(bd,&ii,index_)))return false;
            Microsoft::WRL::ComPtr<ID3DBlob> vs,ps;if(!CompileEditorShaderFile(L"Terrain.hlsl","VSMain","vs_5_0","LTS.Editor.Terrain",vs)||!CompileEditorShaderFile(L"Terrain.hlsl","PSMain","ps_5_0","LTS.Editor.Terrain",ps))return false;
            engine::graphics::ShaderDesc sd{};sd.stage=engine::graphics::ShaderStage::Vertex;sd.bytecode={vs->GetBufferPointer(),vs->GetBufferSize()};if(engine::graphics::Failed(d.CreateShader(sd,vertexShader_)))return false;sd.stage=engine::graphics::ShaderStage::Pixel;sd.bytecode={ps->GetBufferPointer(),ps->GetBufferSize()};if(engine::graphics::Failed(d.CreateShader(sd,pixelShader_)))return false;
            const std::array<engine::graphics::VertexElementDesc,3> elements{{{"POSITION",0,engine::graphics::Format::R32G32B32Float,0,0,engine::graphics::VertexInputRate::PerVertex,0},{"NORMAL",0,engine::graphics::Format::R32G32B32Float,0,12,engine::graphics::VertexInputRate::PerVertex,0},{"TEXCOORD",0,engine::graphics::Format::R32G32Float,0,24,engine::graphics::VertexInputRate::PerVertex,0}}};engine::graphics::InputLayoutDesc ild{};ild.vertexShader=vertexShader_;ild.elements=elements.data();ild.elementCount=elements.size();if(engine::graphics::Failed(d.CreateInputLayout(ild,layout_)))return false;
            bd={};bd.byteSize=sizeof(Constants);bd.bindFlags=engine::graphics::BufferBindFlags::Constant;if(engine::graphics::Failed(d.CreateBuffer(bd,nullptr,constants_)))return false;
            engine::graphics::GraphicsPipelineDesc pd{};pd.vertexShader=vertexShader_;pd.pixelShader=pixelShader_;pd.inputLayout=layout_;pd.topology=engine::graphics::PrimitiveTopology::TriangleList;pd.rasterizer.cullMode=engine::graphics::CullMode::None;pd.depthStencil.depthEnable=true;pd.depthStencil.depthWriteEnable=true;pd.depthStencil.depthFunction=engine::graphics::ComparisonFunction::LessEqual;if(engine::graphics::Failed(d.CreateGraphicsPipeline(pd,pipeline_)))return false;
            Microsoft::WRL::ComPtr<ID3DBlob> brushVs,brushPs;
            if(!CompileEditorShaderFile(L"Grid.hlsl","VSMain","vs_5_0","LTS.Editor.TerrainBrush",brushVs)||
               !CompileEditorShaderFile(L"Grid.hlsl","PSMain","ps_5_0","LTS.Editor.TerrainBrush",brushPs))return false;
            sd={};sd.stage=engine::graphics::ShaderStage::Vertex;sd.bytecode={brushVs->GetBufferPointer(),brushVs->GetBufferSize()};
            if(engine::graphics::Failed(d.CreateShader(sd,brushVertexShader_)))return false;
            sd.stage=engine::graphics::ShaderStage::Pixel;sd.bytecode={brushPs->GetBufferPointer(),brushPs->GetBufferSize()};
            if(engine::graphics::Failed(d.CreateShader(sd,brushPixelShader_)))return false;
            const std::array<engine::graphics::VertexElementDesc,2> brushElements{{
                {"POSITION",0,engine::graphics::Format::R32G32B32Float,0,0,engine::graphics::VertexInputRate::PerVertex,0},
                {"COLOR",0,engine::graphics::Format::R32G32B32A32Float,0,12,engine::graphics::VertexInputRate::PerVertex,0}}};
            engine::graphics::InputLayoutDesc brushLayoutDesc{};brushLayoutDesc.vertexShader=brushVertexShader_;brushLayoutDesc.elements=brushElements.data();brushLayoutDesc.elementCount=brushElements.size();
            if(engine::graphics::Failed(d.CreateInputLayout(brushLayoutDesc,brushLayout_)))return false;
            bd={};bd.byteSize=256U*sizeof(BrushVertex);bd.stride=sizeof(BrushVertex);bd.bindFlags=engine::graphics::BufferBindFlags::Vertex;
            if(engine::graphics::Failed(d.CreateBuffer(bd,nullptr,brushVertexBuffer_)))return false;
            bd={};bd.byteSize=sizeof(BrushConstants);bd.bindFlags=engine::graphics::BufferBindFlags::Constant;
            if(engine::graphics::Failed(d.CreateBuffer(bd,nullptr,brushConstants_)))return false;
            engine::graphics::GraphicsPipelineDesc brushPipelineDesc{};
            brushPipelineDesc.vertexShader=brushVertexShader_;brushPipelineDesc.pixelShader=brushPixelShader_;
            brushPipelineDesc.inputLayout=brushLayout_;brushPipelineDesc.topology=engine::graphics::PrimitiveTopology::LineList;
            brushPipelineDesc.rasterizer.cullMode=engine::graphics::CullMode::None;
            brushPipelineDesc.depthStencil.depthEnable=true;brushPipelineDesc.depthStencil.depthWriteEnable=false;
            brushPipelineDesc.depthStencil.depthFunction=engine::graphics::ComparisonFunction::LessEqual;
            if(engine::graphics::Failed(d.CreateGraphicsPipeline(brushPipelineDesc,brushPipeline_)))return false;

            maskWidth_ = terrain.splatWidth;
            maskHeight_ = terrain.splatHeight;

            /*
             * Реальные маски должны успешно декодироваться.
             * Fallback используется только для свободных GPU slots.
             */
            bool maskDimensionsInitialized = false;

            for (std::size_t index = 0U;
                 index < terrain.masks.size();
                 ++index)
            {
                std::uint32_t decodedWidth = 0U;
                std::uint32_t decodedHeight = 0U;

                if (!DecodeEmbeddedMask(
                        terrain,
                        terrain.masks[index],
                        maskPixels_[index],
                        decodedWidth,
                        decodedHeight) ||
                    decodedWidth == 0U ||
                    decodedHeight == 0U)
                {
                    return false;
                }

                if (!maskDimensionsInitialized)
                {
                    maskWidth_ = decodedWidth;
                    maskHeight_ = decodedHeight;
                    maskDimensionsInitialized = true;
                }
                else if (
                    decodedWidth != maskWidth_ ||
                    decodedHeight != maskHeight_)
                {
                    /*
                     * Нельзя смешивать masks разных размеров.
                     */
                    return false;
                }
            }

            if (maskWidth_ == 0U ||
                maskHeight_ == 0U)
            {
                return false;
            }

            const std::size_t maskByteCount =
                static_cast<std::size_t>(
                    maskWidth_) *
                static_cast<std::size_t>(
                    maskHeight_) *
                4U;

            for (std::size_t index =
                     terrain.masks.size();
                 index < maskPixels_.size();
                 ++index)
            {
                maskPixels_[index].assign(
                    maskByteCount,
                    std::byte{0});
            }

            LoadPaintData(terrain.sourcePath);
            for(std::size_t i=0;i<masks_.size();++i)
                if(!CreateEditableMaskTexture(d,maskPixels_[i],maskWidth_,maskHeight_,masks_[i]))return false;
            std::filesystem::path workspace=std::filesystem::current_path();if(workspace.filename()==L"game")workspace=workspace.parent_path();workspaceRoot_=workspace;for(std::size_t i=0;i<materials_.size();++i){bool created=false;if(i<terrain.layers.size()){materialPaths_[i]=terrain.layers[i].diffusePath;const auto logical=std::filesystem::u8path(materialPaths_[i]);created=CreateDdsFileTexture(d,workspace/L"game"/logical,materials_[i]);if(!created)created=CreateDdsFileTexture(d,workspace/L"bin"/logical,materials_[i]);}if(!created&&!CreateFallbackMask(d,materials_[i]))return false;}
            engine::graphics::SamplerDesc samplerDesc{};samplerDesc.addressU=engine::graphics::TextureAddressMode::Wrap;samplerDesc.addressV=engine::graphics::TextureAddressMode::Wrap;if(engine::graphics::Failed(d.CreateSampler(samplerDesc,sampler_)))return false;terrainAsset_=std::move(terrain);loaded_=true;return true;
        }
        void Shutdown(engine::graphics::RenderDevice& d) noexcept{for(auto& texture:materials_){if(texture.IsValid())static_cast<void>(d.DestroyTexture(texture));texture={};}for(auto& mask:masks_){if(mask.IsValid())static_cast<void>(d.DestroyTexture(mask));mask={};}if(sampler_.IsValid())static_cast<void>(d.DestroySampler(sampler_));sampler_={};if(brushPipeline_.IsValid())static_cast<void>(d.DestroyGraphicsPipeline(brushPipeline_));if(brushLayout_.IsValid())static_cast<void>(d.DestroyInputLayout(brushLayout_));if(brushVertexShader_.IsValid())static_cast<void>(d.DestroyShader(brushVertexShader_));if(brushPixelShader_.IsValid())static_cast<void>(d.DestroyShader(brushPixelShader_));if(brushConstants_.IsValid())static_cast<void>(d.DestroyBuffer(brushConstants_));if(brushVertexBuffer_.IsValid())static_cast<void>(d.DestroyBuffer(brushVertexBuffer_));if(pipeline_.IsValid())static_cast<void>(d.DestroyGraphicsPipeline(pipeline_));if(layout_.IsValid())static_cast<void>(d.DestroyInputLayout(layout_));if(vertexShader_.IsValid())static_cast<void>(d.DestroyShader(vertexShader_));if(pixelShader_.IsValid())static_cast<void>(d.DestroyShader(pixelShader_));if(constants_.IsValid())static_cast<void>(d.DestroyBuffer(constants_));if(index_.IsValid())static_cast<void>(d.DestroyBuffer(index_));if(vertex_.IsValid())static_cast<void>(d.DestroyBuffer(vertex_));brushPipeline_={};brushLayout_={};brushVertexShader_={};brushPixelShader_={};brushConstants_={};brushVertexBuffer_={};pipeline_={};layout_={};vertexShader_={};pixelShader_={};constants_={};index_={};vertex_={};terrainAsset_={};for(auto& pixels:maskPixels_)pixels.clear();activePaintBefore_.clear();paintUndo_.clear();paintRedo_.clear();paintPath_.clear();maskWidth_=0U;maskHeight_=0U;paintStrokeActive_=false;indexCount_=0;loaded_=false;}
        engine::graphics::GraphicsResult Render(engine::graphics::CommandContext& c,const EditorSceneDocument& document,const DirectX::XMFLOAT4X4& vp,const DirectX::XMFLOAT3& camera) noexcept
        {
            if(!loaded_)return engine::graphics::GraphicsResult::Success;const EditorSceneEntity* actor=nullptr;for(const auto& e:document.GetEntities())if(e.terrain.has_value()&&e.terrain->visible){actor=&e;break;}if(actor==nullptr)return engine::graphics::GraphicsResult::Success;
            if(device_!=nullptr){for(std::size_t i=0;i<materials_.size()&&i<actor->terrain->layers.size();++i){const std::string& path=actor->terrain->layers[i].diffusePath;if(path==materialPaths_[i])continue;if(materials_[i].IsValid())static_cast<void>(device_->DestroyTexture(materials_[i]));materials_[i]={};const auto logical=std::filesystem::u8path(path);bool created=CreateDdsFileTexture(*device_,workspaceRoot_/L"game"/logical,materials_[i]);if(!created)created=CreateDdsFileTexture(*device_,workspaceRoot_/L"bin"/logical,materials_[i]);if(!created)static_cast<void>(CreateFallbackMask(*device_,materials_[i]));materialPaths_[i]=path;}}

            Constants constants{};

            const DirectX::XMMATRIX worldMatrix =
                DirectX::XMMatrixScaling(
                    actor->transform.scale[0],
                    actor->transform.scale[1],
                    actor->transform.scale[2]) *
                DirectX::XMMatrixRotationRollPitchYaw(
                    DirectX::XMConvertToRadians(
                        actor->transform.rotationDegrees[0]),
                    DirectX::XMConvertToRadians(
                        actor->transform.rotationDegrees[1]),
                    DirectX::XMConvertToRadians(
                        actor->transform.rotationDegrees[2])) *
                DirectX::XMMatrixTranslation(
                    actor->transform.position[0],
                    actor->transform.position[1],
                    actor->transform.position[2]);

            DirectX::XMStoreFloat4x4(
                &constants.world,
                worldMatrix);

            constants.viewProjection =
                vp;

            /*
             * Все отсутствующие slots выключены.
             */
            for (std::size_t index = 0U;
                 index < constants.placement.size();
                 ++index)
            {
                constants.placement[index] =
                {
                    1.0F,
                    1.0F,
                    0.0F,
                    0.0F
                };

                constants.layerParameters[index] =
                {
                    0.0F,
                    0.0F,
                    0.0F,
                    0.0F
                };
            }

            const bool hasSceneOverrides =
                !actor->terrain->layers.empty();

            const std::size_t sourceLayerCount =
                hasSceneOverrides
                    ? actor->terrain->layers.size()
                    : terrainAsset_.layers.size();

            const std::size_t layerCount =
                (std::min)(
                    sourceLayerCount,
                    constants.placement.size());

            for (std::size_t index = 0U;
                 index < layerCount;
                 ++index)
            {
                if (hasSceneOverrides)
                {
                    const auto& layer =
                        actor->terrain->layers[index];

                    constants.placement[index] =
                    {
                        layer.scaleU,
                        layer.scaleV,
                        layer.offsetU,
                        layer.offsetV
                    };

                    constants.layerParameters[index].x =
                        layer.visible
                            ? 1.0F
                            : 0.0F;
                }
                else
                {
                    const auto& layer =
                        terrainAsset_.layers[index];

                    constants.placement[index] =
                    {
                        layer.scaleU,
                        layer.scaleV,
                        0.0F,
                        0.0F
                    };

                    constants.layerParameters[index].x =
                        1.0F;
                }
            }

            const float terrainWidth =
                static_cast<float>(
                    terrainAsset_.width - 1U) *
                terrainAsset_.tileSize;

            const float terrainDepth =
                static_cast<float>(
                    terrainAsset_.height - 1U) *
                terrainAsset_.tileSize;

            constants.terrainInfo =
            {
                terrainWidth,
                terrainDepth,
                static_cast<float>(layerCount),
                0.0F
            };

            auto r = c.UpdateBuffer(
            constants_,
            &constants,
            sizeof(constants));

            if(engine::graphics::Failed(r))return r;r=c.SetGraphicsPipeline(pipeline_);if(engine::graphics::Failed(r))return r;engine::graphics::VertexBufferBinding vb{vertex_,sizeof(Vertex),0};engine::graphics::IndexBufferBinding ib{index_,0};r=c.SetVertexBuffers(0,&vb,1);if(!engine::graphics::Failed(r))r=c.SetIndexBuffer(ib);if(!engine::graphics::Failed(r))r=c.SetConstantBuffers(engine::graphics::ShaderStage::Vertex,0,&constants_,1);if(!engine::graphics::Failed(r))r=c.SetConstantBuffers(engine::graphics::ShaderStage::Pixel,0,&constants_,1);if(!engine::graphics::Failed(r))r=c.SetShaderResources(engine::graphics::ShaderStage::Pixel,0,masks_.data(),masks_.size());if(!engine::graphics::Failed(r))r=c.SetShaderResources(engine::graphics::ShaderStage::Pixel,6,materials_.data(),materials_.size());if(!engine::graphics::Failed(r))r=c.SetSamplers(engine::graphics::ShaderStage::Pixel,0,&sampler_,1);
            if(!engine::graphics::Failed(r))
            {
                const auto viewProjectionMatrix=DirectX::XMLoadFloat4x4(&vp);
                const auto worldViewProjection=worldMatrix*viewProjectionMatrix;
                for(const Chunk& chunk:chunks_)
                {
                    if(!IsChunkVisible(chunk,worldViewProjection))continue;
                    const auto localCenter=DirectX::XMLoadFloat3(&chunk.center);
                    DirectX::XMFLOAT3 worldCenter{};
                    DirectX::XMStoreFloat3(&worldCenter,DirectX::XMVector3TransformCoord(localCenter,worldMatrix));
                    const float dx=worldCenter.x-camera.x,dy=worldCenter.y-camera.y,dz=worldCenter.z-camera.z;
                    const float distanceSquared=dx*dx+dy*dy+dz*dz;
                    const std::size_t lod=distanceSquared<1200.0F*1200.0F?0U:(distanceSquared<2800.0F*2800.0F?1U:2U);
                    r=c.DrawIndexed(chunk.count[lod],chunk.first[lod],0);
                    if(engine::graphics::Failed(r))break;
                }
            }
            static_cast<void>(c.UnbindShaderResources(engine::graphics::ShaderStage::Pixel,0,24));c.UnbindIndexBuffer();c.UnbindGraphicsPipeline();return r;
        }
        engine::graphics::GraphicsResult RenderBrush(
            engine::graphics::CommandContext& context,
            const EditorSceneDocument& document,
            const DirectX::XMFLOAT4X4& viewProjection,
            const float worldX,const float worldZ,const float radius,
            const bool erase)noexcept
        {
            if(!loaded_||!brushPipeline_.IsValid()||radius<=0.0F)
                return engine::graphics::GraphicsResult::Success;
            constexpr std::uint32_t segments=96U;
            std::array<BrushVertex,segments*2U> vertices{};
            const DirectX::XMFLOAT4 color=erase?
                DirectX::XMFLOAT4{0.95F,0.18F,0.12F,1.0F}:
                DirectX::XMFLOAT4{1.0F,0.62F,0.08F,1.0F};
            std::uint32_t vertexCount=0U;
            for(std::uint32_t segment=0;segment<segments;++segment)
            {
                const float angle0=DirectX::XM_2PI*static_cast<float>(segment)/segments;
                const float angle1=DirectX::XM_2PI*static_cast<float>(segment+1U)/segments;
                const float x0=worldX+std::cos(angle0)*radius,z0=worldZ+std::sin(angle0)*radius;
                const float x1=worldX+std::cos(angle1)*radius,z1=worldZ+std::sin(angle1)*radius;
                float y0=0.0F,y1=0.0F;
                if(!TryGetSurfaceHeight(document,x0,z0,y0)||!TryGetSurfaceHeight(document,x1,z1,y1))
                    continue;
                vertices[vertexCount++]={{x0,y0+0.35F,z0},color};
                vertices[vertexCount++]={{x1,y1+0.35F,z1},color};
            }
            if(vertexCount==0U)return engine::graphics::GraphicsResult::Success;
            BrushConstants constants{viewProjection};
            auto result=context.UpdateBuffer(brushVertexBuffer_,vertices.data(),sizeof(vertices));
            if(engine::graphics::Failed(result))return result;
            result=context.UpdateBuffer(brushConstants_,&constants,sizeof(constants));
            if(engine::graphics::Failed(result))return result;
            result=context.SetGraphicsPipeline(brushPipeline_);
            engine::graphics::VertexBufferBinding binding{brushVertexBuffer_,sizeof(BrushVertex),0U};
            if(!engine::graphics::Failed(result))result=context.SetVertexBuffers(0U,&binding,1U);
            if(!engine::graphics::Failed(result))result=context.SetConstantBuffers(
                engine::graphics::ShaderStage::Vertex,0U,&brushConstants_,1U);
            if(!engine::graphics::Failed(result))result=context.Draw(vertexCount,0U);
            static_cast<void>(context.UnbindConstantBuffers(engine::graphics::ShaderStage::Vertex,0U,1U));
            context.UnbindGraphicsPipeline();return result;
        }
        bool LoadTerrain(engine::graphics::RenderDevice& d,const std::filesystem::path& path)noexcept{Shutdown(d);terrainPath_=path;const bool loaded=Initialize(d);engine::core::GetLogger().Write(loaded?engine::core::LogLevel::Information:engine::core::LogLevel::Error,"LTS.Editor.Terrain",loaded?(path.empty()?"Terrain cleared.":"Terrain GPU resources loaded."):"Failed to load terrain GPU resources.");return loaded;}
        [[nodiscard]]bool HasTerrain()const noexcept{return loaded_;}
        [[nodiscard]] bool TryGetSurfaceHeight(
            const EditorSceneDocument& document,
            const float worldX,
            const float worldZ,
            float& worldHeight) const noexcept
        {
            if (!loaded_ || !terrainAsset_.IsValid()) return false;

            const EditorSceneEntity* actor = nullptr;
            for (const auto& entity : document.GetEntities())
            {
                if (entity.terrain.has_value() && entity.terrain->visible)
                {
                    actor = &entity;
                    break;
                }
            }
            if (actor == nullptr ||
                std::abs(actor->transform.scale[0]) < 0.00001F ||
                std::abs(actor->transform.scale[2]) < 0.00001F)
                return false;

            // Terrain editing currently keeps the heightfield aligned to world X/Z.
            // Scale and translation are still respected by placement.
            const float localX =
                (worldX - actor->transform.position[0]) / actor->transform.scale[0];
            const float localZ =
                (worldZ - actor->transform.position[2]) / actor->transform.scale[2];
            const float sampleX = localX / terrainAsset_.tileSize;
            const float sampleZ = localZ / terrainAsset_.tileSize;
            if (sampleX < 0.0F || sampleZ < 0.0F ||
                sampleX > static_cast<float>(terrainAsset_.width - 1U) ||
                sampleZ > static_cast<float>(terrainAsset_.height - 1U))
                return false;

            const auto x0 = static_cast<std::uint32_t>(std::floor(sampleX));
            const auto z0 = static_cast<std::uint32_t>(std::floor(sampleZ));
            const auto x1 = (std::min)(x0 + 1U, terrainAsset_.width - 1U);
            const auto z1 = (std::min)(z0 + 1U, terrainAsset_.height - 1U);
            const float tx = sampleX - static_cast<float>(x0);
            const float tz = sampleZ - static_cast<float>(z0);
            const float h0 = terrainAsset_.GetHeight(x0, z0) * (1.0F - tx) +
                terrainAsset_.GetHeight(x1, z0) * tx;
            const float h1 = terrainAsset_.GetHeight(x0, z1) * (1.0F - tx) +
                terrainAsset_.GetHeight(x1, z1) * tx;
            const float localHeight = h0 * (1.0F - tz) + h1 * tz;
            worldHeight = actor->transform.position[1] +
                localHeight * actor->transform.scale[1];
            return true;
        }

        [[nodiscard]] bool BeginPaintStroke() noexcept
        {
            if(!loaded_||paintStrokeActive_)return false;
            activePaintBefore_.clear();
            paintStrokeActive_=true;
            return true;
        }

        [[nodiscard]] bool Paint(
            const EditorSceneDocument& document,
            const float worldX,
            const float worldZ,
            const float radius,
            const float strength,
            const float falloff,
            const std::size_t layerIndex,
            const bool erase) noexcept
        {
            if(!paintStrokeActive_||layerIndex>=18U||maskWidth_==0U||maskHeight_==0U)return false;
            const EditorSceneEntity* actor=nullptr;
            for(const auto& entity:document.GetEntities())
                if(entity.terrain.has_value()&&entity.terrain->visible){actor=&entity;break;}
            if(actor==nullptr||std::abs(actor->transform.scale[0])<0.00001F||
               std::abs(actor->transform.scale[2])<0.00001F)return false;

            const float terrainWidth=(terrainAsset_.width-1U)*terrainAsset_.tileSize*actor->transform.scale[0];
            const float terrainDepth=(terrainAsset_.height-1U)*terrainAsset_.tileSize*actor->transform.scale[2];
            const float u=(worldX-actor->transform.position[0])/terrainWidth;
            const float v=(worldZ-actor->transform.position[2])/terrainDepth;
            if(u<0.0F||v<0.0F||u>1.0F||v>1.0F)return false;
            const float centerX=u*static_cast<float>(maskWidth_-1U);
            const float centerY=v*static_cast<float>(maskHeight_-1U);
            const float pixelsPerWorldX=static_cast<float>(maskWidth_-1U)/std::abs(terrainWidth);
            const float pixelsPerWorldZ=static_cast<float>(maskHeight_-1U)/std::abs(terrainDepth);
            const float radiusPixels=(std::max)(1.0F,radius*(pixelsPerWorldX+pixelsPerWorldZ)*0.5F);
            const int minimumX=(std::max)(0,static_cast<int>(std::floor(centerX-radiusPixels)));
            const int maximumX=(std::min)(static_cast<int>(maskWidth_-1U),static_cast<int>(std::ceil(centerX+radiusPixels)));
            const int minimumY=(std::max)(0,static_cast<int>(std::floor(centerY-radiusPixels)));
            const int maximumY=(std::min)(static_cast<int>(maskHeight_-1U),static_cast<int>(std::ceil(centerY+radiusPixels)));
            bool changed=false;
            for(int y=minimumY;y<=maximumY;++y)for(int x=minimumX;x<=maximumX;++x)
            {
                const float dx=static_cast<float>(x)-centerX,dy=static_cast<float>(y)-centerY;
                const float normalizedDistance=std::sqrt(dx*dx+dy*dy)/radiusPixels;
                if(normalizedDistance>1.0F)continue;
                const float hardPart=std::clamp(1.0F-falloff,0.0F,1.0F);
                const float influence=normalizedDistance<=hardPart?1.0F:
                    1.0F-(normalizedDistance-hardPart)/(std::max)(falloff,0.0001F);
                const float amount=std::clamp(strength,0.0F,1.0F)*std::clamp(influence,0.0F,1.0F);
                if(amount<=0.0F)continue;
                const auto pixel=static_cast<std::uint32_t>(y)*maskWidth_+static_cast<std::uint32_t>(x);
                auto weights=ReadWeights(pixel);
                activePaintBefore_.try_emplace(pixel,weights);
                if(layerIndex==0U)
                {
                    for(auto& weight:weights)
                        weight=static_cast<std::uint8_t>(std::lround(weight*(1.0F-amount)));
                }
                else
                {
                    const std::size_t selected=layerIndex-1U;
                    const float target=erase?0.0F:255.0F;
                    const auto newSelected=static_cast<std::uint8_t>(std::clamp(
                        std::lround(weights[selected]+(target-weights[selected])*amount),0L,255L));
                    weights[selected]=newSelected;
                    if(!erase)
                    {
                        std::uint32_t otherTotal=0U;
                        for(std::size_t i=0;i<weights.size();++i)if(i!=selected)otherTotal+=weights[i];
                        const std::uint32_t remaining=255U-newSelected;
                        if(otherTotal>remaining&&otherTotal>0U)
                            for(std::size_t i=0;i<weights.size();++i)if(i!=selected)
                                weights[i]=static_cast<std::uint8_t>(weights[i]*remaining/otherTotal);
                    }
                }
                WriteWeights(pixel,weights);changed=true;
            }
            if(changed)UploadMaskRegion(
                static_cast<std::uint32_t>(minimumX),static_cast<std::uint32_t>(minimumY),
                static_cast<std::uint32_t>(maximumX+1),static_cast<std::uint32_t>(maximumY+1));
            return changed;
        }

        [[nodiscard]] bool EndPaintStroke() noexcept
        {
            if(!paintStrokeActive_)return false;
            paintStrokeActive_=false;
            PaintCommand command;command.reserve(activePaintBefore_.size());
            for(const auto& [pixel,before]:activePaintBefore_)
            {
                const auto after=ReadWeights(pixel);
                if(before!=after)command.push_back({pixel,before,after});
            }
            activePaintBefore_.clear();
            if(command.empty())return false;
            paintUndo_.push_back(std::move(command));
            if(paintUndo_.size()>32U)paintUndo_.erase(paintUndo_.begin());
            paintRedo_.clear();
            SavePaintData();
            return true;
        }

        [[nodiscard]] bool UndoPaint() noexcept
        {
            if(paintUndo_.empty())return false;
            auto command=std::move(paintUndo_.back());paintUndo_.pop_back();
            for(const auto& change:command)WriteWeights(change.pixel,change.before);
            paintRedo_.push_back(std::move(command));RefreshMaskTextures();SavePaintData();return true;
        }

        [[nodiscard]] bool RedoPaint() noexcept
        {
            if(paintRedo_.empty())return false;
            auto command=std::move(paintRedo_.back());paintRedo_.pop_back();
            for(const auto& change:command)WriteWeights(change.pixel,change.after);
            paintUndo_.push_back(std::move(command));RefreshMaskTextures();SavePaintData();return true;
        }

        [[nodiscard]] bool CanUndoPaint()const noexcept{return !paintUndo_.empty();}
        [[nodiscard]] bool CanRedoPaint()const noexcept{return !paintRedo_.empty();}

        [[nodiscard]] std::array<std::uint8_t,17> ReadWeights(const std::uint32_t pixel)const noexcept
        {
            std::array<std::uint8_t,17> weights{};
            for(std::size_t layer=0;layer<weights.size();++layer)
            {
                const std::size_t mask=layer/3U,channel=layer%3U;
                weights[layer]=std::to_integer<std::uint8_t>(maskPixels_[mask][static_cast<std::size_t>(pixel)*4U+channel]);
            }
            return weights;
        }

        void WriteWeights(const std::uint32_t pixel,const std::array<std::uint8_t,17>& weights)noexcept
        {
            for(std::size_t layer=0;layer<weights.size();++layer)
            {
                const std::size_t mask=layer/3U,channel=layer%3U;
                maskPixels_[mask][static_cast<std::size_t>(pixel)*4U+channel]=static_cast<std::byte>(weights[layer]);
            }
        }

        void RefreshMaskTextures()noexcept
        {
            if(device_==nullptr)return;
            for(std::size_t i=0;i<masks_.size();++i)
            {
                if(masks_[i].IsValid())static_cast<void>(device_->DestroyTexture(masks_[i]));
                masks_[i]={};
                static_cast<void>(CreateEditableMaskTexture(*device_,maskPixels_[i],maskWidth_,maskHeight_,masks_[i]));
            }
        }

        void UploadMaskRegion(
            const std::uint32_t left,const std::uint32_t top,
            const std::uint32_t right,const std::uint32_t bottom)noexcept
        {
            auto* dxDevice=static_cast<engine::graphics::d3d11::D3D11Device*>(device_);
            if(dxDevice==nullptr||right<=left||bottom<=top)return;
            ID3D11DeviceContext* context=dxDevice->GetNativeImmediateContext();
            if(context==nullptr)return;
            const D3D11_BOX box{left,top,0U,right,bottom,1U};
            for(std::size_t index=0;index<masks_.size();++index)
            {
                ID3D11Resource* resource=dxDevice->GetNativeTexture(masks_[index]);
                if(resource==nullptr)continue;
                const auto* source=maskPixels_[index].data()+
                    (static_cast<std::size_t>(top)*maskWidth_+left)*4U;
                context->UpdateSubresource(resource,0U,&box,source,maskWidth_*4U,0U);
            }
        }

        void LoadPaintData(const std::filesystem::path& terrainPath)noexcept
        {
            paintPath_=terrainPath;paintPath_+=L".paint";
            std::ifstream stream(paintPath_,std::ios::binary);
            std::uint32_t signature=0U,version=0U,width=0U,height=0U,count=0U;
            if(!stream.read(reinterpret_cast<char*>(&signature),sizeof(signature))||
               !stream.read(reinterpret_cast<char*>(&version),sizeof(version))||
               !stream.read(reinterpret_cast<char*>(&width),sizeof(width))||
               !stream.read(reinterpret_cast<char*>(&height),sizeof(height))||
               !stream.read(reinterpret_cast<char*>(&count),sizeof(count))||
               signature!=0x5053544CU||version!=1U||width!=maskWidth_||height!=maskHeight_||count!=6U)return;
            const std::size_t bytes=static_cast<std::size_t>(width)*height*4U;
            for(auto& mask:maskPixels_)
            {
                std::vector<std::byte> loaded(bytes);
                if(!stream.read(reinterpret_cast<char*>(loaded.data()),static_cast<std::streamsize>(bytes)))return;
                mask=std::move(loaded);
            }
        }

        void SavePaintData()const noexcept
        {
            if(paintPath_.empty())return;
            std::ofstream stream(paintPath_,std::ios::binary|std::ios::trunc);
            const std::uint32_t signature=0x5053544CU,version=1U,count=6U;
            stream.write(reinterpret_cast<const char*>(&signature),sizeof(signature));
            stream.write(reinterpret_cast<const char*>(&version),sizeof(version));
            stream.write(reinterpret_cast<const char*>(&maskWidth_),sizeof(maskWidth_));
            stream.write(reinterpret_cast<const char*>(&maskHeight_),sizeof(maskHeight_));
            stream.write(reinterpret_cast<const char*>(&count),sizeof(count));
            for(const auto& mask:maskPixels_)
                stream.write(reinterpret_cast<const char*>(mask.data()),static_cast<std::streamsize>(mask.size()));
        }
    private:engine::graphics::RenderDevice* device_=nullptr;std::filesystem::path terrainPath_,workspaceRoot_,paintPath_;engine::assets::TerrainAsset terrainAsset_;std::vector<Chunk> chunks_;engine::graphics::BufferHandle vertex_,index_,constants_,brushVertexBuffer_,brushConstants_;engine::graphics::ShaderHandle vertexShader_,pixelShader_,brushVertexShader_,brushPixelShader_;engine::graphics::InputLayoutHandle layout_,brushLayout_;engine::graphics::PipelineStateHandle pipeline_,brushPipeline_;std::array<engine::graphics::TextureHandle,6> masks_{};std::array<std::vector<std::byte>,6> maskPixels_{};std::array<engine::graphics::TextureHandle,18> materials_{};std::array<std::string,18> materialPaths_{};engine::graphics::SamplerHandle sampler_;std::unordered_map<std::uint32_t,std::array<std::uint8_t,17>> activePaintBefore_;std::vector<PaintCommand> paintUndo_,paintRedo_;std::uint32_t maskWidth_=0U,maskHeight_=0U,indexCount_=0;bool paintStrokeActive_=false,loaded_=false;
    };
    EditorTerrainRenderer::EditorTerrainRenderer()noexcept:impl_(std::make_unique<Impl>()){} EditorTerrainRenderer::~EditorTerrainRenderer()noexcept=default;
    bool EditorTerrainRenderer::Initialize(engine::graphics::RenderDevice& d)noexcept{return impl_->Initialize(d);}void EditorTerrainRenderer::Shutdown(engine::graphics::RenderDevice& d)noexcept{impl_->Shutdown(d);}engine::graphics::GraphicsResult EditorTerrainRenderer::Render(engine::graphics::CommandContext& c,const EditorSceneDocument& doc,const DirectX::XMFLOAT4X4& vp,const DirectX::XMFLOAT3& camera)noexcept{return impl_->Render(c,doc,vp,camera);}
    bool EditorTerrainRenderer::LoadTerrain(engine::graphics::RenderDevice& d,const std::filesystem::path& p)noexcept{return impl_->LoadTerrain(d,p);}bool EditorTerrainRenderer::HasTerrain()const noexcept{return impl_->HasTerrain();}
    bool EditorTerrainRenderer::TryGetSurfaceHeight(const EditorSceneDocument& document,const float worldX,const float worldZ,float& worldHeight)const noexcept{return impl_->TryGetSurfaceHeight(document,worldX,worldZ,worldHeight);}
    bool EditorTerrainRenderer::BeginPaintStroke()noexcept{return impl_->BeginPaintStroke();}
    bool EditorTerrainRenderer::Paint(const EditorSceneDocument& document,const float worldX,const float worldZ,const float radius,const float strength,const float falloff,const std::size_t layerIndex,const bool erase)noexcept{return impl_->Paint(document,worldX,worldZ,radius,strength,falloff,layerIndex,erase);}
    bool EditorTerrainRenderer::EndPaintStroke()noexcept{return impl_->EndPaintStroke();}
    bool EditorTerrainRenderer::UndoPaint()noexcept{return impl_->UndoPaint();}
    bool EditorTerrainRenderer::RedoPaint()noexcept{return impl_->RedoPaint();}
    bool EditorTerrainRenderer::CanUndoPaint()const noexcept{return impl_->CanUndoPaint();}
    bool EditorTerrainRenderer::CanRedoPaint()const noexcept{return impl_->CanRedoPaint();}
    engine::graphics::GraphicsResult EditorTerrainRenderer::RenderBrush(engine::graphics::CommandContext& context,const EditorSceneDocument& document,const DirectX::XMFLOAT4X4& viewProjection,const float worldX,const float worldZ,const float radius,const bool erase)noexcept{return impl_->RenderBrush(context,document,viewProjection,worldX,worldZ,radius,erase);}
}
