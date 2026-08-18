#include "Assets/AssetLoaderRegistry.h"
#include "Assets/AssetManager.h"
#include "Assets/AssetMetadata.h"
#include "Assets/AssetPath.h"
#include "Assets/AssetRegistry.h"
#include "Assets/AssetSource.h"
#include "Assets/DdsTextureLoader.h"
#include "Assets/GpuMesh.h"
#include "Assets/MaterialAssetLoader.h"
#include "Assets/MeshAssetBuilder.h"
#include "Assets/MeshAssetLoader.h"
#include "Assets/LtsMeshWriter.h"
#include "Assets/LtsMaterialWriter.h"
#include "Assets/LtsStaticModelWriter.h"
#include "Assets/StaticModelAssetLoader.h"
#include "Assets/ShaderAssetLoader.h"
#include "Assets/LtsShaderWriter.h"
#include "Legacy/Assets/LegacyMaterialConverter.h"
#include "Legacy/Assets/LegacyMaterialLibraryDecoder.h"
#include "Legacy/Assets/LegacyScbMeshDecoder.h"
#include "Assets/TextureAssetCache.h"

#include "Graphics/RenderDevice.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <limits>
#include <unordered_set>
#include <vector>

namespace
{
    using namespace engine;

    [[nodiscard]] bool Check(
        const bool condition,
        const char* const message) noexcept
    {
        if (condition)
        {
            return true;
        }

        std::fprintf(stderr, "FAILED: %s\n", message);
        return false;
    }

    void WriteU32(
        std::byte* const data,
        const std::size_t offset,
        const std::uint32_t value) noexcept
    {
        std::memcpy(data + offset, &value, sizeof(value));
    }
    void WriteU64(std::byte* d,std::size_t o,std::uint64_t v) noexcept{std::memcpy(d+o,&v,8U);}
    void WriteF32(std::byte* d,std::size_t o,float v) noexcept{std::memcpy(d+o,&v,4U);}

    [[nodiscard]] assets::AssetResult BuildTestMesh(assets::AssetData& out,bool use32=false) noexcept
    {
        constexpr std::size_t vo=160U,vs=144U,io=304U,so16=312U,so32=316U;
        const std::size_t is=use32?12U:6U,so=use32?so32:so16,total=so+16U;
        auto r=out.Resize(total);if(assets::Failed(r))return r;auto*b=out.GetData();std::memset(b,0,total);std::memcpy(b,"LTSMESH\0",8);
        WriteU32(b,8,1);WriteU32(b,12,0x01020304);WriteU32(b,16,160);WriteU32(b,20,48);WriteU32(b,24,use32?2:1);WriteU32(b,28,3);WriteU32(b,32,3);WriteU32(b,36,1);WriteU32(b,40,1);
        WriteU64(b,48,vo);WriteU64(b,56,vs);WriteU64(b,64,io);WriteU64(b,72,is);WriteU64(b,80,so);WriteU64(b,88,16);
        WriteF32(b,96,-1);WriteF32(b,108,1);WriteF32(b,112,1);WriteF32(b,124,0.5F);WriteF32(b,132,2.0F);
        assets::StaticMeshVertex v[3]={};v[0].position={-1,0,0};v[1].position={1,0,0};v[2].position={0,1,0};for(auto&x:v){x.normal={0,0,1};x.tangent={1,0,0,1};}std::memcpy(b+vo,v,sizeof(v));
        if(use32){const std::uint32_t idx[]={0,1,2};std::memcpy(b+io,idx,sizeof(idx));}else{const std::uint16_t idx[]={0,1,2};std::memcpy(b+io,idx,sizeof(idx));}
        WriteU32(b,so,0);WriteU32(b,so+4,3);WriteU32(b,so+8,0);WriteU32(b,so+12,0);return assets::AssetResult::Success;
    }
    [[nodiscard]] assets::AssetResult BuildTestMaterial(assets::AssetData& out,const char* path=nullptr) noexcept
    {
        const std::size_t ps=path?std::strlen(path):0,total=160U+ps;auto r=out.Resize(total);if(assets::Failed(r))return r;auto*b=out.GetData();std::memset(b,0,total);std::memcpy(b,"LTSMAT\0\0",8);WriteU32(b,8,1);WriteU32(b,12,0x01020304);WriteU32(b,16,160);WriteU32(b,24,path?2:0);for(int i=0;i<4;++i)WriteF32(b,28+i*4,1);WriteF32(b,60,1);WriteF32(b,64,0.5F);WriteU32(b,68,1);WriteU32(b,72,0);WriteU32(b,76,0);WriteU32(b,80,0);WriteU32(b,88,1);WriteU32(b,92,7);WriteF32(b,116,(std::numeric_limits<float>::max)());WriteU64(b,120,160);WriteU32(b,128,static_cast<std::uint32_t>(ps));if(ps)std::memcpy(b+160,path,ps);return assets::AssetResult::Success;
    }
    [[nodiscard]] assets::AssetResult BuildTestScb(assets::AssetData& out) noexcept
    {
        constexpr std::size_t size=202U;auto r=out.Resize(size);if(assets::Failed(r))return r;auto*b=out.GetData();std::memset(b,0,size);std::size_t o=0U;
        const auto u32=[&](std::uint32_t v){WriteU32(b,o,v);o+=4U;};const auto f32=[&](float v){WriteF32(b,o,v);o+=4U;};
        u32(0xFADC0038U);u32(0U);u32(4U);std::memcpy(b+o,"test",4U);o+=4U;f32(0);f32(0);f32(0);u32(3U);
        const float positions[]={-1,0,0,1,0,0,0,1,0};for(float v:positions)f32(v);const float uv[]={0,0,1,0,0.5F,1};for(float v:uv)f32(v);for(int i=0;i<3;++i){f32(0);f32(0);f32(1);}for(int i=0;i<3;++i){f32(1);f32(0);f32(0);}b[o++]=std::byte{0x7F};b[o++]=std::byte{0x7F};b[o++]=std::byte{0x7F};u32(3U);u32(0);u32(1);u32(2);u32(1);u32(0);u32(3);u32(3);std::memcpy(b+o,"mat",3U);o+=3U;return o==size?assets::AssetResult::Success:assets::AssetResult::InternalError;
    }
    [[nodiscard]] assets::AssetResult BuildCaseDedupScb(assets::AssetData& out) noexcept
    {
        constexpr std::size_t size=233U;auto r=out.Resize(size);if(assets::Failed(r))return r;auto*b=out.GetData();std::memset(b,0,size);std::size_t o=0U;
        const auto u32=[&](std::uint32_t v){WriteU32(b,o,v);o+=4U;};const auto f32=[&](float v){WriteF32(b,o,v);o+=4U;};
        u32(0xFADC0038U);u32(0U);u32(4U);std::memcpy(b+o,"test",4U);o+=4U;f32(0);f32(0);f32(0);u32(3U);
        const float positions[]={-1,0,0,1,0,0,0,1,0};for(float v:positions)f32(v);const float uv[]={0,0,1,0,0.5F,1};for(float v:uv)f32(v);for(int i=0;i<3;++i){f32(0);f32(0);f32(1);}for(int i=0;i<3;++i){f32(1);f32(0);f32(0);}b[o++]=std::byte{1};b[o++]=std::byte{1};b[o++]=std::byte{1};
        u32(6U);u32(0);u32(1);u32(2);u32(0);u32(2);u32(1);u32(2U);
        u32(0);u32(3);u32(5);std::memcpy(b+o,"Metal",5U);o+=5U;u32(3);u32(6);u32(5);std::memcpy(b+o,"metal",5U);o+=5U;
        return o==size?assets::AssetResult::Success:assets::AssetResult::InternalError;
    }

    [[nodiscard]] assets::AssetResult BuildBlockDds(assets::AssetData& out,const std::uint32_t fourCc,const std::uint64_t block) noexcept
    {
        const std::size_t blockSize=fourCc==0x31545844U?8U:16U;const std::size_t size=128U+blockSize;auto r=out.Resize(size);if(assets::Failed(r))return r;auto*b=out.GetData();std::memset(b,0,size);
        WriteU32(b,0U,0x20534444U);WriteU32(b,4U,124U);WriteU32(b,12U,4U);WriteU32(b,16U,4U);WriteU32(b,20U,static_cast<std::uint32_t>(blockSize));WriteU32(b,28U,1U);
        WriteU32(b,76U,32U);WriteU32(b,80U,4U);WriteU32(b,84U,fourCc);WriteU32(b,108U,0x1000U);std::memcpy(b+128U,&block,8U);return assets::AssetResult::Success;
    }

    [[nodiscard]] assets::AssetResult TextData(const char* text, assets::AssetData& out) noexcept
    {
        const std::size_t size = std::strlen(text);
        const auto result = out.Resize(size);
        if (assets::Failed(result)) return result;
        std::memcpy(out.GetData(), text, size);
        return assets::AssetResult::Success;
    }

    [[nodiscard]] assets::AssetResult BuildTestDds(
        assets::AssetData& outData) noexcept
    {
        constexpr std::size_t fileSize = 132U;
        constexpr std::size_t pixelOffset = 128U;

        const assets::AssetResult resizeResult =
            outData.Resize(fileSize);

        if (assets::Failed(resizeResult))
        {
            return resizeResult;
        }

        std::byte* const bytes = outData.GetData();
        std::memset(bytes, 0, fileSize);

        WriteU32(bytes, 0U, 0x20534444U);
        WriteU32(bytes, 4U, 124U);
        WriteU32(bytes, 12U, 1U);
        WriteU32(bytes, 16U, 1U);
        WriteU32(bytes, 20U, 4U);
        WriteU32(bytes, 28U, 1U);

        WriteU32(bytes, 76U, 32U);
        WriteU32(bytes, 80U, 0x00000041U);
        WriteU32(bytes, 88U, 32U);
        WriteU32(bytes, 92U, 0x000000FFU);
        WriteU32(bytes, 96U, 0x0000FF00U);
        WriteU32(bytes, 100U, 0x00FF0000U);
        WriteU32(bytes, 104U, 0xFF000000U);
        WriteU32(bytes, 108U, 0x00001000U);

        bytes[pixelOffset + 0U] = std::byte{0x11U};
        bytes[pixelOffset + 1U] = std::byte{0x22U};
        bytes[pixelOffset + 2U] = std::byte{0x33U};
        bytes[pixelOffset + 3U] = std::byte{0xFFU};

        return assets::AssetResult::Success;
    }

    class MemoryAssetSource final : public assets::AssetSource
    {
    public:
        MemoryAssetSource(
            assets::AssetPath path,
            const assets::AssetData& source)
            : path_(std::move(path))
        {
            bytes_.resize(source.GetSize());

            if (!bytes_.empty())
            {
                std::memcpy(
                    bytes_.data(),
                    source.GetData(),
                    source.GetSize());
            }
        }

        [[nodiscard]] assets::AssetResult Read(
            const assets::AssetPath& path,
            assets::AssetData& outData) noexcept override
        {
            outData.Clear();

            if (path != path_)
            {
                return assets::AssetResult::NotFound;
            }

            const assets::AssetResult resizeResult =
                outData.Resize(bytes_.size());

            if (assets::Failed(resizeResult))
            {
                return resizeResult;
            }

            if (!bytes_.empty())
            {
                std::memcpy(
                    outData.GetData(),
                    bytes_.data(),
                    bytes_.size());
            }

            return assets::AssetResult::Success;
        }

        [[nodiscard]] bool Exists(
            const assets::AssetPath& path) const noexcept override
        {
            return path == path_;
        }

    private:
        assets::AssetPath path_;
        std::vector<std::byte> bytes_;
    };

    class FakeRenderDevice final : public graphics::RenderDevice
    {
    public:
        [[nodiscard]] graphics::GraphicsBackend GetBackend() const noexcept override
        {
            return backend_;
        }

        [[nodiscard]] graphics::DeviceState GetState() const noexcept override
        {
            return state_;
        }

        [[nodiscard]] graphics::GraphicsResult Initialize(
            const graphics::RenderDeviceDesc& desc) noexcept override
        {
            if (!desc.IsValid() || state_ == graphics::DeviceState::Ready)
            {
                return graphics::GraphicsResult::InvalidArgument;
            }

            backend_ = desc.backend;
            state_ = graphics::DeviceState::Ready;
            return graphics::GraphicsResult::Success;
        }

        void Shutdown() noexcept override
        {
            textures_.clear();
            buffers_.clear();
            backend_ = graphics::GraphicsBackend::None;
            state_ = graphics::DeviceState::Stopped;
        }

        [[nodiscard]] graphics::GraphicsResult CreateSwapChain(
            const graphics::SwapChainDesc&,
            std::unique_ptr<graphics::SwapChain>& outSwapChain) noexcept override
        {
            outSwapChain.reset();
            return graphics::GraphicsResult::Unsupported;
        }

        [[nodiscard]] graphics::GraphicsResult CreateTexture(
            const graphics::TextureDesc& desc,
            const graphics::TextureSubresourceData* initialData,
            const std::size_t initialDataCount,
            graphics::TextureHandle& outTexture) noexcept override
        {
            outTexture = graphics::TextureHandle{};

            if (state_ != graphics::DeviceState::Ready)
            {
                return graphics::GraphicsResult::InvalidState;
            }

            if (
                !desc.IsValid() ||
                initialData == nullptr ||
                initialDataCount == 0U)
            {
                return graphics::GraphicsResult::InvalidArgument;
            }

            for (std::size_t index = 0U; index < initialDataCount; ++index)
            {
                if (!initialData[index].IsValid())
                {
                    return graphics::GraphicsResult::InvalidArgument;
                }
            }

            outTexture = graphics::TextureHandle::FromParts(
                nextTextureIndex_++,
                1U);

            try
            {
                textures_.insert(outTexture.Value());
            }
            catch (...)
            {
                outTexture = graphics::TextureHandle{};
                return graphics::GraphicsResult::OutOfMemory;
            }

            return graphics::GraphicsResult::Success;
        }

        [[nodiscard]] graphics::GraphicsResult DestroyTexture(
            const graphics::TextureHandle texture) noexcept override
        {
            if (!texture.IsValid())
            {
                return graphics::GraphicsResult::InvalidArgument;
            }

            return textures_.erase(texture.Value()) != 0U
                ? graphics::GraphicsResult::Success
                : graphics::GraphicsResult::NotFound;
        }

        [[nodiscard]] graphics::GraphicsResult CreateBuffer(
            const graphics::BufferDesc& desc,
            const graphics::BufferInitialData* data,
            graphics::BufferHandle& outBuffer) noexcept override
        {
            outBuffer = graphics::BufferHandle{};
            if(failBufferCreate_!=0U&&++bufferCreateCalls_==failBufferCreate_)return graphics::GraphicsResult::OutOfMemory;
            if(!desc.IsValid()||!data||!data->IsValid())return graphics::GraphicsResult::InvalidArgument;
            outBuffer=graphics::BufferHandle::FromParts(nextBufferIndex_++,1U);try{buffers_.insert(outBuffer.Value());}catch(...){outBuffer={};return graphics::GraphicsResult::OutOfMemory;}return graphics::GraphicsResult::Success;
        }

        [[nodiscard]] graphics::GraphicsResult DestroyBuffer(
            const graphics::BufferHandle buffer) noexcept override
        {
            return buffers_.erase(buffer.Value())?graphics::GraphicsResult::Success:graphics::GraphicsResult::NotFound;
        }

        void FailBufferCreate(std::uint32_t call) noexcept{bufferCreateCalls_=0;failBufferCreate_=call;}
        [[nodiscard]] std::size_t GetBufferCount()const noexcept{return buffers_.size();}

        [[nodiscard]] std::size_t GetTextureCount() const noexcept
        {
            return textures_.size();
        }

    private:
        graphics::GraphicsBackend backend_ =
            graphics::GraphicsBackend::None;

        graphics::DeviceState state_ =
            graphics::DeviceState::Uninitialized;

        std::uint32_t nextTextureIndex_ = 1U;
        std::uint32_t nextBufferIndex_=1U,bufferCreateCalls_=0U,failBufferCreate_=0U;
        std::unordered_set<std::uint64_t> textures_;
        std::unordered_set<std::uint64_t> buffers_;
    };
}

int main()
{
    if(!Check(graphics::ToSrgbFormat(graphics::Format::BC3UNorm)==graphics::Format::BC3UNormSrgb&&graphics::ToLinearFormat(graphics::Format::BC3UNormSrgb)==graphics::Format::BC3UNorm&&graphics::AreBitCompatibleFormats(graphics::Format::R8G8B8A8UNorm,graphics::Format::R8G8B8A8UNormSrgb)&&!graphics::AreBitCompatibleFormats(graphics::Format::BC1UNorm,graphics::Format::BC3UNorm),"color-space format compatibility helpers"))return 1;
    {
        assets::ShaderAssetDesc shaderDesc;shaderDesc.stage=graphics::ShaderStage::Vertex;shaderDesc.targetProfile="vs_4_0";shaderDesc.entryPoint="VSMain";shaderDesc.sourceHash=0x123456789abcdef0ULL;shaderDesc.debugName="test shader";shaderDesc.bytecode={std::byte{'D'},std::byte{'X'},std::byte{'B'},std::byte{'C'},std::byte{1},std::byte{2}};
        assets::ShaderAsset shader;if(!Check(assets::Succeeded(shader.Initialize(std::move(shaderDesc))),"valid ShaderAsset"))return 1;assets::AssetData first,second;if(!Check(assets::Succeeded(assets::LtsShaderWriter::Encode(shader,first))&&assets::Succeeded(assets::LtsShaderWriter::Encode(shader,second))&&first.GetSize()==second.GetSize()&&std::memcmp(first.GetData(),second.GetData(),first.GetSize())==0,"deterministic ltsshader writer"))return 1;
        assets::AssetPath shaderPath;(void)assets::AssetPath::TryCreate("shaders/test.ltsshader",shaderPath);assets::AssetMetadata metadata;metadata.path=shaderPath;metadata.id=shaderPath.GetId();metadata.type=assets::AssetType::Shader;assets::ShaderAssetLoader loader;std::unique_ptr<assets::LoadedAsset> loaded;if(!Check(assets::Succeeded(loader.Load(metadata,first,loaded))&&loaded&&static_cast<assets::ShaderLoadedAsset*>(loaded.get())->GetShader().GetSourceHash()==0x123456789abcdef0ULL,"ltsshader loader round-trip"))return 1;
        const std::byte saved=first.GetData()[108];first.GetData()[108]=std::byte{1};if(!Check(assets::Failed(loader.Load(metadata,first,loaded)),"ltsshader reserved bytes rejected"))return 1;first.GetData()[108]=saved;first.GetData()[0]=std::byte{'X'};if(!Check(assets::Succeeded(loader.Load(metadata,second,loaded))&&assets::Failed(loader.Load(metadata,first,loaded)),"ltsshader magic rejected"))return 1;
        assets::ShaderAssetDesc mismatch;mismatch.stage=graphics::ShaderStage::Vertex;mismatch.targetProfile="ps_4_0";mismatch.entryPoint="x";mismatch.sourceHash=1;mismatch.bytecode={std::byte{1}};assets::ShaderAsset invalid;if(!Check(assets::Failed(invalid.Initialize(std::move(mismatch))),"shader profile stage mismatch"))return 1;
    }
    using namespace engine;

    assets::AssetPath path;

    if (!Check(
            assets::Succeeded(
                assets::AssetPath::TryCreate(
                    "Textures\\Test.DDS",
                    path)),
            "AssetPath::TryCreate"))
    {
        return 1;
    }

    if (!Check(
            path.View() == "textures/test.dds",
            "AssetPath normalization"))
    {
        return 1;
    }

    assets::AssetData ddsData;

    if (!Check(
            assets::Succeeded(BuildTestDds(ddsData)),
            "BuildTestDds"))
    {
        return 1;
    }

    assets::DdsTextureLoader ddsLoader;
    assets::MeshAssetLoader meshLoader;
    assets::MaterialAssetLoader materialLoader;
    assets::StaticModelAssetLoader modelLoader;
    assets::AssetLoaderRegistry loaderRegistry;

    if (!Check(
            assets::Succeeded(loaderRegistry.Register(ddsLoader)),
            "AssetLoaderRegistry::Register"))
    {
        return 1;
    }
    if(!Check(assets::Succeeded(loaderRegistry.Register(meshLoader))&&assets::Succeeded(loaderRegistry.Register(materialLoader))&&assets::Succeeded(loaderRegistry.Register(modelLoader)),"mesh/material/model loader registration"))return 1;

    assets::AssetData mesh16,mesh32,materialData,materialTextureData;
    if(!Check(assets::Succeeded(BuildTestMesh(mesh16))&&assets::Succeeded(BuildTestMesh(mesh32,true))&&assets::Succeeded(BuildTestMaterial(materialData))&&assets::Succeeded(BuildTestMaterial(materialTextureData,"textures/test.dds")),"mesh/material fixtures"))return 1;
    assets::AssetPath meshPath,materialPath; (void)assets::AssetPath::TryCreate("meshes/test.mesh",meshPath);(void)assets::AssetPath::TryCreate("materials/test.ltsmat",materialPath);
    assets::AssetMetadata meshMetadata;meshMetadata.path=meshPath;meshMetadata.id=meshPath.GetId();meshMetadata.type=assets::AssetType::Mesh;meshMetadata.sourceSize=mesh16.GetSize();
    assets::AssetMetadata materialMetadata;materialMetadata.path=materialPath;materialMetadata.id=materialPath.GetId();materialMetadata.type=assets::AssetType::Material;materialMetadata.sourceSize=materialData.GetSize();
    std::unique_ptr<assets::LoadedAsset> foundationAsset;
    if(!Check(assets::Succeeded(loaderRegistry.Load(meshMetadata,mesh16,foundationAsset))&&foundationAsset&&foundationAsset->GetType()==assets::AssetType::Mesh,"valid UInt16 mesh load"))return 1;
    if(!Check(assets::Succeeded(loaderRegistry.Load(meshMetadata,mesh32,foundationAsset)),"valid UInt32 mesh load"))return 1;
    if(!Check(assets::Succeeded(loaderRegistry.Load(materialMetadata,materialData,foundationAsset))&&foundationAsset->GetType()==assets::AssetType::Material&&static_cast<assets::MaterialLoadedAsset*>(foundationAsset.get())->GetMaterial().GetDesc().specularPower==32.0F,"valid v1 material load with v2 defaults"))return 1;
    if(!Check(assets::Succeeded(loaderRegistry.Load(materialMetadata,materialTextureData,foundationAsset)),"material texture path load"))return 1;
    (void)loaderRegistry.Load(meshMetadata,mesh16,foundationAsset);assets::MeshAsset roundMesh=static_cast<assets::MeshLoadedAsset*>(foundationAsset.get())->ReleaseMesh();assets::AssetData encodedMeshA,encodedMeshB;
    if(!Check(assets::Succeeded(assets::LtsMeshWriter::Encode(roundMesh,encodedMeshA))&&assets::Succeeded(assets::LtsMeshWriter::Encode(roundMesh,encodedMeshB))&&encodedMeshA.GetSize()==encodedMeshB.GetSize()&&std::memcmp(encodedMeshA.GetData(),encodedMeshB.GetData(),encodedMeshA.GetSize())==0&&assets::Succeeded(loaderRegistry.Load(meshMetadata,encodedMeshA,foundationAsset)),"deterministic mesh writer round-trip"))return 1;
    (void)loaderRegistry.Load(materialMetadata,materialTextureData,foundationAsset);assets::MaterialAsset roundMaterial=static_cast<assets::MaterialLoadedAsset*>(foundationAsset.get())->ReleaseMaterial();assets::AssetData encodedMaterial,encodedMaterial2;
    assets::MaterialAssetDesc v2desc=roundMaterial.GetDesc();assets::AssetPath normalPath,specPath,roughPath,emissivePath;(void)assets::AssetPath::TryCreate("textures/normal.dds",normalPath);(void)assets::AssetPath::TryCreate("textures/spec.dds",specPath);(void)assets::AssetPath::TryCreate("textures/rough.dds",roughPath);(void)assets::AssetPath::TryCreate("textures/glow.dds",emissivePath);v2desc.normalTexture=normalPath;v2desc.specularGlossTexture=specPath;v2desc.roughnessTexture=roughPath;v2desc.emissiveTexture=emissivePath;v2desc.specularPowerTexture=roughPath;v2desc.specularIntensity=1.0F;v2desc.specularPower=64.0F;v2desc.reflectionFactor=0.5F;v2desc.emissiveStrength=2.0F;roundMaterial.Clear();if(!Check(assets::Succeeded(roundMaterial.Initialize(std::move(v2desc))),"v2 material initialize with duplicate path across semantics"))return 1;
    if(!Check(assets::Succeeded(assets::LtsMaterialWriter::Encode(roundMaterial,encodedMaterial))&&assets::Succeeded(assets::LtsMaterialWriter::Encode(roundMaterial,encodedMaterial2))&&encodedMaterial.GetSize()==encodedMaterial2.GetSize()&&std::memcmp(encodedMaterial.GetData(),encodedMaterial2.GetData(),encodedMaterial.GetSize())==0&&encodedMaterial.GetSize()>192U&&assets::Succeeded(loaderRegistry.Load(materialMetadata,encodedMaterial,foundationAsset))&&static_cast<assets::MaterialLoadedAsset*>(foundationAsset.get())->GetMaterial().GetDesc().specularPowerTexture.has_value(),"deterministic v2 all-semantic writer round-trip"))return 1;
    assets::AssetData scbData;if(!Check(assets::Succeeded(BuildTestScb(scbData)),"SCB fixture"))return 1;engine::legacy::assets::LegacyStaticMeshData legacyMesh;
    if(!Check(assets::Succeeded(engine::legacy::assets::LegacyScbMeshDecoder::Decode(scbData,legacyMesh))&&legacyMesh.mesh.IsValid()&&legacyMesh.materialSlotNames.size()==1U,"legacy SCB decode"))return 1;assets::AssetData scbCooked;
    if(!Check(assets::Succeeded(assets::LtsMeshWriter::Encode(legacyMesh.mesh,scbCooked))&&assets::Succeeded(loaderRegistry.Load(meshMetadata,scbCooked,foundationAsset)),"SCB to LTSMESH round-trip"))return 1;
    assets::AssetData badScb;(void)badScb.Resize(scbData.GetSize());std::memcpy(badScb.GetData(),scbData.GetData(),scbData.GetSize());WriteU32(badScb.GetData(),0U,1U);if(!Check(assets::Failed(engine::legacy::assets::LegacyScbMeshDecoder::Decode(badScb,legacyMesh))&&legacyMesh.IsEmpty(),"wrong SCB version leaves empty output"))return 1;std::memcpy(badScb.GetData(),scbData.GetData(),scbData.GetSize());WriteU32(badScb.GetData(),4U,1U);if(!Check(engine::legacy::assets::LegacyScbMeshDecoder::Decode(badScb,legacyMesh)==assets::AssetResult::UnsupportedFeature&&legacyMesh.IsEmpty(),"SCB weights explicitly unsupported"))return 1;(void)badScb.Resize(scbData.GetSize()+1U);std::memcpy(badScb.GetData(),scbData.GetData(),scbData.GetSize());badScb.GetData()[scbData.GetSize()]=std::byte{1};if(!Check(assets::Failed(engine::legacy::assets::LegacyScbMeshDecoder::Decode(badScb,legacyMesh))&&legacyMesh.IsEmpty(),"SCB trailing garbage rejected"))return 1;
    (void)badScb.Resize(scbData.GetSize());std::memcpy(badScb.GetData(),scbData.GetData(),scbData.GetSize());WriteU32(badScb.GetData(),187U,3U);if(!Check(assets::Failed(engine::legacy::assets::LegacyScbMeshDecoder::Decode(badScb,legacyMesh)),"SCB first chunk must start at zero"))return 1;
    std::memcpy(badScb.GetData(),scbData.GetData(),scbData.GetSize());WriteU32(badScb.GetData(),191U,2U);if(!Check(assets::Failed(engine::legacy::assets::LegacyScbMeshDecoder::Decode(badScb,legacyMesh)),"SCB final chunk covers every index"))return 1;
    std::memcpy(badScb.GetData(),scbData.GetData(),scbData.GetSize());WriteF32(badScb.GetData(),128U,0.0F);if(!Check(assets::Failed(engine::legacy::assets::LegacyScbMeshDecoder::Decode(badScb,legacyMesh)),"zero SCB tangent rejected"))return 1;
    std::memcpy(badScb.GetData(),scbData.GetData(),scbData.GetSize());WriteF32(badScb.GetData(),128U,(std::numeric_limits<float>::infinity)());if(!Check(assets::Failed(engine::legacy::assets::LegacyScbMeshDecoder::Decode(badScb,legacyMesh)),"non-finite SCB tangent rejected"))return 1;
    std::memcpy(badScb.GetData(),scbData.GetData(),scbData.GetSize());WriteF32(badScb.GetData(),128U,2.0F);if(!Check(assets::Succeeded(engine::legacy::assets::LegacyScbMeshDecoder::Decode(badScb,legacyMesh))&&legacyMesh.mesh.GetVertexData()[0].tangent[0]==1.0F,"SCB tangent normalized"))return 1;
    assets::AssetData caseScb;if(!Check(assets::Succeeded(BuildCaseDedupScb(caseScb))&&assets::Succeeded(engine::legacy::assets::LegacyScbMeshDecoder::Decode(caseScb,legacyMesh))&&legacyMesh.materialSlotNames.size()==1U&&legacyMesh.materialSlotNames[0]=="Metal"&&legacyMesh.mesh.GetSubmeshCount()==2U&&legacyMesh.mesh.GetSubmesh(1U)->materialSlot==0U,"SCB material names deduplicate ASCII case-insensitively with first spelling"))return 1;

    assets::AssetData legacyMaterialText;
    const char* materialText="[MaterialBegin]\r\n Name = TestMat \r\nColor24=128 64 32\r\nDoubleSided=1\r\nAlphaTransparent=0\r\nForceTransparent=1\r\nTexture=diffuse.tga\r\nNormalMap=normal.dds\r\nSpecularMap=spec.dds\r\nEnvMap=rough.dds\r\nGlowMap=glow.dds\r\nSpecPowMap=power.dds\r\nDetailNMap=detail.dds\r\nSelfIllumMultiplier=2\r\nSpecularPower=64\r\nSpecular1Power=32\r\nReflectionPower=0.5\r\nlowQSelfIllum=0.2\r\nlowQMetallness=0.9\r\nImagesDir=Data/Images\r\nUnknownField=ok\r\n[MaterialEnd]\r\n[MaterialBegin]\r\nName=Second\r\nTexture=NONE\r\nNormalMap=NONE\r\n[MaterialEnd]\r\n";
    engine::legacy::assets::LegacyMaterialLibraryData legacyLibrary;
    if(!Check(assets::Succeeded(TextData(materialText,legacyMaterialText))&&assets::Succeeded(engine::legacy::assets::LegacyMaterialLibraryDecoder::Decode(legacyMaterialText,legacyLibrary))&&legacyLibrary.GetMaterialCount()==2U&&legacyLibrary.FindMaterial("testmat")!=nullptr,"legacy material CRLF/multiple/case-insensitive lookup"))return 1;
    const auto* legacyRecord=legacyLibrary.FindMaterial("TESTMAT");assets::MaterialAsset convertedMaterial;std::vector<std::string> conversionDiagnostics;
    if(!Check(legacyRecord&&legacyRecord->normalMap=="normal.dds"&&legacyRecord->specularPowerMap=="power.dds"&&assets::Succeeded(engine::legacy::assets::LegacyMaterialConverter::Convert(*legacyRecord,nullptr,convertedMaterial,conversionDiagnostics))&&convertedMaterial.GetDesc().alphaMode==assets::MaterialAlphaMode::Mask&&convertedMaterial.GetDesc().doubleSided&&!convertedMaterial.GetDesc().baseColorTexture&&convertedMaterial.GetDesc().metallicFactor==0.0F&&convertedMaterial.GetDesc().specularIntensity==16.0F&&convertedMaterial.GetDesc().specularPower==2048.0F&&convertedMaterial.GetDesc().reflectionFactor==0.5F&&convertedMaterial.GetDesc().emissiveStrength==2.0F,"legacy material surface conversion without metallic heuristic"))return 1;
    engine::legacy::assets::LegacyResolvedMaterialTextures resolvedMaps;resolvedMaps.baseColor=&path;resolvedMaps.normal=&normalPath;resolvedMaps.specularGloss=&specPath;resolvedMaps.roughness=&roughPath;resolvedMaps.emissive=&emissivePath;resolvedMaps.specularPower=&roughPath;
    if(!Check(assets::Succeeded(engine::legacy::assets::LegacyMaterialConverter::Convert(*legacyRecord,resolvedMaps,engine::legacy::assets::LegacyTextureAlpha::NoAlpha,convertedMaterial,conversionDiagnostics))&&convertedMaterial.GetDesc().baseColorTexture&&convertedMaterial.GetDesc().normalTexture&&convertedMaterial.GetDesc().specularGlossTexture&&convertedMaterial.GetDesc().roughnessTexture&&convertedMaterial.GetDesc().emissiveTexture&&convertedMaterial.GetDesc().specularPowerTexture&&convertedMaterial.GetDesc().emissiveFactor[0]==1.0F&&convertedMaterial.GetDesc().emissiveFactor[1]==1.0F&&convertedMaterial.GetDesc().emissiveFactor[2]==1.0F&&convertedMaterial.GetDesc().metallicFactor==0.0F,"legacy all-map compatibility mapping"))return 1;
    assets::AssetData bc1OpaqueData,bc1AlphaData,bc3Data;assets::TextureAsset alphaTexture;
    constexpr std::uint32_t dxt1=0x31545844U,dxt5=0x35545844U;
    if(!Check(assets::Succeeded(BuildBlockDds(bc1OpaqueData,dxt1,0x000000000000FFFFULL))&&assets::Succeeded(assets::DdsTextureDecoder::Decode(bc1OpaqueData,alphaTexture))&&engine::legacy::assets::LegacyMaterialConverter::DetectTextureAlpha(alphaTexture)==engine::legacy::assets::LegacyTextureAlpha::NoAlpha,"BC1 opaque alpha detection"))return 1;
    if(!Check(assets::Succeeded(BuildBlockDds(bc1AlphaData,dxt1,0xFFFFFFFF00000000ULL))&&assets::Succeeded(assets::DdsTextureDecoder::Decode(bc1AlphaData,alphaTexture))&&engine::legacy::assets::LegacyMaterialConverter::DetectTextureAlpha(alphaTexture)==engine::legacy::assets::LegacyTextureAlpha::HasAlpha,"BC1 one-bit alpha detection"))return 1;
    if(!Check(assets::Succeeded(BuildBlockDds(bc3Data,dxt5,0ULL))&&assets::Succeeded(assets::DdsTextureDecoder::Decode(bc3Data,alphaTexture))&&engine::legacy::assets::LegacyMaterialConverter::DetectTextureAlpha(alphaTexture)==engine::legacy::assets::LegacyTextureAlpha::HasAlpha,"BC3 alpha-capable detection"))return 1;
    engine::legacy::assets::LegacyMaterialRecord alphaRecord;alphaRecord.name="Alpha";
    if(!Check(assets::Succeeded(engine::legacy::assets::LegacyMaterialConverter::Convert(alphaRecord,nullptr,engine::legacy::assets::LegacyTextureAlpha::HasAlpha,convertedMaterial,conversionDiagnostics))&&convertedMaterial.GetDesc().alphaMode==assets::MaterialAlphaMode::Mask,"texture alpha selects Mask"))return 1;
    alphaRecord.alphaTransparent=true;if(!Check(assets::Succeeded(engine::legacy::assets::LegacyMaterialConverter::Convert(alphaRecord,nullptr,engine::legacy::assets::LegacyTextureAlpha::HasAlpha,convertedMaterial,conversionDiagnostics))&&convertedMaterial.GetDesc().alphaMode==assets::MaterialAlphaMode::Blend,"AlphaTransparent has Blend priority"))return 1;
    assets::AssetData invalidText;(void)TextData("[MaterialBegin]\nName=X\nColor24=1 2 nope\n[MaterialEnd]\n",invalidText);if(!Check(assets::Failed(engine::legacy::assets::LegacyMaterialLibraryDecoder::Decode(invalidText,legacyLibrary)),"invalid Color24 rejected"))return 1;
    (void)TextData("[MaterialBegin]\nName=X\nDoubleSided=2\n[MaterialEnd]\n",invalidText);if(!Check(assets::Failed(engine::legacy::assets::LegacyMaterialLibraryDecoder::Decode(invalidText,legacyLibrary)),"invalid bool rejected"))return 1;
    (void)TextData("[MaterialBegin]\nName=X\nSpecularPower=nan\n[MaterialEnd]\n",invalidText);if(!Check(assets::Failed(engine::legacy::assets::LegacyMaterialLibraryDecoder::Decode(invalidText,legacyLibrary)),"NaN rejected"))return 1;
    (void)TextData("[MaterialBegin]\nName=X\nReflectionPower=inf\n[MaterialEnd]\n",invalidText);if(!Check(assets::Failed(engine::legacy::assets::LegacyMaterialLibraryDecoder::Decode(invalidText,legacyLibrary)),"Infinity rejected"))return 1;
    (void)TextData("[MaterialBegin]\nName=X\n",invalidText);if(!Check(assets::Failed(engine::legacy::assets::LegacyMaterialLibraryDecoder::Decode(invalidText,legacyLibrary)),"missing MaterialEnd rejected"))return 1;
    (void)TextData("[MaterialBegin]\nName=   \n[MaterialEnd]\n",invalidText);if(!Check(assets::Failed(engine::legacy::assets::LegacyMaterialLibraryDecoder::Decode(invalidText,legacyLibrary)),"empty material name rejected"))return 1;
    const std::string overlong="[MaterialBegin]\nName=X\nUnknown="+std::string(1025U,'x')+"\n[MaterialEnd]\n";(void)TextData(overlong.c_str(),invalidText);if(!Check(assets::Failed(engine::legacy::assets::LegacyMaterialLibraryDecoder::Decode(invalidText,legacyLibrary)),"overlong material line rejected"))return 1;
    (void)TextData("[MaterialBegin]\nName=X\n[MaterialEnd]\n[MaterialBegin]\nName=x\n[MaterialEnd]\n",invalidText);if(!Check(engine::legacy::assets::LegacyMaterialLibraryDecoder::Decode(invalidText,legacyLibrary)==assets::AssetResult::AlreadyExists,"duplicate material names rejected"))return 1;
    (void)TextData("[MaterialBegin]\nName=X\nNormalMap=a.dds\nnormalmap=b.dds\n[MaterialEnd]\n",invalidText);if(!Check(engine::legacy::assets::LegacyMaterialLibraryDecoder::Decode(invalidText,legacyLibrary)==assets::AssetResult::AlreadyExists,"conflicting duplicate fields rejected"))return 1;

    assets::AssetPath modelMeshPath,modelMaterialPath,modelPath;(void)assets::AssetPath::TryCreate("models/test/model.mesh",modelMeshPath);(void)assets::AssetPath::TryCreate("models/test/test.ltsmat",modelMaterialPath);(void)assets::AssetPath::TryCreate("models/test/model.ltsmodel",modelPath);
    std::vector<assets::AssetPath> modelMaterials;modelMaterials.push_back(modelMaterialPath);assets::StaticModelAsset staticModel;
    if(!Check(assets::Succeeded(staticModel.Initialize(std::move(modelMeshPath),std::move(modelMaterials),"test model")),"StaticModelAsset valid"))return 1;
    assets::AssetData modelBytesA,modelBytesB;if(!Check(assets::Succeeded(assets::LtsStaticModelWriter::Encode(staticModel,modelBytesA))&&assets::Succeeded(assets::LtsStaticModelWriter::Encode(staticModel,modelBytesB))&&modelBytesA.GetSize()==modelBytesB.GetSize()&&std::memcmp(modelBytesA.GetData(),modelBytesB.GetData(),modelBytesA.GetSize())==0,"deterministic static model writer"))return 1;
    assets::AssetMetadata modelMetadata;modelMetadata.path=modelPath;modelMetadata.id=modelPath.GetId();modelMetadata.type=assets::AssetType::StaticModel;modelMetadata.sourceSize=modelBytesA.GetSize();
    if(!Check(assets::Succeeded(loaderRegistry.Load(modelMetadata,modelBytesA,foundationAsset))&&static_cast<assets::StaticModelLoadedAsset*>(foundationAsset.get())->GetModel().GetMaterialCount()==1U,"static model writer/loader round-trip"))return 1;
    assets::AssetData badModel;(void)badModel.Resize(modelBytesA.GetSize());std::memcpy(badModel.GetData(),modelBytesA.GetData(),modelBytesA.GetSize());WriteU32(badModel.GetData(),60U,1U);if(!Check(assets::Failed(loaderRegistry.Load(modelMetadata,badModel,foundationAsset)),"static model reserved bytes rejected"))return 1;
    std::memcpy(badModel.GetData(),modelBytesA.GetData(),modelBytesA.GetSize());badModel.GetData()[0]=std::byte{'X'};if(!Check(assets::Failed(loaderRegistry.Load(modelMetadata,badModel,foundationAsset)),"static model wrong magic rejected"))return 1;
    std::memcpy(badModel.GetData(),modelBytesA.GetData(),modelBytesA.GetSize());WriteU32(badModel.GetData(),8U,2U);if(!Check(assets::Failed(loaderRegistry.Load(modelMetadata,badModel,foundationAsset)),"static model wrong version rejected"))return 1;
    std::memcpy(badModel.GetData(),modelBytesA.GetData(),modelBytesA.GetSize());WriteU64(badModel.GetData(),24U,64U);if(!Check(assets::Failed(loaderRegistry.Load(modelMetadata,badModel,foundationAsset)),"static model overlapping regions rejected"))return 1;
    (void)badModel.Resize(modelBytesA.GetSize()+1U);std::memcpy(badModel.GetData(),modelBytesA.GetData(),modelBytesA.GetSize());if(!Check(assets::Failed(loaderRegistry.Load(modelMetadata,badModel,foundationAsset)),"static model trailing bytes rejected"))return 1;
    assets::AssetData malformed; (void)malformed.Resize(mesh16.GetSize());std::memcpy(malformed.GetData(),mesh16.GetData(),mesh16.GetSize());malformed.GetData()[0]=std::byte{'X'};
    if(!Check(assets::Failed(loaderRegistry.Load(meshMetadata,malformed,foundationAsset)),"wrong mesh magic rejected"))return 1;
    (void)malformed.Resize(80U);if(!Check(assets::Failed(loaderRegistry.Load(meshMetadata,malformed,foundationAsset)),"truncated mesh rejected"))return 1;
    (void)malformed.Resize(materialData.GetSize());std::memcpy(malformed.GetData(),materialData.GetData(),materialData.GetSize());WriteU32(malformed.GetData(),20,99U);
    if(!Check(assets::Failed(loaderRegistry.Load(materialMetadata,malformed,foundationAsset)),"invalid material enum rejected"))return 1;
    const auto rejectMesh=[&](std::size_t offset,std::uint32_t value,const char* message){assets::AssetData bad;if(assets::Failed(bad.Resize(mesh16.GetSize())))return false;std::memcpy(bad.GetData(),mesh16.GetData(),mesh16.GetSize());WriteU32(bad.GetData(),offset,value);return Check(assets::Failed(loaderRegistry.Load(meshMetadata,bad,foundationAsset)),message);};
    if(!rejectMesh(8U,2U,"unsupported mesh version")||!rejectMesh(28U,0xFFFFFFFFU,"overflowed mesh count")||!rejectMesh(304U,99U,"mesh index outside range")||!rejectMesh(316U,99U,"invalid submesh range")||!rejectMesh(324U,4U,"invalid material slot"))return 1;
    if(!rejectMesh(16U,164U,"extended mesh header rejected")||!rejectMesh(44U,1U,"reserved mesh field rejected")||!rejectMesh(136U,1U,"reserved mesh bytes rejected")||!rejectMesh(64U,160U,"overlapping mesh regions rejected"))return 1;
    assets::AssetData badBounds;(void)badBounds.Resize(mesh16.GetSize());std::memcpy(badBounds.GetData(),mesh16.GetData(),mesh16.GetSize());WriteF32(badBounds.GetData(),96,(std::numeric_limits<float>::quiet_NaN)());if(!Check(assets::Failed(loaderRegistry.Load(meshMetadata,badBounds,foundationAsset)),"invalid mesh bounds"))return 1;
    const auto rejectMaterial=[&](std::size_t offset,std::uint32_t value,const char* message){assets::AssetData bad;if(assets::Failed(bad.Resize(materialData.GetSize())))return false;std::memcpy(bad.GetData(),materialData.GetData(),materialData.GetSize());WriteU32(bad.GetData(),offset,value);return Check(assets::Failed(loaderRegistry.Load(materialMetadata,bad,foundationAsset)),message);};
    if(!rejectMaterial(8U,2U,"unsupported material version")||!rejectMaterial(68U,99U,"invalid material sampler"))return 1;
    if(!rejectMaterial(132U,1U,"reserved material bytes rejected"))return 1;
    assets::AssetData badScalar;(void)badScalar.Resize(materialData.GetSize());std::memcpy(badScalar.GetData(),materialData.GetData(),materialData.GetSize());WriteF32(badScalar.GetData(),60,(std::numeric_limits<float>::infinity)());if(!Check(assets::Failed(loaderRegistry.Load(materialMetadata,badScalar,foundationAsset)),"invalid material scalar"))return 1;
    assets::AssetData truncatedPath;(void)truncatedPath.Resize(materialTextureData.GetSize()-1U);std::memcpy(truncatedPath.GetData(),materialTextureData.GetData(),truncatedPath.GetSize());if(!Check(assets::Failed(loaderRegistry.Load(materialMetadata,truncatedPath,foundationAsset)),"truncated material path"))return 1;
    const auto rejectV2U32=[&](std::size_t offset,std::uint32_t value,const char* message){assets::AssetData bad;if(assets::Failed(bad.Resize(encodedMaterial.GetSize())))return false;std::memcpy(bad.GetData(),encodedMaterial.GetData(),encodedMaterial.GetSize());WriteU32(bad.GetData(),offset,value);return Check(assets::Failed(loaderRegistry.Load(materialMetadata,bad,foundationAsset)),message);};
    if(!rejectV2U32(192U,99U,"v2 invalid semantic")||!rejectV2U32(216U,0U,"v2 duplicate/unsorted semantic")||!rejectV2U32(196U,1U,"v2 entry reserved rejected")||!rejectV2U32(152U,1U,"v2 header reserved rejected"))return 1;
    assets::AssetData badV2;(void)badV2.Resize(encodedMaterial.GetSize());std::memcpy(badV2.GetData(),encodedMaterial.GetData(),encodedMaterial.GetSize());WriteU64(badV2.GetData(),200U,192U);if(!Check(assets::Failed(loaderRegistry.Load(materialMetadata,badV2,foundationAsset)),"v2 path before table rejected"))return 1;
    std::memcpy(badV2.GetData(),encodedMaterial.GetData(),encodedMaterial.GetSize());WriteF32(badV2.GetData(),128U,(std::numeric_limits<float>::quiet_NaN)());if(!Check(assets::Failed(loaderRegistry.Load(materialMetadata,badV2,foundationAsset)),"v2 invalid scalar rejected"))return 1;
    (void)badV2.Resize(encodedMaterial.GetSize()+1U);std::memcpy(badV2.GetData(),encodedMaterial.GetData(),encodedMaterial.GetSize());if(!Check(assets::Failed(loaderRegistry.Load(materialMetadata,badV2,foundationAsset)),"v2 trailing bytes rejected"))return 1;

    assets::AssetMetadata metadata;
    metadata.path = path;
    metadata.id = path.GetId();
    metadata.type = assets::AssetType::Texture;
    metadata.sourceSize = ddsData.GetSize();

    std::unique_ptr<assets::LoadedAsset> loadedAsset;

    if (!Check(
            assets::Succeeded(
                loaderRegistry.Load(
                    metadata,
                    ddsData,
                    loadedAsset)),
            "AssetLoaderRegistry::Load"))
    {
        return 1;
    }

    if (!Check(
            loadedAsset &&
                loadedAsset->GetType() == assets::AssetType::Texture,
            "typed DDS load"))
    {
        return 1;
    }

    MemoryAssetSource source(path, ddsData);
    assets::AssetManager assetManager;

    if (!Check(
            assets::Succeeded(assetManager.Initialize(source)),
            "AssetManager::Initialize"))
    {
        return 1;
    }

    assets::AssetHandle assetHandle;

    if (!Check(
            assets::Succeeded(
                assetManager.Register(
                    metadata,
                    assetHandle)),
            "AssetManager::Register"))
    {
        return 1;
    }

    FakeRenderDevice renderDevice;
    graphics::RenderDeviceDesc deviceDesc;
    deviceDesc.backend = graphics::GraphicsBackend::D3D11;

    if (!Check(
            graphics::Succeeded(renderDevice.Initialize(deviceDesc)),
            "FakeRenderDevice::Initialize"))
    {
        return 1;
    }

    (void)loaderRegistry.Load(meshMetadata,mesh16,foundationAsset);
    assets::MeshAsset gpuSource=static_cast<assets::MeshLoadedAsset*>(foundationAsset.get())->ReleaseMesh();
    assets::GpuMesh gpuMesh;
    if(!Check(graphics::Succeeded(gpuMesh.Upload(renderDevice,gpuSource))&&gpuMesh.IsValid()&&renderDevice.GetBufferCount()==2U,"GpuMesh upload"))return 1;
    if(!Check(graphics::Succeeded(gpuMesh.Replace(renderDevice,gpuSource))&&renderDevice.GetBufferCount()==2U,"GpuMesh replace"))return 1;
    if(!Check(graphics::Succeeded(gpuMesh.Release(renderDevice))&&renderDevice.GetBufferCount()==0U,"GpuMesh release"))return 1;
    renderDevice.FailBufferCreate(2U);assets::GpuMesh failedMesh;
    if(!Check(graphics::Failed(failedMesh.Upload(renderDevice,gpuSource))&&!failedMesh.IsValid()&&renderDevice.GetBufferCount()==0U,"GpuMesh partial upload cleanup"))return 1;
    renderDevice.FailBufferCreate(0U);

    MemoryAssetSource meshSource(meshPath,mesh16);assets::AssetManager meshManager;assets::AssetHandle meshHandle;
    if(!Check(assets::Succeeded(meshManager.Initialize(meshSource))&&assets::Succeeded(meshManager.Register(meshMetadata,meshHandle))&&assets::Succeeded(meshManager.Load(meshHandle)),"AssetManager mesh load"))return 1;
    const assets::AssetData* managedMesh=nullptr;if(!Check(assets::Succeeded(meshManager.GetData(meshHandle,managedMesh))&&managedMesh&&assets::Succeeded(loaderRegistry.Load(meshMetadata,*managedMesh,foundationAsset)),"AssetManager to MeshAssetLoader path"))return 1;
    meshManager.Shutdown();

    assets::TextureAssetCache textureCache;

    if (!Check(
            textureCache.Initialize(
                assetManager,
                renderDevice).Succeeded(),
            "TextureAssetCache::Initialize"))
    {
        return 1;
    }

    graphics::TextureHandle firstTexture;

    if (!Check(
            textureCache.Acquire(
                assetHandle,
                firstTexture).Succeeded(),
            "TextureAssetCache::Acquire first"))
    {
        return 1;
    }

    graphics::TextureHandle secondTexture;

    if (!Check(
            textureCache.Acquire(
                assetHandle,
                secondTexture).Succeeded(),
            "TextureAssetCache::Acquire second"))
    {
        return 1;
    }

    if (!Check(
            firstTexture == secondTexture,
            "texture cache reuses GPU handle"))
    {
        return 1;
    }

    std::uint32_t referenceCount = 0U;

    if (!Check(
            textureCache.GetReferenceCount(
                assetHandle,
                referenceCount).Succeeded() &&
                referenceCount == 2U,
            "texture reference count"))
    {
        return 1;
    }

    if (!Check(
            textureCache.Reload(assetHandle).Succeeded(),
            "TextureAssetCache::Reload"))
    {
        return 1;
    }

    if (!Check(
            textureCache.Release(assetHandle).Succeeded(),
            "TextureAssetCache::Release first"))
    {
        return 1;
    }

    if (!Check(
            textureCache.Release(assetHandle).Succeeded(),
            "TextureAssetCache::Release final"))
    {
        return 1;
    }

    if (!Check(
            !textureCache.Contains(assetHandle) &&
                renderDevice.GetTextureCount() == 0U,
            "texture destruction"))
    {
        return 1;
    }

    if (!Check(
            textureCache.Shutdown().Succeeded(),
            "TextureAssetCache::Shutdown"))
    {
        return 1;
    }

    if (!Check(
            assets::Succeeded(assetManager.Unload(assetHandle)),
            "AssetManager::Unload"))
    {
        return 1;
    }

    if (!Check(
            assets::Succeeded(assetManager.Unregister(assetHandle)),
            "AssetManager::Unregister"))
    {
        return 1;
    }

    assetManager.Shutdown();
    renderDevice.Shutdown();

    std::puts("LTS.Assets tests passed");
    return 0;
}
