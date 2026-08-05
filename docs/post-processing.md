# Post-Processing Pipeline — Future Feature

> An FBO-based post-processing chain supporting bloom, tone mapping, and extensible
> full-screen effects. Runs after the main render passes and composites onto the backbuffer.

## 1. Motivation

The current renderer draws directly to the default framebuffer with no intermediate render targets. This prevents:
- **Bloom** — bright areas bleeding into surrounding pixels for a realistic high-dynamic-range look
- **Tone mapping** — proper HDR-to-LDR conversion (the equirect sky is HDR but displayed raw)
- **Color grading** — LUT-based look adjustments
- **Anti-aliasing** — no MSAA or post-process AA (FXAA/SMAA)
- **Any post effect** that requires reading the rendered image

Adding an FBO abstraction and at least a bloom + tone mapping pass would dramatically improve the visual output for minimal effort.

## 2. Scope

### Owned by this feature

- **FBO abstraction** — `RenderTarget` class managing FBO + color/depth attachments
- **Full-screen quad pass** — reusable post-processing infrastructure
- **Bloom** — bright-pass extract → downsample → blur (tent/box) → upsample → composite
- **Tone mapping** — Reinhard, ACES filmic, or Uncharted 2 operator
- **Exposure control** — auto-exposure or manual exposure tied to `CameraComponent`

### NOT owned by this feature

- SSAO / HBAO
- Motion blur
- Depth of field
- TAA (temporal anti-aliasing)
- SSR (screen-space reflections)
- Volumetric light shafts
- Lens flares

## 3. Architecture

### 3.1 RenderTarget Abstraction

```cpp
class RenderTarget {
public:
    struct Config {
        uint32_t width  = 1920;
        uint32_t height = 1080;
        bool     has_color      = true;
        bool     has_depth      = true;
        int      color_samples  = 1;   // MSAA samples (1 = no MSAA)
        GLenum   color_format   = GL_RGBA16F;  // HDR format
        GLenum   depth_format   = GL_DEPTH_COMPONENT24;
        int      mip_levels     = 1;   // 0 = generate full mip chain
    };

    explicit RenderTarget(const Config& cfg);
    ~RenderTarget();

    void bind();           // glBindFramebuffer
    void unbind();         // back to default framebuffer
    void resize(uint32_t w, uint32_t h);  // handle window resize

    uint32_t color_texture() const;
    uint32_t depth_texture() const;
    uint32_t width() const;
    uint32_t height() const;

private:
    uint32_t fbo_ = 0;
    uint32_t color_tex_ = 0;
    uint32_t depth_tex_ = 0;
    Config config_;
};
```

### 3.2 Render Pipeline with FBOs

```
RenderSystem::update()
  │
  ├─ HDR RenderTarget::bind()
  │   ├─ Cubemap pass
  │   ├─ Equirect pass
  │   ├─ Opaque pass
  │   ├─ Reflective pass
  │   ├─ Particle pass
  │   ├─ Volume pass
  │   └─ Highlight pass
  │   └─ HDR RenderTarget::unbind()
  │
  ├─ PostProcessPass::execute(hdr_color_texture, hdr_depth_texture)
  │   ├─ Downscale chain (1/2, 1/4, 1/8, 1/16)
  │   │   ├─ Bright-pass extract at each level
  │   │   └─ Box/tent blur at each level
  │   ├─ Upsample + accumulate bloom (additive blending)
  │   ├─ Tone map + composite bloom onto backbuffer
  │   └─ (Optional) Color grading LUT
```

### 3.3 Bloom Pass

```
HDR input (RGBA16F)
    │
    ▼
Bright-pass extract:
  luminance = dot(color, vec3(0.2126, 0.7152, 0.0722))
  output = max(luminance - threshold, 0) * color   (clamped)

    │
    ▼
Downsample chain (each step: 1/2 resolution, box filter 4-tap):
  level[0] = bright_pass(input)          // full res
  level[1] = downsample(level[0])        // 1/2
  level[2] = downsample(level[1])        // 1/4
  level[3] = downsample(level[2])        // 1/8
  level[4] = downsample(level[3])        // 1/16

    │
    ▼
Blur each level (separable gaussian, 2-pass horizontal + vertical):
  For each level i (1..4):
    blur_h(level[i])
    blur_v(level[i])

    │
    ▼
Upsample + accumulate (from smallest to largest):
  result = level[4]
  result = upsample(result) + level[3]
  result = upsample(result) + level[2]
  result = upsample(result) + level[1]
  result = upsample(result) + level[0]

    │
    ▼
Bloom output (additive blend onto HDR scene)
```

### 3.4 Tone Mapping

Multiple operators, selectable at runtime:

```glsl
// Reinhard (simple, good default)
vec3 reinhard(vec3 hdr) {
    return hdr / (hdr + vec3(1.0));
}

// ACES filmic (industry standard, preserves saturation)
vec3 aces(vec3 hdr) {
    // Narkowicz 2015 fit
    vec3 a = hdr * (hdr * 2.51 + 0.03);
    vec3 b = hdr * (hdr * 2.43 + 0.59) + 0.14;
    return clamp(a / b, 0.0, 1.0);
}

// Uncharted 2 (John Hable's filmic curve)
vec3 uncharted2(vec3 hdr) {
    const float A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30;
    return ((hdr * (A * hdr + C * B) + D * E) / (hdr * (A * hdr + B) + D * F)) - E / F;
}

// Final: tone_map(scene_color + bloom * bloom_intensity) * exposure
// Output gamma correction (linear → sRGB, pow 1/2.2)
```

### 3.5 FXAA (Optional, Phase 2)

A simple post-process anti-aliasing pass that runs on the final LDR image before presentation. Single full-screen quad, reads luminance edges, blurs along edges. ~15 lines of GLSL.

### 3.6 Full-Screen Quad Utility

```cpp
class FullScreenQuad {
public:
    FullScreenQuad();
    void draw();  // renders a single triangle covering the viewport
private:
    uint32_t vao_ = 0;  // no vertex data needed — generated in vertex shader from gl_VertexID
};
```

Vertex shader:
```glsl
#version 330 core
out vec2 v_uv;
void main() {
    v_uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(v_uv * 2.0 - 1.0, 0.0, 1.0);
}
```

One quad, one VAO, reused by every post-processing shader.

## 4. Implementation Plan

### Phase 1: FBO + Full-Screen Quad (~3 hours)

| Task | Description |
|---|---|
| 1.1 | Implement `RenderTarget` class (FBO, color/depth textures, resize) |
| 1.2 | Implement `FullScreenQuad` utility (single-triangle VAO) |
| 1.3 | Modify `RenderSystem` to render into an HDR `RenderTarget` instead of backbuffer |
| 1.4 | Add simple copy-to-backbuffer pass to verify FBO pipeline works |

### Phase 2: Bloom (~5 hours)

| Task | Description |
|---|---|
| 2.1 | Implement bright-pass extract shader |
| 2.2 | Implement downsample shader (box filter, 4-tap) |
| 2.3 | Implement separable gaussian blur shaders (horizontal + vertical) |
| 2.4 | Implement upsample + accumulate shader |
| 2.5 | Implement `BloomPass` class managing the downsample chain render targets |
| 2.6 | Expose bloom parameters: threshold, intensity, blur radius |

### Phase 3: Tone Mapping (~3 hours)

| Task | Description |
|---|---|
| 3.1 | Implement tone mapping + gamma correction shader |
| 3.2 | Add ACES and Reinhard operators |
| 3.3 | Add `ToneMapParams` component (operator selection, exposure) |
| 3.4 | Auto-exposure: compute scene average luminance (histogram or downsample chain) |
| 3.5 | Final composite pass: tone_map(scene + bloom * intensity) → backbuffer |

### Phase 4: Polish (~2 hours)

| Task | Description |
|---|---|
| 4.1 | Color grading via 3D LUT texture (optional, .cube format) |
| 4.2 | FXAA pass (single shader, runs after tone mapping) |
| 4.3 | Runtime toggle: enable/disable post-processing |
| 4.4 | Handle window resize: recreate all render targets |

## 5. File Layout (planned additions)

```
include/exd/render/
├── graphics/
│   ├── render_target.hpp              # NEW: RenderTarget
│   ├── fullscreen_quad.hpp            # NEW: FullScreenQuad
│   └── techniques/
│       └── post_process.hpp           # NEW: PostProcessPass (bloom + tone map)
├── components/
│   └── tone_map_params.hpp            # NEW: tone map settings

src/graphics/
├── render_target.cpp                  # NEW
├── fullscreen_quad.cpp                # NEW
└── techniques/
    └── post_process.cpp               # NEW

shaders/opengl/post/
├── fullscreen.vert                    # NEW: single-triangle vertex shader
├── bright_pass.frag                   # NEW
├── downsample.frag                    # NEW
├── blur_h.frag                        # NEW
├── blur_v.frag                        # NEW
├── upsample.frag                      # NEW
├── tone_map.frag                      # NEW
└── fxaa.frag                          # NEW
```

## 6. Design Decisions

### HDR render target format
`GL_RGBA16F` (half-float). Sufficient precision for HDR lighting, supported on OpenGL 3.3+ and GLES 3.0+. No need for `GL_RGBA32F` unless doing path-traced accumulation.

### Bloom downsample vs. compute shader
Traditional multi-pass downsample/blur/upsample using full-screen quads. Works on GL 3.3, simple to implement. A compute shader approach (GL 4.3+) would be more efficient but not universally available (especially on WebGL 2.0 which lacks compute shaders).

### Bloom as part of RenderSystem vs. separate system
Part of `RenderSystem` since it needs to run between the HDR render and the final framebuffer presentation. Could be extracted to a `PostProcessSystem` but the tight coupling to the render pipeline makes inlining simpler for v1.

### Auto-exposure
Histogram-based: downsample the HDR scene to 1×1, read back the average luminance on CPU, smooth over time with exponential moving average. Simple, effective, no GPU readback stall if done with PBO or delayed by one frame.

## 7. Non-Goals

- SSAO / HBAO (separate feature, significant shader complexity)
- Depth of field (requires circle-of-confusion calculation, expensive blur)
- Motion blur (requires velocity buffer)
- TAA (requires jittered projection + history buffer)
- Volumetric lighting / god rays
- Lens flares / dirt masks
