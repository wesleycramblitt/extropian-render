#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/app/window_state.hpp>

namespace exd::render {

/// FPS camera controller.
class CameraSystem {
public:
    explicit CameraSystem(app::WindowState* win) : window_(win) {}
    void update(exd::ecs::Registry& registry, double dt);
private:
    app::WindowState* window_;
};

} // namespace exd::render
