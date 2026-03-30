# Copilot Instructions — 360-VJ Mirror Dome

## Project Overview

This repo contains **two FFGL (FreeFrame GL) plugins** for [Resolume](https://resolume.com/) VJ software that reproject video between projection formats (equirectangular, fisheye, flat, cubemap, mirror dome) in real time using a shared OpenGL fragment shader.

1. **Reprojection** — General-purpose reprojection between equirectangular, fisheye, flat, and cubemap formats. No mirror dome parameters.
2. **MirrorDome** — Full reprojection plus Paul Bourke's spherical mirror projection (projector → mirror → dome ray tracing). Exposes additional mirror dome parameters.

## Architecture

```
CMakeLists.txt              — Defines both plugin targets using add_ffgl_plugin()
Reprojection/
    Reprojection.h / .cpp   — Reprojection plugin host interface (plugin ID "RPRJ")
    Shader.h                — Shared GLSL 410 fragment shader (raw string literal)
MirrorDome/
    MirrorDome.h / .cpp     — MirrorDome plugin host interface (plugin ID "MRRD")
```

- Both plugins' classes are named `AddSubtract` (inherited from the FFGL SDK example — do NOT rename, it must match the SDK build scaffolding).
- **Shader.h** lives in `Reprojection/` and is `#include`d by both plugins. It contains the *entire* GLSL 410 fragment shader as a C++ raw string literal (`_fragmentShaderCode[]`). The string is split with `)" R"(` because of MSVC string length limits.
- **CMakeLists.txt** defines two `add_ffgl_plugin()` targets. The MirrorDome target references `Reprojection/Shader.h` as a source.
- **Reprojection** has only the common parameters: input/output projection, stereo, pitch, roll, yaw, fov in/out.
- **MirrorDome** has all of the above plus mirror dome parameters: mirror radius, proj distance, proj lift, mirror proj FoV, proj tilt, dome radius.

## Build Process (non-obvious)

This repo does **not** build standalone. To build:
1. Clone [resolume/ffgl](https://github.com/resolume/ffgl).
2. Place these files inside the SDK's plugin directory (e.g., replace the `AddSubtract` example).
3. Per-plugin: rename the `.cpp`/`.h` to `AddSubtract.cpp`/`AddSubtract.h` and update the `#include` accordingly.
4. Build using the SDK's CMake pipeline. CI/CD uses Joris De Jong's pipeline from the FFGL repo.

## Parameter Convention

All user-facing parameters are **normalized [0,1] floats** from Resolume sliders. They are mapped to physical ranges in **two places that must stay in sync**:
- **C++ (`ProcessOpenGL`)**: Maps slider → uniform value (e.g., `mirrorRadius`: `0.01 + val * 0.49`)
- **C++ (`GetParameterDisplay`)**: Maps slider → display string using the same formula
- **GLSL**: Receives the already-mapped value; comments document expected ranges

When adding or changing a parameter: update the `ParamType` enum, constructor (`SetParamInfof`/`SetOptionParamInfo`), `SetFloatParameter`, `GetFloatParameter`, `GetParameterDisplay`, `ProcessOpenGL` uniform upload, and the GLSL uniform declaration + usage. If the parameter is shared, update **both** plugin .cpp files.

## Shader Conventions

- All projection math goes through a **lat/lon intermediate**: output UV → lat/lon → 3D point → rotation → lat/lon → input UV.
- Transparency is signaled via a global `bool isTransparent` flag and returning `SET_TO_TRANSPARENT` (`vec2(-1, -1)`). Always check `isTransparent` after calling any UV-to-latlon or latlon-to-UV function.
- Projection type constants (`EQUI=0, FISHEYE=1, FLAT=2, CUBEMAP=3, MIRROR_DOME=4`) must match between the GLSL `const int` values and the C++ `SetParamElementInfo` option indices.
- Input projections support: equirectangular, fisheye, flat, cubemap. Output projections support all five including cubemap and mirror dome.
- The shader contains all code for both plugins. Mirror dome uniforms that are not uploaded by the Reprojection plugin simply retain their default values.

## Mirror Dome Specifics (MirrorDome plugin only)

The mirror dome pipeline chains three stages in GLSL:
1. `mirrorUvToMirrorLatLon` — ray from projector through pixel → sphere intersection on mirror
2. `mirrorLatLonToDomePoint` — reflect off mirror → intersect dome hemisphere (radius 1.0)
3. `mirrorDomeUvToLatLon` — chains the above, returns lat/lon on dome

Mirror parameters and their slider-to-physical mappings:
| Parameter | Slider [0,1] → Physical |
|-----------|------------------------|
| `mirrorRadius` | `0.01 + v * 0.49` |
| `projDistance` | `0.5 + v * 4.5` |
| `projLift` | `v * 2.0` |
| `mirrorProjFov` | `0.1 + v * 2.94` (radians) |
| `projTilt` | `(v - 0.5) * π` (radians) |

## Key Gotchas

- The class name `AddSubtract` is used in **both** plugins — it matches the FFGL SDK example for build compatibility.
- Plugin unique IDs: Reprojection = `"RPRJ"`, MirrorDome = `"MRRD"` (max 4 chars, registered with FFGL).
- Stereo mode (Over/Under, Side by Side) halves and recomposes UVs in the GLSL `main()` — edits to UV handling must account for this.
- `MaxUV` is applied **after** all reprojection math to fix texture seam artifacts (see [issue #10](https://github.com/DanielArnett/360-VJ/issues/10)).
- The Reprojection plugin does **not** expose mirror dome output or parameters — its output projection options stop at Cubemap.
