#pragma once

#include <exd/math/vec3.hpp>

namespace exd::render {

struct Skew {
    math::Vec3f shear{0.0f, 0.0f, 0.0f};  // xy, xz, yz shear factors
};

} // namespace exd::render
