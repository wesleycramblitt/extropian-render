#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/core/window_state.hpp>
#include <exd/render/graphics/graphics_context.hpp>
#include <exd/render/graphics/techniques/lambertian_technique.hpp>
#include <exd/render/graphics/techniques/reflective_technique.hpp>
#include <exd/render/graphics/techniques/cubemap_technique.hpp>
#include <exd/render/graphics/techniques/equirect_technique.hpp>
#include <exd/render/graphics/techniques/particle_technique.hpp>
#include <exd/render/graphics/techniques/volume_technique.hpp>
#include <exd/render/graphics/techniques/highlight_technique.hpp>
#include <exd/math/mat4.hpp>

namespace exd::render {

/// Main rendering orchestrator — dispatches to all render passes.
class RenderSystem {
public:
    RenderSystem(GraphicsContext& ctx, core::WindowState* win)
        : ctx_(ctx), window_(win), cubemap_(ctx), equirect_(ctx),
          lambertian_(ctx),
          reflective_(ctx), particles_(ctx), volume_(ctx),
          highlight_(ctx) {}
    ~RenderSystem() = default;

    void update(exd::ecs::Registry& registry, double dt);

    /// Set the background clear color (default: dark gray 0.15,0.15,0.15).
    void set_clear_color(float r, float g, float b, float a = 1.0f);

private:
    void render_cubemap_pass(exd::ecs::Registry&, const math::Mat4& view, const math::Mat4& proj);
    void render_equirect_pass(exd::ecs::Registry&, const math::Mat4& view, const math::Mat4& proj);
    void render_opaque_pass(exd::ecs::Registry&, const math::Mat4& view, const math::Mat4& proj, const math::Vec3f& cam_pos);
    void render_reflective_pass(exd::ecs::Registry&, const math::Mat4& view, const math::Mat4& proj, const math::Vec3f& cam_pos);
    void render_particle_pass(exd::ecs::Registry&, const math::Mat4& view, const math::Mat4& proj);
    void render_volume_pass(exd::ecs::Registry&, const math::Mat4& view, const math::Mat4& proj, const math::Vec3f& cam_pos);
    void render_highlight_pass(exd::ecs::Registry&, const math::Mat4& view, const math::Mat4& proj);
    static math::Mat4 compute_model(exd::ecs::Registry&, exd::ecs::Entity e);

    GraphicsContext& ctx_;
    core::WindowState* window_;
    float clear_r_ = 0.15f, clear_g_ = 0.15f, clear_b_ = 0.15f, clear_a_ = 1.0f;
    CubeMapRenderTechnique cubemap_;
    EquirectTechnique equirect_;
    LambertianTechnique lambertian_;
    ReflectiveTechnique reflective_;
    ParticleRenderTechnique particles_;
    VolumeRenderTechnique volume_;
    HighlightTechnique highlight_;
};

} // namespace exd::render
