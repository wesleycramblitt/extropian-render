# Orbit Camera — Future Feature

> A turntable-style orbit camera for inspecting objects from any angle. Complements the
> existing FPS fly-camera with a mode optimized for scene exploration and editing.

## 1. Motivation

The current `CameraSystem` provides only an FPS fly-camera. While excellent for first-person walkthroughs, it is awkward for inspecting individual objects or rotating around a fixed point of interest. An orbit camera that rotates around a target point, with zoom (distance) and pan (target offset) controls, is essential for:

- Modeling / layout workflows
- Inspecting imported assets
- Debug visualization (orbit around a selected entity)
- Any non-first-person use case

This is one of the highest-ROI features: the math is simple, the implementation is small, and the utility is immediate.

## 2. Scope

### Owned by this feature

- `OrbitCameraController` — mode toggleable from FPS to orbit
- Orbit parameters: target point, distance, azimuth (yaw), elevation (pitch)
- Input mapping: left-drag = orbit, scroll = zoom, middle-drag = pan
- Smooth interpolation between FPS and orbit modes (optional)
- Auto-framing: orbit around the selected entity's bounds center

### NOT owned by this feature

- Cinematic camera paths / keyframes
- Dolly zoom
- Multi-viewport camera sync
- Camera collision (covered in `docs/camera-collision.md` — orbit camera could optionally use it)

## 3. Architecture

### 3.1 Orbit Parameters

```cpp
struct OrbitCameraController {
    math::Vec3f target{0.0f, 2.0f, 0.0f};  // point being orbited
    float       distance      = 10.0f;       // distance from target
    float       azimuth       = 0.0f;        // horizontal rotation (radians), 0 = -Z
    float       elevation     = 0.5f;        // vertical angle (radians), constrained to [-π/2, π/2]
    float       min_distance  = 0.5f;        // minimum zoom distance
    float       max_distance  = 200.0f;      // maximum zoom distance
    float       orbit_speed   = 0.005f;      // radians per pixel of mouse drag
    float       zoom_speed    = 0.1f;        // distance per scroll tick
    float       pan_speed     = 0.01f;       // world units per pixel of pan drag
    bool        enabled       = true;        // runtime toggle
};
```

### 3.2 Position Computation

```
camera_position = target + spherical_to_cartesian(azimuth, elevation, distance)

spherical_to_cartesian(azimuth, elevation, radius):
    x = radius * cos(elevation) * sin(azimuth)
    y = radius * sin(elevation)
    z = radius * cos(elevation) * cos(azimuth)
    return (x, y, z)

camera_forward = normalize(target - camera_position)
camera_up     = (0, 1, 0)  // world up (with special case at poles)
```

### 3.3 Input Mapping

In **UI input mode** (cursor visible, same as gizmo interaction):

| Input | Action | Target Audience |
|---|---|---|
| Left mouse drag | Orbit (azimuth + elevation) | Rotation around target |
| Scroll wheel | Zoom (distance) | Move closer/farther |
| Middle mouse drag | Pan (move target on camera plane) | Lateral movement |
| Right mouse drag | (reserved for context menu) | — |

All orbit inputs are disabled while the gizmo is being dragged, avoiding input conflicts.

### 3.4 Camera Mode Switching

Add a `CameraMode` concept that allows toggling between FPS and orbit:

```cpp
enum class CameraMode {
    FPS,     // existing: WASD + mouse look, cursor hidden
    Orbit,   // new: drag to orbit, scroll to zoom, cursor visible
};
```

`CameraSystem` already checks `window_->input_mode`. Orbit mode operates in `InputMode::UI` (cursor visible), so it doesn't need raw mouse delta — it reads mouse button + scroll events instead.

This can be added as a new system (`OrbitCameraSystem`) that runs instead of `CameraSystem` when in orbit mode, or integrated into `CameraSystem` with a mode switch.

Recommendation: **Separate `OrbitCameraSystem`** for clean separation. The demo switches between them based on mode.

### 3.5 Orbit Around Selection

When an entity is selected, the orbit target can auto-follow:

```cpp
void OrbitCameraSystem::update(Registry& registry, float dt) {
    // If an entity is selected and auto-follow is on, update target
    auto selected = registry.view<Selected, Transform>();
    if (selected.begin() != selected.end() && auto_follow) {
        auto& t = registry.get<Transform>(*selected.begin());
        target = t.position;  // or compute bounds center for multi-object selection
    }

    // Process orbit/zoom/pan input
    // ...
}
```

### 3.6 Smooth Transition

When switching from FPS to orbit (or vice versa), smoothly interpolate the camera position over ~0.3 seconds to avoid jarring jumps. Store a transition state with start/end parameters and lerp.

## 4. Implementation Plan

### Phase 1: Core Orbit (~3 hours)

| Task | Description |
|---|---|
| 1.1 | Add `OrbitCameraController` component |
| 1.2 | Implement `OrbitCameraSystem` with azimuth/elevation/distance math |
| 1.3 | Input handling: left-drag orbit, scroll zoom, middle-drag pan |
| 1.4 | Wire into demo: add orbit camera entity, mode switch (e.g., Tab cycles FPS→Orbit→UI) |

### Phase 2: Quality of Life (~2 hours)

| Task | Description |
|---|---|
| 2.1 | Auto-follow selected entity's position |
| 2.2 | Mouse wheel zoom with exponential scale (faster at distance, finer up close) |
| 2.3 | Elevation clamping (prevent flipping: keep within [-89°, 89°]) |
| 2.4 | Target-visualization crosshair (small debug marker at orbit target) |

### Phase 3: Polish (~2 hours)

| Task | Description |
|---|---|
| 3.1 | Smooth transition between FPS and orbit modes |
| 3.2 | Momentum/inertia for orbit (optional: coast after releasing drag) |
| 3.3 | Touchpad gesture support (two-finger orbit, pinch zoom) |
| 3.4 | Preset views: top, front, right, perspective (with smooth animation) |

## 5. File Layout (planned additions)

```
include/exd/render/
├── components/
│   └── orbit_camera_controller.hpp    # NEW: OrbitCameraController
└── systems/
    └── orbit_camera_system.hpp        # NEW: OrbitCameraSystem

src/systems/
└── orbit_camera_system.cpp            # NEW

tests/
└── test_orbit_camera.cpp              # NEW
```

### Modified files

| File | Change |
|---|---|
| `demo/main.cpp` | Add orbit camera entity + system, Tab cycles through FPS→Orbit→UI modes |

## 6. Design Decisions

### Separate system vs. integrated into CameraSystem
Separate system. The FPS and orbit cameras have fundamentally different input models (cursor-locked delta vs. cursor-visible button drag) and different state. A clean separation avoids conditional spaghetti.

### Orbit in UI mode vs. its own mode
Orbit uses the existing `InputMode::UI` (cursor visible). This means orbit and gizmo interaction coexist naturally: left-click on gizmo = drag, left-click on empty space = orbit. The gizmo hit-test runs first; if it misses, orbit takes over.

### Orbit target = selected entity center vs. explicit target
Auto-following the selection is the most intuitive default. A double-click on empty space could set an explicit orbit target independent of selection. For v1, just auto-follow the selection.

### Inertia vs. dead stop
Adding momentum (the camera continues rotating briefly after releasing the drag) feels polished but adds complexity. Make it optional (`inertia_enabled` flag) and disabled by default for precision editing.

## 7. Non-Goals

- Cinematic camera paths / keyframe animation
- Multi-viewport synchronized cameras
- Dolly zoom (Vertigo effect)
- Touch/pinch-to-zoom on mobile (WebGL mobile targets)
- Camera collision with orbit mode (can be layered on later from `docs/camera-collision.md`)
