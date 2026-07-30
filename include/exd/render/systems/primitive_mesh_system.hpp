#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/core/window_state.hpp>
#include <exd/render/graphics/graphics_context.hpp>
#include <exd/render/graphics/mesh.hpp>

namespace exd::render {

/// Generates GPU meshes for CubePrimitive entities.
/// More complex shapes (sphere, cylinder, cone) are handled by
/// extropian-geometry in the consuming application.
class PrimitiveMeshSystem {
public:
    PrimitiveMeshSystem(GraphicsContext& ctx, core::WindowState*) : ctx_(ctx) {}
    void update(exd::ecs::Registry& registry, double) { update_primitives(registry); }
    void update_primitives(exd::ecs::Registry& registry);
    Mesh create_cube_mesh(float size);

private:
    GraphicsContext& ctx_;
};

} // namespace exd::render
