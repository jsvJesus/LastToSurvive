# LTS Shader Asset Format

`.ltsshader` is a deterministic little-endian container for compiler output. Runtime modules consume bytecode only; HLSL and D3DCompiler types are not part of `ShaderAsset`.

## Version 1 header

The fixed header is 128 bytes.

| Offset | Type | Meaning |
|---:|---|---|
| 0 | char[8] | `LTSSHDR\0` |
| 8 | u32 | version, `1` |
| 12 | u32 | endian marker, `0x01020304` |
| 16 | u32 | exact header size, `128` |
| 20 | u32 | fixed `ShaderStage` value |
| 24 | u32 | flags, zero in v1 |
| 28 | u32 | reserved, zero |
| 32 | u64 + u32 + u32 | target-profile offset/length/reserved |
| 48 | u64 + u32 + u32 | entry-point offset/length/reserved |
| 64 | u64 + u32 + u32 | debug-name offset/length/reserved |
| 80 | u64 + u32 + u32 | bytecode offset/size/reserved |
| 96 | u64 | source/content hash |
| 104 | byte[24] | reserved, zero |

Canonical payload ordering is profile, entry point, optional debug name, then bytecode. Regions must be dense and non-overlapping, the bytecode begins with `DXBC`, and trailing bytes are rejected. Metadata is limited to 1024 bytes per string and bytecode to 16 MiB.

The container version and the source-hash contract are independent. Container version 1 currently uses source-hash contract version 2. The 64-bit framed FNV-1a hash covers the main source path relative to the canonical include root, main source bytes, each relative include path and its bytes in compiler-open order, shader stage, entry point, profile, sorted definitions and their values, compiler flags, and the hash-contract version. Absolute checkout paths are never hashed. Cooked output uses strictness, warnings-as-errors and optimization level 3 without debug information or absolute source paths.

`ID3DInclude::Open` and `Close` catch every C++ exception at the COM callback boundary. Allocation failures become `E_OUTOFMEMORY`; other failures become `E_FAIL`. Include sizes are checked against `UINT` before conversion and partially inserted callback state is rolled back.

## Cooker

```powershell
LTS.AssetCooker.exe shader --input StaticModelCompatibility.hlsl `
  --entry VSMain --stage vertex --profile vs_4_0 `
  --include-root Shaders --output StaticModelCompatibility.vs.ltsshader --force
```

Pixel shaders use `--stage pixel --profile ps_4_0`. Includes must resolve inside the canonical include root; absolute paths, traversal/reparse escape and cycles are rejected.
