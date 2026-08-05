# PBR Pipeline & glTF 2.0 — Future Feature

> Physically-based rendering with metallic-roughness workflow, image-based lighting from
> the existing skybox/skydome, and glTF 2.0 asset import for industry-standard models.

## 1. Motivation

The current material model is a bare Lambert diffuse shader with an additive ambient term. It has no specular response, no roughness, no metal parameter, and no environment reflections baked into the shading model (the `ReflectiveTechnique` is a separate pass for full-mirror surfaces, not integrated into the main shader). This limits visual quality and prevents loading standard glTF assets with their PBR materials intact.

glTF 2.0 is the de facto standard for real-time 3D asset interchange. It carries PBR material data (base color, metallic, roughness, normal maps, occlusion, emissive), skeletal animation data, and scene hierarchy — all of which are currently discarded by the Assimp-based loader.

## 2. Scope

### Owned by this feature

- **PBR surface shader** — metallic-roughness GGX BRDF with multi-scatter approximation
- **Image-based lighting (IBL)** — diffuse irradiance + specular pre-filtered environment map from existing skybox
- **PBR material component** — extends or replaces current `Material` with PBR parameters
- **glTF 2.0 loader** — standalone loader or Assimp post-process that extracts PBR data
- **Environment map pre-compute** — irradiance map + pre-filtered specular mip chain at load time
- **Normal mapping** — tangent-space normal map support in the vertex + fragment shader

### NOT owned by this feature

- glTF animation (covered in `docs/animation.md`)
- glTF skinning (covered in `docs/animation.md`)
- Clearcoat, sheen, transmission, volume extensions (KHR_materials_*)
- Texture compression (basisu / KTX2)
- glTF scene export

## 3. Architecture

### 3.1 PBR Material Component

Replace or extend the current `Material` component:

```cpp
struct PbrMaterial {
    // Base color (albedo) — replaces Material::baseColor
    math::Vec3f base_color{1.0f, 1.0f, 1.0f};
    float       metallic  = 0.0f;   // 0 = dielectric, 1 = metallic
    float       roughness = 0.5f;   // 0 = smooth, 1 = rough
    float       ao        = 1.0f;   // ambient occlusion multiplier

    // Optional: texture handles for PBR maps
    uint32_t base_color_tex   = 0;  // TextureManager handle
    uint32_t metallic_roughness_tex = 0; // combined R=ao G=roughness B=metallic
    uint32_t normal_tex       = 0;
    uint32_t emissive_tex     = 0;
    math::Vec3f emissive_factor{0.0f, 0.0f, 0.0f};

    // Legacy compatibility
    BlendMode blend_mode = BlendMode::Opaque;
    DepthMode depth_mode = DepthMode::ReadWrite;
    CullMode  cull_mode  = CullMode::Back;
};
```

Keep the old `Material` component for backward compatibility; the PBR shader checks for `PbrMaterial` presence and falls back to `Material`.

### 3.2 PBR Shader (pbr.vert / pbr.frag)

New shader pair replacing `lambertian.vert/.frag`:

**Vertex shader inputs:**
```
a_pos(0), a_norm(1), a_uv(2), a_tangent(3), a_color(4)
```
Outputs: world position, world normal, tangent-space matrix (TBN), UV, vertex color.

**Fragment shader BRDF:**

```
Cook-Torrance microfacet specular:
  D = GGX_TrowbridgeReitz(N, H, roughness)
  G = Smith_Schlick(N, V, roughness) * Smith_Schlick(N, L, roughness)
  F = Schlick_Fresnel(F0, VdotH)   where F0 = lerp(0.04, base_color, metallic)

Specular = D * G * F / (4 * NdotL * NdotV)
Diffuse  = base_color / PI * (1 - F) * (1 - metallic)

Final = (Diffuse * irradiance + Specular * prefiltered_env) * ao +
        emissive_factor * emissive_tex_sample
```

### 3.3 Image-Based Lighting

Pre-compute from the existing skybox cubemap or equirectangular HDR at load time:

| Map | Type | Resolution | Purpose |
|---|---|---|---|
| Irradiance map | Cubemap | 32×32 per face | Diffuse IBL (convolved with cos lobe) |
| Pre-filtered env map | Cubemap mip chain | 128×128 base, 5 mips | Specular IBL (each mip = increasing roughness) |
| BRDF LUT | 2D texture | 512×512 | Pre-computed BRDF integration (split-sum LUT) |

These are computed once per skybox load, either:
- **On GPU**: Full-screen quad passes rendering into cubemap face FBOs
- **On CPU**: Slow but works headlessly — acceptable for 32×32 irradiance
- **Hybrid**: Compute shaders (requires GL 4.3, not yet used in this codebase)

### 3.4 glTF 2.0 Loader

Two options:

**Option A: Assimp post-process** — use the existing `MeshAssetSystem` + Assimp, then walk Assimp's material stack extracting aiMaterial properties (base color, metallic, roughness, normal map path). This is the faster path but limited by Assimp's glTF support quality.

**Option B: Standalone glTF loader** — parse glTF JSON + binary buffers directly using nlohmann/json (already a dependency). Full control, supports extensions, but more code.

Recommendation: Start with **Option A** for mesh geometry, supplement with **Option B** for material + texture data. The existing `MeshAssetSystem` handles the mesh, and a new `GltfMaterialLoader` handles the PBR material assignment.

```cpp
class GltfAssetSystem {
public:
    void configure(core::WindowState* window);
    void update(exd::ecs::Registry& registry, float dt);

private:
    // Loads a .gltf/.glb file, spawns entities with:
    // Transform hierarchy, RenderableComponent, PbrMaterial, optional animation data
    void load_gltf(const std::string& path, exd::ecs::Registry& registry);
};
```

### 3.5 Normal Mapping

Tangent calculation: either import tangents from the mesh file (glTF includes them), or compute them from UVs + positions in `MeshAssetSystem`.

Vertex shader constructs the TBN matrix:
```glsl
vec3 T = normalize(mat3(model) * a_tangent);
vec3 B = normalize(mat3(model) * a_bitangent);
vec3 N = normalize(mat3(model) * a_normal);
mat3 TBN = mat3(T, B, N);
```

Fragment shader samples the normal map and transforms:
```glsl
vec3 sampled_normal = texture(u_normal_map, uv).rgb * 2.0 - 1.0;
vec3 world_normal = normalize(TBN * sampled_normal);
```

## 4. Implementation Plan

### Phase 1: PBR Shader (~6 hours)

| Task | Description |
|---|---|
| 1.1 | Add `PbrMaterial` component definition |
| 1.2 | Implement `pbr.vert` / `pbr.frag` shader pair with GGX BRDF |
| 1.3 | Add TBN matrix support to `MeshManager` (tangent/bitangent vertex attributes) |
| 1.4 | Implement normal mapping in the PBR shader |
| 1.5 | Add `PbrTechnique` render technique class |
| 1.6 | Wire `RenderSystem` to use PBR pass for entities with `PbrMaterial` |

### Phase 2: IBL Pre-compute (~5 hours)

| Task | Description |
|---|---|
| 2.1 | Implement irradiance cubemap convolution (cos-weighted hemisphere sampling) |
| 2.2 | Implement pre-filtered environment cubemap (GGX importance sampling per mip) |
| 2.3 | Generate BRDF integration LUT (2D texture, split-sum approximation) |
| 2.4 | Bind IBL textures in PBR shader, implement split-sum specular lookup |
| 2.5 | Trigger IBL recompute on skybox change |

### Phase 3: glTF 2.0 Loader (~8 hours)

| Task | Description |
|---|---|
| 3.1 | Parse glTF JSON structure (scenes, nodes, meshes, materials, textures) |
| 3.2 | Import mesh data (positions, normals, UVs, tangents, indices) |
| 3.3 | Import PBR material parameters (base color, metallic, roughness, normal map, emissive) |
| 3.4 | Import textures via stb_image, register with TextureManager |
| 3.5 | Spawn ECS entity hierarchy from glTF node tree |
| 3.6 | Handle `.glb` binary container format |
| 3.7 | Handle common extensions: KHR_texture_transform, KHR_materials_emissive_strength |

### Phase 4: Compatibility & Polish (~3 hours)

| Task | Description |
|---|---|
| 4.1 | Fallback shader: entities without `PbrMaterial` use existing lambertian pass |
| 4.2 | glTF default scene selection |
| 4.3 | Support glTF cameras and lights as ECS entities |
| 4.4 | Demo scene: load a glTF PBR model (e.g., DamagedHelmet, Sponza) |

## 5. File Layout (planned additions)

```
include/exd/render/
├── components/
│   └── pbr_material.hpp               # NEW: PbrMaterial
├── graphics/techniques/
│   └── pbr_technique.hpp              # NEW: PbrTechnique
└── systems/
    └── gltf_asset_system.hpp          # NEW: GltfAssetSystem

src/
├── graphics/techniques/
│   └── pbr_technique.cpp              # NEW
└── systems/
    └── gltf_asset_system.cpp          # NEW

shaders/opengl/
├── pbr/
│   ├── pbr.vert                       # NEW
│   └── pbr.frag                       # NEW
└── ibl/
    ├── irradiance_conv.vert/.frag     # NEW: hemisphere convolution
    ├── prefilter_env.vert/.frag       # NEW: GGX importance sampling
    └── brdf_lut.vert/.frag            # NEW: split-sum LUT generation
```

## 6. Design Decisions

### Separate PbrTechnique vs. modifying LambertianTechnique
A separate technique keeps the existing Lambertian path working for backward compatibility. The `RenderTechnique_Lambertian` tag routes to the old path; a new `RenderTechnique_PBR` tag (or detection of `PbrMaterial` component) routes to the new path. This avoids breaking the existing demo and allows incremental migration.

### CPU vs. GPU IBL pre-compute
GPU pre-compute is preferred (FBO rendering into cubemap faces) since it's fast and the technique already does rendering. CPU path as fallback for the Null backend. No compute shader dependency for v1.

### glTF loader: standalone vs. through Assimp
Standalone parser for materials and textures (control, extensions), Assimp post-process for mesh geometry (reuse existing vertex import). This hybrid approach gives the best of both.

## 7. Non-Goals

- Clearcoat, sheen, anisotropy, transmission (KHR_materials_* extensions) — can be added later
- KTX2 / Basis Universal texture compression
- glTF export (this is an import-only feature)
- Ray-traced / path-traced PBR
- Shader compilation to SPIR-V (GLSL source only for OpenGL)
