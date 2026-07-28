#pragma once

#include <string_view>

namespace engine::assets::file_extensions
{
    inline constexpr std::string_view Level = ".level";
    inline constexpr std::string_view Mesh = ".mesh";
    inline constexpr std::string_view Model = ".model";
    inline constexpr std::string_view SkeletalMesh = ".skmesh";
    inline constexpr std::string_view Skeleton = ".sk";
    inline constexpr std::string_view Animation = ".anim";
    inline constexpr std::string_view Material = ".material";
    inline constexpr std::string_view Shader = ".shader";
    inline constexpr std::string_view Texture = ".dds";

    inline constexpr std::wstring_view LevelWide = L".level";
    inline constexpr std::wstring_view MeshWide = L".mesh";
    inline constexpr std::wstring_view ModelWide = L".model";
    inline constexpr std::wstring_view SkeletalMeshWide = L".skmesh";
    inline constexpr std::wstring_view SkeletonWide = L".sk";
    inline constexpr std::wstring_view AnimationWide = L".anim";
    inline constexpr std::wstring_view MaterialWide = L".material";
    inline constexpr std::wstring_view ShaderWide = L".shader";
    inline constexpr std::wstring_view TextureWide = L".dds";
}
