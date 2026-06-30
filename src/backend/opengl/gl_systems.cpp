#include <exd/render/systems.hpp>
#include <exd/render/components.hpp>
#include <exd/render/mesh.hpp>
#include <exd/render/draw_data.hpp>
#include <exd/render/graphics_context.hpp>
#include <exd/core/macros.hpp>
#include <exd/math/mat4.hpp>
#include <exd/math/quat.hpp>
#include <exd/math/vec3.hpp>
// assimp disabled for first build
// assimp disabled
// assimp disabled
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <array>

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
        Renderable data{r.mesh, cm.texture_handle, {{"u_view", view}, {"u_proj", proj}}};
        cubemap_.draw(data);
    }
    cubemap_.unbind();
}

void RenderSystem::render_opaque_pass(exd::ecs::Registry& registry,
                                       const math::Mat4& view, const math::Mat4& proj) {
    auto v = registry.view<Transform, RenderableComponent, RenderTechnique_Lambertian>();
    if (v.begin() == v.end()) return;
    lambertian_.bind(view, proj);
    for (auto e : v) {
        if (registry.has<Disabled>(e)) continue;
        auto& r = registry.get<RenderableComponent>(e);
        if (r.mesh == 0) continue;
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
        cubemap_tex = registry.get<CubeMapComponent>(e).texture_handle;
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

void RenderSystem::update(exd::ecs::Registry& registry, double /*dt*/) {
    // Find camera entity
    const Transform* cam_xform = nullptr;
    const Camera* cam = nullptr;
    for (auto e : registry.view<Camera, Transform>()) {
        cam = &registry.get<Camera>(e);
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
    render_opaque_pass(registry, view_mat, proj_mat);
    render_reflective_pass(registry, view_mat, proj_mat, cam_xform->position);
    render_particle_pass(registry, view_mat, proj_mat);
    render_volume_pass(registry, view_mat, proj_mat, cam_xform->position);
}

// ════════════════════════════════════════════════════════════════════
// CameraSystem
// ════════════════════════════════════════════════════════════════════

void CameraSystem::update(exd::ecs::Registry& registry, double dt) {
    using namespace exd::math;
    using exd::app::InputMode;
    if (window_->input_mode() != InputMode::FPS) return;
    if (!window_->event_state.keyboard_state) return;

    for (auto e : registry.view<CameraController, Camera, Transform>()) {
        auto& cc = registry.get<CameraController>(e);
        float dx = -window_->event_state.mouse_rel_x;
        float dy = -window_->event_state.mouse_rel_y;

        cc.yaw   += dx * cc.mouse_sensitivity;
        cc.pitch += dy * cc.mouse_sensitivity;
        cc.pitch = std::clamp(cc.pitch, -1.55f, 1.55f);
        if (cc.yaw > 6.283f)  cc.yaw -= 6.283f;
        if (cc.yaw < -6.283f) cc.yaw += 6.283f;

        Vec3f world_up{0.0f, 1.0f, 0.0f};
        Quat q_yaw = Quat::from_axis_angle(world_up, cc.yaw);
        Vec3f local_right = (q_yaw * Vec3f{1.0f, 0.0f, 0.0f}).normalized();
        Quat q_pitch = Quat::from_axis_angle(local_right, cc.pitch);
        auto& xform = registry.get<Transform>(e);
        xform.rotation = (q_pitch * q_yaw).norm();

        Vec3f cam_fwd = (xform.rotation * Vec3f{0.0f, 0.0f, -1.0f}).normalized();
        Vec3f front = (cam_fwd - world_up * cam_fwd.dot(world_up)).normalized();
        float s = cc.move_speed * dt *
            (window_->event_state.keyboard_state[SDL_SCANCODE_LSHIFT] ? cc.sprint_mult : 1.0f);
        Vec3f move{0.0f, 0.0f, 0.0f};
        auto& ks = window_->event_state.keyboard_state;
        if (ks[SDL_SCANCODE_W]) move = move + front * s;
        if (ks[SDL_SCANCODE_S]) move = move - front * s;
        if (ks[SDL_SCANCODE_A]) move = move - local_right * s;
        if (ks[SDL_SCANCODE_D]) move = move + local_right * s;
        if (ks[SDL_SCANCODE_Q]) move = move - world_up * s;
        if (ks[SDL_SCANCODE_E]) move = move + world_up * s;
        xform.position = xform.position + move;

        break;
    }
    window_->event_state.mouse_rel_x = 0;
    window_->event_state.mouse_rel_y = 0;
}

// ════════════════════════════════════════════════════════════════════
// PrimitiveMeshSystem
// ════════════════════════════════════════════════════════════════════

void PrimitiveMeshSystem::update_primitives(exd::ecs::Registry& registry) {
    for (auto e : registry.view<CubePrimitive>()) {
        auto& cube = registry.get<CubePrimitive>(e);
        Mesh mesh = create_cube_mesh(cube.size);
        uint32_t handle = ctx_.mesh_manager.create(mesh);
        if (registry.has<RenderableComponent>(e))
            registry.get<RenderableComponent>(e).mesh = handle;
        else
            registry.emplace<RenderableComponent>(e, handle);
    }
}

Mesh PrimitiveMeshSystem::create_cube_mesh(float size) {
    Mesh mesh;
    float h = size * 0.5f;
    struct Face { math::Vec3f n, v0, v1, v2, v3; };
    Face faces[6] = {
        {{1,0,0}, {h,-h,-h},{h,h,-h},{h,h,h},{h,-h,h}},
        {{-1,0,0},{-h,-h,h},{-h,h,h},{-h,h,-h},{-h,-h,-h}},
        {{0,1,0},{-h,h,-h},{-h,h,h},{h,h,h},{h,h,-h}},
        {{0,-1,0},{-h,-h,h},{-h,-h,-h},{h,-h,-h},{h,-h,h}},
        {{0,0,1},{-h,-h,h},{h,-h,h},{h,h,h},{-h,h,h}},
        {{0,0,-1},{-h,-h,-h},{-h,h,-h},{h,h,-h},{h,-h,-h}},
    };
    for (auto& f : faces) {
        uint32_t start = mesh.vertices.size();
        mesh.vertices.push_back({f.v0, f.n});
        mesh.vertices.push_back({f.v1, f.n});
        mesh.vertices.push_back({f.v2, f.n});
        mesh.vertices.push_back({f.v3, f.n});
        mesh.indices.insert(mesh.indices.end(), {start+0,start+1,start+2,start+0,start+2,start+3});
    }
    return mesh;
}

// ════════════════════════════════════════════════════════════════════
// CubeMapSystem
// ════════════════════════════════════════════════════════════════════

void CubeMapSystem::update_impl(exd::ecs::Registry& registry) {
    for (auto e : registry.view<CubeMapComponent>()) {
        auto& cm = registry.get<CubeMapComponent>(e);
        // Cubemap texture loading is deferred to ITextureSource
        // For now, create the mesh
        Mesh mesh = create_cubemap_mesh();
        uint32_t mesh_handle = ctx_.mesh_manager.create(mesh);
        if (!registry.has<RenderableComponent>(e))
            registry.emplace<RenderableComponent>(e, mesh_handle);
    }
}

Mesh CubeMapSystem::create_cubemap_mesh() {
    Mesh mesh;
    float v[] = {
        -1,1,-1, -1,-1,-1, 1,-1,-1, 1,-1,-1, 1,1,-1, -1,1,-1,
        -1,-1,1, -1,-1,-1, -1,1,-1, -1,1,-1, -1,1,1, -1,-1,1,
        1,-1,-1, 1,-1,1, 1,1,1, 1,1,1, 1,1,-1, 1,-1,-1,
        -1,-1,1, -1,1,1, 1,1,1, 1,1,1, 1,-1,1, -1,-1,1,
        -1,1,-1, 1,1,-1, 1,1,1, 1,1,1, -1,1,1, -1,1,-1,
        -1,-1,-1, -1,-1,1, 1,-1,-1, 1,-1,-1, -1,-1,1, 1,-1,1,
    };
    for (size_t i = 0; i < 108; i += 3)
        mesh.vertices.push_back({{v[i], v[i+1], v[i+2]}});
    return mesh;
}

// ════════════════════════════════════════════════════════════════════
// MeshAssetSystem
// ════════════════════════════════════════════════════════════════════


void MeshAssetSystem::update_impl(exd::ecs::Registry& registry) {
    for (auto e : registry.view<MeshAssetComponent>()) {
        auto& ma = registry.get<MeshAssetComponent>(e);
        if (ma.path.empty()) continue;

        // Assimp not available for first build — skipping mesh import
    }
}

// ════════════════════════════════════════════════════════════════════
// GridSystem
// ════════════════════════════════════════════════════════════════════

void GridSystem::update(exd::ecs::Registry& registry, double) {
    for (auto e : registry.view<GridComponent, Transform>()) {
        if (registry.has<Disabled>(e)) continue;
        auto& grid = registry.get<GridComponent>(e);
        if (window_->grid_visible && !registry.has<RenderableComponent>(e)) {
            Mesh mesh;
            mesh.topology = Topology::Lines;
            float s = grid.spacing > 0 ? grid.spacing : 1.0f;
            int N = 10; float extent = N * s;
            for (int i = -N; i <= N; ++i) {
                float c = i * s;
                math::Vec3f col{grid.color.w, grid.color.x, grid.color.y};
                Vertex v1; v1.position = {-extent, 0.0f, c}; v1.normal = col; mesh.vertices.push_back(v1);
                Vertex v2; v2.position = {+extent, 0.0f, c}; v2.normal = col; mesh.vertices.push_back(v2);
                Vertex v3; v3.position = {c, 0.0f, -extent}; v3.normal = col; mesh.vertices.push_back(v3);
                Vertex v4; v4.position = {c, 0.0f, +extent}; v4.normal = col; mesh.vertices.push_back(v4);
            }
            uint32_t handle = ctx_.mesh_manager.create(mesh);
            registry.emplace<RenderableComponent>(e, handle);
        } else if (!window_->grid_visible && registry.has<RenderableComponent>(e)) {
            registry.remove<RenderableComponent>(e);
        }
    }
}

// ════════════════════════════════════════════════════════════════════
// PolygonModeSystem
// ════════════════════════════════════════════════════════════════════

void PolygonModeSystem::update(exd::ecs::Registry&, double) {
    if (window_->event_state.was_key_released(SDL_SCANCODE_X)) {
        GL_CALL(glPolygonMode(GL_FRONT_AND_BACK,
                window_->wireframe ? GL_FILL : GL_LINE));
        if (window_->wireframe) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
        window_->wireframe = !window_->wireframe;
    }
}

} // namespace exd::render
