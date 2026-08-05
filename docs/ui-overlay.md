# UI Overlay & Text Rendering — Future Feature

> On-screen text, debug overlays, and optional Dear ImGui integration for property editing
> and scene inspection. Renders as a 2D overlay on top of the 3D viewport.

## 1. Motivation

The current renderer has zero on-screen text or UI capabilities. There is no way to:
- Display FPS, frame time, or draw call counts
- Show entity labels (name above object)
- Present a property editor for selected entities
- Display a scene hierarchy / outliner
- Show help text, notifications, or console output
- Provide sliders, checkboxes, or color pickers for tuning parameters

This makes debugging and iteration slow — every parameter change requires a recompile. Adding even a basic text overlay and optional ImGui integration would dramatically improve developer productivity.

## 2. Scope

### Owned by this feature

- **Debug text overlay** — FPS counter, frame time, entity count, draw calls via simple bitmap font
- **Text rendering** — monospace bitmap font rasterizer for debug labels and HUD
- **Dear ImGui integration** (optional) — full UI toolkit for property editors, scene explorers
- **Named entities** — `NameComponent` for human-readable entity identifiers

### NOT owned by this feature

- Custom UI widget toolkit (ImGui handles this)
- Rich text / variable-width fonts / internationalization
- HTML/CSS-based UI
- 2D sprite / billboard rendering
- In-world 3D UI (diegetic interfaces)

## 3. Architecture

### 3.1 Debug Overlay (Minimal — Phase 1)

A lightweight, dependency-free debug text overlay using a bitmap font:

```cpp
class DebugOverlay {
public:
    DebugOverlay();
    void begin_frame();
    void draw_text(const std::string& text, float x, float y,
                   const math::Vec3f& color = {1,1,1});
    void draw_text_3d(const std::string& text, const math::Vec3f& world_pos,
                      const math::Mat4& view, const math::Mat4& proj);
    void end_frame();
    void resize(uint32_t width, uint32_t height);

private:
    uint32_t font_tex_ = 0;
    uint32_t vao_ = 0, vbo_ = 0;
    uint32_t shader_;
};
```

**Font:** 8×16 bitmap font baked into a 256×256 RGBA texture at build time (ASCII 32-126, ~95 glyphs in a 16×8 grid). No external dependency — the font data is a `const uint8_t[]` array compiled into the binary.

**Rendering:** Each character is a small textured quad. Glyph quads are batched into a single draw call with an orthographic projection. The shader is trivial: sample the glyph texture, multiply by the text color, discard alpha=0 pixels.

```glsl
// debug_text.vert
uniform mat4 u_projection;  // ortho, pixel coords
in vec2 a_pos, a_uv;
in vec4 a_color;
out vec2 v_uv;
out vec4 v_color;
void main() {
    gl_Position = u_projection * vec4(a_pos, 0.0, 1.0);
    v_uv = a_uv;
    v_color = a_color;
}

// debug_text.frag
uniform sampler2D u_font_tex;
in vec2 v_uv;
in vec4 v_color;
out vec4 frag_color;
void main() {
    float alpha = texture(u_font_tex, v_uv).r;
    frag_color = vec4(v_color.rgb, v_color.a * alpha);
}
```

### 3.2 World-Space Labels

`draw_text_3d()` projects a 3D world position to screen coordinates using the camera's view-projection matrix, then renders the text at the projected screen position. Optionally draws a small line from the text baseline to the projected point.

Useful for: entity names, gizmo axis labels, distance measurements.

### 3.3 NameComponent

```cpp
struct NameComponent {
    std::string name;  // e.g., "Spawner_01", "MainCamera", "DirectionalLight"
};
```

Assignable during entity creation. Used by debug overlay to label entities, by the outliner for hierarchy display, by logging for readable entity references.

### 3.4 Dear ImGui Integration (Phase 2)

Dear ImGui is a well-established immediate-mode GUI library for C++. It's header-only, dependency-free, and has first-class OpenGL + SDL3 backends.

```cpp
class ImGuiRenderer {
public:
    ImGuiRenderer(SDL_Window* window, SDL_GLContext gl_context);
    ~ImGuiRenderer();

    void begin_frame();
    void end_frame();         // renders ImGui draw data
    void process_event(const SDL_Event& event);

private:
    // ImGui context, font atlas texture, shader, VAO/VBO/EBO
};
```

Integration points in the demo loop:
```cpp
// Main loop
while (running) {
    poll_events();           // forward SDL events to ImGui
    ImGui::begin_frame();

    // Existing systems...
    camera_system.update();
    render_system.update();

    // ImGui windows
    if (show_scene_explorer) {
        ImGui::Begin("Scene Explorer");
        // hierarchy tree of named entities
        ImGui::End();
    }
    if (show_property_editor) {
        ImGui::Begin("Properties");
        // editable fields for the selected entity's components
        ImGui::End();
    }
    if (show_performance) {
        ImGui::Begin("Performance");
        ImGui::Text("FPS: %.1f", fps);
        ImGui::Text("Draw calls: %d", draw_calls);
        ImGui::End();
    }

    ImGui::end_frame();      // render ImGui
    SDL_GL_SwapWindow();
}
```

### 3.5 Layering

```
Backbuffer
  │
  ├─ 3D scene (RenderSystem)           ← depth-tested, perspective
  │
  ├─ Debug overlay (DebugOverlay)       ← no depth test, orthographic
  │   ├─ FPS counter
  │   ├─ Entity labels (world-space projected)
  │   └─ Help text / notifications
  │
  └─ ImGui (ImGuiRenderer)             ← no depth test, orthographic, last
      ├─ Scene explorer
      ├─ Property editor
      ├─ Console
      └─ Performance stats
```

## 4. Implementation Plan

### Phase 1: Debug Text Overlay (~4 hours)

| Task | Description |
|---|---|
| 1.1 | Bake 8×16 bitmap font into a C array (tool: generate from a .ttf or hand-author) |
| 1.2 | Implement font texture upload + glyph UV lookup |
| 1.3 | Implement `DebugOverlay` class (quad batching, ortho projection, text rendering) |
| 1.4 | Add `NameComponent` ECS component |
| 1.5 | Wire into demo: FPS counter, entity count, draw call count overlay |
| 1.6 | Wire into demo: entity name labels (world-space projected) |

### Phase 2: Dear ImGui (~4 hours)

| Task | Description |
|---|---|
| 2.1 | Add Dear ImGui as a dependency (header-only, or FetchContent in CMake) |
| 2.2 | Implement `ImGuiRenderer` (SDL3 + OpenGL backend) |
| 2.3 | Wire SDL3 events into ImGui input |
| 2.4 | Scene explorer: tree view of all named entities with selection |
| 2.5 | Property editor: editable fields for Transform, Material, PbrMaterial |
| 2.6 | Performance window: FPS graph, frame time histogram |

### Phase 3: Polish (~2 hours)

| Task | Description |
|---|---|
| 3.1 | Toggle UI visibility (F1 = ImGui, F2 = debug overlay, F3 = labels) |
| 3.2 | Undo/redo stack for ImGui-driven property edits |
| 3.3 | Console window: log messages from systems, shader compile errors |
| 3.4 | Dark theme / style configuration for ImGui |

## 5. File Layout (planned additions)

```
include/exd/render/
├── ui/
│   ├── debug_overlay.hpp              # NEW: DebugOverlay
│   ├── imgui_renderer.hpp             # NEW: ImGuiRenderer
│   └── bitmap_font.hpp                # NEW: font glyph data
├── components/
│   └── name.hpp                       # NEW: NameComponent

src/ui/
├── debug_overlay.cpp                  # NEW
├── imgui_renderer.cpp                 # NEW
└── bitmap_font.cpp                    # NEW: baked font data

shaders/opengl/ui/
├── debug_text.vert                    # NEW
└── debug_text.frag                    # NEW

external/
└── imgui/                             # NEW: vendored or FetchContent
```

## 6. Design Decisions

### Bitmap font vs. signed-distance-field (SDF)
Bitmap font for Phase 1. It's simpler, no shader math needed, works at fixed sizes. SDF would allow arbitrary scaling with sharp edges, worth upgrading later if variable-size text is needed.

### ImGui as optional vs. required dependency
Optional. The `DebugOverlay` has no ImGui dependency — just the baked font. ImGui is added via CMake `option(EXD_RENDER_IMGUI "Enable ImGui integration" ON)` so it can be disabled for headless/CI builds.

### ImGui backend: SDL3 + OpenGL
Matches the existing platform stack. The `ImGuiRenderer` uses the SDL3 backend for input, OpenGL 3.3 backend for rendering. Both are provided by ImGui's stock backends with minimal modification.

### Text rendering vs. full 2D layer
The `DebugOverlay` is intentionally minimal — just monospace text for debug info. A full 2D sprite/screen-space rendering layer would be a separate feature. The line between "debug text" and "UI" is intentionally fuzzy here since ImGui absorbs most UI needs.

## 7. Non-Goals

- Variable-width / proportional fonts
- Rich text formatting (bold, italic, color spans)
- Internationalization / Unicode beyond ASCII
- HTML/CSS-based UI
- In-world 3D UI panels (diegetic interfaces)
- Custom widget toolkit beyond what ImGui provides
