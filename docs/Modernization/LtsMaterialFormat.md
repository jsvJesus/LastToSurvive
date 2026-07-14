# LTS Material format (`.ltsmat`)

Version 1 is a strictly little-endian, fixed-width material record with an optional normalized UTF-8 asset path. Trailing bytes are forbidden.

The 160-byte header contains: magic `LTSMAT\0\0` (offset 0), version 1 (8), endian marker `0x01020304` (12), header size (16), alpha mode u32 (20: opaque/mask/blend), flags u32 (24: bit 0 double-sided, bit 1 base-color texture), base color f32x4 (28), emissive f32x3 (44), metallic/roughness/alpha cutoff f32 (56/60/64), sampler filter/address U/V/W u32 (68..80), mip bias f32 (84), anisotropy u32 (88), comparison u32 (92), border f32x4 (96), min/max LOD f32 (112/116), path offset u64 (120), and path byte length u32 (128). Bytes 132..159 are reserved and zero.

When bit 1 is clear, path length is zero and file size is exactly 160. When set, the path immediately follows the header, is 1..1024 bytes, has no terminator, and must pass `AssetPath` normalization rules. Runtime pointers, `bool`, `size_t`, enum object layouts, and C++ strings are never serialized. Unknown flags, comparison sampling, invalid enum/scalar ranges, and non-finite values are rejected. Incompatible changes require a new version.
