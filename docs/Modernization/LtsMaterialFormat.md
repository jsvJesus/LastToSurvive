# LTS Material format (`.ltsmat`)

All integers and floats are little-endian and fixed-width. Runtime pointers, `bool`, `size_t`, C++ enum layouts and terminated strings are never serialized.

## Version 1 compatibility

Version 1 remains readable without layout changes. Its header is exactly 160 bytes: magic `LTSMAT\0\0`, version at 8, endian marker `0x01020304` at 12, header size at 16, alpha/flags at 20/24, material and sampler fields at 28..119, one optional BaseColor path descriptor at 120..131, and zero reserved bytes at 132..159. Version 1 receives v2 defaults: normal scale 1, specular intensity 0, specular power 32, reflection factor 0, emissive strength 0, and no additional maps.

## Version 2

The v2 fixed header is exactly 192 bytes:

| Offset | Type | Meaning |
|---:|---|---|
| 0 | char[8] | `LTSMAT\0\0` |
| 8 | u32 | version = 2 |
| 12 | u32 | endian = `0x01020304` |
| 16 | u32 | header size = 192 |
| 20 | u32 | alpha mode |
| 24 | u32 | flags; bit 0 DoubleSided |
| 28..119 | fixed fields | v1 color/material/sampler fields |
| 120 | f32 | normalScale |
| 124 | f32 | specularIntensity |
| 128 | f32 | specularPower |
| 132 | f32 | reflectionFactor |
| 136 | f32 | emissiveStrength |
| 140 | u32 | texture entry count, 0..6 |
| 144 | u64 | table offset, 192 when non-empty, otherwise zero |
| 152..191 | bytes | reserved zero |

Each 24-byte table entry is `(semantic u32, reserved u32, pathOffset u64, pathLength u32, reserved u32)`. Semantics are 0 BaseColor, 1 Normal, 2 SpecularGloss, 3 Roughness, 4 Emissive and 5 SpecularPower. Entries are strictly increasing and paths immediately follow the complete table in entry order, with no gaps, overlaps or terminators. Duplicate paths across semantics are legal; duplicate semantics are not. Unknown flags/semantics, non-zero reserved fields, invalid paths/scalars, non-canonical ordering and trailing bytes are rejected. The writer always emits v2 deterministically.

## Color space and compatibility

BaseColor and Emissive are color data and are sampled as sRGB. Normal, SpecularGloss, Roughness and SpecularPower are linear data. Studio uses native sRGB formats where present and shader-side sRGB-to-linear conversion only for legacy UNorm color DDS. The cache key contains the color/data interpretation, preventing an incompatible view from being reused.

This is a compatibility material, not metallic/roughness PBR. Legacy SpecularMap remains a specular/gloss control and never changes `metallicFactor`. EnvMap is the legacy roughness/environment-power scalar map. ReflectionPower is a compatibility factor, not a cubemap reflection strength.
