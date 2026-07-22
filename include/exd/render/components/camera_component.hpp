#pragma once

namespace exd::render {

struct CameraComponent {
    float fov_y_radians = 1.047f;  // 60 degrees
    float near_plane = 0.1f;
    float far_plane = 1000.0f;
    float exposure = 1.0f;
};

} // namespace exd::render
