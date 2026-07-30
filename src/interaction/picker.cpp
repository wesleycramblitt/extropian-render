#include <exd/render/interaction/picker.hpp>
#include <exd/render/components/transform.hpp>
#include <exd/render/components/renderable.hpp>
#include <exd/render/components/disabled.hpp>
#include <exd/render/graphics/mesh.hpp>
#include <exd/ecs/view.hpp>
#include <limits>
#include <cmath>

namespace exd::render {

std::optional<ecs::Entity> PickerSystem::pick(
    ecs::Registry& registry,
    const math::Vec3f& camera_pos,
    const math::Vec3f& camera_forward,
    const math::Vec3f& camera_up,
    float fov_y_rad,
    float aspect,
    float screen_x, float screen_y,
    float screen_w, float screen_h) const
{
    interaction::Ray ray = interaction::screen_ray(
        camera_pos, camera_forward, camera_up,
        fov_y_rad, aspect, screen_x, screen_y, screen_w, screen_h);

    float best_dist = std::numeric_limits<float>::max();
    std::optional<ecs::Entity> best_entity;

    auto view = registry.view<Transform, RenderableComponent>();
    for (auto e : view) {
        if (registry.has<Disabled>(e)) continue;

        auto& xform = registry.get<Transform>(e);
        auto& rc = registry.get<RenderableComponent>(e);
        if (rc.mesh == 0) continue;

        const Mesh* mesh = mesh_mgr_.get_mesh(rc.mesh);
        if (!mesh || mesh->vertices.empty()) continue;

        // Transform ray into model/local space for intersection testing.
        float ix = 1.0f / xform.scale.x;
        float iy = 1.0f / xform.scale.y;
        float iz = 1.0f / xform.scale.z;

        math::Quat inv_rot{xform.rotation.w, -xform.rotation.x,
                           -xform.rotation.y, -xform.rotation.z};
        math::Vec3f local_origin = (inv_rot * (ray.origin - xform.position));
        local_origin = {local_origin.x * ix, local_origin.y * iy, local_origin.z * iz};
        math::Vec3f local_dir = (inv_rot * ray.direction);
        local_dir = {local_dir.x * ix, local_dir.y * iy, local_dir.z * iz};
        float len = local_dir.length();
        if (len < 1e-7f) continue;
        local_dir = {local_dir.x / len, local_dir.y / len, local_dir.z / len};

        interaction::Ray local_ray{local_origin, local_dir};

        // Compute AABB in local space (cached per mesh would be better)
        math::Vec3f bmin{FLT_MAX, FLT_MAX, FLT_MAX};
        math::Vec3f bmax{-FLT_MAX, -FLT_MAX, -FLT_MAX};
        for (auto& v : mesh->vertices) {
            bmin.x = std::min(bmin.x, v.position.x);
            bmin.y = std::min(bmin.y, v.position.y);
            bmin.z = std::min(bmin.z, v.position.z);
            bmax.x = std::max(bmax.x, v.position.x);
            bmax.y = std::max(bmax.y, v.position.y);
            bmax.z = std::max(bmax.z, v.position.z);
        }

        auto aabb_t = interaction::ray_aabb(local_ray, bmin, bmax);
        if (!aabb_t) continue;

        // Triangle-precise test
        float local_best_t = FLT_MAX;
        for (size_t i = 0; i + 2 < mesh->indices.size(); i += 3) {
            const auto& v0 = mesh->vertices[mesh->indices[i]].position;
            const auto& v1 = mesh->vertices[mesh->indices[i + 1]].position;
            const auto& v2 = mesh->vertices[mesh->indices[i + 2]].position;

            auto tri_t = interaction::ray_triangle(local_ray, v0, v1, v2);
            if (tri_t && *tri_t < local_best_t) {
                local_best_t = *tri_t;
            }
        }

        if (local_best_t >= FLT_MAX) continue;

        // Transform hit point back to world space and compare world distance.
        math::Vec3f local_hit = local_ray.point_at(local_best_t);
        math::Vec3f world_hit{
            local_hit.x * xform.scale.x,
            local_hit.y * xform.scale.y,
            local_hit.z * xform.scale.z
        };
        world_hit = xform.rotation * world_hit + xform.position;
        float world_dist = camera_pos.distance(world_hit);

        if (world_dist < best_dist) {
            best_dist = world_dist;
            best_entity = e;
        }
    }

    return best_entity;
}

} // namespace exd::render
