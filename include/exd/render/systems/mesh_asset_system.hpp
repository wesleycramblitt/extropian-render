#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/core/window_state.hpp>
#include <exd/render/graphics/graphics_context.hpp>

namespace exd::render {

/// Loads external mesh files.
class MeshAssetSystem {
public:
    MeshAssetSystem(GraphicsContext& ctx, core::WindowState* /*unused*/) : ctx_(ctx) {}
    void update(exd::ecs::Registry& registry, double) { update_impl(registry); }
    void update_impl(exd::ecs::Registry& registry);
private:
    GraphicsContext& ctx_;
};

} // namespace exd::render
