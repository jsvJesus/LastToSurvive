# Reusable Static Model Renderer

Dependency direction is `GameEditor -> LTS.Renderer -> LTS.Math + LTS.Graphics + LTS.Assets`. Renderer public contracts contain no HWND, D3D11, D3DCompiler, Eternity or r3d types.

`StaticModelRenderer` owns compiled shader handles, input layout, six alpha/sided pipeline variants, shared semantic fallback textures, frame/object buffers, a generation-safe model registry and the `(AssetPath, requestedColorSpace)` texture cache. A model resource owns one `GpuMesh`, ordered GPU materials, samplers and b2 material buffers. Instances own only a model handle, world transform, tint, visibility and object id.

The shader binding contract is:

- b0: view-projection, camera/time, directional light, ambient and debug mode;
- b1: world, inverse-transpose normal matrix, tint and object id;
- b2: material factors, alpha flags and compatibility surface scalars;
- t0..t5: BaseColor, Normal, SpecularGloss, Roughness, Emissive and SpecularPower;
- s0: material sampler.

BaseColor and Emissive request an sRGB-compatible GPU format. Normal, SpecularGloss, Roughness and SpecularPower request the corresponding linear format. `GpuTexture` only changes the format tag for bit-compatible UNorm/sRGB pairs; texture bytes are unchanged and unsupported reinterpretation returns `Unsupported`.

Opaque and Mask draws are grouped by resource/material/pipeline. Blend draws are ordered back-to-front using transformed bounds centers. The host owns render-target/depth clear, resize and Present. Shutdown unbinds b0/b1/b2, t0..t5 and samplers before releasing models, cached DDS resources, fallbacks, buffers, pipelines, layout and shaders.
