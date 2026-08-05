# Shadow Mapping — Future Feature

> Directional shadow mapping for the existing sun light, with optional cascaded shadow maps (CSM)
> for high-quality shadows at both near and far distances.

## 1. Motivation

The current lighting model has a single directional sun (`SceneLighting::sun_direction`, `sun_color`) with no shadow casting. All geometry receives full sunlight regardless of occluders. This is the single largest gap between the current renderer and a professional-quality real-time engine. Shadows provide depth cues, ground contact, and spatial relationships that are critical for scene readability.

## 2. Scope

### Owned by this feature

- `ShadowMapComponent` — ECS component tagging the shadow-casting light entity
- `ShadowMapPass` — depth-only render pass from light perspective
- Shadow map texture management (depth texture + sampler)
- Shadow sampling in `lambertian.frag` (PCF or basic percentage-closer filtering)
- Optional: cascaded shadow maps (2-4 cascades for large outdoor scenes)
- Shadow bias parameters (depth bias, normal offset)

### NOT owned by this feature

- Point light or spot light shadows (future: omnidirectional shadow maps)
- Shadow map caching / static shadow maps
- Screen-space shadows
- Ray-traced shadows

## 3. Architecture

### 3.1 Data Flow

```
SceneLighting (sun_direction, sun_color)
       │
       ▼
  ShadowMapPass::render_depth()
  ┌─────────────────────────────┐
  │ 1. Compute light view matrix│
  │    from sun_direction       │
  │ 2. Compute ortho projection │
  │    bounding the camera      │
  │    frustum                  │
  │ 3. Render all shadow-casting│
  │    entities into depth FBO  │
  │ 4. Store depth texture      │
  └─────────────────────────────┘
       │
       ▼
  Opaque pass (lambertian.frag)
  ┌─────────────────────────────┐
  │ 1. Transform fragment to    │
  │    light clip space         │
  │ 2. Sample shadow map        │
  │ 3. Compare depth, compute   │
  │    shadow factor (PCF)      │
  │ 4. Multiply sun contribution│
  │    by shadow factor         │
  └─────────────────────────────┘
```

### 3.2 Pass Execution Order

```
RenderSystem::update()
  → ShadowMapPass::render()         ← NEW: depth-only into FBO
  → Cubemap pass
  → Equirect pass
  → Opaque pass                     ← reads shadow map
  → Reflective pass
  → Particle pass
  → Volume pass
  → Highlight pass
```

### 3.3 New Components

```cpp
// Tags the directional light entity as the shadow caster.
// Only one active at a time (first found wins).
struct ShadowMapComponent {
    uint32_t resolution  = 2048;    // shadow map texture size (e.g., 2048x2048)
    float    near_plane  = 0.1f;    // ortho near plane
    float    far_plane   = 500.0f;  // ortho far plane
    float    depth_bias  = 0.005f;  // constant depth bias
    float    normal_bias = 0.02f;   // world-space normal offset bias
    bool     pcf_enabled = true;    // percentage-closer filtering
    int      pcf_samples = 16;      // PCF sample count (4x4 kernel)
};
```

### 3.4 Shadow Map Technique

```cpp
class ShadowMapTechnique {
public:
    ShadowMapTechnique(GraphicsContext& ctx);

    void begin_pass(const math::Mat4& light_view, const math::Mat4& light_proj,
                    uint32_t resolution);
    void draw(uint32_t mesh);
    void end_pass();
    uint32_t depth_texture() const;  // for binding in subsequent passes

private:
    uint32_t fbo_ = 0;
    uint32_t depth_tex_ = 0;
    uint32_t shader_program_;
    // light_view and light_proj stored for fragment shader upload
};
```

The technique creates and manages:
- An FBO with a single depth attachment (no color attachment — depth-only pass)
- A simple vertex shader that transforms by `light_vp` (no fragment shader needed, or a discard-only one)
- Depth texture bound to a dedicated texture unit for the opaque pass

### 3.5 Light View/Projection Computation

For a directional light, the light view matrix is computed as:

```cpp
// Look at the camera frustum center from the sun's direction
Vec3f cam_center = camera_position + camera_forward * (far_plane * 0.5f);
Vec3f light_pos = cam_center - sun_direction * far_plane;
Mat4 light_view = Mat4::look_at(light_pos, cam_center, Vec3f{0, 1, 0});

// Orthographic projection bounding the camera frustum
// Compute the 8 corners of the camera frustum in world space
// Transform by light_view, then fit an ortho box
float left, right, bottom, top, near, far;
compute_frustum_bounds(camera, light_view, left, right, bottom, top, near, far);
Mat4 light_proj = Mat4::ortho(left, right, bottom, top, near, far);
```

### 3.6 Shader Changes (lambertian.frag)

```glsl
// New uniforms
uniform sampler2DShadow u_shadow_map;  // or sampler2D with manual compare
uniform mat4 u_light_vp;
uniform float u_depth_bias;
uniform vec2 u_shadow_map_size;        // for PCF texel size

// In main():
vec4 light_clip = u_light_vp * vec4(world_pos, 1.0);
vec3 light_ndc = light_clip.xyz / light_clip.w;
vec3 light_uv = light_ndc * 0.5 + 0.5;  // [0,1] range

float shadow = 1.0;
if (light_uv.x >= 0.0 && light_uv.x <= 1.0 &&
    light_uv.y >= 0.0 && light_uv.y <= 1.0) {
    shadow = pcf_sample(u_shadow_map, light_uv, u_shadow_map_size, u_depth_bias);
}

// Apply shadow to sun contribution
sun_light *= shadow;
```

### 3.7 Cascaded Shadow Maps (Phase 2)

For large outdoor scenes, a single shadow map produces poor quality at distance:

| Cascade | Coverage | Resolution | Quality |
|---|---|---|---|
| 0 | Near camera (0-15m) | 1024×1024 | High |
| 1 | Mid range (15-50m) | 1024×1024 | Medium |
| 2 | Far range (50-150m) | 1024×1024 | Low |
| 3 | Distant (150m+) | 512×512 | Minimal |

Implementation approach: render all cascades into a single texture atlas (2×2 grid for 4 cascades) or a 2D texture array, then select the appropriate cascade per fragment based on view-space depth.

CSM is a Phase 2 optimization — start with a single shadow map and add cascades only when needed.

## 4. Implementation Plan

### Phase 1: Single Shadow Map (~6 hours)

| Task | Description |
|---|---|
| 1.1 | Add `ShadowMapComponent` ECS component |
| 1.2 | Implement `ShadowMapTechnique` (FBO, depth texture, shader, draw) |
| 1.3 | Implement light view/projection computation from camera frustum + sun direction |
| 1.4 | Modify `RenderSystem` to run shadow pass before opaque pass |
| 1.5 | Modify `lambertian.frag` to sample shadow map with basic depth comparison |
| 1.6 | Wire into demo: tag sun entity with `ShadowMapComponent`, verify shadows appear |

### Phase 2: PCF Filtering (~3 hours)

| Task | Description |
|---|---|
| 2.1 | Implement Poisson disk or uniform PCF kernel in fragment shader |
| 2.2 | Add depth bias + normal offset bias uniforms |
| 2.3 | Expose PCF parameters in `ShadowMapComponent` |
| 2.4 | Handle shadow map edge clamping (GL_CLAMP_TO_BORDER, border=1.0 for outside-shadow) |

### Phase 3: Cascaded Shadow Maps (~6 hours)

| Task | Description |
|---|---|
| 3.1 | Compute N cascaded frustum splits (logarithmic or PSSM) |
| 3.2 | Render N depth passes per frame (or single pass with geometry shader / multi-view) |
| 3.3 | Store cascades in 2D texture array |
| 3.4 | Select cascade in fragment shader based on view-space depth |
| 3.5 | Blend between cascade boundaries to hide transitions |

## 5. File Layout (planned additions)

```
include/exd/render/
├── components/
│   └── shadow_map.hpp                  # NEW: ShadowMapComponent
└── graphics/techniques/
    └── shadow_map_technique.hpp        # NEW: ShadowMapTechnique

src/graphics/techniques/
└── shadow_map_technique.cpp            # NEW

shaders/opengl/
├── shadow/
│   └── shadow.vert                     # NEW: depth-only vertex shader
├── lambertian/lambertian.frag          # MODIFIED: add shadow sampling
└── lambertian/lambertian.vert          # MODIFIED: output world_pos for shadow
```

## 6. Design Decisions

### Depth comparison in shader vs. hardware PCF
- **Hardware PCF** (`sampler2DShadow` + `GL_TEXTURE_COMPARE_MODE`) gives free 2×2 PCF on most GPUs. Preferred for basic filtering.
- **Manual PCF** with Poisson disk gives softer, more natural shadows. Use for higher quality preset.

### Single shadow map vs. cascades first
Start with single shadow map. It's simpler to debug, covers the common case (indoor/small scenes), and cascades can be added orthogonally later.

### Shadow-caster selection
By default, all entities with `RenderableComponent` + `RenderTechnique_Lambertian` + opaque material cast shadows. A future `ShadowCaster` tag could exclude specific entities (e.g., transparent objects, particles).

## 7. Non-Goals

- Point light / spot light shadows (requires omnidirectional shadow maps or dual-paraboloid)
- Dynamic shadow map resolution scaling
- Contact shadows / screen-space shadows
- Shadow map caching for static geometry
- Ray-traced shadows
