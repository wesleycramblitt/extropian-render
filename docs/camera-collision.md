# Camera Collision & Terrain Walking — Future Feature

> A collision-aware, terrain-following camera controller built on top of the existing fly-camera.
> Replaces unrestricted free-flight with grounded walking, height clamping, and object avoidance.

## 1. Motivation

The current `CameraSystem` is a pure fly-camera — it does `position += movement_delta` with zero awareness of the world. This is ideal for a debug/inspection camera but unsuitable for first-person or third-person experiences where the camera should:

- Stay at eye-height above terrain
- Slide down slopes (not fly through them)
- Not clip through walls, buildings, or objects
- Optionally support step-climbing and slope limits

The building blocks are already present: the `ray_triangle()`, `ray_aabb()`, and `ray_sphere()` intersection functions in `interaction/ray.hpp`, and the existing per-triangle scanning pattern in `PickerSystem`.

## 2. Scope

This feature spans the render module only. It does not introduce a physics engine — no rigid-body dynamics, no constraint solver, no continuous collision detection. The goal is a **kinematic character controller**: the camera requests a movement, the system validates and corrects it against the world.

### Owned by this feature

- `CameraCollisionSystem` — new ECS system that runs after `CameraSystem`, validating and correcting the camera position
- `ColliderComponent` — ECS tag component marking entities the camera must avoid
- `TerrainComponent` — ECS component identifying which entity is "the ground"
- `query_terrain_height(x, z) -> optional<float>` — downward raycast utility
- Cached `MeshData::bounds` population at mesh load time
- World-space AABB computation from `Transform` + cached mesh bounds
- Collision resolution: sphere-vs-AABB pushout with wall sliding

### NOT owned by this feature

- Physics simulation (gravity, momentum, force accumulation)
- Continuous collision detection (CCD)
- Networked/predicted movement
- NPC pathfinding or navigation meshes
- Ragdoll or skeletal collision

## 3. Architecture

### 3.1 Data Flow

```
Input (WASD + mouse)
       │
       ▼
  CameraSystem                  ← existing, unchanged
  (computes desired position from input)
       │
       ▼
  CameraCollisionSystem         ← NEW
  ┌─────────────────────────────┐
  │ 1. Query terrain height     │
  │    at (new_x, new_z)        │
  │ 2. Clamp Y to terrain +     │
  │    eye_offset               │
  │ 3. Build camera collision   │
  │    sphere/capsule           │
  │ 4. Query collider AABBs     │
  │    for overlaps             │
  │ 5. Resolve: push out,       │
  │    slide along normals      │
  │ 6. Write final position     │
  └─────────────────────────────┘
       │
       ▼
  Transform updated
       │
       ▼
  RenderSystem, Picker, etc. (read camera as before)
```

### 3.2 New ECS Components

```cpp
// Marker component: this entity has a collider volume the camera should avoid.
// The collider shape is derived from the entity's mesh AABB transformed to world space.
struct ColliderComponent {};

// Marker component: this entity represents walkable terrain.
// The CameraCollisionSystem raycasts downward against this mesh for height queries.
struct TerrainComponent {};

// Optional override: camera collision parameters (separate from fly-camera params).
// If absent, defaults are used.
struct CameraCollisionParams {
    float eye_height     = 1.6f;   // height above terrain surface
    float collider_radius = 0.3f;  // camera bounding sphere/capsule radius
    float max_step_height = 0.4f;  // max vertical step the camera can climb (0 = disabled)
    float max_slope_angle = 45.0f; // max walkable slope in degrees (0 = disabled)
    bool  enabled          = true; // runtime toggle
};
```

### 3.3 New System

```cpp
class CameraCollisionSystem {
public:
    void configure(core::WindowState* window);
    void update(exd::ecs::Registry& registry, float dt);
};
```

`CameraCollisionSystem` must run **after** `CameraSystem` in the update loop, so the desired fly-camera position is available before collision correction is applied.

Execution order:
```
HierarchySystem
  → MeshAssetSystem
  → PrimitiveMeshSystem
  → CameraSystem           (writes desired position)
  → CameraCollisionSystem  (corrects position)  ← NEW
  → RenderSystem           (reads corrected position)
```

### 3.4 Terrain Height Query

```
query_terrain_height(registry, x, z) -> optional<float>
```

Implementation:
1. Find the entity with `TerrainComponent + Transform + RenderableComponent`
2. Get its mesh data and world transform
3. Shoot a downward ray: `origin = (x, y_max, z)`, `dir = (0, -1, 0)`
4. Test against all terrain mesh triangles (using existing `ray_triangle()`)
5. Return y-coordinate of nearest hit, or `nullopt` if no terrain loaded

**Performance note:** For large terrain meshes, a naive per-triangle scan may be slow. Two mitigations:
- **Short term:** AABB broadphase — skip triangles outside the ray's XZ footprint
- **Long term:** Build a 2D uniform height grid from the terrain mesh at load time, giving O(1) height lookup. This is a simple preprocess step: project mesh triangles onto XZ plane and sample a grid.

### 3.5 Collision Detection & Resolution

For each frame:

1. **Build camera collider**: A vertical capsule or sphere at the candidate position.
   ```
   capsule_bottom = candidate_position
   capsule_top    = candidate_position + (0, eye_height, 0)
   ```

2. **Broadphase**: Iterate entities with `ColliderComponent + Transform + RenderableComponent`. For each, compute world-space AABB from `Transform` + cached mesh bounds. Test overlap with camera capsule AABB.

3. **Resolution** (sphere simplification for v1):
   - Test camera sphere against each overlapping collider AABB
   - If penetrating, compute minimum translation vector (MTV) to push camera out
   - Accumulate pushout vectors, prioritize horizontal separation over vertical
   - Re-query terrain height at resolved position
   - Clamp Y to resolved terrain height

### 3.6 Wall Sliding

When the camera moves diagonally into a wall, naive rejection would stop all movement. Instead:

1. Attempt full desired movement
2. If blocked along XZ, decompose movement into wall-normal and wall-tangent components
3. Accept the tangent component (slide along wall)
4. Re-check against other colliders
5. Iterate up to N times (e.g., 3 iterations) for corner cases

### 3.7 Mesh Bounds Caching

Currently `MeshData::bounds` exists but is never populated. At mesh load time:

```cpp
// In MeshAssetSystem and PrimitiveMeshSystem, after vertex data is ready:
mesh.bounds = Bounds3::from_vertices(mesh.vertices);
```

This is a one-time O(n) scan. The cost is negligible.

World-space AABB is then computed as:
```cpp
// Recompute when Transform changes, cache result
world_aabb = transform_bbox(mesh.bounds, entity_transform);
```

where `transform_bbox()` rotates the 8 corners of the local AABB by the entity's rotation quaternion and offsets by position.

## 4. Configuration

The camera collision system is opt-in. Existing fly-camera behavior is preserved unless the user:

1. Tags a mesh entity with `TerrainComponent` (for height queries)
2. Tags mesh entities with `ColliderComponent` (for collision avoidance)
3. Adds `CameraCollisionSystem` to the update loop

Without these, the camera behaves identically to today.

### Example scene setup (JSON via EnvironmentSystem)

```json
{
  "terrain": {
    "mesh": "assets/terrain.glb",
    "components": ["terrain", "collider"]
  },
  "objects": [
    { "mesh": "assets/house.glb", "components": ["collider"] },
    { "mesh": "assets/tree.glb", "components": ["collider"] },
    { "mesh": "assets/rock.glb" }
  ]
}
```

Components specified in JSON are translated to ECS component tags at load time. Entities without `"collider"` are ignored by `CameraCollisionSystem`.

## 5. Implementation Plan

### Phase 1: Foundations (~4 hours)

| Task | Description |
|---|---|
| 1.1 | Populate `MeshData::bounds` in `MeshAssetSystem` (Assimp load) and `PrimitiveMeshSystem` (cube, sphere, etc.) |
| 1.2 | Add `ColliderComponent` and `TerrainComponent` tag components |
| 1.3 | Implement `world_space_aabb(entity, mesh) -> Bounds3` utility |
| 1.4 | Implement `query_terrain_height(registry, x, z) -> optional<float>` |
| 1.5 | Add `CameraCollisionParams` component |

### Phase 2: Core Collision (~5 hours)

| Task | Description |
|---|---|
| 2.1 | Implement `CameraCollisionSystem` skeleton (update loop, entity queries) |
| 2.2 | Implement sphere-vs-AABB overlap detection and MTV computation |
| 2.3 | Implement terrain height clamping in the collision system |
| 2.4 | Wire into demo: tag terrain + a few objects as colliders, add system to loop |
| 2.5 | Manual testing: walk on terrain, verify no Y-axis drift |

### Phase 3: Polish (~4 hours)

| Task | Description |
|---|---|
| 3.1 | Implement wall sliding (decompose movement into normal/tangent) |
| 3.2 | Add step-climbing logic (`max_step_height` parameter) |
| 3.3 | Add slope angle limit (`max_slope_angle` parameter) |
| 3.4 | Debug visualization: render camera collider sphere, terrain raycast, collider AABBs |
| 3.5 | Unit tests: `query_terrain_height`, AABB overlap, MTV correctness, wall sliding |

### Phase 4: Optimization (optional, ~3 hours)

| Task | Description |
|---|---|
| 4.1 | Build 2D uniform height grid from terrain mesh for O(1) height queries |
| 4.2 | Simple XZ spatial grid for collider broadphase (avoids O(n) scan with many objects) |
| 4.3 | Lazy/cached world-space AABB invalidation (recompute only when Transform changes) |

## 6. File Layout (planned additions)

```
include/exd/render/
├── components/
│   ├── collider_component.hpp          # NEW: ColliderComponent tag
│   ├── terrain_component.hpp           # NEW: TerrainComponent tag
│   └── camera_collision_params.hpp     # NEW: CameraCollisionParams
├── systems/
│   └── camera_collision_system.hpp     # NEW: CameraCollisionSystem
└── interaction/
    └── collision_query.hpp             # NEW: query_terrain_height, world_aabb, MTV

src/
├── systems/
│   └── camera_collision_system.cpp     # NEW
└── interaction/
    └── collision_query.cpp             # NEW

tests/
└── test_camera_collision.cpp           # NEW
```

### Modified files (existing)

| File | Change |
|---|---|
| `src/systems/mesh_asset_system.cpp` | Populate `mesh.bounds` after Assimp load |
| `src/systems/primitive_mesh_system.cpp` | Populate `mesh.bounds` after procedural generation |
| `demo/main.cpp` | Add `CameraCollisionSystem` to update loop (behind config flag) |

## 7. Design Decisions & Tradeoffs

### Sphere vs. Capsule collider
- **Sphere** is simpler: one MTV computation, works for most cases. Chosen for v1.
- **Capsule** handles the full camera height (feet to head) correctly. Better for tight corridors. Worth adding in Phase 3 if needed.

### Terrain raycast vs. height grid
- **Raycast**: Works immediately with any mesh terrain, no preprocess. O(triangles) per query. Fine for small-to-medium terrains.
- **Height grid**: O(1) lookup, but requires building a 2D grid at load time and bilinear interpolation. Only needed if terrain mesh has >>10k triangles.

### No separate physics dependency
The math for what we need (sphere-AABB overlap, minimum translation vector, ray-triangle intersection) is 100-200 lines total. Pulling in Bullet or PhysX would add a massive dependency for a tiny subset of functionality. All required math primitives already exist in `exd::math` and `interaction/ray.hpp`.

### Collider shape = mesh AABB (not mesh triangles)
Testing against AABBs is fast and simple. Per-triangle collision (like the picker does) would be more accurate but far more expensive and unnecessary for a camera controller — the camera is a sphere/capsule, not a point, so AABB approximations are already conservative. If a specific object needs tighter collision, a future `ColliderShape` component could specify sphere/capsule/box overrides.

## 8. Non-Goals

- No physics simulation (gravity, momentum, friction)
- No rigid-body collision response (only camera collides, objects are static)
- No continuous collision detection (tunneling through thin walls at high speed is not prevented)
- No multiplayer/server-authoritative movement
- No third-person camera mode (but the collision system is camera-agnostic — could be reused)
- No navigation mesh generation
- No destructible terrain
