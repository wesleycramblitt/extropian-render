#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/render/graphics/graphics_context.hpp>
#include <exd/core/window_state.hpp>

namespace exd::render {

/// Loads equirectangular .hdr files as 2D textures and generates
/// the sky-dome sphere mesh the first time one is needed.
class EquirectSkySystem {
public:
    EquirectSkySystem(GraphicsContext& ctx, core::WindowState* win)
        : ctx_(ctx), window_(win) {}

    void update(exd::ecs::Registry& registry, double dt);

private:
    GraphicsContext& ctx_;
    core::WindowState* window_;
    uint32_t sphere_mesh_ = 0;  // shared sphere mesh (generated once)
};

} // namespace exd::render
