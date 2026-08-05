# GPU Profiling & Debug Visualization — Future Feature

> Performance measurement via GPU timer queries, on-screen performance HUD,
> and debug rendering utilities for visualizing normals, AABBs, rays, and collider volumes.

## 1. Motivation

The current codebase has no GPU performance measurement. The demo prints FPS every 60 frames via `SDL_GetTicks()`, but there is no per-pass timing, no draw-call counter, no way to answer "where is the frame time going?" This makes optimization guesswork.

Additionally, there is no debug visualization. When implementing systems like collision detection, camera collision, or animation, being able to visually see AABBs, rays, bone hierarchies, and normal vectors is essential for debugging. Currently this requires external tools like RenderDoc.

## 2. Scope

### Owned by this feature

- **GPU timer queries** — `GL_TIME_ELAPSED` for per-pass timing
- **Performance HUD** — on-screen display of frame time breakdown, draw calls, triangle counts
- **Frame profiler** — scoped CPU+GPU timing markers with hierarchical grouping
- **Debug draw utilities** — AABB wireframe, ray lines, normals, bone skeletons, collider spheres
- **Debug draw manager** — collects debug primitives each frame, renders them as overlay

### NOT owned by this feature

- External profiler integration (Tracy, Optick, etc.) — can be layered on later
- RenderDoc capture API integration
- GPU memory usage tracking
- Automated performance regression testing
- Shader performance analysis

## 3. Architecture

### 3.1 GPU Timer Queries

OpenGL's `GL_TIME_ELAPSED` query measures time spent on the GPU for all commands between a `glBeginQuery` / `glEndQuery` pair:

```cpp
class GpuTimer {
public:
    GpuTimer();
    ~GpuTimer();

    void begin();         // glBeginQuery(GL_TIME_ELAPSED, query_)
    void end();           // glEndQuery(GL_TIME_ELAPSED)
    bool available();     // glGetQueryObjectuiv(query_, GL_QUERY_RESULT_AVAILABLE)
    float elapsed_ms();   // glGetQueryObjectuiv(query_, GL_QUERY_RESULT) / 1e6

private:
    uint32_t query_ = 0;
};

class GpuTimerPool {
public:
    GpuTimer* acquire();  // get an available timer from the pool
    void release(GpuTimer* timer);
    void collect_results();  // read back all completed queries

private:
    std::vector<std::unique_ptr<GpuTimer>> pool_;
    std::vector<GpuTimer*> active_;
};
```

**Latency note:** GPU queries are asynchronous. Results are available 1-3 frames after the query ends. The performance HUD displays the most recent available results, not the current frame. This is standard practice.

### 3.2 Per-Pass Timing

`RenderSystem` wraps each render pass with timer queries:

```cpp
void RenderSystem::update(exd::ecs::Registry& registry, double dt) {
    // ...

    // Shadow pass
    timer_shadow_.begin();
    render_shadow_pass(registry);
    timer_shadow_.end();

    // Cubemap pass
    timer_cubemap_.begin();
    render_cubemap_pass(registry, view, proj);
    timer_cubemap_.end();

    // Opaque pass
    timer_opaque_.begin();
    render_opaque_pass(registry, view, proj, cam_pos);
    timer_opaque_.end();

    // ... etc
}
```

### 3.3 Scoped CPU+GPU Profiler

A lightweight scoped profiler for both CPU and GPU timing:

```cpp
class Profiler {
public:
    struct Entry {
        std::string name;
        float cpu_ms;
        float gpu_ms;       // from GpuTimer, 0 if no GPU timing
        int   depth;        // nesting level
    };

    // CPU timing
    void begin_cpu_section(const std::string& name);
    void end_cpu_section();

    // GPU timing (uses GpuTimerPool)
    void begin_gpu_section(const std::string& name);
    void end_gpu_section();

    void end_frame();         // collect all results, build entry list
    const std::vector<Entry>& entries() const;

private:
    std::vector<Entry> entries_;
    // ...
};

// RAII helper
class ScopedProfile {
public:
    ScopedProfile(Profiler& p, const std::string& name, bool gpu = false)
        : profiler_(p), name_(name), gpu_(gpu) {
        profiler_.begin_cpu_section(name_);
        if (gpu_) profiler_.begin_gpu_section(name_);
    }
    ~ScopedProfile() {
        if (gpu_) profiler_.end_gpu_section();
        profiler_.end_cpu_section();
    }
private:
    Profiler& profiler_;
    std::string name_;
    bool gpu_;
};

// Usage:
void render_opaque_pass(...) {
    SCOPED_PROFILE(profiler_, "Opaque Pass", /*gpu=*/true);
    // ...
}
```

### 3.4 Performance HUD

Renders the profiler data as text on screen using `DebugOverlay` (see `docs/ui-overlay.md`):

```
┌─ Performance ────────────────────┐
│ FPS: 142.3  Frame: 7.03ms       │
│ CPU: 5.2ms  GPU: 6.8ms          │
│                                 │
│ Pass           CPU     GPU      │
│ Shadow Map    0.3ms   0.8ms    │
│ Cubemap       0.1ms   0.2ms    │
│ Opaque        2.1ms   3.5ms    │
│ Reflective    0.4ms   0.6ms    │
│ Particles     0.1ms   0.1ms    │
│ Post-Process  0.8ms   1.2ms    │
│                                 │
│ Draw Calls: 47                  │
│ Triangles:  124,832             │
│ Entities:   89                  │
└─────────────────────────────────┘
```

### 3.5 Debug Draw Manager

A system for visualizing debug geometry without modifying the scene:

```cpp
enum class DebugDrawPrimitive {
    AABB,        // wireframe box
    Sphere,      // wireframe sphere (icosphere LOD 1)
    Line,        // single line segment
    Ray,         // line with an arrowhead
    Axis,        // RGB-colored XYZ axes
    Grid,        // 2D grid on a plane
};

struct DebugDrawCommand {
    DebugDrawPrimitive type;
    math::Mat4 transform;        // position, rotation, scale
    math::Vec4 color;            // RGBA
    float duration;              // seconds (0 = one frame)
};

class DebugDrawManager {
public:
    // Queue a debug primitive for this frame
    void draw_aabb(const math::Vec3f& min, const math::Vec3f& max,
                   const math::Vec4f& color = {0,1,0,1}, float duration = 0.0f);
    void draw_sphere(const math::Vec3f& center, float radius,
                     const math::Vec4f& color = {0,1,0,1}, float duration = 0.0f);
    void draw_line(const math::Vec3f& from, const math::Vec3f& to,
                   const math::Vec4f& color = {1,1,1,1}, float duration = 0.0f);
    void draw_ray(const math::Vec3f& origin, const math::Vec3f& direction,
                  const math::Vec4f& color = {1,0,0,1}, float duration = 0.0f);
    void draw_axis(const math::Mat4& transform, float size = 1.0f,
                   float duration = 0.0f);

    // Render all queued primitives (called once per frame, after main scene)
    void render(const math::Mat4& view, const math::Mat4& proj);

    // Age-out expired duration-based commands
    void update(float dt);

private:
    std::vector<DebugDrawCommand> commands_;
    // Wireframe meshes for sphere, cone (arrow), etc.
    uint32_t sphere_mesh_ = 0;
    uint32_t cone_mesh_ = 0;
    uint32_t cube_mesh_ = 0;
    uint32_t shader_;
};
```

Rendering: uses `glPolygonMode(GL_LINE)` or generates wireframe meshes. Depth-tested but no depth writes. Always visible regardless of `Disabled` components.

### 3.6 Debug Toggles

Add `WindowState` flags for debug visualization toggles:

```cpp
struct WindowState {
    // ... existing ...

    // Debug visualization toggles
    bool show_perf_hud    = false;   // F3 toggles
    bool show_colliders   = false;   // F4 toggles — AABBs for ColliderComponent entities
    bool show_normals     = false;   // F5 toggles — vertex normals as lines
    bool show_bones       = false;   // F6 toggles — skeleton bone hierarchy
    bool show_rays        = false;   // F7 toggles — picker/gizmo rays
};
```

## 4. Implementation Plan

### Phase 1: GPU Timers (~3 hours)

| Task | Description |
|---|---|
| 1.1 | Implement `GpuTimer` and `GpuTimerPool` classes |
| 1.2 | Add timer queries to `RenderSystem` for each render pass |
| 1.3 | Implement result collection (asynchronous, 1-3 frame delay) |
| 1.4 | Print per-pass GPU times to stdout as initial verification |

### Phase 2: Performance HUD (~3 hours)

| Task | Description |
|---|---|
| 2.1 | Implement `Profiler` with scoped CPU+GPU sections |
| 2.2 | Display profiler data on-screen via `DebugOverlay` (text rendering) |
| 2.3 | Add FPS graph (rolling window of last 120 frames as ASCII chart or simple bars) |
| 2.4 | Add draw-call + triangle counters to `RenderSystem` |
| 2.5 | Toggle HUD visibility with F3 key |

### Phase 3: Debug Draw (~4 hours)

| Task | Description |
|---|---|
| 3.1 | Implement `DebugDrawManager` with AABB, sphere, line, ray primitives |
| 3.2 | Generate or re-use wireframe meshes (cube = existing, sphere = generate, cone/arrow = generate) |
| 3.3 | Implement debug shader (solid color, no lighting, depth-tested) |
| 3.4 | Add `DebugDrawManager::render()` call after main scene in render loop |
| 3.5 | Wire debug drawing into camera collision system (show collider AABBs + terrain raycast) |
| 3.6 | Wire debug drawing into animation system (show bone hierarchy) |
| 3.7 | Wire debug drawing into picker (show pick ray on F7) |

### Phase 4: Integration & Polish (~2 hours)

| Task | Description |
|---|---|
| 4.1 | Add debug toggle flags to `WindowState` |
| 4.2 | Wire F3-F7 toggles in demo |
| 4.3 | Persistent debug commands with duration timer (auto-expire) |
| 4.4 | GPU memory usage query (GL_NVX_gpu_memory_info or equivalent) |

## 5. File Layout (planned additions)

```
include/exd/render/
├── debug/
│   ├── gpu_timer.hpp                  # NEW: GpuTimer, GpuTimerPool
│   ├── profiler.hpp                   # NEW: Profiler, ScopedProfile
│   └── debug_draw.hpp                 # NEW: DebugDrawManager, DebugDrawCommand
├── components/
│   └── debug_draw_params.hpp          # NEW: per-entity debug draw flags

src/debug/
├── gpu_timer.cpp                      # NEW
├── profiler.cpp                       # NEW
└── debug_draw.cpp                     # NEW

shaders/opengl/debug/
└── debug_draw.vert/.frag              # NEW: solid-color debug shader
```

### Modified files

| File | Change |
|---|---|
| `src/systems/render_system.cpp` | Add per-pass GPU timers, draw-call counters |
| `demo/main.cpp` | Wire debug toggle keys, call DebugDrawManager::render() |
| `include/exd/core/window_state.hpp` (in extropian-core) | Add debug toggle fields |

## 6. Design Decisions

### GPU timer result latency
Accept the 1-3 frame delay in GPU timer results. Alternatives (like `glFinish()` before reading) would stall the pipeline and defeat the purpose. The profiler displays the most recent available data, which is close enough for live tuning.

### Debug draw lifetime
Commands default to single-frame (duration=0). Duration-based commands auto-expire — useful for transient events like collision hits or raycast results. A helper like `draw_sphere(hit_point, 0.1, red, 2.0f)` leaves a red dot for 2 seconds.

### Debug draw vs. component-based visualization
Debug draw is intentionally ephemeral — it doesn't live in the ECS, doesn't participate in picking, doesn't affect the scene. This is the right tradeoff for debugging. If a visualization needs to be persistent and interactable, it should be a regular entity with its own renderable.

### Profiler overhead
The `Profiler` itself has negligible overhead (<0.01ms per section) since it uses `std::chrono::high_resolution_clock` for CPU timing and the GPU timer queries are hardware-accelerated. It's safe to leave enabled in debug builds.

## 7. Non-Goals

- External profiler integration (Tracy, Optick, Remotery)
- Automated performance regression testing
- GPU memory leak detection
- Shader compilation time tracking
- RenderDoc capture API
- Frame capture / replay
- GPU crash dump / debug callback (GL_KHR_debug)
