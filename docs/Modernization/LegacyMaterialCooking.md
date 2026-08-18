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

## Surface-map compatibility pass

The cooker resolves every declared slot independently through the same bounded resolver. Strict lookup remains the default and `--allow-missing-textures` applies to every slot.

| Legacy field/register | MaterialAsset v2 | Shader channel/formula | Fallback |
|---|---|---|---|
| Texture / s0 | BaseColor | sRGB RGB; alpha unchanged | white |
| NormalMap / s1 | Normal | linear RGB, `rgb*2-1`, TBN | `(0.5,0.5,1)` |
| SpecularMap / s2 | SpecularGloss | linear R × legacy `SpecularPower` strength | black |
| EnvMap / s3 | Roughness | linear R × roughnessFactor; ReflectionPower remains separate | white |
| GlowMap / s4 | Emissive | sRGB RGB × emissiveFactor × SelfIllumMultiplier | black |
| SpecPowMap / s7 | SpecularPower | linear R modulates `Specular1Power` before exponent decoding | white |

Legacy tangent handedness is decoded by the SCB reader and the compatibility shader uses `B = cross(N,T) * tangent.w`. The old high-quality shader does not invert the normal-map green channel, so the compatibility path does not invert it either. Double-sided rendering flips the geometric normal before building the back-face TBN.

The legacy names are misleading: `SpecularPower` is the gloss/specular strength,
while `Specular1Power` controls highlight size. The converter stores the latter as
the decoded exponent `2^(1 + clamp(Specular1Power,0,1) * 10)`; the shader recovers
the normalized control so `SpecPowMap.r` is applied before exponent decoding, as
in the high-quality legacy fill/light passes. `ReflectionPower` and
`SelfIllumMultiplier` remain explicit compatibility scalars. `lowQSelfIllum` and
`lowQMetallness` remain diagnostics and are not inferred as modern PBR fields.

## Cooker output transaction

Model output is accepted only as a dedicated nested directory below the canonical
data root (for example `Data/CookedPreview/Model`). The data root, input directory,
any ancestor containing the input SCB, ordinary files, traversal through reparse
points, and derived temporary/backup paths intersecting source assets are rejected
before any temporary directory is created. Forced replacement first renames the old
output to a sibling backup, publishes the complete temporary directory, and removes
the backup. Publish failure rolls the backup back and removes temporary output; a
failed rollback is reported and the backup is preserved.
