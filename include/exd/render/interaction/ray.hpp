#pragma once

/// Thin compatibility wrapper — delegates to exd::math::raycast.hpp in core.
/// Keeps only render-specific screen_ray() which depends on camera concepts.

#include <exd/math/raycast.hpp>

namespace exd::render::interaction {

// Re-export core types
using exd::math::Ray;
using exd::math::ray_triangle;
using exd::math::ray_plane;
using exd::math::ray_sphere;
using exd::math::ray_aabb;
using exd::math::closest_point_on_ray;

// ── Screen-to-world ray (render-specific — depends on camera) ──

/// Build a world-space ray from a screen-space mouse position.
/// Camera must be at `eye`, looking along `forward`, with `up` as the Y axis.
/// `fov_y_rad` is the vertical field of view in radians.
/// `aspect` is width / height.
/// `screen_x, screen_y` are pixel coords (0,0 = top-left).
/// `screen_w, screen_h` are the viewport dimensions.
[[nodiscard]] inline Ray screen_ray(
    const math::Vec3f& eye,
    const math::Vec3f& forward,
    const math::Vec3f& up,
    float fov_y_rad,
    float aspect,
    float screen_x, float screen_y,
    float screen_w, float screen_h)
{
    // Convert to NDC (-1 to 1, Y flipped)
    float ndc_x = (2.0f * screen_x / screen_w) - 1.0f;
    float ndc_y = 1.0f - (2.0f * screen_y / screen_h);

    float half_h = std::tan(fov_y_rad * 0.5f);
    float half_w = half_h * aspect;

    math::Vec3f right = forward.cross(up).normalized();
    math::Vec3f cam_up = right.cross(forward).normalized();

    math::Vec3f dir{
        forward.x + right.x * ndc_x * half_w + cam_up.x * ndc_y * half_h,
        forward.y + right.y * ndc_x * half_w + cam_up.y * ndc_y * half_h,
        forward.z + right.z * ndc_x * half_w + cam_up.z * ndc_y * half_h
    };

    return {eye, dir.normalized()};
}

} // namespace exd::render::interaction
