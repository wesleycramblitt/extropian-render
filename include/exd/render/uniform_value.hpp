#pragma once

#include <variant>
#include <exd/math/vec3.hpp>
#include <exd/math/quat.hpp>
#include <exd/math/mat4.hpp>

namespace exd::render {

using UniformValue = std::variant<
    int,
    float,
    math::Vec3,
    math::Quat,
    math::Mat4
>;

} // namespace exd::render
