#pragma once

#include <exd/math/vec3.hpp>
#include <optional>
#include <cfloat>

namespace exd::render::interaction {

// ── Ray ──────────────────────────────────────────

struct Ray {
    math::Vec3f origin;
    math::Vec3f direction;  // must be normalized

    /// Point at distance t along the ray.
    [[nodiscard]] math::Vec3f point_at(float t) const {
        return math::Vec3f{origin.x + direction.x * t,
                           origin.y + direction.y * t,
                           origin.z + direction.z * t};
    }
};

// ── Screen-to-world ray ──────────────────────────

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

// ── Intersection tests ───────────────────────────

/// Möller–Trumbore ray-triangle intersection.
/// Returns t (distance along ray) if hit, nullopt otherwise.
[[nodiscard]] inline std::optional<float> ray_triangle(
    const Ray& ray,
    const math::Vec3f& v0, const math::Vec3f& v1, const math::Vec3f& v2)
{
    const float eps = 1e-7f;
    math::Vec3f e1{v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    math::Vec3f e2{v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};

    math::Vec3f h = ray.direction.cross(e2);
    float a = e1.dot(h);
    if (a > -eps && a < eps) return std::nullopt;  // parallel

    float f = 1.0f / a;
    math::Vec3f s{ray.origin.x - v0.x, ray.origin.y - v0.y, ray.origin.z - v0.z};
    float u = f * s.dot(h);
    if (u < 0.0f || u > 1.0f) return std::nullopt;

    math::Vec3f q = s.cross(e1);
    float v = f * ray.direction.dot(q);
    if (v < 0.0f || u + v > 1.0f) return std::nullopt;

    float t = f * e2.dot(q);
    if (t > eps) return t;
    return std::nullopt;
}

/// Ray-plane intersection.
/// Plane defined by a point and normal (normal must be unit length).
/// Returns t, or nullopt if ray is parallel to plane.
[[nodiscard]] inline std::optional<float> ray_plane(
    const Ray& ray,
    const math::Vec3f& plane_point,
    const math::Vec3f& plane_normal)
{
    float denom = ray.direction.dot(plane_normal);
    if (std::fabs(denom) < 1e-7f) return std::nullopt;

    math::Vec3f diff{ray.origin.x - plane_point.x,
                     ray.origin.y - plane_point.y,
                     ray.origin.z - plane_point.z};
    float t = -diff.dot(plane_normal) / denom;
    if (t >= 0.0f) return t;
    return std::nullopt;
}

/// Ray-sphere intersection. Returns the nearest positive t, or nullopt.
[[nodiscard]] inline std::optional<float> ray_sphere(
    const Ray& ray,
    const math::Vec3f& center,
    float radius)
{
    math::Vec3f oc{ray.origin.x - center.x,
                   ray.origin.y - center.y,
                   ray.origin.z - center.z};
    float b = oc.dot(ray.direction);
    float c = oc.dot(oc) - radius * radius;
    float disc = b * b - c;
    if (disc < 0.0f) return std::nullopt;

    float sqrt_disc = std::sqrt(disc);
    float t0 = -b - sqrt_disc;
    float t1 = -b + sqrt_disc;

    if (t0 >= 0.0f) return t0;
    if (t1 >= 0.0f) return t1;
    return std::nullopt;
}

/// Ray-AABB intersection (slab method). Returns nearest positive t, or nullopt.
[[nodiscard]] inline std::optional<float> ray_aabb(
    const Ray& ray,
    const math::Vec3f& bmin,
    const math::Vec3f& bmax)
{
    float tmin = -FLT_MAX, tmax = FLT_MAX;
    float ox = ray.origin.x, oy = ray.origin.y, oz = ray.origin.z;
    float dx = ray.direction.x, dy = ray.direction.y, dz = ray.direction.z;

    // X slab
    if (std::fabs(dx) > 1e-7f) {
        float t1 = (bmin.x - ox) / dx;
        float t2 = (bmax.x - ox) / dx;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
    } else if (ox < bmin.x || ox > bmax.x) {
        return std::nullopt;
    }

    // Y slab
    if (std::fabs(dy) > 1e-7f) {
        float t1 = (bmin.y - oy) / dy;
        float t2 = (bmax.y - oy) / dy;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
    } else if (oy < bmin.y || oy > bmax.y) {
        return std::nullopt;
    }

    // Z slab
    if (std::fabs(dz) > 1e-7f) {
        float t1 = (bmin.z - oz) / dz;
        float t2 = (bmax.z - oz) / dz;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
    } else if (oz < bmin.z || oz > bmax.z) {
        return std::nullopt;
    }

    if (tmin <= tmax && tmax >= 0.0f) return tmin >= 0.0f ? tmin : tmax;
    return std::nullopt;
}

/// Closest point on ray to a given point (for axis snapping).
[[nodiscard]] inline math::Vec3f closest_point_on_ray(
    const math::Vec3f& ray_origin,
    const math::Vec3f& ray_dir,
    const math::Vec3f& point)
{
    float t = (point - ray_origin).dot(ray_dir);
    return ray_origin + ray_dir * t;
}

} // namespace exd::render::interaction
