# Legacy material cooking contract

The static-model cooker reproduces the observed `r3dMaterialLibrary` lookup without
constructing an `r3dMaterial`. For a mesh below `Data/ObjectsDepot/<depot>`, a
material slot named `Name` resolves to
`Data/ObjectsDepot/<depot>/Materials/Name.mat`. In-memory material-name lookup is
ASCII case-insensitive, matching `r3dMaterialLibrary::HasMaterial` (`stricmp`).

The default diffuse directory is `Data/ObjectsDepot/<depot>/Textures`. An explicit
`ImagesDir` replaces that base. The resolver checks the legacy source spelling and
the same extension with `.dds`; Windows path matching is case-insensitive. A
material-directory candidate supports standalone material libraries. Every
candidate is normalized and must remain below the supplied data root. Cooked paths
are relative normalized `AssetPath` values; absolute paths and `..` escapes are
rejected.

Reliable field mapping is deliberately small: `Color24` becomes linear preview
base-color bytes divided by 255, `DoubleSided` controls culling,
`AlphaTransparent` selects Blend, and `ForceTransparent` or
`TransparentShadows` selects Mask with cutoff 0.5. Diffuse `Texture` becomes the
base-color texture with a Linear/Wrap sampler. Specular/reflection/low-quality
metalness, self illumination, and all other texture slots remain explicit
diagnostics; no implicit PBR or emissive heuristic is applied.
