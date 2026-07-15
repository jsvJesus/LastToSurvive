# Legacy material cooking contract

The static-model cooker reproduces the observed `r3dMaterialLibrary` lookup without
constructing an `r3dMaterial`. For a mesh below `Data/ObjectsDepot/<depot>`, a
material slot named `Name` resolves to
`Data/ObjectsDepot/<depot>/Materials/Name.mat`. In-memory material-name lookup is
ASCII case-insensitive, matching `r3dMaterialLibrary::HasMaterial` (`stricmp`).

The default strict diffuse directory is `Data/ObjectsDepot/<depot>/Textures`. An explicit
`ImagesDir` replaces that base completely. The resolver checks the legacy source spelling and
then the same path with `.dds`; Windows path matching is case-insensitive. It does not fall back
to the depot or material directory when `ImagesDir` is present. A material-directory candidate
exists only behind the explicit `--relaxed-texture-lookup` cooker flag and emits a warning.
Every candidate is normalized and must remain below the supplied data root. Cooked paths
are relative normalized `AssetPath` values; absolute paths and `..` escapes are
rejected.

Reliable field mapping is deliberately small: `Color24` becomes linear preview
base-color bytes divided by 255, `DoubleSided` controls culling,
`AlphaTransparent` selects Blend, and `ForceTransparent` or
`TransparentShadows` selects Mask with cutoff 0.5. Otherwise an alpha-capable diffuse format
selects Mask, matching the intent of legacy `SetAlphaFlag`. BC2/BC3 and RGBA/BGRA formats are
alpha-capable. BC1 is opaque unless a one-bit transparent selector is found. RGB, single/two
channel, BC4 and BC5 formats are opaque. BC7 is ambiguous and conservatively selects Mask with
a diagnostic. Diffuse `Texture` becomes the
base-color texture with a Linear/Wrap sampler. Specular/reflection/low-quality
metalness, self illumination, and all other texture slots remain explicit
diagnostics; no implicit PBR or emissive heuristic is applied.

## Cooker output transaction

Model output is accepted only as a dedicated nested directory below the canonical
data root (for example `Data/CookedPreview/Model`). The data root, input directory,
any ancestor containing the input SCB, ordinary files, traversal through reparse
points, and derived temporary/backup paths intersecting source assets are rejected
before any temporary directory is created. Forced replacement first renames the old
output to a sibling backup, publishes the complete temporary directory, and removes
the backup. Publish failure rolls the backup back and removes temporary output; a
failed rollback is reported and the backup is preserved.
