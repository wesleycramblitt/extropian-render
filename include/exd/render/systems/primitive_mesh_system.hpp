#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/app/window_state.hpp>
#include <exd/render/graphics/graphics_context.hpp>
#include <exd/render/graphics/mesh.hpp>

namespace exd::render {

/// Generates GPU meshes for primitive shape components.
/// Supports CubePrimitive, SpherePrimitive, CylinderPrimitive, ConePrimitive.
/// Delegates mesh generation to extropian-geometry.
class PrimitiveMeshSystem {
public:
    PrimitiveMeshSystem(GraphicsContext& ctx, app::WindowState*) : ctx_(ctx) {}
    void update(exd::ecs::Registry& registry, double) { update_primitives(registry); }
    void update_primitives(exd::ecs::Registry& registry);

    Mesh create_cube_mesh(float size);
    Mesh create_sphere_mesh(float radius, int segments);
    Mesh create_cylinder_mesh(float radius, float height, int segments);
    Mesh create_cone_mesh(float radius, float height, int segments);

private:
    GraphicsContext& ctx_;
};

} // namespace exd::render
