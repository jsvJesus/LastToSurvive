#ifndef LTS_STUDIO_DX9
#define LTS_STUDIO_DX9 1
#endif

#ifndef LTS_STUDIO_DX11
#define LTS_STUDIO_DX11 0
#endif

#ifndef LTS_STUDIO_DX11_WORLD
#define LTS_STUDIO_DX11_WORLD LTS_STUDIO_DX11
#endif

#if !LTS_STUDIO_DX9 && !LTS_STUDIO_DX11
#error No Studio renderer backend enabled. Enable LTS_STUDIO_DX9 or LTS_STUDIO_DX11.
#endif