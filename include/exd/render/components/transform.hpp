#pragma once

#include <exd/math/vec3.hpp>
#include <exd/math/quat.hpp>

namespace exd::render {

struct Transform {
    math::Vec3f position{0.0f, 0.0f, 0.0f};
    math::Quat rotation{1, 0, 0, 0};
    math::Vec3f scale{1.0f, 1.0f, 1.0f};
};

} // namespace exd::render
