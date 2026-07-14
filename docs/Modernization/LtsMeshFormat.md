# LTS Mesh format (`.ltsmesh`)

Version 1 is a strictly little-endian cooked static-mesh format. Trailing bytes are forbidden. Readers must reject arithmetic overflow, overlapping/out-of-file regions, unreasonable counts, invalid bounds, indices, submeshes, and material slots.

## Header (160 bytes)

| Offset | Type | Field |
|---:|---|---|
| 0 | u8[8] | `LTSMESH\0` |
| 8 | u32 | version (`1`) |
| 12 | u32 | endian marker (`0x01020304`) |
| 16 | u32 | header size (at least 160) |
| 20 | u32 | vertex stride (`48`) |
| 24 | u32 | index format (`1` u16, `2` u32) |
| 28..40 | u32 | vertex, index, submesh, material-slot counts |
| 44 | u32 | reserved, zero |
| 48..88 | u64 pairs | vertex, index, submesh offset and byte size |
| 96..132 | f32 | AABB min3, max3, sphere center3, radius |
| 136..159 | u8 | reserved, zero |

Each 48-byte vertex is position f32x3, normal f32x3, tangent f32x4, UV0 f32x2. Indices are a packed array of the declared type. Each 16-byte submesh is `firstIndex u32`, `indexCount u32`, `baseVertex i32`, `materialSlot u32`.

Version-1 limits are 10,000,000 vertices, 30,000,000 indices, and 65,536 submeshes/material slots. New compatible metadata may use reserved bytes with a later header/version; incompatible layout changes require a new version.
