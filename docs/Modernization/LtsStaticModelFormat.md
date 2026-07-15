# LTS Static Model Format (`.ltsmodel`)

Version 1 is a deterministic little-endian container linking one cooked mesh to
an ordered list of cooked materials. Material entry order is the mesh material
slot order. Paths are normalized `AssetPath` values relative to one asset root.

The 64-byte header contains `LTSMODEL`, version `1`, endian marker `0x01020304`,
exact header size, material count, mesh path offset/length, material table offset,
and optional debug-name offset/length. Reserved fields are zero. Each 16-byte
material table entry contains a 64-bit path offset, 32-bit byte length, and a
zero reserved field.

Regions are canonical and contiguous: header, path table, mesh path, ordered
material paths, optional debug name. Empty required paths, absolute/non-normalized
paths, wrong suffixes, gaps, overlaps, arithmetic overflow, non-zero reserved
fields, unsupported versions, and trailing bytes are rejected.
