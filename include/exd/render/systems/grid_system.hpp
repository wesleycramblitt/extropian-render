#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/app/window_state.hpp>
#include <exd/render/graphics/graphics_context.hpp>

namespace exd::render {

/// Renders a grid on the XZ plane.
class GridSystem {
public:
    GridSystem(GraphicsContext& ctx, app::WindowState* win) : ctx_(ctx), window_(win) {}
    void update(exd::ecs::Registry& registry, double);
private:
    GraphicsContext& ctx_;
    app::WindowState* window_;
};

} // namespace exd::render
