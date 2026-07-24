#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/render/graphics/graphics_context.hpp>
#include <exd/render/graphics/mesh.hpp>
#include <exd/render/interaction/ray.hpp>
#include <exd/math/mat4.hpp>
#include <exd/math/vec3.hpp>
#include <glad/gl.h>
#include <cstdint>
#include <optional>

namespace exd::render::interaction {

enum class GizmoMode { Translate, Rotate, Scale };
enum class GizmoAxis { None, X, Y, Z, XY, XZ, YZ, Center };

} // namespace exd::render::interaction

namespace exd::render {

using interaction::Ray;
using interaction::GizmoMode;
using interaction::GizmoAxis;

class GizmoSystem {
public:
    GizmoSystem(GraphicsContext& ctx);

    void render(ecs::Registry& registry,
                const math::Mat4& view, const math::Mat4& proj,
                const math::Vec3f& cam_pos);

    /// Call every frame (before render) with current mouse position
    /// so the gizmo can highlight the axis under the cursor.
    void update_hover(ecs::Registry& registry,
                      const math::Vec3f& cam_pos, const math::Vec3f& cam_forward,
                      const math::Vec3f& cam_up, float fov_y_rad, float aspect,
                      float screen_x, float screen_y, float screen_w, float screen_h);

    bool on_mouse_press(ecs::Registry& registry,
                        const math::Vec3f& cam_pos, const math::Vec3f& cam_forward,
                        const math::Vec3f& cam_up, float fov_y_rad, float aspect,
                        float screen_x, float screen_y, float screen_w, float screen_h);

    void on_mouse_drag(ecs::Registry& registry,
                       const math::Vec3f& cam_pos, const math::Vec3f& cam_forward,
                       const math::Vec3f& cam_up, float fov_y_rad, float aspect,
                       float screen_x, float screen_y, float screen_w, float screen_h);

    void on_mouse_release();

    void set_mode(GizmoMode m) { mode_ = m; on_mouse_release(); }
    GizmoMode mode() const { return mode_; }
    bool is_dragging() const { return active_axis_ != GizmoAxis::None; }

private:
    void ensure_geometry();
    uint32_t upload_mesh(const Mesh& m);

    GizmoAxis hit_test(const Ray& ray);
    /// Transform mouse to local ray and hit-test. Stores result in hovered_axis_.
    GizmoAxis hit_test_screen(ecs::Registry& registry,
                              const math::Vec3f& cam_pos, const math::Vec3f& cam_forward,
                              const math::Vec3f& cam_up, float fov_y_rad, float aspect,
                              float screen_x, float screen_y, float screen_w, float screen_h);
    std::optional<float> hit_arrow(const Ray& ray, float length, float head_len, float head_r);
    std::optional<float> hit_ring(const Ray& ray, float radius);
    std::optional<float> hit_box(const Ray& ray, float size);

    math::Vec3f get_gizmo_position(ecs::Registry& registry) const;
    float get_gizmo_scale(const math::Vec3f& gizmo_pos, const math::Vec3f& cam_pos,
                          const math::Mat4& proj) const;
    math::Mat4 get_axis_rotation(GizmoAxis axis) const;
    math::Mat4 gizmo_model(const math::Vec3f& pos, float scale) const;
    math::Vec3f axis_direction(GizmoAxis axis) const;
    math::Vec3f drag_delta(const math::Vec3f& cam_pos, const math::Vec3f& cam_forward,
                           const math::Vec3f& cam_up, float fov_y_rad, float aspect,
                           float screen_x, float screen_y, float screen_w, float screen_h);

    void draw_handle(uint32_t mesh_id, const math::Mat4& model, const math::Vec3f& color,
                     GizmoAxis axis, GizmoMode for_mode);
    void draw_translate_gizmo(const math::Mat4& view, const math::Mat4& proj,
                              const math::Vec3f& pos, float scale);
    void draw_rotate_gizmo(const math::Mat4& view, const math::Mat4& proj,
                           const math::Vec3f& pos, float scale);
    void draw_scale_gizmo(const math::Mat4& view, const math::Mat4& proj,
                          const math::Vec3f& pos, float scale);

    GraphicsContext& ctx_;
    GizmoMode mode_ = GizmoMode::Translate;
    GizmoAxis active_axis_ = GizmoAxis::None;
    GizmoAxis hovered_axis_ = GizmoAxis::None;

    math::Vec3f drag_origin_{};
    math::Vec3f drag_start_pos_{};
    math::Vec3f drag_accum_{};   // total drag delta since press (reset on release)
    float last_mouse_x_ = 0.0f;
    float last_mouse_y_ = 0.0f;

    uint32_t gizmo_program_ = 0;

    uint32_t arrow_x_ = 0, arrow_y_ = 0, arrow_z_ = 0;
    uint32_t ring_x_ = 0, ring_y_ = 0, ring_z_ = 0;
    uint32_t box_handle_ = 0;
    uint32_t plane_xy_ = 0, plane_xz_ = 0, plane_yz_ = 0;
    uint32_t scale_line_ = 0;
    bool geometry_ready_ = false;

    static constexpr math::Vec3f X_COLOR{1.0f, 0.2f, 0.2f};
    static constexpr math::Vec3f Y_COLOR{0.2f, 1.0f, 0.2f};
    static constexpr math::Vec3f Z_COLOR{0.2f, 0.3f, 1.0f};
    static constexpr math::Vec3f CENTER_COLOR{0.9f, 0.9f, 0.9f};
    static constexpr math::Vec3f ACTIVE_COLOR{1.0f, 1.0f, 0.2f};   // yellow when dragging
    static constexpr math::Vec3f HOVER_COLOR{1.0f, 1.0f, 0.6f};    // pale yellow on hover
};

} // namespace exd::render
