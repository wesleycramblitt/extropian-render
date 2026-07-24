#include <exd/render/systems/primitive_mesh_system.hpp>
#include <exd/render/components/cube.hpp>
#include <exd/render/components/sphere.hpp>
#include <exd/render/components/cylinder.hpp>
#include <exd/render/components/cone.hpp>
#include <exd/render/components/renderable.hpp>
#include <exd/render/graphics/mesh_convert.hpp>
#include <exd/geometry/primitives3d.hpp>
#include <cstdio>

namespace exd::render {

static uint32_t create_and_assign(ecs::Registry& registry, ecs::Entity e,
                                   GraphicsContext& ctx, const Mesh& mesh,
                                   const char* label) {
    uint32_t handle = ctx.mesh_manager.create(mesh);
    if (registry.has<RenderableComponent>(e))
        registry.get<RenderableComponent>(e).mesh = handle;
    else
        registry.emplace<RenderableComponent>(e, handle);
    std::printf("[PrimitiveMesh] %s entity %u\n", label, e.id);
    return handle;
}

void PrimitiveMeshSystem::update_primitives(exd::ecs::Registry& registry) {
    for (auto e : registry.view<CubePrimitive>()) {
        auto& c = registry.get<CubePrimitive>(e);
        create_and_assign(registry, e, ctx_, create_cube_mesh(c.size), "Cube");
    }
    for (auto e : registry.view<SpherePrimitive>()) {
        auto& s = registry.get<SpherePrimitive>(e);
        create_and_assign(registry, e, ctx_, create_sphere_mesh(s.radius, s.segments), "Sphere");
    }
    for (auto e : registry.view<CylinderPrimitive>()) {
        auto& c = registry.get<CylinderPrimitive>(e);
        create_and_assign(registry, e, ctx_, create_cylinder_mesh(c.radius, c.height, c.segments), "Cylinder");
    }
    for (auto e : registry.view<ConePrimitive>()) {
        auto& c = registry.get<ConePrimitive>(e);
        create_and_assign(registry, e, ctx_, create_cone_mesh(c.radius, c.height, c.segments), "Cone");
    }
}

Mesh PrimitiveMeshSystem::create_cube_mesh(float size) {
    return convert_geometry_mesh(exd::geometry::generate_box_mesh(
        exd::geometry::BoxGeometry{.size = {size, size, size}}));
}

Mesh PrimitiveMeshSystem::create_sphere_mesh(float radius, int segments) {
    return convert_geometry_mesh(exd::geometry::generate_sphere_mesh(
        exd::geometry::SphereGeometry{
            .radius = radius,
            .latitudeSegments = static_cast<uint32_t>(segments / 2),
            .longitudeSegments = static_cast<uint32_t>(segments)
        }));
}

Mesh PrimitiveMeshSystem::create_cylinder_mesh(float radius, float height, int segments) {
    return convert_geometry_mesh(exd::geometry::generate_cylinder_mesh(
        exd::geometry::CylinderGeometry{
            .radius = radius, .height = height,
            .slices = static_cast<uint32_t>(segments), .capped = true
        }));
}

Mesh PrimitiveMeshSystem::create_cone_mesh(float radius, float height, int segments) {
    return convert_geometry_mesh(exd::geometry::generate_cone_mesh(
        exd::geometry::ConeGeometry{
            .radius = radius, .height = height,
            .slices = static_cast<uint32_t>(segments), .capped = true
        }));
}

} // namespace exd::render
