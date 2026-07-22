# extropian-render

**Reusable ECS-based 3D rendering framework with default shaders, GPU resource management, and scientific visualization techniques.**

Depends on `extropian-core` and `extropian-app`. Does NOT depend on `extropian-canvas` or `extropian-composer`.

## Position in the Architecture

```
composer → canvas → renderer → app → core
                        │         │
                        └──→ core ─┘
```

Renderer provides the rendering ECS components and systems. It is reusable for any visual application — with or without Canvas. When used with Canvas, Canvas writes renderer-understandable ECS components and calls `renderer.render_frame()`.

Renderer has no knowledge of Canvas, Composer, or semantic layers.

## Architecture

```
ECS Systems (application-independent)
├── RenderSystem         — 5-pass draw (cubemap → opaque → reflective → particles → volume)
├── CameraSystem         — FPS camera controller (WASD + mouse)
├── PrimitiveMeshSystem  — convenience: CubePrimitive → mesh gen (via geometry lib) → upload
├── MeshAssetSystem      — Assimp model loading
├── CubeMapSystem        — Skybox cubemap loading
├── GridSystem           — Reference grid rendering
└── PolygonModeSystem    — Wireframe/fill toggle

GPU Resource Managers
├── MeshManager          — Pool of MeshGPU (VAO/VBO/EBO). Accepts geom::MeshData from core.
├── ShaderManager        — Compile/link/cache GL programs, hot-reload
├── TextureManager       — Upload/bind/destroy GPU textures
└── GraphicsContext      — Aggregates the three managers

Backend Abstraction
└── IRenderer            — Abstract interface for backend-independent rendering
    ├── OpenGL backend   (current production)
    ├── Vulkan backend   (future)
    ├── WebGL backend    (future)
    └── Null backend     (headless / CI / batch export)

Input Paths (dual)
├── Direct ECS:  app creates entities with render::Transform, RenderableComponent, etc.
└── Via Canvas:  canvas writes same ECS components; renderer is unaware of the source
```

## ECS Components

All in namespace `exd::render`:

| Component | Purpose |
|---|---|
| `Transform` | position, rotation (Quat), scale |
| `Camera` | FOV, near/far planes, exposure |
| `CameraController` | move speed, sensitivity, yaw/pitch |
| `RenderableComponent` | mesh handle (from MeshManager) |
| `RenderTechnique_Lambertian` | tag — standard diffuse shading |
| `RenderTechnique_Mirror` | tag — environment-reflection |
| `RenderTechnique_CubeMap` | tag — skybox rendering |
| `RenderTechnique_Lit` | tag — lit rendering |
| `CubeMapComponent` | cubemap texture reference |
| `GridComponent` | grid spacing, color |
| `MeshAssetComponent` | file path for Assimp loading |
| `CubePrimitive` | cube descriptor for convenience system |
| `ParticleCloudComponent` | particle positions + colors |
| `VolumeFieldComponent` | 3D texture handle for ray-march |
| `SimulationDomain` | grid dimensions for sim vis |
| `Disabled`, `Selected`, `ReadOnly` | utility tags |

## ECS Systems

All systems receive `exd::ecs::Registry&` from the application. Renderer does NOT own the registry.

The main render pass (`RenderSystem`) iterates entities with `Transform + RenderableComponent + technique_tag`. It has no idea whether those entities came from Canvas, from direct ECS usage, or from a model loader — all are identical to the renderer.

## MeshManager

Uses `exd::geom::MeshData` from core (unified mesh type shared across the ecosystem):

```cpp
class MeshManager {
public:
    MeshHandle create(const geom::MeshData& mesh);     // allocate + upload
    void       update(MeshHandle h, const geom::MeshData& m); // glBufferSubData
    void       destroy(MeshHandle h);                   // glDelete*
    const MeshGPU* bind(MeshHandle h);                 // glBindVertexArray
    uint32_t   index_count(MeshHandle h) const;
    uint32_t   vertex_count(MeshHandle h) const;
};
```

## Relationship with extropian-geometry

Mesh generation (sphere, box, cylinder, path tessellation, font shaping) lives in `extropian-geometry`. The renderer's `PrimitiveMeshSystem` calls `geometry::generate_cube_mesh()` internally — the generation logic is not duplicated. Other systems and Canvas also call these same geometry functions.

## Dependencies

```
exd-render → exd-core    (math, ECS, geom types)
exd-render → exd-app     (window state, input, OpenGL context)
exd-render → OpenGL      (system package)
exd-render → SDL3        (windowing)
exd-render → Assimp      (model loading)
```

Renderer does NOT depend on: canvas, composer, geometry (uses only core geom types, not generation functions).

## Building

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

## License

Business Source License 1.1 — see [LICENSE](LICENSE).
Converts to Apache 2.0 on 2029-05-26.
