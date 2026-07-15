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
    assets::AssetLoaderRegistry loaderRegistry;

    if (!Check(
            assets::Succeeded(loaderRegistry.Register(ddsLoader)),
            "AssetLoaderRegistry::Register"))
    {
        return 1;
    }
    if(!Check(assets::Succeeded(loaderRegistry.Register(meshLoader))&&assets::Succeeded(loaderRegistry.Register(materialLoader)),"mesh/material loader registration"))return 1;

    assets::AssetData mesh16,mesh32,materialData,materialTextureData;
    if(!Check(assets::Succeeded(BuildTestMesh(mesh16))&&assets::Succeeded(BuildTestMesh(mesh32,true))&&assets::Succeeded(BuildTestMaterial(materialData))&&assets::Succeeded(BuildTestMaterial(materialTextureData,"textures/test.dds")),"mesh/material fixtures"))return 1;
    assets::AssetPath meshPath,materialPath; (void)assets::AssetPath::TryCreate("meshes/test.ltsmesh",meshPath);(void)assets::AssetPath::TryCreate("materials/test.ltsmat",materialPath);
    assets::AssetMetadata meshMetadata;meshMetadata.path=meshPath;meshMetadata.id=meshPath.GetId();meshMetadata.type=assets::AssetType::Mesh;meshMetadata.sourceSize=mesh16.GetSize();
    assets::AssetMetadata materialMetadata;materialMetadata.path=materialPath;materialMetadata.id=materialPath.GetId();materialMetadata.type=assets::AssetType::Material;materialMetadata.sourceSize=materialData.GetSize();
    std::unique_ptr<assets::LoadedAsset> foundationAsset;
    if(!Check(assets::Succeeded(loaderRegistry.Load(meshMetadata,mesh16,foundationAsset))&&foundationAsset&&foundationAsset->GetType()==assets::AssetType::Mesh,"valid UInt16 mesh load"))return 1;
    if(!Check(assets::Succeeded(loaderRegistry.Load(meshMetadata,mesh32,foundationAsset)),"valid UInt32 mesh load"))return 1;
    if(!Check(assets::Succeeded(loaderRegistry.Load(materialMetadata,materialData,foundationAsset))&&foundationAsset->GetType()==assets::AssetType::Material,"valid material load"))return 1;
    if(!Check(assets::Succeeded(loaderRegistry.Load(materialMetadata,materialTextureData,foundationAsset)),"material texture path load"))return 1;
    (void)loaderRegistry.Load(meshMetadata,mesh16,foundationAsset);assets::MeshAsset roundMesh=static_cast<assets::MeshLoadedAsset*>(foundationAsset.get())->ReleaseMesh();assets::AssetData encodedMeshA,encodedMeshB;
    if(!Check(assets::Succeeded(assets::LtsMeshWriter::Encode(roundMesh,encodedMeshA))&&assets::Succeeded(assets::LtsMeshWriter::Encode(roundMesh,encodedMeshB))&&encodedMeshA.GetSize()==encodedMeshB.GetSize()&&std::memcmp(encodedMeshA.GetData(),encodedMeshB.GetData(),encodedMeshA.GetSize())==0&&assets::Succeeded(loaderRegistry.Load(meshMetadata,encodedMeshA,foundationAsset)),"deterministic mesh writer round-trip"))return 1;
    (void)loaderRegistry.Load(materialMetadata,materialTextureData,foundationAsset);assets::MaterialAsset roundMaterial=static_cast<assets::MaterialLoadedAsset*>(foundationAsset.get())->ReleaseMaterial();assets::AssetData encodedMaterial;
    if(!Check(assets::Succeeded(assets::LtsMaterialWriter::Encode(roundMaterial,encodedMaterial))&&assets::Succeeded(loaderRegistry.Load(materialMetadata,encodedMaterial,foundationAsset)),"material writer round-trip"))return 1;
    assets::AssetData scbData;if(!Check(assets::Succeeded(BuildTestScb(scbData)),"SCB fixture"))return 1;engine::legacy::assets::LegacyStaticMeshData legacyMesh;
    if(!Check(assets::Succeeded(engine::legacy::assets::LegacyScbMeshDecoder::Decode(scbData,legacyMesh))&&legacyMesh.mesh.IsValid()&&legacyMesh.materialSlotNames.size()==1U,"legacy SCB decode"))return 1;assets::AssetData scbCooked;
    if(!Check(assets::Succeeded(assets::LtsMeshWriter::Encode(legacyMesh.mesh,scbCooked))&&assets::Succeeded(loaderRegistry.Load(meshMetadata,scbCooked,foundationAsset)),"SCB to LTSMESH round-trip"))return 1;
    assets::AssetData badScb;(void)badScb.Resize(scbData.GetSize());std::memcpy(badScb.GetData(),scbData.GetData(),scbData.GetSize());WriteU32(badScb.GetData(),0U,1U);if(!Check(assets::Failed(engine::legacy::assets::LegacyScbMeshDecoder::Decode(badScb,legacyMesh))&&legacyMesh.IsEmpty(),"wrong SCB version leaves empty output"))return 1;std::memcpy(badScb.GetData(),scbData.GetData(),scbData.GetSize());WriteU32(badScb.GetData(),4U,1U);if(!Check(engine::legacy::assets::LegacyScbMeshDecoder::Decode(badScb,legacyMesh)==assets::AssetResult::UnsupportedFeature&&legacyMesh.IsEmpty(),"SCB weights explicitly unsupported"))return 1;(void)badScb.Resize(scbData.GetSize()+1U);std::memcpy(badScb.GetData(),scbData.GetData(),scbData.GetSize());badScb.GetData()[scbData.GetSize()]=std::byte{1};if(!Check(assets::Failed(engine::legacy::assets::LegacyScbMeshDecoder::Decode(badScb,legacyMesh))&&legacyMesh.IsEmpty(),"SCB trailing garbage rejected"))return 1;
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
