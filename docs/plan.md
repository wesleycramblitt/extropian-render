# Extropian Render — GPU Rendering Abstraction

> ECS-based rendering framework with multiple backends (OpenGL, WebGL, Null).
> Platform-agnostic ECS systems + backend-specific renderer implementations.

## 1. Purpose

Render is the GPU abstraction and execution layer. It takes compiled visual data (ECS entities with render components) and draws them efficiently on any supported graphics API.

Render answers:

> How is the visual scene rendered efficiently on the available graphics API?

Render owns:

- `IRenderer` — abstract renderer interface with backend implementations
- GPU resource managers: `MeshManager`, `ShaderManager`, `TextureManager`
- Render techniques: `LambertianTechnique`, `ReflectiveTechnique`, `CubeMapTechnique`, `ParticleRenderTechnique`, `VolumeRenderTechnique`, `HighlightTechnique`
- ECS systems: `RenderSystem`, `CameraSystem`, `HierarchySystem`, `PickerSystem`, `SelectionSystem`, `CubeMapSystem`, `GridSystem`, `PolygonModeSystem`, `MeshAssetSystem`, `PrimitiveMeshSystem`
- Interaction: `Picker`, `Selection`, `Gizmo`
- `RenderGraph` — DAG of render passes
- `GraphicsContext` — aggregates all resource managers

Render does NOT own:

- Conversational AI or orchestration (conductor)
- Semantic meaning of objects (composer)
- Visual document compilation (canvas)
- Audio capture or playback (voice)
- Window creation or event polling (app or browser)

## 2. IRenderer Interface

```cpp
class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void initialize(void* window_handle) = 0;
    virtual void shutdown() = 0;
    virtual void resize(uint32_t width, uint32_t height) = 0;
    virtual void begin_frame() = 0;
    virtual void execute(const RenderGraph& graph, const Camera& camera) = 0;
    virtual void end_frame() = 0;

    virtual std::string_view backend_name() const = 0;
    virtual std::string_view renderer_info() const = 0;

    enum class Backend { OpenGL, Vulkan, WebGL, Null };
    static std::unique_ptr<IRenderer> create(Backend backend);
};
```

All ECS systems and render techniques consume `core::WindowState*` (from extropian-core), never `app::Window*` directly. The renderer has no dependency on extropian-app.

## 3. Backends

### 3.1 OpenGL (desktop)
- OpenGL 4.6 core profile via GLAD
- SDL3 window + context (from extropian-app, but accessed via `core::WindowState*`)
- GLSL 330 core shaders
- Full DSA where available, with non-DSA fallbacks
- `glPolygonMode` for wireframe debug

### 3.2 WebGL (browser / WASM)
- WebGL 2.0 via Emscripten's GLES3 → WebGL translation
- HTML5 canvas API for context creation
- GLSL ES 300 shaders (converted from GLSL 330 by changing version + minor syntax)
- Traditional GL API (no DSA)
- No `glPolygonMode` (stubbed with `#ifndef __EMSCRIPTEN__`)
- Same `IRenderer` interface, same ECS systems, same techniques

### 3.3 Null (headless testing)
- No GPU, no window
- All operations are no-ops
- Used for unit tests, CI, and headless rendering

### 3.4 Vulkan (planned)
- Future backend, not yet implemented

## 4. WebGL Backend — Implementation

The existing codebase already uses GLES3/WebGL2-compatible GL calls almost exclusively. The WebGL backend is a thin shim, not a rewrite.

**Changed files (total ~150 lines):**

| File | Change |
|---|---|
| `include/exd/render/renderer.hpp` | `app::WindowState*` → `core::WindowState*` |
| `include/exd/render/systems/*.hpp` | Same type change (7 headers) |
| `src/backends/webgl/webgl_renderer.cpp` | **NEW**: Emscripten context, ~80 lines |
| `src/renderer.cpp` | `case Backend::WebGL:` in factory |
| `src/systems/polygon_mode_system.cpp` | `#ifndef __EMSCRIPTEN__` guard |
| `src/graphics/shader_manager.cpp` | Shader prefix patch for ES version |
| `*shaders/*.vert, *.frag` | `#version 330 core` → `#version 300 es` |

**GL calls used that are WebGL 2.0 compatible** (all existing code works unchanged):
- VAO/VBO/IBO management (`glGenVertexArrays`, `glBindBuffer`, `glBufferData`, etc.)
- Shader compile/link (`glCreateShader`, `glCompileShader`, `glLinkProgram`, etc.)
- Uniforms (`glUniform*`)
- Draw calls (`glDrawElements`, `glDrawArrays`)
- Blending, depth, culling (`glEnable`, `glBlendFunc`, `glDepthMask`, etc.)
- Textures (`glGenTextures`, `glTexImage2D`, `glTexParameteri`, etc.)

**The only call NOT available in WebGL 2.0**: `glPolygonMode` (wireframe debug, conditionally compiled out).

## 5. GraphicsContext

Shared by all techniques and systems:

```cpp
struct GraphicsContext {
    MeshManager    mesh_manager;      // CPU→GPU mesh upload, cache, binding
    ShaderManager  shader_manager;    // Shader compile/link/cache, hot-reload
    TextureManager texture_manager;   // Texture upload/bind/cache
};
```

Identical across OpenGL and WebGL backends.

## 6. ECS Systems

All systems are platform-agnostic:

| System | Role | Backend-specific? |
|---|---|---|
| `RenderSystem` | Orchestrates all render passes | No |
| `CameraSystem` | Manages camera, view/proj matrices | No |
| `HierarchySystem` | Parent/Children/SiblingLink linked lists | No |
| `PickerSystem` | CPU raycast against renderable entities | No |
| `SelectionSystem` | Manages Selected/Hovered tags | No |
| `CubeMapSystem` | Skybox rendering | No |
| `GridSystem` | Ground grid | No |
| `PolygonModeSystem` | Wireframe toggle | Yes (`glPolygonMode` on web) |
| `MeshAssetSystem` | Loads mesh files via Assimp (CPU only) | No |
| `PrimitiveMeshSystem` | Generates primitives via extropian-geometry (CPU) | No |

## 7. Render Techniques

Each technique manages its own shader program, uniforms, and draw dispatch:

| Technique | Shader | Render pass |
|---|---|---|
| `CubeMapTechnique` | cubemap.vert/frag | Skybox |
| `LambertianTechnique` | lambertian.vert/frag | Opaque objects |
| `ReflectiveTechnique` | reflective.vert/frag | Reflective surfaces |
| `ParticleTechnique` | particle.vert/frag | Point-based particles |
| `VolumeTechnique` | ray_march.vert/frag | Volumetric rendering |
| `HighlightTechnique` | highlight.vert/frag | Outline/highlight |

All techniques use standard GL calls (`glUseProgram`, `glUniform*`, `glDrawElements`, `glDrawArrays`). They are identical across backends — only the shader language version differs (`330 core` vs `300 es`).

## 8. Dependencies

| Dependency | OpenGL | WebGL |
|---|---|---|
| `exd::core` | ECS, math, `WindowState*` | Same (WASM) |
| `exd::app` | ❌ (only `core::WindowState*`) | ❌ |
| OpenGL 4.6 + GLAD | Yes | ❌ |
| GLES3 / WebGL 2.0 | ❌ | Yes (Emscripten) |
| SDL3 | Via `exd::app` | ❌ |
| Emscripten | ❌ | Yes |
| Assimp | Model loading | Optional (stubbed) |
| nlohmann/json | Config | Config |

Key architectural rule: extropian-render never includes `<exd/app/*.h>` — only consumes `core::WindowState*`.

## 9. File Layout

```
include/exd/render/
├── renderer.hpp              # IRenderer + Backend enum
├── render_graph.hpp          # RenderGraph (DAG of passes)
├── camera.hpp                # Camera struct
├── components/               # ECS component headers
│   ├── transform.hpp, renderable.hpp, material.hpp
│   ├── parent.hpp, children.hpp, disabled.hpp
│   ├── camera_component.hpp, render_technique_tags.hpp
│   ├── mesh_asset.hpp, cubemap.hpp
│   ├── particle_cloud.hpp, volume_field.hpp
│   └── ...
├── systems/                  # ECS system headers (platform-agnostic)
│   ├── render_system.hpp, camera_system.hpp
│   ├── hierarchy_system.hpp, grid_system.hpp
│   ├── polygon_mode_system.hpp, mesh_asset_system.hpp
│   ├── primitive_mesh_system.hpp
│   ├── cubemap_system.hpp, environment_system.hpp
├── graphics/                 # GPU resource management
│   ├── graphics_context.hpp
│   ├── mesh.hpp, mesh_gpu.hpp, mesh_manager.hpp
│   ├── shader_manager.hpp
│   ├── texture.hpp, texture_manager.hpp
│   ├── cubemap_texture.hpp, uniform.hpp
│   └── techniques/
│       ├── lambertian_technique.hpp, reflective_technique.hpp
│       ├── cubemap_technique.hpp, particle_technique.hpp
│       ├── volume_technique.hpp, highlight_technique.hpp
└── interaction/
    ├── picker.hpp, selection.hpp, ray.hpp, gizmo.hpp

src/
├── renderer.cpp               # Factory: create(Backend)
├── render_graph.cpp, camera.cpp
├── backends/
│   ├── null/null_renderer.cpp
│   ├── opengl/                 # Desktop GL backend (existing)
│   └── webgl/                  # WebGL backend (NEW, ~80 lines)
│       └── webgl_renderer.cpp
├── graphics/
│   ├── mesh_manager.cpp, shader_manager.cpp, texture_manager.cpp
│   └── techniques/
│       ├── lambertian_technique.cpp, reflective_technique.cpp
│       ├── cubemap_technique.cpp, particle_technique.cpp
│       ├── volume_technique.cpp, highlight_technique.cpp
├── systems/
│   ├── camera_system.cpp, cubemap_system.cpp, environment_system.cpp
│   ├── grid_system.cpp, hierarchy_system.cpp
│   ├── mesh_asset_system.cpp, polygon_mode_system.cpp
│   ├── primitive_mesh_system.cpp, render_system.cpp
└── interaction/
    ├── picker.cpp, selection.cpp, gizmo.cpp
```

## 10. Non-Goals

- No application state or AI orchestration
- No audio processing
- No UI component generation (extropian-ui, desktop-only)
- No semantic meaning (composer)
- No visual document compilation (canvas)
- No window creation (app or browser)
