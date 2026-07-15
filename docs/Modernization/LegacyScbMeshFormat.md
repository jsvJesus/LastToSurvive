# Legacy WarZ SCB static mesh contract

The compatibility decoder supports little-endian version `0xFADC0038` only. It reads fixed-width values with a bounded cursor and never reinterprets legacy runtime structures.

Order: `u32 version`, `u32 flags`, `i32 meshNameLength`, name bytes, pivot `f32x3`, `i32 vertexCount`, then position `f32x3[]`, UV `f32x2[]`, normal `f32x3[]`, tangent `f32x3[]`, signed tangent-hand byte `i8[]`; `i32 indexCount`, `u32 indices[]`; `i32 materialChunkCount`, then each chunk as `i32 startIndex`, `i32 endIndex` (exclusive), `i32 nameLength`, name bytes. Flag bit 0 appends legacy skin weights and is deliberately unsupported by the static cooker. Flag bit 1 appends four color bytes per vertex; colors are validated/skipped because the canonical vertex currently has no color channel. Unknown flags and trailing data are rejected.

Names are treated as bounded legacy byte strings without embedded NUL rather than claimed as UTF-8. Material names are deduplicated by exact byte value in first-appearance order. Normals are safely normalized, tangent sign values greater than zero map to `+1`, and zero/negative values map to `-1`.
