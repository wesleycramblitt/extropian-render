#pragma once

#include <exd/ecs/registry.hpp>
#include <exd/core/window_state.hpp>

namespace exd::render {

/// FPS camera controller.
class CameraSystem {
public:
    explicit CameraSystem(core::WindowState* win) : window_(win) {}
    void update(exd::ecs::Registry& registry, double dt);
private:
    core::WindowState* window_;
};

} // namespace exd::render
