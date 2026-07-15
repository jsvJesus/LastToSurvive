# Reusable Static Model Renderer

Dependency direction is `GameEditor -> LTS.Renderer -> LTS.Math + LTS.Graphics + LTS.Assets`. Renderer public contracts contain no HWND, D3D11, D3DCompiler, Eternity or r3d types.

`StaticModelRenderer` owns compiled shader handles, input layout, six alpha/sided pipeline variants, shared semantic fallback textures, frame/object buffers, a generation-safe handle registry and two reference-counted caches. Normalized `StaticModel AssetPath` keys share one model resource while independent public handles retain generation safety. `(normalized AssetPath, RequestedColorSpace)` keys share one `GpuTexture`; Preserve, Linear and Srgb remain distinct keys. A model resource owns one `GpuMesh`, ordered GPU materials, samplers, b2 material buffers and the texture keys it acquired. Destroying the last model handle releases its materials and decrements every texture reference; a zero reference releases and erases that GPU texture. Fallbacks are outside the cache and survive until renderer shutdown.

Model construction is transactional. Mesh buffers, material buffers, samplers and texture acquisitions belong to a temporary resource until both the model-cache entry and public handle are published. Every failure and exception explicitly rolls them back through `RenderDevice`; `GpuMesh` and `GpuTexture` destructors are not used as implicit GPU cleanup.

The shader binding contract is:

- b0: view-projection, camera/time, directional light, ambient and debug mode;
- b1: world, inverse-transpose normal matrix, tint and object id;
- b2: material factors, alpha flags and compatibility surface scalars;
- t0..t5: BaseColor, Normal, SpecularGloss, Roughness, Emissive and SpecularPower;
- s0: material sampler.

BaseColor and Emissive request an sRGB-compatible GPU format. Normal, SpecularGloss, Roughness and SpecularPower request the corresponding linear format. `GpuTexture` only changes the format tag for bit-compatible UNorm/sRGB pairs; texture bytes are unchanged and unsupported reinterpretation returns `Unsupported`.

Opaque and Mask draws are stably grouped by pipeline/model/material with object id only as a deterministic tie breaker. Blend draws are ordered back-to-front using transformed bounds centers, followed by deterministic ties. Required bind results are propagated immediately and no draw follows a failed bind. Cleanup is best effort and records its first failed operation in diagnostics.

The normal matrix is the inverse-transpose linear transform. Translation is removed. Non-uniform scale is supported; singular transforms are rejected. Mirrored transforms multiply tangent handedness by the linear determinant sign.

The host owns render-target/depth clear, resize and Present. Shutdown unbinds b0/b1/b2, t0..t5 and samplers before releasing models, cached DDS resources, fallbacks, buffers, pipelines, layout and shaders.
