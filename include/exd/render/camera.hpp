#pragma once

#include <exd/math/vec3.hpp>
#include <exd/math/mat4.hpp>

namespace exd::render {

struct Camera {
    math::Vec3 position{0, 0, 0};
    math::Vec3 forward{0, 0, -1};
    math::Vec3 up{0, 1, 0};
    float fov_y_radians = 1.047f;
    float near_plane = 0.1f;
    float far_plane = 1000.0f;

    math::Mat4 view_matrix() const;
    math::Mat4 projection_matrix(float aspect) const;
};

} // namespace exd::render
