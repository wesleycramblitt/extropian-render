#pragma once

namespace exd::render {

struct CameraController {
    float move_speed = 30.0f;
    float sprint_mult = 2.0f;
    float mouse_sensitivity = 0.002f;
    float yaw = 0.0f;
    float pitch = 0.0f;
};

} // namespace exd::render
