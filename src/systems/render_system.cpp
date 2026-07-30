#include <exd/render/systems/render_system.hpp>
#include <exd/render/components/camera_component.hpp>
#include <exd/render/components/cubemap.hpp>
#include <exd/render/components/disabled.hpp>
#include <exd/render/components/environment.hpp>
#include <exd/render/components/particle_cloud.hpp>
#include <exd/render/components/render_technique_tags.hpp>
#include <exd/render/components/renderable.hpp>
#include <exd/render/components/material.hpp>
#include <exd/render/components/selected.hpp>
#include <exd/render/components/simulation_domain.hpp>
#include <exd/render/components/simulation_reference.hpp>
#include <exd/render/components/skew.hpp>
#include <exd/render/components/transform.hpp>
#include <exd/ecs/view.hpp>
#include <exd/render/components/volume_field.hpp>

namespace exd::render {

// ════════════════════════════════════════════════════════════════════
// RenderSystem
// ════════════════════════════════════════════════════════════════════

math::Mat4 RenderSystem::compute_model(exd::ecs::Registry& registry, exd::ecs::Entity e) {
    auto& xform = registry.get<Transform>(e);
    if (registry.has<Skew>(e)) {
        auto& sk = registry.get<Skew>(e);
        return math::Mat4::trs(xform.position, xform.rotation, xform.scale, sk.shear);
    }
    return math::Mat4::trs(xform.position, xform.rotation, xform.scale);
}

void RenderSystem::render_cubemap_pass(exd::ecs::Registry& registry,
                                        const math::Mat4& view, const math::Mat4& proj) {
    auto v = registry.view<CubeMapComponent, RenderableComponent, RenderTechnique_CubeMap>();
    if (v.begin() == v.end()) return;

    cubemap_.bind();
    for (auto e : v) {
        if (registry.has<Disabled>(e)) continue;
        auto& cm = registry.get<CubeMapComponent>(e);
        auto& r = registry.get<RenderableComponent>(e);
        if (r.mesh == 0) continue;
        Renderable data{r.mesh, cm.gl_cubemap, {{"u_view", view}, {"u_proj", proj}}};
        cubemap_.draw(data);
    }
    cubemap_.unbind();
}

void RenderSystem::render_opaque_pass(exd::ecs::Registry& registry,
                                       const math::Mat4& view, const math::Mat4& proj,
                                       const math::Vec3f& cam_pos) {
    auto v = registry.view<Transform, RenderableComponent, RenderTechnique_Lambertian>();
    if (v.begin() == v.end()) return;

    // discover scene-wide fog / lighting from ECS (first entity wins)
    math::Vec3f fog_color{0.5f, 0.5f, 0.5f};
    float fog_density = 0.0f;
    math::Vec3f ambient{0.1f, 0.1f, 0.1f};
    math::Vec3f sun_dir{0.5f, 1.0f, 0.3f};
    math::Vec3f sun_color{1.0f, 1.0f, 1.0f};

    for (auto e : registry.view<FogComponent>()) {
        auto& f = registry.get<FogComponent>(e);
        fog_color = f.color;
        fog_density = f.density;
        break;
    }
    for (auto e : registry.view<SceneLighting>()) {
        auto& sl = registry.get<SceneLighting>(e);
        ambient = sl.ambient;
        sun_dir = sl.sun_direction;
        sun_color = sl.sun_color;
        break;
    }

    lambertian_.bind(view, proj, cam_pos, fog_color, fog_density,
                     ambient, sun_dir, sun_color);
    for (auto e : v) {
        if (registry.has<Disabled>(e)) continue;
        auto& r = registry.get<RenderableComponent>(e);
        if (r.mesh == 0) continue;

        // Apply per-entity material color (default white if no Material component)
        if (auto* mat = registry.try_get<Material>(e)) {
            lambertian_.setBaseColor(mat->baseColor);
        } else {
            lambertian_.setBaseColor(math::Quat{1.0f, 1.0f, 1.0f, 1.0f});
        }
        lambertian_.draw(r.mesh, compute_model(registry, e));
    }
    lambertian_.unbind();
}

void RenderSystem::render_reflective_pass(exd::ecs::Registry& registry,
                                           const math::Mat4& view, const math::Mat4& proj,
                                           const math::Vec3f& cam_pos) {
    auto v = registry.view<Transform, RenderableComponent, RenderTechnique_Mirror>();
    if (v.begin() == v.end()) return;

    uint32_t cubemap_tex = 0;
    for (auto e : registry.view<CubeMapComponent, RenderTechnique_CubeMap>()) {
        cubemap_tex = registry.get<CubeMapComponent>(e).gl_cubemap;
        break;
    }
    if (cubemap_tex == 0) return;

    reflective_.bind(view, proj, cam_pos, cubemap_tex);
    for (auto e : v) {
        if (registry.has<Disabled>(e)) continue;
        auto& r = registry.get<RenderableComponent>(e);
        if (r.mesh == 0) continue;
        reflective_.draw(r.mesh, compute_model(registry, e));
    }
    reflective_.unbind();
}

void RenderSystem::render_particle_pass(exd::ecs::Registry& registry,
                                         const math::Mat4& view, const math::Mat4& proj) {
    auto v = registry.view<ParticleCloudComponent, SimulationReference>();
    if (v.begin() == v.end()) return;

    particles_.bind();
    for (auto e : v) {
        if (registry.has<Disabled>(e)) continue;
        auto& pc = registry.get<ParticleCloudComponent>(e);
        if (pc.particle_count == 0 || pc.positions.empty()) continue;

        auto sim_id = registry.get<SimulationReference>(e).simulation_entity_id;
        const Transform* xform = nullptr;
        for (auto db : registry.view<Transform, SimulationReference>()) {
            if (registry.get<SimulationReference>(db).simulation_entity_id == sim_id) {
                xform = &registry.get<Transform>(db);
                break;
            }
        }
        if (!xform) continue;

        ParticleDrawData data{
            pc.positions.data(),
            pc.colors.empty() ? nullptr : pc.colors.data(),
            pc.particle_count,
            {{"u_model", math::Mat4::identity()},
             {"u_view", view}, {"u_proj", proj}}
        };
        particles_.draw(data);
    }
    particles_.unbind();
}

void RenderSystem::render_volume_pass(exd::ecs::Registry& registry,
                                       const math::Mat4& view, const math::Mat4& proj,
                                       const math::Vec3f& cam_pos) {
    auto v = registry.view<VolumeFieldComponent, SimulationReference>();
    if (v.begin() == v.end()) return;

    volume_.bind();
    for (auto e : v) {
        if (registry.has<Disabled>(e)) continue;
        auto& vol = registry.get<VolumeFieldComponent>(e);
        if (!vol.interop_ready || vol.texture_handle == 0) continue;

        auto sim_id = registry.get<SimulationReference>(e).simulation_entity_id;
        exd::ecs::Entity domain_entity{};
        for (auto db : registry.view<Transform, SimulationDomain, SimulationReference>()) {
            if (registry.get<SimulationReference>(db).simulation_entity_id == sim_id) {
                domain_entity = db; break;
            }
        }
        if (!registry.valid(domain_entity)) continue;

        auto& dom = registry.get<SimulationDomain>(domain_entity);
        VolumeDrawData data{vol.texture_handle, 0, dom.nx, dom.ny, dom.nz,
            {{"u_view", view}, {"u_proj", proj}, {"u_cam_pos", cam_pos}}};
        volume_.draw(data);
    }
    volume_.unbind();
}

// ════════════════════════════════════════════════════════════════════
// Highlight pass — wireframe overlay on selected entities
// ════════════════════════════════════════════════════════════════════

void RenderSystem::render_highlight_pass(exd::ecs::Registry& registry,
                                          const math::Mat4& view,
                                          const math::Mat4& proj) {
    auto view_selected = registry.view<Selected, Transform, RenderableComponent>();
    if (view_selected.begin() == view_selected.end()) return;

    highlight_.bind(view, proj);
    for (auto e : view_selected) {
        if (registry.has<Disabled>(e)) continue;
        auto& rc = registry.get<RenderableComponent>(e);
        if (rc.mesh == 0) continue;

        math::Mat4 model = compute_model(registry, e);
        highlight_.draw(rc.mesh, model);
    }
    highlight_.unbind();
}

void RenderSystem::update(exd::ecs::Registry& registry, double /*dt*/) {
    glEnable(GL_DEPTH_TEST);
    glClearColor(clear_r_, clear_g_, clear_b_, clear_a_);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Find camera entity
    const Transform* cam_xform = nullptr;
    const CameraComponent* cam = nullptr;
    for (auto e : registry.view<CameraComponent, Transform>()) {
        cam = &registry.get<CameraComponent>(e);
        cam_xform = &registry.get<Transform>(e);
        break;
    }
    if (!cam || !cam_xform) return;

    int w, h; float aspect;
    window_->get_dimensions(w, h, aspect);

    math::Vec3f fwd = (cam_xform->rotation * math::Vec3f{0, 0, -1}).normalized();
    math::Vec3f up  = (cam_xform->rotation * math::Vec3f{0, 1,  0}).normalized();
    math::Mat4 view_mat = math::Mat4::look_at(cam_xform->position, cam_xform->position + fwd, up);
    math::Mat4 proj_mat = math::Mat4::perspective(cam->fov_y_radians, aspect,
                                                   cam->near_plane, cam->far_plane);

    render_cubemap_pass(registry, view_mat, proj_mat);
    render_opaque_pass(registry, view_mat, proj_mat, cam_xform->position);
    render_reflective_pass(registry, view_mat, proj_mat, cam_xform->position);
    render_particle_pass(registry, view_mat, proj_mat);
    render_volume_pass(registry, view_mat, proj_mat, cam_xform->position);
    render_highlight_pass(registry, view_mat, proj_mat);
}

void RenderSystem::set_clear_color(float r, float g, float b, float a) {
    clear_r_ = r; clear_g_ = g; clear_b_ = b; clear_a_ = a;
}

} // namespace exd::render
