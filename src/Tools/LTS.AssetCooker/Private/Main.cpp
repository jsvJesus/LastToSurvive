#include "Assets/LtsMeshWriter.h"
#include "Legacy/Assets/LegacyScbMeshDecoder.h"
#include "Platform/File.h"

#include <Windows.h>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <limits>

namespace
{
using engine::assets::AssetData;
using engine::assets::AssetResult;

AssetResult ReadFile(const std::filesystem::path &path, AssetData &output) noexcept
{
    engine::platform::File file(path);
    const auto size = file.GetSize();
    if (!file || !size)
        return AssetResult::IoError;
    if (*size > (std::numeric_limits<std::size_t>::max)())
        return AssetResult::FileTooLarge;
    const AssetResult resize = output.Resize(static_cast<std::size_t>(*size));
    if (engine::assets::Failed(resize))
        return resize;
    const auto read = file.Read(output.GetData(), output.GetSize());
    if (!read || read.bytesTransferred != output.GetSize())
    {
        output.Clear();
        return AssetResult::IoError;
    }
    return AssetResult::Success;
}

AssetResult WriteAtomic(const std::filesystem::path &path, const AssetData &data, const bool force) noexcept
{
    std::error_code error;
    if (std::filesystem::exists(path, error) && !force)
        return AssetResult::AlreadyExists;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error)
        return AssetResult::IoError;
    std::filesystem::path temporary = path;
    temporary += L".tmp";
    std::filesystem::remove(temporary, error);
    engine::platform::File file(temporary, engine::platform::FileAccess::Write,
                                engine::platform::FileCreation::CreateNew);
    if (!file)
        return AssetResult::IoError;
    const auto written = file.Write(data.GetData(), data.GetSize());
    if (!written || written.bytesTransferred != data.GetSize() || !file.Flush())
    {
        file.Close();
        std::filesystem::remove(temporary, error);
        return AssetResult::IoError;
    }
    file.Close();
    const DWORD flags = MOVEFILE_WRITE_THROUGH | (force ? MOVEFILE_REPLACE_EXISTING : 0U);
    if (!MoveFileExW(temporary.c_str(), path.c_str(), flags))
    {
        std::filesystem::remove(temporary, error);
        return AssetResult::IoError;
    }
    return AssetResult::Success;
}

void Print(const engine::legacy::assets::LegacyStaticMeshData &data, const std::filesystem::path &source)
{
    const auto &mesh = data.mesh;
    std::wprintf(L"source: %ls\nversion: 0x%08X\n", source.c_str(),
                 engine::legacy::assets::LegacyScbMeshDecoder::SupportedVersion);
    std::printf("mesh: %s\nvertices: %zu\nindices: %zu\ntriangles: %zu\nsubmeshes: %zu\n", data.sourceName.c_str(),
                mesh.GetVertexCount(), mesh.GetIndexCount(), mesh.GetIndexCount() / 3U, mesh.GetSubmeshCount());
    for (std::size_t i = 0U; i < data.materialSlotNames.size(); ++i)
        std::printf("material[%zu]: %s\n", i, data.materialSlotNames[i].c_str());
    const auto &b = mesh.GetBounds();
    std::printf("bounds: min %.3f %.3f %.3f, max %.3f %.3f %.3f, radius %.3f\n", b.minimum[0], b.minimum[1],
                b.minimum[2], b.maximum[0], b.maximum[1], b.maximum[2], b.sphereRadius);
}
} // namespace

int wmain(int argc, wchar_t **argv)
{
    if (argc < 3)
    {
        std::fwprintf(stderr, L"usage: LTS.AssetCooker inspect-mesh <input.scb> | "
                              L"mesh <input.scb> <output.ltsmesh> [--force]\n");
        return 2;
    }
    const bool inspect = std::wstring_view(argv[1]) == L"inspect-mesh";
    const bool convert = std::wstring_view(argv[1]) == L"mesh";
    if ((!inspect && !convert) || (inspect && argc != 3) || (convert && (argc < 4 || argc > 5)))
        return 2;
    const std::filesystem::path input = std::filesystem::absolute(argv[2]).lexically_normal();
    const std::filesystem::path output =
        convert ? std::filesystem::absolute(argv[3]).lexically_normal() : std::filesystem::path{};
    const bool force = argc == 5 && std::wstring_view(argv[4]) == L"--force";
    if (convert && input == output)
    {
        std::fwprintf(stderr, L"input and output must differ\n");
        return 2;
    }
    const auto start = std::chrono::steady_clock::now();
    AssetData source;
    AssetResult result = ReadFile(input, source);
    engine::legacy::assets::LegacyStaticMeshData decoded;
    if (engine::assets::Succeeded(result))
        result = engine::legacy::assets::LegacyScbMeshDecoder::Decode(source, decoded);
    if (engine::assets::Failed(result))
    {
        std::fprintf(stderr, "decode failed: %s\n", engine::assets::ToString(result));
        return 3;
    }
    Print(decoded, input);
    if (convert)
    {
        AssetData cooked;
        result = engine::assets::LtsMeshWriter::Encode(decoded.mesh, cooked);
        if (engine::assets::Succeeded(result))
            result = WriteAtomic(output, cooked, force);
        if (engine::assets::Failed(result))
        {
            std::fprintf(stderr, "write failed: %s\n", engine::assets::ToString(result));
            return 4;
        }
        std::printf("output bytes: %zu\n", cooked.GetSize());
    }
    const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::printf("elapsed ms: %.3f\n", elapsed);
    return 0;
}
