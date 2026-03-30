# Reprojection

Two [FFGL](https://github.com/resolume/ffgl) plugins for [Resolume](https://resolume.com/) that reproject video between projection formats in real time.

- **Reprojection** — Convert between equirectangular, fisheye, flat, and cubemap projections. Rotate and adjust FoV.
- **MirrorDome** — Everything in Reprojection, plus Paul Bourke's spherical mirror dome projection (projector → mirror → dome ray tracing).

Both plugins share a single GLSL fragment shader (`Reprojection/Shader.h`).

## Building

1. Clone [resolume/ffgl](https://github.com/resolume/ffgl).
2. Place these files inside the SDK's plugin directory (e.g., replace the `AddSubtract` example).
3. Per-plugin: rename the `.cpp`/`.h` to `AddSubtract.cpp`/`AddSubtract.h` and update the `#include` accordingly.
4. Build using the SDK's CMake pipeline.

Build artifacts are kept out of this repo so we can use Joris De Jong's CI/CD pipeline.

## License

If you use this software, please send me pics of cool projects you do with it.
