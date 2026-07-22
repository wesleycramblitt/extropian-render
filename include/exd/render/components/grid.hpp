#pragma once

#include <exd/math/quat.hpp>

namespace exd::render {

struct GridComponent {
    float spacing = 50.0f;
    math::Quat color{0.4f, 0.4f, 0.4f, 0.4f};
};

} // namespace exd::render
