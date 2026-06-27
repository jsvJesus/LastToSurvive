#pragma once

// ============================================================
// Studio renderer configuration
//
// Stage 2:
// - UI / RmlUI / main renderer stay DX9
// - World renderer switch is prepared separately
// - DX11 world is disabled by default
// ============================================================

#ifndef LTS_STUDIO_DX9
#define LTS_STUDIO_DX9 1
#endif

#ifndef LTS_STUDIO_DX11
#define LTS_STUDIO_DX11 0
#endif

// Important:
// Do NOT inherit this from LTS_STUDIO_DX11 automatically.
// We want to be able to compile DX11 code later without switching
// the world renderer immediately.
#ifndef LTS_STUDIO_DX11_WORLD
#define LTS_STUDIO_DX11_WORLD 0
#endif

#if LTS_STUDIO_DX11_WORLD && !LTS_STUDIO_DX11
#error LTS_STUDIO_DX11_WORLD requires LTS_STUDIO_DX11.
#endif

#if !LTS_STUDIO_DX9 && !LTS_STUDIO_DX11
#error No Studio renderer backend enabled. Enable LTS_STUDIO_DX9 or LTS_STUDIO_DX11.
#endif