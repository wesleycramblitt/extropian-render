#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/app/window.hpp>
#include <exd/render/graphics_context.hpp>
#include <exd/render/techniques.hpp>
#include <exd/render/particle_volume.hpp>
#include <exd/render/components.hpp>
#include <exd/math/mat4.hpp>

namespace exd::render {

/// Main rendering orchestrator — dispatches to all render passes.
class RenderSystem {
public:
    RenderSystem(GraphicsContext& ctx, exd::app::Window* win)
        : ctx_(ctx), window_(win), cubemap_(ctx), lambertian_(ctx),
          reflective_(ctx), particles_(ctx), volume_(ctx) {}
    ~RenderSystem() = default;

    void update(exd::ecs::Registry& registry, double dt);

private:
    void render_cubemap_pass(exd::ecs::Registry&, const math::Mat4f& view, const math::Mat4f& proj);
    void render_opaque_pass(exd::ecs::Registry&, const math::Mat4f& view, const math::Mat4f& proj);
    void render_reflective_pass(exd::ecs::Registry&, const math::Mat4f& view, const math::Mat4f& proj, const math::Vec3f& cam_pos);
    void render_particle_pass(exd::ecs::Registry&, const math::Mat4f& view, const math::Mat4f& proj);
    void render_volume_pass(exd::ecs::Registry&, const math::Mat4f& view, const math::Mat4f& proj, const math::Vec3f& cam_pos);
    static math::Mat4 compute_model(exd::ecs::Registry&, exd::ecs::Entity e);

    GraphicsContext& ctx_;
    exd::app::Window* window_;
    CubeMapRenderTechnique cubemap_;
    LambertianTechnique lambertian_;
    ReflectiveTechnique reflective_;
    ParticleRenderTechnique particles_;
    VolumeRenderTechnique volume_;
};

/// FPS camera controller.
class CameraSystem {
public:
    explicit CameraSystem(exd::app::Window* win) : window_(win) {}
    void update(exd::ecs::Registry& registry, double dt);
private:
    exd::app::Window* window_;
};

/// Generates GPU meshes for cube primitives.
class PrimitiveMeshSystem {
public:
    PrimitiveMeshSystem(GraphicsContext& ctx, exd::app::Window*) : ctx_(ctx) {}
    void update(exd::ecs::Registry& registry, double) { update_primitives(registry); }
    void update_primitives(exd::ecs::Registry& registry);
    Mesh create_cube_mesh(float size);
private:
    GraphicsContext& ctx_;
};

/// Loads cubemap textures.
class CubeMapSystem {
public:
    CubeMapSystem(GraphicsContext& ctx, exd::app::Window*) : ctx_(ctx) {}
    void update(exd::ecs::Registry& registry, double) { update_impl(registry); }
    void update_impl(exd::ecs::Registry& registry);
    Mesh create_cubemap_mesh();
private:
    GraphicsContext& ctx_;
};

/// Loads external mesh files.
class MeshAssetSystem {
public:
    MeshAssetSystem(GraphicsContext& ctx, exd::app::Window*) : ctx_(ctx) {}
    void update(exd::ecs::Registry& registry, double) { update_impl(registry); }
    void update_impl(exd::ecs::Registry& registry);
private:
    GraphicsContext& ctx_;
};

/// Renders a grid on the XZ plane.
class GridSystem {
public:
    GridSystem(GraphicsContext& ctx, exd::app::Window* win) : ctx_(ctx), window_(win) {}
    void update(exd::ecs::Registry& registry, double);
private:
    GraphicsContext& ctx_;
    exd::app::Window* window_;
};

/// Toggles wireframe/fill polygon mode.
class PolygonModeSystem {
public:
    explicit PolygonModeSystem(exd::app::Window* win) : window_(win) {}
    void update(exd::ecs::Registry&, double);
private:
    exd::app::Window* window_;
};

} // namespace exd::render
