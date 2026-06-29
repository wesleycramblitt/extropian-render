#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/app/window.hpp>
#include <exd/render/graphics_context.hpp>
#include <exd/render/techniques.hpp>
#include <exd/render/particle_volume.hpp>
#include <exd/render/components.hpp>
#include <exd/math/mat4.hpp>

namespace exd::render {

/// @brief Main rendering orchestrator — dispatches to all render passes.
///
/// Pass order: CubeMap → Opaque → Reflective → Particles → Volume
class RenderSystem {
public:
    explicit RenderSystem(GraphicsContext& ctx);
    ~RenderSystem();

    void update(exd::ecs::Registry& registry, const exd::app::Window& window, float dt);

private:
    void render_cubemap_pass(exd::ecs::Registry& registry,
                             const math::Mat4& view, const math::Mat4& proj);
    void render_opaque_pass(exd::ecs::Registry& registry,
                            const math::Mat4& view, const math::Mat4& proj);
    void render_reflective_pass(exd::ecs::Registry& registry,
                                const math::Mat4& view, const math::Mat4& proj,
                                const math::Vec3& cam_pos);
    void render_particle_pass(exd::ecs::Registry& registry,
                              const math::Mat4& view, const math::Mat4& proj);
    void render_volume_pass(exd::ecs::Registry& registry,
                            const math::Mat4& view, const math::Mat4& proj,
                            const math::Vec3& cam_pos);

    static math::Mat4 compute_model(exd::ecs::Registry& registry, exd::ecs::Entity e);

    GraphicsContext& ctx_;
    CubeMapRenderTechnique  cubemap_;
    LambertianTechnique     lambertian_;
    ReflectiveTechnique     reflective_;
    ParticleRenderTechnique particles_;
    VolumeRenderTechnique   volume_;
};

/// Camera controller using FPS-style input.
class CameraSystem {
public:
    void update(exd::ecs::Registry& registry, exd::app::Window& window, float dt);
};

/// Generates GPU meshes for cube primitives.
class PrimitiveMeshSystem {
public:
    explicit PrimitiveMeshSystem(GraphicsContext& ctx) : ctx_(ctx) {}
    void update(exd::ecs::Registry& registry, exd::app::Window& window);
    Mesh create_cube_mesh(float size);
private:
    GraphicsContext& ctx_;
};

/// Loads cubemap textures.
class CubeMapSystem {
public:
    explicit CubeMapSystem(GraphicsContext& ctx) : ctx_(ctx) {}
    void update(exd::ecs::Registry& registry, exd::app::Window& window);
    Mesh create_cubemap_mesh();
private:
    GraphicsContext& ctx_;
};

/// Loads external mesh files via Assimp.
class MeshAssetSystem {
public:
    explicit MeshAssetSystem(GraphicsContext& ctx) : ctx_(ctx) {}
    void update(exd::ecs::Registry& registry, exd::app::Window& window);
private:
    GraphicsContext& ctx_;
};

/// Renders a grid on the XZ plane.
class GridSystem {
public:
    explicit GridSystem(GraphicsContext& ctx) : ctx_(ctx) {}
    void update(exd::ecs::Registry& registry, exd::app::Window& window);
private:
    GraphicsContext& ctx_;
};

/// Toggles wireframe/fill polygon mode.
class PolygonModeSystem {
public:
    void update(exd::ecs::Registry& registry, exd::app::Window& window, float dt);
};

} // namespace exd::render
