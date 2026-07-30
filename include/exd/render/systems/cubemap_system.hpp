#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/core/window_state.hpp>
#include <exd/render/graphics/graphics_context.hpp>
#include <exd/render/graphics/mesh.hpp>

namespace exd::render {

/// Loads cubemap textures.
class CubeMapSystem {
public:
    CubeMapSystem(GraphicsContext& ctx, core::WindowState* /*unused*/) : ctx_(ctx) {}
    void update(exd::ecs::Registry& registry, double) { update_impl(registry); }
    void update_impl(exd::ecs::Registry& registry);
    Mesh create_cubemap_mesh();
private:
    GraphicsContext& ctx_;
};

} // namespace exd::render
