# Render Architecture — Future Feature

> Wiring the IRenderer abstraction to the actual rendering pipeline, implementing the
> RenderGraph for pass dependency management, adding an FBO abstraction for render
> targets, a multi-light system, and frustum culling for viewport-aware entity selection.

## 1. Motivation

The current architecture has several gaps between its abstractions and reality:

1. **IRenderer is not wired** — `IRenderer` exists as a clean interface but the demo (and all real rendering) bypasses it entirely, calling `RenderSystem::update()` directly. The OpenGL "backend" returns `nullptr` from the factory.

2. **RenderGraph is unused** — defined with `add_pass()` and `priority`, but no renderer reads it. Pass ordering is hardcoded in `RenderSystem::update()`.

3. **No multi-light support** — only a single directional sun. Adding point lights, spot lights, and area lights would dramatically improve scene lighting.

4. **No frustum culling** — every entity is drawn every frame, even those behind the camera or outside the view frustum.

5. **Dangling include** — `render.hpp` line 47 references `gizmo_mesh.hpp` which does not exist, breaking the umbrella header.

These are foundational issues that block several other features (shadow mapping needs multi-light, post-processing needs FBOs, etc.).

## 2. Scope

### Owned by this feature

- **Wire IRenderer** — make `Backend::OpenGL` actually return a working renderer
- **Implement RenderGraph** — DAG-based pass ordering with dependency edges
- **FBO abstraction** — `RenderTarget` class (also covered in `docs/post-processing.md`, defined here as the canonical implementation)
- **Multi-light system** — point lights, spot lights, configurable light limits
- **Frustum culling** — view-frustum AABB test to skip off-screen entities
- **Entity name/tag** — `NameComponent` for human-readable entity identifiers
- **Fix dangling include** — remove or create `gizmo_mesh.hpp`

### NOT owned by this feature

- Deferred rendering (forward-only for now — deferred is a separate architectural decision)
- Light probe / lightmap baking
- GPU culling (compute shader or transform feedback)
- Occlusion culling (hardware queries or software portal/occluder)

## 3. Architecture

### 3.1 Wire IRenderer to Real Rendering

The IRenderer currently has:
```cpp
class IRenderer {
    virtual void execute(const RenderGraph& graph, const Camera& camera) = 0;
};
```

The plan: create a concrete `OpenGLRenderer` that owns a `RenderSystem` (or receives it via dependency injection) and delegates `execute()` to it:

```cpp
class OpenGLRenderer : public IRenderer {
public:
    OpenGLRenderer(GraphicsContext& ctx, core::WindowState* win);

    void initialize(void* window_handle) override;
    void shutdown() override;
    void resize(uint32_t width, uint32_t height) override;
    void begin_frame() override;
    void execute(const RenderGraph& graph, const Camera& camera) override;
    void end_frame() override;

    std::string_view backend_name() const override { return "OpenGL 4.6"; }
    std::string_view renderer_info() const override;

    // Access to the ECS registry for systems that need it
    exd::ecs::Registry& registry() { return registry_; }

private:
    exd::ecs::Registry registry_;
    GraphicsContext& ctx_;
    core::WindowState* window_;
    RenderSystem render_system_;
    CameraSystem camera_system_;
    // Other systems...
};
```

The demo loop then becomes:
```cpp
// New pattern
auto renderer = IRenderer::create(IRenderer::Backend::OpenGL);
renderer->initialize(window_handle);

while (running) {
    renderer->begin_frame();

    // Application updates the ECS registry
    renderer->registry().get<Transform>(camera).position = ...;

    // Renderer executes the frame
    RenderGraph graph;
    graph.add_pass({"shadow", 0});
    graph.add_pass({"opaque", 1});
    renderer->execute(graph, camera);

    renderer->end_frame();
}
```

### 3.2 RenderGraph — DAG with Dependencies

Replace the current flat priority-based graph with actual edges:

```cpp
class RenderGraph {
public:
    struct PassHandle { uint32_t id; };

    PassHandle add_pass(const std::string& name);

    // Declare that pass_b depends on pass_a (a must complete before b)
    void add_dependency(PassHandle from, PassHandle to);

    // Declare that a pass reads from a render target
    void set_input(PassHandle pass, const std::string& name, RenderTarget* target);

    // Declare that a pass writes to a render target
    void set_output(PassHandle pass, const std::string& name, RenderTarget* target);

    // Compile: validate DAG (no cycles), compute execution order via topological sort
    bool compile();

    // Execute all passes in dependency order
    void execute(exd::ecs::Registry& registry);

private:
    struct Pass {
        std::string name;
        std::function<void(exd::ecs::Registry&)> execute_fn;
        // ...
    };
    std::vector<Pass> passes_;
    std::vector<std::pair<uint32_t, uint32_t>> edges_; // from → to
    std::vector<uint32_t> execution_order_;
};
```

**Example: Shadow Mapping + Main Pass**
```cpp
RenderGraph graph;
auto shadow = graph.add_pass("ShadowMap");
auto opaque = graph.add_pass("Opaque");
auto post   = graph.add_pass("PostProcess");

graph.add_dependency(shadow, opaque);     // opaque needs shadow map
graph.add_dependency(opaque, post);       // post needs opaque output

graph.set_output(shadow, "shadow_map", &shadow_target);
graph.set_output(opaque, "hdr_color", &hdr_target);
graph.set_input(opaque, "shadow_map", &shadow_target);
graph.set_input(post, "hdr_color", &hdr_target);

graph.compile();
graph.execute(registry);
```

The RenderGraph owns render target transitions (barriers) and can optimize by detecting unused render targets, reordering independent passes, etc. For OpenGL 4.6 this is mostly about validation and clarity — actual barrier insertion is handled by the driver.

### 3.3 RenderTarget (FBO Abstraction)

Defined here as the canonical location (referenced by `docs/post-processing.md` and `docs/shadow-mapping.md`):

```cpp
class RenderTarget {
public:
    struct Config {
        uint32_t width  = 1920;
        uint32_t height = 1080;
        bool     has_color      = true;
        bool     has_depth      = true;
        int      color_samples  = 1;
        GLenum   color_format   = GL_RGBA16F;
        GLenum   depth_format   = GL_DEPTH_COMPONENT24;
        int      mip_levels     = 1;
        bool     is_cubemap     = false;  // for shadow/IBL rendering into cubemap faces
    };

    explicit RenderTarget(const Config& cfg);
    ~RenderTarget();

    void bind(int face = 0);     // face only for cubemap targets
    void unbind();
    void resize(uint32_t w, uint32_t h);

    uint32_t fbo() const;
    uint32_t color_texture() const;
    uint32_t depth_texture() const;
    const Config& config() const;

    // Blit from this target to another (or to backbuffer)
    void blit_to(RenderTarget& dst, GLenum filter = GL_NEAREST);
    void blit_to_backbuffer();

private:
    uint32_t fbo_ = 0;
    uint32_t color_tex_ = 0;
    uint32_t depth_tex_ = 0;
    Config config_;
};
```

### 3.4 Multi-Light System

Replace the single `SceneLighting` component with a flexible light system:

```cpp
enum class LightType : uint8_t {
    Directional = 0,
    Point       = 1,
    Spot        = 2,
};

struct LightComponent {
    LightType type = LightType::Point;

    // Common
    math::Vec3f color{1.0f, 1.0f, 1.0f};
    float       intensity = 1.0f;
    bool        cast_shadows = false;

    // Directional
    math::Vec3f direction{0.0f, -1.0f, 0.0f};

    // Point
    float range = 10.0f;

    // Spot
    float inner_cone_angle = 0.5f;  // radians (inner = full brightness)
    float outer_cone_angle = 0.7f;  // radians (outer = falloff edge)
};

// Replaces SceneLighting on the environment entity
struct SceneLighting {
    math::Vec3f ambient{0.1f, 0.1f, 0.15f};  // global ambient term
    // Light entities are found via registry.view<LightComponent>()
};
```

**Shader approach:** Forward rendering with configurable light limits. Use uniform arrays:

```glsl
#define MAX_POINT_LIGHTS 8
#define MAX_SPOT_LIGHTS  4

uniform vec3 u_ambient;
uniform vec3 u_sun_direction;    // primary directional light (always present for compatibility)
uniform vec3 u_sun_color;

uniform int   u_point_light_count;
uniform vec3  u_point_light_pos[MAX_POINT_LIGHTS];
uniform vec3  u_point_light_color[MAX_POINT_LIGHTS];
uniform float u_point_light_range[MAX_POINT_LIGHTS];

// In fragment shader:
for (int i = 0; i < u_point_light_count; i++) {
    vec3 L = u_point_light_pos[i] - world_pos;
    float dist = length(L);
    float attenuation = 1.0 / (1.0 + dist * dist / (u_point_light_range[i] * u_point_light_range[i]));
    // Attenuation also used for shadow map selection / light culling
    frag_color += compute_light(L/dist, N, V, u_point_light_color[i]) * attenuation;
}
```

A `LightCullingSystem` runs before rendering, selecting the N closest lights to the camera (or per-object, which is more advanced). For v1, just the N closest lights globally.

### 3.5 Frustum Culling

A simple view-frustum vs. AABB test to skip entities entirely outside the view:

```cpp
class Frustum {
public:
    // Construct from view-projection matrix
    explicit Frustum(const math::Mat4& view_proj);

    // Test: is the AABB inside, intersecting, or outside the frustum?
    enum class Result { Inside, Intersect, Outside };
    Result test_aabb(const math::Vec3f& min, const math::Vec3f& max) const;

private:
    // 6 planes: left, right, bottom, top, near, far
    math::Vec4f planes_[6]; // ax + by + cz + d = 0, where (a,b,c) is normal pointing inward
};
```

Frustum plane extraction from view-projection matrix:
```cpp
Frustum::Frustum(const Mat4& vp) {
    // Left plane: row3 + row0
    planes_[0] = vp.column(3) + vp.column(0);
    // Right: row3 - row0
    planes_[1] = vp.column(3) - vp.column(0);
    // Bottom: row3 + row1
    planes_[2] = vp.column(3) + vp.column(1);
    // Top: row3 - row1
    planes_[3] = vp.column(3) - vp.column(1);
    // Near: row3 + row2 (or row2 for reverse-Z)
    planes_[4] = vp.column(3) + vp.column(2);
    // Far: row3 - row2
    planes_[5] = vp.column(3) - vp.column(2);

    // Normalize all planes
    for (auto& p : planes_) {
        float len = sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
        p = p / len;
    }
}
```

**Integration:** `RenderSystem` builds the frustum from the camera's view-projection matrix before dispatching render passes. Each pass gets a reference to the frustum and skips entities whose world-space AABB tests as `Outside`.

```cpp
void RenderSystem::render_opaque_pass(Registry& registry, const Frustum& frustum, ...) {
    for (auto e : view) {
        // Skip if entity has no cached bounds (backward compat: draw anyway)
        if (auto* aabb = get_cached_aabb(e)) {
            if (frustum.test_aabb(aabb->min, aabb->max) == Frustum::Result::Outside) {
                continue;  // skip this entity
            }
        }
        // ... draw entity
    }
}
```

### 3.6 NameComponent

```cpp
struct NameComponent {
    std::string name;  // "MainCamera", "SunLight", "Terrain", "Player"
};
```

Used by:
- Debug overlay for entity labels
- Scene explorer / outliner in ImGui
- Logging (human-readable entity references)
- Serialization (save/load by name)

This is so small and universally useful it should go in immediately, not wait for a larger feature.

### 3.7 Fix Dangling Include

`include/exd/render/render.hpp` line 47:
```cpp
#include <exd/render/interaction/gizmo_mesh.hpp>  // FILE DOES NOT EXIST
```

Either:
- **Remove the include** (if no code references it)
- **Create a stub `gizmo_mesh.hpp`** (if something depends on it)

Check for references:
```bash
rg "gizmo_mesh" --include="*.cpp" --include="*.hpp"
```

If no references exist beyond the include itself, remove it. If `GizmoSystem` references it, create a minimal header.

## 4. Implementation Plan

### Phase 1: Fixes & Foundations (~3 hours)

| Task | Description |
|---|---|
| 1.1 | Fix dangling `gizmo_mesh.hpp` include (remove or create stub) |
| 1.2 | Add `NameComponent` ECS component |
| 1.3 | Implement `RenderTarget` class (FBO + color/depth textures + resize + blit) |
| 1.4 | Implement `Frustum` class (view-projection plane extraction + AABB test) |

### Phase 2: Wire IRenderer + RenderGraph (~5 hours)

| Task | Description |
|---|---|
| 2.1 | Implement `OpenGLRenderer` concrete class |
| 2.2 | Move ECS registry ownership into `OpenGLRenderer` |
| 2.3 | Implement RenderGraph DAG (passes, edges, topological sort, validation) |
| 2.4 | Wire `RenderSystem` passes through RenderGraph |
| 2.5 | Update demo to use `IRenderer::create(OpenGL)` path |

### Phase 3: Multi-Light (~4 hours)

| Task | Description |
|---|---|
| 3.1 | Add `LightComponent` with directional/point/spot types |
| 3.2 | Implement `LightCullingSystem` (select N nearest lights per frame) |
| 3.3 | Update lambertian shader with point light support (uniform arrays) |
| 3.4 | Update PBR shader with point light support (when PBR is implemented) |
| 3.5 | Demo: add colored point lights to the scene |

### Phase 4: Frustum Culling Integration (~3 hours)

| Task | Description |
|---|---|
| 4.1 | Build and cache world-space AABBs per entity (use populated `MeshData::bounds`) |
| 4.2 | Integrate frustum culling into each render pass |
| 4.3 | Track culled entity count in performance HUD |
| 4.4 | Debug draw: render frustum planes + culled entity count visualization |

### Phase 5: Cleanup (~2 hours)

| Task | Description |
|---|---|
| 5.1 | Deprecate `RenderGraph::add_pass(Pass p)` flat API in favor of DAG |
| 5.2 | Add `IRenderer::backend_name()` and `renderer_info()` implementations |
| 5.3 | Wire WebGL backend to same `IRenderer` path (Emscripten) |
| 5.4 | Update all docs referencing the old architecture |

## 5. File Layout (planned additions)

```
include/exd/render/
├── backends/opengl/
│   └── opengl_renderer.hpp            # NEW: OpenGLRenderer
├── graphics/
│   ├── render_target.hpp              # NEW: RenderTarget
│   └── frustum.hpp                    # NEW: Frustum
├── components/
│   ├── light.hpp                      # NEW: LightComponent, LightType
│   └── name.hpp                       # NEW: NameComponent

src/
├── backends/opengl/
│   └── opengl_renderer.cpp            # NEW
├── graphics/
│   ├── render_target.cpp              # NEW
│   └── frustum.cpp                    # NEW
└── systems/
    └── light_culling_system.cpp       # NEW

include/exd/render/render_graph.hpp    # REWRITTEN: DAG with edges
src/render_graph.cpp                   # REWRITTEN
```

### Modified files

| File | Change |
|---|---|
| `include/exd/render/render.hpp` | Remove dangling `gizmo_mesh.hpp` include |
| `src/renderer.cpp` | Wire `Backend::OpenGL` → `OpenGLRenderer` |
| `src/systems/render_system.cpp` | Frustum culling integration, render target usage |
| `demo/main.cpp` | Use `IRenderer` path instead of direct `RenderSystem` calls |
| `src/systems/environment_system.cpp` | Use `SceneLighting` + spawn `LightComponent` entities |

## 6. Design Decisions

### Forward vs. Deferred Rendering
Stay forward-rendered for v1. Deferred would require a G-buffer (albedo, normal, depth, material properties), multiple render targets, and a full-screen light accumulation pass. It's more complex than forward with light culling for a scene with < 50 lights. Deferred is worth it for 100+ lights — separate feature.

### RenderGraph: full DAG vs. ordered list
Full DAG with dependency edges. This allows independent passes to execute in any order and catches errors (e.g., reading a render target before it's written). The compile step validates the graph and produces a topological order. Missed dependencies are detected as missing output→input connections.

### Renderer owns ECS registry
Currently the demo owns the registry and passes it to systems. Moving the registry into `OpenGLRenderer` (or having the renderer provide access) centralizes the ECS lifecycle. The application creates entities through the renderer. This is a philosophical shift — the demo becomes a thin layer over the renderer rather than the renderer being a library called by the demo.

### Light limits
Start with 8 point lights and 4 spot lights (shader uniform arrays). These are compile-time constants in the shader via `#define`. At runtime, the `LightCullingSystem` selects the N nearest lights. If more lights are needed, techniques like tiled/clustered forward rendering can be added later.

## 7. Non-Goals

- Deferred rendering pipeline
- Tiled/clustered forward rendering
- Light probe / irradiance volume baking
- Occlusion culling (hardware queries or portal-based)
- GPU-driven culling (compute shader)
- Multi-threaded render command generation
