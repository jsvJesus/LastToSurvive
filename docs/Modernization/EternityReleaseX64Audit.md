# Release x64: Eternity removal status

Updated: 2026-08-09.

## Result

`src/EclipseStudio/WarZ.sln` builds successfully as `Release|x64` after the
physical removal of `src/Eternity`.

The production solution graph now contains:

- `Studio` (`GameEditor.vcxproj`)
- `LTS.Core`, `LTS.Math`, `LTS.Platform`, `LTS.Tasks`, `LTS.Runtime`
- `LTS.Graphics`, `LTS.Graphics.DX11`, `LTS.Assets`, `LTS.Renderer`
- `LTS.Application`, `LTS.ImGui`, `LTS.UI`, `LTS.Scene`
- `LTS.Navigation` (Recast/Detour built from `External/recastnavigation`)
- `SLikeNetLibStatic` and the backend `Api`

The following obsolete projects are not part of `WarZ.sln` anymore:

- `Eternity`
- `LTS.Legacy`
- `LTS.Graphics.DX9`
- the old `GameEditor`, `MasterServer`, `SupervisorServer`, and `GameServer`
  targets whose translation units require `r3dPCH.h`

The DX9 and Legacy engine module sources were removed after their last active
consumer disappeared. `GraphicsBackend` and `RendererBackend` no longer expose
a D3D9 value, and `EngineConfig` defaults to D3D11.

## Studio

`GameEditor.vcxproj` is now the standalone x64 DX11 Studio executable. It does
not compile the legacy EclipseStudio/GameEngine source list and contains no
Eternity include directory, library, PCH, or project reference.

The executable uses the existing engine implementations directly:

1. `LTS.Application` creates the Win32 window and owns the message/frame loop.
2. `LTS.Graphics.DX11` owns the D3D11 device, immediate context, and swap chain.
3. `LTS.ImGui` owns the editor UI context and DX11/Win32 bindings.
4. `LTS.Renderer`, `LTS.Assets`, `LTS.Scene`, and `LTS.Navigation` are available
   as normal LTS project dependencies without an r3d compatibility layer.

The resulting `build/x64/Release/Bin/Studio.exe` imports `d3d11.dll` and does
not import `d3d9.dll` or `d3dx9_43.dll`.

## Navigation boundary

Autodesk Navigation and NVIDIA APEX sources were removed from `src/GameEngine`.
`LTS.Navigation` is independent from r3d and uses the Recast sources already
present in `External/recastnavigation`.

No level-file convention is imposed by this slice. The previously introduced
editor generator and hard-coded `navigation/recast/level.nav` loading were
removed. `ServerNavigation::LoadFromFile` accepts an explicit path; connecting
it to a future level format remains a separate task.

## Terrain boundary

The legacy TrueNature/Terrain2 code is not compiled, copied, or ported by the
new solution. No new terrain renderer, editor, level format, or navigation
geometry extraction was introduced here.

## Legacy source boundary

The old `src/GameEngine`, `src/EclipseStudio/Sources`, and `server` source trees
still contain r3d-based code and are retained only as migration input. They are
not dependencies of any project in `WarZ.sln`. Reintroducing any of those units
requires rewriting the consumer against `Core`, `Math`, `Platform`, `Assets`,
`Runtime`, `GraphicsDX11`, `Renderer`, `Scene`, or `Navigation`; restoring an
r3d PCH or compatibility API is not an allowed path.

## Verification

Build command:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' `
  src\EclipseStudio\WarZ.sln /t:Rebuild `
  '/p:Configuration=Release;Platform=x64' /m /v:minimal /nologo
```

Expected result: exit code `0` and `build/x64/Release/Bin/Studio.exe`.
