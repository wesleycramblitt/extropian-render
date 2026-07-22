#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/app/window_state.hpp>
#include <exd/render/graphics/graphics_context.hpp>
#include <exd/render/graphics/techniques/lambertian_technique.hpp>
#include <exd/render/graphics/techniques/reflective_technique.hpp>
#include <exd/render/graphics/techniques/cubemap_technique.hpp>
#include <exd/render/graphics/techniques/particle_technique.hpp>
#include <exd/render/graphics/techniques/volume_technique.hpp>
#include <exd/math/mat4.hpp>

namespace exd::render {

/// Main rendering orchestrator — dispatches to all render passes.
class RenderSystem {
public:
    RenderSystem(GraphicsContext& ctx, app::WindowState* win)
        : ctx_(ctx), window_(win), cubemap_(ctx), lambertian_(ctx),
          reflective_(ctx), particles_(ctx), volume_(ctx) {}
    ~RenderSystem() = default;

    void update(exd::ecs::Registry& registry, double dt);

private:
    void render_cubemap_pass(exd::ecs::Registry&, const math::Mat4& view, const math::Mat4& proj);
    void render_opaque_pass(exd::ecs::Registry&, const math::Mat4& view, const math::Mat4& proj);
    void render_reflective_pass(exd::ecs::Registry&, const math::Mat4& view, const math::Mat4& proj, const math::Vec3f& cam_pos);
    void render_particle_pass(exd::ecs::Registry&, const math::Mat4& view, const math::Mat4& proj);
    void render_volume_pass(exd::ecs::Registry&, const math::Mat4& view, const math::Mat4& proj, const math::Vec3f& cam_pos);
    static math::Mat4 compute_model(exd::ecs::Registry&, exd::ecs::Entity e);

    GraphicsContext& ctx_;
    app::WindowState* window_;
    CubeMapRenderTechnique cubemap_;
    LambertianTechnique lambertian_;
    ReflectiveTechnique reflective_;
    ParticleRenderTechnique particles_;
    VolumeRenderTechnique volume_;
};

} // namespace exd::render
