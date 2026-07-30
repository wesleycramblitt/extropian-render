#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/core/window_state.hpp>

namespace exd::render {

/// Toggles wireframe/fill polygon mode.
class PolygonModeSystem {
public:
    explicit PolygonModeSystem(core::WindowState* win) : window_(win) {}
    void update(exd::ecs::Registry&, double);
private:
    core::WindowState* window_;
};

} // namespace exd::render
