#pragma once
// Shared test utilities for extropian-render tests.

#include <exd/render/components/camera_component.hpp>
#include <exd/render/components/transform.hpp>
#include <exd/render/camera.hpp>
#include <exd/render/graphics/mesh.hpp>
#include <exd/render/graphics/graphics_context.hpp>
#include <exd/render/render_graph.hpp>
#include <exd/ecs/registry.hpp>
#include <exd/math/mat4.hpp>
#include <exd/math/vec3.hpp>
#include <cmath>
#include <cstdio>

namespace exd::render::test {

// ── Math helpers ────────────────────────────────────

inline bool near(float a, float b, float eps = 1e-5f) {
    return std::fabs(a - b) < eps;
}

inline bool near(const math::Vec3f& a, const math::Vec3f& b, float eps = 1e-5f) {
    return near(a.x, b.x, eps) && near(a.y, b.y, eps) && near(a.z, b.z, eps);
}

// Compare Mat4 by comparing m[16] arrays
inline bool near_mat4(const math::Mat4& a, const math::Mat4& b, float eps = 1e-5f) {
    for (int i = 0; i < 16; ++i)
        if (!near(a.m[i], b.m[i], eps)) return false;
    return true;
}

// ── ECS helpers ─────────────────────────────────────

inline ecs::Entity create_camera_entity(ecs::Registry& reg, const math::Vec3f& pos = {}) {
    auto e = reg.create("TestCamera");
    reg.emplace<Transform>(e, pos, math::Quat{}, math::Vec3f{1,1,1});
    reg.emplace<CameraComponent>(e);
    return e;
}

// ── Mesh helpers ────────────────────────────────────

inline bool mesh_is_valid(const Mesh& m) {
    return !m.vertices.empty() && !m.indices.empty()
        && m.topology == Topology::Triangles;
}

} // namespace exd::render::test
