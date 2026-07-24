#include <exd/render/interaction/gizmo.hpp>
#include <exd/render/components/transform.hpp>
#include <exd/render/components/selected.hpp>
#include <exd/core/macros.hpp>

#include <cmath>
#include <algorithm>

namespace exd::render {

static const math::Vec3f X_AXIS{1,0,0}, Y_AXIS{0,1,0}, Z_AXIS{0,0,1};
static const float ARROW_LENGTH = 1.0f;
static const float ARROW_HEAD_LEN = 0.25f;
static const float ARROW_HEAD_R   = 0.10f;
static const float BOX_SIZE        = 0.12f;
static const float RING_RADIUS     = 0.85f;
static const float PLANE_SIZE      = 0.20f;

GizmoSystem::GizmoSystem(GraphicsContext& ctx) : ctx_(ctx) {}

// ── Inline mesh generators (no external geometry dep) ──

static Mesh make_arrow_mesh() {
    Mesh m;
    float body_top = ARROW_LENGTH - ARROW_HEAD_LEN;
    float body_r = 0.04f, head_r = ARROW_HEAD_R;
    int segs = 16;
    float step = 2.0f * 3.14159265f / segs;

    // Cylinder body
    for (int i = 0; i <= segs; ++i) {
        float a = i * step;
        float x = std::cos(a) * body_r, z = std::sin(a) * body_r;
        m.vertices.push_back({{x, 0, z}});
        m.vertices.push_back({{x, body_top, z}});
    }
    for (int i = 0; i < segs; ++i) {
        uint32_t b0 = i*2, b1 = i*2+1, t0 = (i+1)*2, t1 = (i+1)*2+1;
        m.indices.insert(m.indices.end(), {b0,b1,t0, t0,b1,t1});
    }
    // Cone tip
    uint32_t tip = m.vertices.size();
    m.vertices.push_back({{0, ARROW_LENGTH, 0}});
    uint32_t base_start = m.vertices.size();
    for (int i = 0; i <= segs; ++i) {
        float a = i * step;
        m.vertices.push_back({{std::cos(a)*head_r, body_top, std::sin(a)*head_r}});
    }
    for (int i = 0; i < segs; ++i)
        m.indices.insert(m.indices.end(), {base_start+(uint32_t)i, base_start+(uint32_t)i+1, tip});
    // Cone base cap
    uint32_t cap_center = m.vertices.size();
    m.vertices.push_back({{0, body_top, 0}});
    uint32_t cap_start = m.vertices.size();
    for (int i = 0; i <= segs; ++i) {
        float a = i * step;
        m.vertices.push_back({{std::cos(a)*head_r, body_top, std::sin(a)*head_r}});
    }
    for (int i = 0; i < segs; ++i)
        m.indices.insert(m.indices.end(), {cap_center, cap_start+(uint32_t)i, cap_start+(uint32_t)i+1});
    return m;
}

static Mesh make_ring_mesh() {
    Mesh m;
    int segs = 48, tube_segs = 8;
    float r = RING_RADIUS, tr = 0.03f;
    float a_step = 2.0f*3.14159265f/segs, t_step = 2.0f*3.14159265f/tube_segs;
    for (int i = 0; i <= segs; ++i) {
        float a = i*a_step;
        float cx = std::cos(a)*r, cy = std::sin(a)*r;
        for (int j = 0; j < tube_segs; ++j) {
            float t = j*t_step;
            float tx = std::cos(a)*std::cos(t)*tr;
            float ty = std::sin(a)*std::cos(t)*tr;
            float tz = std::sin(t)*tr;
            m.vertices.push_back({{cx+tx, cy+ty, tz}});
        }
    }
    for (int i = 0; i < segs; ++i) {
        for (int j = 0; j < tube_segs; ++j) {
            uint32_t a = i*tube_segs+j, b = i*tube_segs+(j+1)%tube_segs;
            uint32_t c = (i+1)*tube_segs+j, d = (i+1)*tube_segs+(j+1)%tube_segs;
            m.indices.insert(m.indices.end(), {a,c,b, b,c,d});
        }
    }
    return m;
}

static Mesh make_box_mesh() {
    Mesh m;
    float h = BOX_SIZE * 0.5f;
    math::Vec3f c[8] = {{-h,-h,-h},{h,-h,-h},{h,h,-h},{-h,h,-h},
                         {-h,-h,h},{h,-h,h},{h,h,h},{-h,h,h}};
    uint32_t f[6][4] = {{0,1,2,3},{4,5,6,7},{0,1,5,4},{2,3,7,6},{0,3,7,4},{1,2,6,5}};
    math::Vec3f n[6] = {{0,0,-1},{0,0,1},{0,-1,0},{0,1,0},{-1,0,0},{1,0,0}};
    for (int i = 0; i < 6; ++i) {
        uint32_t s = m.vertices.size();
        for (int v = 0; v < 4; ++v) m.vertices.push_back({c[f[i][v]], n[i]});
        m.indices.insert(m.indices.end(), {s,s+1,s+2, s,s+2,s+3});
    }
    return m;
}

static Mesh make_plane_mesh() {
    Mesh m;
    float h = PLANE_SIZE * 0.5f;
    uint32_t s = m.vertices.size();
    m.vertices.push_back({{-h,-h,0},{0,0,1}});
    m.vertices.push_back({{ h,-h,0},{0,0,1}});
    m.vertices.push_back({{ h, h,0},{0,0,1}});
    m.vertices.push_back({{-h, h,0},{0,0,1}});
    m.indices.insert(m.indices.end(), {s,s+1,s+2, s,s+2,s+3});
    return m;
}

// ── Geometry upload ───────────────────────────────

void GizmoSystem::ensure_geometry() {
    if (geometry_ready_) return;
    geometry_ready_ = true;

    Mesh arrow  = make_arrow_mesh();
    arrow_x_ = upload_mesh(arrow);
    arrow_y_ = arrow_x_;
    arrow_z_ = arrow_x_;

    Mesh ring   = make_ring_mesh();
    ring_x_ = upload_mesh(ring);
    ring_y_ = ring_x_;
    ring_z_ = ring_x_;

    box_handle_ = upload_mesh(make_box_mesh());
    plane_xy_   = upload_mesh(make_plane_mesh());
    plane_xz_ = plane_xy_;
    plane_yz_ = plane_xy_;
}

uint32_t GizmoSystem::upload_mesh(const Mesh& m) {
    return ctx_.mesh_manager.create(m);
}

// ── Gizmo position & scale ────────────────────────

math::Vec3f GizmoSystem::get_gizmo_position(ecs::Registry& registry) const {
    math::Vec3f centroid{0,0,0};
    int count = 0;
    for (auto e : registry.view<Selected, Transform>()) {
        centroid = centroid + registry.get<Transform>(e).position;
        count++;
    }
    if (count == 0) return {0,0,0};
    return {centroid.x / count, centroid.y / count, centroid.z / count};
}

float GizmoSystem::get_gizmo_scale(const math::Vec3f& gizmo_pos,
                                    const math::Vec3f& cam_pos,
                                    const math::Mat4&) const {
    float dist = gizmo_pos.distance(cam_pos);
    return std::max(0.5f, dist * 0.15f);
}

math::Mat4 GizmoSystem::gizmo_model(const math::Vec3f& pos, float scale) const {
    return math::Mat4::trs(pos, math::Quat{}, {scale, scale, scale});
}

math::Mat4 GizmoSystem::get_axis_rotation(GizmoAxis axis) const {
    switch (axis) {
        case GizmoAxis::X:
            return math::Mat4::trs({0,0,0},
                math::Quat::from_axis_angle(Z_AXIS, -3.14159265f*0.5f), {1,1,1});
        case GizmoAxis::Z:
            return math::Mat4::trs({0,0,0},
                math::Quat::from_axis_angle(X_AXIS, 3.14159265f*0.5f), {1,1,1});
        default: return math::Mat4::identity();
    }
}

math::Vec3f GizmoSystem::axis_direction(GizmoAxis axis) const {
    switch (axis) {
        case GizmoAxis::X:  return X_AXIS;
        case GizmoAxis::Y:  return Y_AXIS;
        case GizmoAxis::Z:  return Z_AXIS;
        case GizmoAxis::XY: return (X_AXIS + Y_AXIS).normalized();
        case GizmoAxis::XZ: return (X_AXIS + Z_AXIS).normalized();
        case GizmoAxis::YZ: return (Y_AXIS + Z_AXIS).normalized();
        default: return {0,0,0};
    }
}

// ── Hit testing ───────────────────────────────────

GizmoAxis GizmoSystem::hit_test(const Ray& ray) {
    if (mode_ == GizmoMode::Translate) {
        if (hit_arrow(ray, ARROW_LENGTH, ARROW_HEAD_LEN, ARROW_HEAD_R)) {
            float best = FLT_MAX;
            GizmoAxis best_axis = GizmoAxis::None;
            auto test_axis = [&](GizmoAxis a, math::Vec3f dir) {
                math::Vec3f w = ray.origin;
                float a_dir_dot = ray.direction.dot(dir);
                float t = -(w.dot(dir)) / std::max(std::fabs(a_dir_dot), 1e-7f);
                if (t > 0 && t < best) {
                    math::Vec3f pt = ray.point_at(t);
                    float axis_t = pt.dot(dir);
                    if (axis_t > 0 && axis_t < ARROW_LENGTH * 1.2f) {
                        float dist = (pt - dir * axis_t).length();
                        if (dist < 0.15f) { best = t; best_axis = a; }
                    }
                }
            };
            test_axis(GizmoAxis::X, X_AXIS);
            test_axis(GizmoAxis::Y, Y_AXIS);
            test_axis(GizmoAxis::Z, Z_AXIS);
            return best_axis;
        }
        if (hit_box(ray, PLANE_SIZE * 1.5f)) {
            math::Vec3f plane_centers[3] = {
                {PLANE_SIZE*0.5f, PLANE_SIZE*0.5f, 0},
                {PLANE_SIZE*0.5f, 0, PLANE_SIZE*0.5f},
                {0, PLANE_SIZE*0.5f, PLANE_SIZE*0.5f},
            };
            math::Vec3f plane_normals[3] = {Z_AXIS, Y_AXIS, X_AXIS};
            GizmoAxis plane_axes[3] = {GizmoAxis::XY, GizmoAxis::XZ, GizmoAxis::YZ};
            float best = FLT_MAX;
            GizmoAxis best_plane = GizmoAxis::None;
            for (int i = 0; i < 3; ++i) {
                auto t = interaction::ray_plane(ray, plane_centers[i], plane_normals[i]);
                if (t && *t < best && *t > 0) {
                    math::Vec3f hit = ray.point_at(*t);
                    float hs = PLANE_SIZE * 0.6f;
                    if (std::fabs(hit.x - plane_centers[i].x) < hs &&
                        std::fabs(hit.y - plane_centers[i].y) < hs &&
                        std::fabs(hit.z - plane_centers[i].z) < hs) {
                        best = *t; best_plane = plane_axes[i];
                    }
                }
            }
            if (best_plane != GizmoAxis::None) return best_plane;
        }
    }
    if (mode_ == GizmoMode::Rotate) {
        GizmoAxis ring_axes[3] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};
        for (int i = 0; i < 3; ++i)
            if (hit_ring(ray, RING_RADIUS)) return ring_axes[i];
    }
    if (mode_ == GizmoMode::Scale) {
        if (hit_box(ray, BOX_SIZE * 1.5f)) {
            float best = FLT_MAX;
            GizmoAxis best_axis = GizmoAxis::None;
            GizmoAxis axes[3] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};
            math::Vec3f dirs[3] = {X_AXIS, Y_AXIS, Z_AXIS};
            for (int i = 0; i < 3; ++i) {
                auto t = interaction::ray_sphere(ray, dirs[i] * 0.9f, 0.12f);
                if (t && *t < best) { best = *t; best_axis = axes[i]; }
            }
            if (best_axis == GizmoAxis::None) {
                auto t = interaction::ray_sphere(ray, {0,0,0}, 0.15f);
                if (t) return GizmoAxis::Center;
            }
            return best_axis;
        }
    }
    if (hit_box(ray, BOX_SIZE * 1.3f)) {
        auto t = interaction::ray_sphere(ray, {0,0,0}, 0.12f);
        if (t) return GizmoAxis::Center;
    }
    return GizmoAxis::None;
}

std::optional<float> GizmoSystem::hit_arrow(const Ray& ray, float length,
                                               float head_len, float head_r) {
    for (float y = 0.05f; y <= length; y += 0.08f) {
        float r = (y > length - head_len) ? head_r * 0.8f : 0.12f;
        auto t = interaction::ray_sphere(ray, {0, y, 0}, r);
        if (t) return t;
    }
    return std::nullopt;
}

std::optional<float> GizmoSystem::hit_ring(const Ray& ray, float radius) {
    int samples = 32;
    for (int i = 0; i < samples; ++i) {
        float a = 2.0f * 3.14159265f * i / samples;
        math::Vec3f pt{std::cos(a) * radius, std::sin(a) * radius, 0};
        auto t = interaction::ray_sphere(ray, pt, 0.10f);
        if (t) return t;
    }
    return std::nullopt;
}

std::optional<float> GizmoSystem::hit_box(const Ray& ray, float size) {
    float h = size * 0.5f;
    return interaction::ray_aabb(ray, {-h,-h,-h}, {h,h,h});
}

// ── Mouse interaction ────────────────────────────

bool GizmoSystem::on_mouse_press(ecs::Registry& registry,
                                  const math::Vec3f& cam_pos,
                                  const math::Vec3f& cam_forward,
                                  const math::Vec3f& cam_up,
                                  float fov_y_rad, float aspect,
                                  float screen_x, float screen_y,
                                  float screen_w, float screen_h) {
    math::Vec3f gizmo_pos = get_gizmo_position(registry);
    if (gizmo_pos == math::Vec3f{0,0,0}) return false;

    float scale = get_gizmo_scale(gizmo_pos, cam_pos, math::Mat4{});

    interaction::Ray world_ray = interaction::screen_ray(
        cam_pos, cam_forward, cam_up, fov_y_rad, aspect,
        screen_x, screen_y, screen_w, screen_h);

    math::Vec3f local_origin{(world_ray.origin.x - gizmo_pos.x) / scale,
                             (world_ray.origin.y - gizmo_pos.y) / scale,
                             (world_ray.origin.z - gizmo_pos.z) / scale};
    math::Vec3f local_dir = world_ray.direction / scale;
    float len = local_dir.length();
    if (len < 1e-7f) return false;
    local_dir = local_dir / len;

    interaction::Ray local_ray{local_origin, local_dir.normalized()};

    active_axis_ = hit_test(local_ray);
    if (active_axis_ != GizmoAxis::None) {
        last_mouse_x_ = screen_x;
        last_mouse_y_ = screen_y;
        drag_origin_ = gizmo_pos;
        for (auto e : registry.view<Selected, Transform>()) {
            drag_start_pos_ = registry.get<Transform>(e).position;
            break;
        }
        return true;
    }
    return false;
}

void GizmoSystem::on_mouse_drag(ecs::Registry& registry,
                                 const math::Vec3f& cam_pos,
                                 const math::Vec3f& cam_forward,
                                 const math::Vec3f& cam_up,
                                 float fov_y_rad, float aspect,
                                 float screen_x, float screen_y,
                                 float screen_w, float screen_h) {
    if (active_axis_ == GizmoAxis::None) return;

    math::Vec3f delta = drag_delta(cam_pos, cam_forward, cam_up,
                                     fov_y_rad, aspect,
                                     screen_x, screen_y, screen_w, screen_h);

    for (auto e : registry.view<Selected, Transform>()) {
        auto& xform = registry.get<Transform>(e);
        if (mode_ == GizmoMode::Translate) {
            if (active_axis_ == GizmoAxis::Center)
                xform.position = drag_start_pos_ + delta;
            else {
                math::Vec3f axis = axis_direction(active_axis_);
                xform.position = drag_start_pos_ + axis * delta.dot(axis);
            }
        } else if (mode_ == GizmoMode::Scale) {
            math::Vec3f axis = active_axis_ == GizmoAxis::Center
                ? math::Vec3f{1,1,1} : axis_direction(active_axis_);
            float amt = delta.length() * 0.01f;
            float sign = (delta.dot(axis_direction(active_axis_)) >= 0) ? 1.0f : -1.0f;
            if (active_axis_ == GizmoAxis::Center)
                xform.scale = xform.scale + math::Vec3f{amt,amt,amt} * sign;
            else {
                auto s = xform.scale;
                if (active_axis_ == GizmoAxis::X) s.x += amt * sign;
                if (active_axis_ == GizmoAxis::Y) s.y += amt * sign;
                if (active_axis_ == GizmoAxis::Z) s.z += amt * sign;
                xform.scale = {std::max(0.01f,s.x), std::max(0.01f,s.y), std::max(0.01f,s.z)};
            }
        } else if (mode_ == GizmoMode::Rotate) {
            math::Vec3f axis = axis_direction(active_axis_);
            float angle = delta.length() * 0.005f;
            float sign = (delta.dot(axis.cross(cam_forward)) >= 0) ? 1.0f : -1.0f;
            xform.rotation = math::Quat::from_axis_angle(axis, angle*sign) * xform.rotation;
        }
        break;
    }
    last_mouse_x_ = screen_x;
    last_mouse_y_ = screen_y;
}

void GizmoSystem::on_mouse_release() {
    active_axis_ = GizmoAxis::None;
}

math::Vec3f GizmoSystem::drag_delta(const math::Vec3f& cam_pos,
                                      const math::Vec3f& cam_forward,
                                      const math::Vec3f& cam_up,
                                      float fov_y_rad, float aspect,
                                      float screen_x, float screen_y,
                                      float screen_w, float screen_h) {
    float dx = screen_x - last_mouse_x_;
    float dy = screen_y - last_mouse_y_;
    float dist = drag_origin_.distance(cam_pos);
    float half_h = std::tan(fov_y_rad * 0.5f);
    float world_per_pixel = 2.0f * half_h * dist / screen_h;
    math::Vec3f right = cam_forward.cross(cam_up).normalized();
    return right * dx * world_per_pixel + cam_up * (-dy) * world_per_pixel;
}

// ── Rendering ─────────────────────────────────────

void GizmoSystem::render(ecs::Registry& registry,
                          const math::Mat4& view, const math::Mat4& proj,
                          const math::Vec3f& cam_pos) {
    math::Vec3f pos = get_gizmo_position(registry);
    if (pos == math::Vec3f{0,0,0}) return;

    ensure_geometry();

    gizmo_program_ = ctx_.shader_manager.get_or_load(
        "gizmo", "shaders/opengl/gizmo/gizmo.vert", "shaders/opengl/gizmo/gizmo.frag");
    GL_CALL(glUseProgram(gizmo_program_));

    float scale = get_gizmo_scale(pos, cam_pos, proj);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    switch (mode_) {
        case GizmoMode::Translate: draw_translate_gizmo(view, proj, pos, scale); break;
        case GizmoMode::Rotate:    draw_rotate_gizmo(view, proj, pos, scale);    break;
        case GizmoMode::Scale:     draw_scale_gizmo(view, proj, pos, scale);     break;
    }
    GL_CALL(glUseProgram(0));
}

void GizmoSystem::draw_handle(uint32_t mesh_id, const math::Mat4& model,
                                const math::Vec3f& color, GizmoAxis axis,
                                GizmoMode for_mode) {
    if (mesh_id == 0) return;
    const math::Vec3f& c = (active_axis_ == axis && for_mode == mode_)
        ? ACTIVE_COLOR : color;
    GLint loc;
    loc = glGetUniformLocation(gizmo_program_, "u_model");
    GL_CALL(glUniformMatrix4fv(loc, 1, GL_FALSE, model.m));
    loc = glGetUniformLocation(gizmo_program_, "u_color");
    GL_CALL(glUniform4f(loc, c.x, c.y, c.z, 1.0f));

    const auto* mesh = ctx_.mesh_manager.bind(mesh_id);
    if (mesh->index_count > 0)
        GL_CALL(glDrawElements(mesh->topology, mesh->index_count, GL_UNSIGNED_INT, nullptr));
    else
        GL_CALL(glDrawArrays(mesh->topology, 0, mesh->vertex_count));
}

void GizmoSystem::draw_translate_gizmo(const math::Mat4& view, const math::Mat4& proj,
                                         const math::Vec3f& pos, float scale) {
    GLint vloc = glGetUniformLocation(gizmo_program_, "u_view");
    GL_CALL(glUniformMatrix4fv(vloc, 1, GL_FALSE, view.m));
    GLint ploc = glGetUniformLocation(gizmo_program_, "u_proj");
    GL_CALL(glUniformMatrix4fv(ploc, 1, GL_FALSE, proj.m));

    math::Mat4 base = gizmo_model(pos, scale);

    math::Mat4 xm = math::Mat4::mul(base, get_axis_rotation(GizmoAxis::X));
    draw_handle(arrow_x_, xm, X_COLOR, GizmoAxis::X, GizmoMode::Translate);
    draw_handle(arrow_y_, base, Y_COLOR, GizmoAxis::Y, GizmoMode::Translate);
    math::Mat4 zm = math::Mat4::mul(base, get_axis_rotation(GizmoAxis::Z));
    draw_handle(arrow_z_, zm, Z_COLOR, GizmoAxis::Z, GizmoMode::Translate);

    float ps = PLANE_SIZE * 0.5f;
    math::Mat4 p_xy = math::Mat4::mul(base, math::Mat4::trs({ps,ps,0}, math::Quat{}, {scale,scale,scale}));
    draw_handle(plane_xy_, p_xy, Z_COLOR*0.6f, GizmoAxis::XY, GizmoMode::Translate);
    math::Mat4 p_xz = math::Mat4::mul(base, math::Mat4::trs({ps,0,ps}, math::Quat::from_axis_angle(X_AXIS, 3.14159265f*0.5f), {scale,scale,scale}));
    draw_handle(plane_xz_, p_xz, Y_COLOR*0.6f, GizmoAxis::XZ, GizmoMode::Translate);
    math::Mat4 p_yz = math::Mat4::mul(base, math::Mat4::trs({0,ps,ps}, math::Quat::from_axis_angle(Z_AXIS, -3.14159265f*0.5f), {scale,scale,scale}));
    draw_handle(plane_yz_, p_yz, X_COLOR*0.6f, GizmoAxis::YZ, GizmoMode::Translate);

    draw_handle(box_handle_, base, CENTER_COLOR, GizmoAxis::Center, GizmoMode::Translate);
}

void GizmoSystem::draw_rotate_gizmo(const math::Mat4& view, const math::Mat4& proj,
                                      const math::Vec3f& pos, float scale) {
    GLint vloc = glGetUniformLocation(gizmo_program_, "u_view");
    GL_CALL(glUniformMatrix4fv(vloc, 1, GL_FALSE, view.m));
    GLint ploc = glGetUniformLocation(gizmo_program_, "u_proj");
    GL_CALL(glUniformMatrix4fv(ploc, 1, GL_FALSE, proj.m));

    math::Mat4 base = gizmo_model(pos, scale);

    math::Mat4 xr = math::Mat4::mul(base, get_axis_rotation(GizmoAxis::X));
    draw_handle(ring_x_, xr, X_COLOR, GizmoAxis::X, GizmoMode::Rotate);
    draw_handle(ring_y_, base, Y_COLOR, GizmoAxis::Y, GizmoMode::Rotate);
    math::Mat4 zr = math::Mat4::mul(base, get_axis_rotation(GizmoAxis::Z));
    draw_handle(ring_z_, zr, Z_COLOR, GizmoAxis::Z, GizmoMode::Rotate);

    draw_handle(box_handle_, base, CENTER_COLOR, GizmoAxis::Center, GizmoMode::Rotate);
}

void GizmoSystem::draw_scale_gizmo(const math::Mat4& view, const math::Mat4& proj,
                                     const math::Vec3f& pos, float scale) {
    GLint vloc = glGetUniformLocation(gizmo_program_, "u_view");
    GL_CALL(glUniformMatrix4fv(vloc, 1, GL_FALSE, view.m));
    GLint ploc = glGetUniformLocation(gizmo_program_, "u_proj");
    GL_CALL(glUniformMatrix4fv(ploc, 1, GL_FALSE, proj.m));

    math::Mat4 base = gizmo_model(pos, scale);

    math::Mat4 bx = math::Mat4::mul(base, math::Mat4::trs({0.9f*scale,0,0}, math::Quat{}, {1,1,1}));
    draw_handle(box_handle_, bx, X_COLOR, GizmoAxis::X, GizmoMode::Scale);
    math::Mat4 by = math::Mat4::mul(base, math::Mat4::trs({0,0.9f*scale,0}, math::Quat{}, {1,1,1}));
    draw_handle(box_handle_, by, Y_COLOR, GizmoAxis::Y, GizmoMode::Scale);
    math::Mat4 bz = math::Mat4::mul(base, math::Mat4::trs({0,0,0.9f*scale}, math::Quat{}, {1,1,1}));
    draw_handle(box_handle_, bz, Z_COLOR, GizmoAxis::Z, GizmoMode::Scale);

    draw_handle(box_handle_, base, CENTER_COLOR, GizmoAxis::Center, GizmoMode::Scale);
}

} // namespace exd::render
