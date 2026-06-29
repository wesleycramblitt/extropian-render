#pragma once

#include <variant>
#include <ext/math/vec3.hpp>
#include <ext/math/quat.hpp>
#include <ext/math/mat4.hpp>

namespace ext::render {

using UniformValue = std::variant<
    int,
    float,
    math::Vec3,
    math::Quat,
    math::Mat4
>;

} // namespace ext::render
