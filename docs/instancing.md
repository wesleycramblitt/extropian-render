# Instanced Rendering & Draw-Call Batching — Future Feature

> GPU instancing for rendering many copies of the same mesh in a single draw call,
> plus material-based batching to reduce draw-call overhead across the pipeline.

## 1. Motivation

Currently every entity triggers a separate `glDrawElements()` call. In a scene with 1000 trees (same mesh, same material), that's 1000 draw calls — each with its own uniform upload, state change, and CPU→GPU command overhead. This doesn't scale.

GPU instancing allows rendering N copies of a mesh with per-instance data (transform, color) in a single draw call. Combined with material-based batching (grouping entities by shader + mesh + material), the draw-call count can drop from O(n) to O(unique meshes), a 10-1000× reduction.

## 2. Scope

### Owned by this feature

- **Instanced draw support** — `glDrawElementsInstanced()` with per-instance transform data
- **Instance buffer** — streaming VBO for per-instance model matrices (and optional per-instance color)
- **Instance data collection** — gather all entities sharing a mesh+material into instance arrays
- **Material-based batching** — sort/render entities grouped by shader → mesh → material
- **ECS integration** — new `Instanced` tag component, or automatic detection of repeat meshes

### NOT owned by this feature

- GPU-driven rendering (indirect draws, compute culling)
- Hierarchical instancing (nested transforms)
- LOD-aware instancing
- Impostor rendering / billboard instancing
- Draw-call merging across different meshes (requires mesh atlasing, out of scope)

## 3. Architecture

### 3.1 Core Mechanism

```cpp
// Traditional (current): N draw calls for N entities
for (auto entity : entities) {
    upload_model_matrix(entity.transform);
    glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, 0);
}

// Instanced: 1 draw call for all entities sharing the same mesh
std::vector<math::Mat4> instance_matrices;
for (auto entity : entities_using_this_mesh) {
    instance_matrices.push_back(compute_model_matrix(entity));
}
upload_instance_data(instance_matrices);
glDrawElementsInstanced(GL_TRIANGLES, mesh.index_count,
                         GL_UNSIGNED_INT, 0, instance_matrices.size());
```

### 3.2 Instance Buffer

A ring-buffer-style streaming VBO for per-instance data:

```cpp
class InstanceBuffer {
public:
    explicit InstanceBuffer(size_t max_instances = 4096);

    // Upload instance matrices and return the number uploaded
    size_t upload(const std::vector<math::Mat4>& matrices);

    // Bind to the instanced attribute divisor
    void bind();

private:
    uint32_t vbo_ = 0;
    size_t max_instances_;
    size_t cursor_ = 0;  // ring buffer write position (or orphan+re-allocate)
};
```

The instance VBO is bound to the VAO with `glVertexAttribDivisor(attrib_index, 1)` — meaning the attribute advances once per instance, not once per vertex.

### 3.3 Shader Changes

The vertex shader needs an additional instanced attribute for the model matrix:

```glsl
// lambertian.vert — instanced variant
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_norm;
layout(location = 4) in vec4 a_color;

// Per-instance: 4 vec4s for a mat4 (locations 5-8)
layout(location = 5) in mat4 a_instance_model;

uniform mat4 u_view;
uniform mat4 u_proj;

void main() {
    mat4 mvp = u_proj * u_view * a_instance_model;
    gl_Position = mvp * vec4(a_pos, 1.0);
    // world_pos = a_instance_model * vec4(a_pos, 1.0)
    // world_norm = mat3(a_instance_model) * a_norm
}
```

For the non-instanced fallback (single entities), keep the original shader or use a uniform for a single model matrix. Two technique variants: `LambertianTechnique` (single) and `LambertianInstancedTechnique` (instanced), or one technique that detects instance count and switches between `glDrawElements` and `glDrawElementsInstanced`.

### 3.4 ECS Integration

Two approaches:

**Approach A: Explicit tag** — entities with `Instanced` tag get batched. Simple, explicit, but requires manual tagging.

**Approach B: Automatic grouping** — the render pass groups all entities by `(mesh_handle, material_hash)` and automatically instances groups of size > 1. Transparent but more complex.

Recommendation: **Approach B for v1** — it's the most user-friendly. Single entities use the non-instanced draw path (no wasted instance buffer upload), groups of 2+ use instancing. The render pass collects entity data, groups it, and dispatches.

### 3.5 Material-Based Batching

Group entities by:
```
shader_program → mesh_handle → material_properties → blend_mode
```

Within each group, entities are rendered either instanced (if > 1) or individually. The grouping avoids redundant shader binds, texture binds, and uniform uploads.

```cpp
struct DrawBatch {
    uint32_t mesh_handle;
    uint32_t shader_program;
    uint32_t base_color_tex;  // for PBR: additional texture handles
    BlendMode blend_mode;
    std::vector<math::Mat4> instance_matrices;
    // Pointer to the render technique that owns the shader/uniforms
};

void build_batches(Registry& registry, std::vector<DrawBatch>& batches) {
    // 1. Collect all visible entities with RenderableComponent
    // 2. Group by (shader, mesh, material)
    // 3. Build instance matrix arrays
    // 4. Sort batches to minimize state changes
}
```

### 3.6 Batch Sorting

Sort batches by state change cost:
1. **Opaque before transparent** — correct rendering order
2. **Same shader** — group together to avoid `glUseProgram` changes
3. **Same mesh** — group together (instance buffer doesn't change)
4. **Same material/textures** — group together to avoid texture binds

## 4. Implementation Plan

### Phase 1: Basic Instancing (~4 hours)

| Task | Description |
|---|---|
| 1.1 | Implement `InstanceBuffer` streaming VBO class |
| 1.2 | Create instanced variant of lambertian vertex shader (`lambertian_instanced.vert`) |
| 1.3 | Add `glVertexAttribDivisor` setup in MeshManager for instanced attributes |
| 1.4 | Implement `draw_instanced(mesh, instance_count)` in LambertianTechnique |
| 1.5 | Manual test: spawn 1000 cubes, verify single draw call |

### Phase 2: Automatic Batching (~4 hours)

| Task | Description |
|---|---|
| 2.1 | Implement `build_batches()` grouping logic |
| 2.2 | Modify `RenderSystem::render_opaque_pass()` to use batch list |
| 2.3 | Handle non-instanced fallback for groups of size 1 |
| 2.4 | Sort batches by shader→mesh→material to minimize state changes |
| 2.5 | Add per-instance color support (for variety in repeated meshes) |

### Phase 3: Performance & Polish (~3 hours)

| Task | Description |
|---|---|
| 3.1 | Ring-buffer orphan-and-reallocate for InstanceBuffer (avoid GPU sync) |
| 3.2 | Extend instancing to particle technique (draw particles as instances) |
| 3.3 | Extend instancing to highlight technique (selected wireframe) |
| 3.4 | Draw-call counter in debug overlay to verify batching effectiveness |

## 5. File Layout (planned additions)

```
include/exd/render/graphics/
└── instance_buffer.hpp                # NEW: InstanceBuffer

src/graphics/
└── instance_buffer.cpp                # NEW

shaders/opengl/lambertian/
└── lambertian_instanced.vert          # NEW: instanced variant

shaders/opengl/reflective/
└── reflective_instanced.vert          # NEW: instanced variant
```

### Modified files

| File | Change |
|---|---|
| `src/graphics/techniques/lambertian_technique.cpp` | Add `draw_instanced()` overload |
| `src/systems/render_system.cpp` | Batch grouping + dispatch logic |
| `include/exd/render/systems/render_system.hpp` | Batch struct, build_batches() declaration |

## 6. Design Decisions

### Instance VBO size and strategy
Start with a conservative 4096 instances per batch (4096 × 64 bytes = 256 KB). Use `glBufferData(nullptr, size, GL_STREAM_DRAW)` to orphan-and-reallocate each frame — this avoids GPU sync stalls on dynamic data. On GL 4.4+ could use `glBufferStorage` with `GL_MAP_PERSISTENT_BIT` for true ring-buffer behavior.

### Per-instance data scope
v1: model matrix only (4×vec4 = locations 5-8).
v2: optional per-instance color override (location 9 = vec4).

This keeps the vertex attribute count manageable (max 16 on most hardware, currently using 3-5).

### Instancing across different shaders
The `PbrTechnique` (see pbr-pipeline.md) will also need an instanced variant. Each render technique owns its instancing — there's no global instancing system. This is fine because each technique already manages its own shader and VAO.

### Automatic grouping vs. explicit tag
Automatic grouping by (mesh, material) is transparent and requires no user intervention. The only downside is that the grouping is rebuilt every frame, but the cost is O(n) and trivial compared to the draw-call savings. No explicit tag needed.

## 7. Non-Goals

- GPU-driven rendering (indirect multidraw, compute shader culling)
- Hierarchical instancing (entity transforms nested under parent transforms)
- Mesh atlasing (combining different meshes into one buffer for single draw call)
- LOD-aware instancing (different mesh variants at different distances)
- Impostor rendering
