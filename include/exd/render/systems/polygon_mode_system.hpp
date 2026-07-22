#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/app/window_state.hpp>

namespace exd::render {

/// Toggles wireframe/fill polygon mode.
class PolygonModeSystem {
public:
    explicit PolygonModeSystem(app::WindowState* win) : window_(win) {}
    void update(exd::ecs::Registry&, double);
private:
    app::WindowState* window_;
};

} // namespace exd::render
