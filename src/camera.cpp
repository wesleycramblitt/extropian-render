#include <exd/render/camera.hpp>
#include <exd/math/mat4.hpp>
#include <cmath>

namespace exd::render {

math::Mat4 Camera::view_matrix() const {
    return math::Mat4::look_at(position, position + forward, up);
}

math::Mat4 Camera::projection_matrix(float aspect) const {
    return math::Mat4::perspective(fov_y_radians, aspect, near_plane, far_plane);
}

} // namespace exd::render
