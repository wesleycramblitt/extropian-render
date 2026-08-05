# Skeletal Animation & Skinning — Future Feature

> Bone-based skeletal animation with GPU skinning, animation clip playback,
> and blending between clips. Enables animated characters and creatures.

## 1. Motivation

The renderer currently has no animation system. All meshes are static — there is no way to animate a character walking, a flag waving, or a door opening. This is a critical gap for any application beyond static scene rendering. Combined with glTF 2.0 import (see `docs/pbr-pipeline.md`), this enables loading and playing animated characters from standard asset pipelines.

## 2. Scope

### Owned by this feature

- **Skeleton component** — bone hierarchy (names, parent indices, inverse bind matrices)
- **Animation clip component** — keyframe data (translations, rotations, scales per bone per timestamp)
- **Animation controller** — clip playback (play, pause, stop, loop, speed)
- **GPU skinning** — vertex shader transforms vertices by bone weights
- **Blending** — cross-fade between two animation clips (walk → run, idle → jump)
- **Skinning data** — bone indices + weights per vertex (up to 4 influences per vertex)

### NOT owned by this feature

- Animation state machines / behavior trees
- Procedural animation / IK
- Morph targets / blend shapes (separate feature — common in facial animation)
- Animation compression
- Animation retargeting between different skeletons
- Physics-driven animation (ragdolls)

## 3. Architecture

### 3.1 New ECS Components

```cpp
// Per-vertex skinning data stored in the mesh
struct SkinnedVertex {
    math::Vec3f position;
    math::Vec3f normal;
    math::Vec2f uv;
    math::Vec4f tangent;     // xyz = tangent, w = handedness
    uint8_t     bone_ids[4]; // indices into the skeleton's bone array
    float       bone_weights[4]; // normalized weights (sum to 1.0)
};

// The skeleton (shared by multiple entities using the same rig)
struct SkeletonComponent {
    std::vector<std::string> bone_names;      // human-readable
    std::vector<int32_t>     bone_parents;    // parent index (-1 = root)
    std::vector<math::Mat4>  inverse_bind_matrices; // bind-pose inverse
    // Runtime (per-instance) bone transforms are in BoneTransformComponent
};

// Per-entity, per-frame bone transforms (computed by AnimationSystem)
struct BoneTransformComponent {
    std::vector<math::Mat4> bone_matrices;  // model-space, final skinning matrices
    // bone_matrices[i] = global_bone_transform[i] * inverse_bind_matrices[i]
};

// Animation clip (keyframe data, shared resource)
struct AnimationClip {
    std::string name;
    float duration; // seconds

    struct BoneChannel {
        std::string bone_name;
        std::vector<float> timestamps;              // keyframe times
        std::vector<math::Vec3f> translations;
        std::vector<math::Quat> rotations;
        std::vector<math::Vec3f> scales;
    };
    std::vector<BoneChannel> channels;
};

// Playback state (per-entity)
struct AnimationController {
    uint32_t clip_index       = 0;     // which clip is playing
    float    current_time     = 0.0f;  // playback position in seconds
    float    playback_speed   = 1.0f;  // 1.0 = normal, 0.0 = paused, -1.0 = reverse
    bool     looping          = true;
    bool     playing          = true;

    // Blending state
    uint32_t blend_clip_index = 0xFFFFFFFF; // second clip for cross-fade
    float    blend_weight     = 0.0f;       // 0 = first clip, 1 = second clip
    float    blend_duration   = 0.3f;       // cross-fade duration
    float    blend_elapsed    = 0.0f;
};
```

### 3.2 Animation System

```cpp
class AnimationSystem {
public:
    void update(exd::ecs::Registry& registry, float dt);

private:
    // Sample a clip at a given time, return bone transforms
    void sample_clip(const AnimationClip& clip, float time,
                     const SkeletonComponent& skeleton,
                     std::vector<math::Mat4>& out_transforms);

    // Blend two sets of bone transforms
    void blend_transforms(const std::vector<math::Mat4>& a,
                          const std::vector<math::Mat4>& b,
                          float weight,
                          std::vector<math::Mat4>& out);
};
```

**Per-frame update:**
1. For each entity with `AnimationController + SkeletonComponent + BoneTransformComponent`:
   - Advance `current_time` by `dt * playback_speed`
   - If looping, wrap `current_time` modulo `clip_duration`
   - Sample the animation clip at `current_time` → get local bone transforms
   - Walk the skeleton hierarchy to compute global (model-space) bone transforms
   - Multiply by inverse bind matrices → final skinning matrices
   - If blending: sample the second clip, blend with the first, store result
   - Write `bone_matrices` to `BoneTransformComponent`

### 3.3 GPU Skinning

The vertex shader receives bone matrices as a uniform array and transforms each vertex by its weighted bone influences:

```glsl
// skinned.vert
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_norm;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec4 a_tangent;
layout(location = 5) in uvec4 a_bone_ids;     // uint8 → uint in GLSL
layout(location = 6) in vec4 a_bone_weights;

uniform mat4 u_bone_matrices[64];   // max 64 bones (adjustable)
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

void main() {
    mat4 skin = u_bone_matrices[a_bone_ids.x] * a_bone_weights.x +
                u_bone_matrices[a_bone_ids.y] * a_bone_weights.y +
                u_bone_matrices[a_bone_ids.z] * a_bone_weights.z +
                u_bone_matrices[a_bone_ids.w] * a_bone_weights.w;

    vec4 skinned_pos = skin * vec4(a_pos, 1.0);
    vec3 skinned_norm = mat3(skin) * a_norm;

    gl_Position = u_proj * u_view * u_model * skinned_pos;
    // Pass skinned normal and world position to fragment shader...
}
```

**Mesh format changes:**
- `MeshManager` needs to support the skinned vertex layout (bone IDs + weights as vertex attributes)
- A `uint8_t[4]` bone ID array is uploaded as `GL_UNSIGNED_BYTE` with `glVertexAttribIPointer` (integer, non-normalized)
- `MeshData` gains a `bool is_skinned` flag

### 3.4 Dual Quaternion Skinning (Optional, Phase 3)

Linear blend skinning (LBS, a.k.a. matrix palette skinning) suffers from "candy-wrapper" artifacts at joints (volume collapse during twist). Dual quaternion skinning (DQS) eliminates this by blending rotations correctly. The tradeoff is ~2× uniform data (8 floats per bone vs. 12 for mat4) and slightly more shader math. Worth adding as an optional quality toggle.

### 3.5 glTF Animation Import

Add animation import to `GltfAssetSystem` (see `docs/pbr-pipeline.md`):

```cpp
// In GltfAssetSystem::load_gltf():
for (auto& gltf_anim : gltf_doc.animations) {
    AnimationClip clip;
    clip.name = gltf_anim.name;
    for (auto& gltf_channel : gltf_anim.channels) {
        // glTF channels: target node + target path (translation/rotation/scale)
        // Samplers: input (timestamps) + output (keyframe values)
        clip.channels.push_back(extract_channel(gltf_channel, gltf_anim));
    }
    clip.duration = compute_duration(clip.channels);
    animation_clips_.push_back(clip);
}

// Assign skeleton + animation controller to entity
registry.emplace<SkeletonComponent>(entity, skeleton_data);
registry.emplace<AnimationController>(entity, controller);
```

## 4. Implementation Plan

### Phase 1: Skeleton + Static Skinning (~6 hours)

| Task | Description |
|---|---|
| 1.1 | Add `SkeletonComponent`, `BoneTransformComponent`, `AnimationClip`, `AnimationController` |
| 1.2 | Extend `MeshData` with bone IDs + weights per vertex (`SkinnedVertex`) |
| 1.3 | Extend `MeshManager` to handle skinned vertex layout (bone attribs as GL_UNSIGNED_BYTE) |
| 1.4 | Implement `skinned.vert` shader with linear blend skinning |
| 1.5 | Test with a static pose (identity bone matrices) — verify skinned mesh renders correctly |
| 1.6 | Test with a manually-authored bone transform — verify deformation |

### Phase 2: Animation Playback (~6 hours)

| Task | Description |
|---|---|
| 2.1 | Implement `AnimationSystem` — clip sampling, time advancement, looping |
| 2.2 | Implement bone hierarchy traversal (parent→child transform accumulation) |
| 2.3 | Implement final skinning matrix computation (global_bone × inverse_bind) |
| 2.4 | Uniform upload: bone matrices to shader (uniform array, max 64 bones) |
| 2.5 | Add animation clip import to `GltfAssetSystem` |
| 2.6 | Test with a glTF animated model (e.g., simple walk cycle) |

### Phase 3: Blending & Polish (~4 hours)

| Task | Description |
|---|---|
| 3.1 | Implement clip blending (cross-fade between two clips) |
| 3.2 | Implement `AnimationController` play/pause/stop/loop/speed API |
| 3.3 | Optional: dual quaternion skinning for high-quality joint deformation |
| 3.4 | Skeleton debug visualization (draw bones as lines + joints as points) |
| 3.5 | Animation event callbacks (footstep, weapon swing, etc.) |

## 5. File Layout (planned additions)

```
include/exd/render/
├── components/
│   ├── skeleton.hpp                   # NEW: SkeletonComponent
│   ├── bone_transform.hpp             # NEW: BoneTransformComponent
│   └── animation_controller.hpp       # NEW: AnimationController, AnimationClip
├── systems/
│   └── animation_system.hpp           # NEW: AnimationSystem

src/systems/
└── animation_system.cpp               # NEW

shaders/opengl/skinned/
├── skinned.vert                        # NEW: linear blend skinning vertex shader
└── pbr_skinned.vert                    # NEW: PBR + skinning combined (or #include approach)

tests/
└── test_animation.cpp                 # NEW
```

### Modified files

| File | Change |
|---|---|
| `include/exd/render/graphics/mesh.hpp` | Add `SkinnedVertex`, `is_skinned` flag |
| `src/graphics/mesh_manager.cpp` | Handle skinned vertex layout |
| `src/systems/gltf_asset_system.cpp` | Import animation clips + skeleton from glTF |

## 6. Design Decisions

### Bone limit: 64
Maximum 64 bones per mesh in v1. This covers most character rigs. The uniform array size is baked into the shader. To support more bones, switch to a texture-based bone matrix lookup (`sampler2D` where each row is a bone matrix) — this supports hundreds of bones with no shader recompilation. Can be added later.

### Keyframe interpolation
Linear interpolation for translations, spherical linear interpolation (slerp) for rotations, linear for scales. Cubic spline interpolation (glTF CUBICSPLINE) should be supported for high-quality animation but is lower priority than basic linear/slerp.

### Separate skinned shader vs. conditional branching
A separate `skinned.vert` shader is cleaner than `#ifdef USE_SKINNING` in a unified shader. Each technique gets both a static and skinned variant (or a single variant that uses a uniform model matrix when `bone_count == 0`). The technique selects the appropriate shader based on whether the entity has a `BoneTransformComponent`.

### Skeleton sharing
Multiple entities can share the same `SkeletonComponent` (e.g., all soldiers using the same rig). The `AnimationController` and `BoneTransformComponent` are per-instance. A resource handle pattern (like `MeshManager`) avoids duplicating skeleton data.

## 7. Non-Goals

- Animation state machines (use external library or user code)
- Morph targets / blend shapes (separate feature)
- IK (inverse kinematics)
- Animation compression / quantization
- Animation retargeting
- Physics-driven secondary animation (jiggle, cloth)
- GPU compute-based skinning
