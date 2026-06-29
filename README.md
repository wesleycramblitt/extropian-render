# extropian-render

**3D rendering abstraction with multiple backends and scientific visualization.**

Depends on `extropian-core`. Does NOT depend on `extropian-app` or `extropian-physics`.

## Architecture

```
ext::render::IRenderer     ← Backend-neutral interface
    ├── OpenGL backend      (current production)
    ├── Vulkan backend      (future)
    ├── WebGL backend       (future, via Emscripten)
    └── Null backend        (headless / CI / batch export)

ext::render::RenderGraph    ← Pass DAG with resource handles
ext::render::Camera         ← Camera model
ext::render::Material       ← PBR material
ext::render::Light          ← Light types

ext::render::vis::*         ← Scientific visualization (backend-agnostic)
    ├── Isosurface
    ├── Streamline
    ├── Volume render
    ├── Slice plane
    └── Particle system
```

## Building

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

Requires: `extropian-core`, OpenGL, SDL3.

## License

Business Source License 1.1 — see [LICENSE](LICENSE).
Converts to Apache 2.0 on 2029-05-26.
