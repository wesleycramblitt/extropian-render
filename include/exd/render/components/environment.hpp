#pragma once

#include <string>
#include <exd/math/vec3.hpp>

namespace exd::render {

struct EnvironmentComponent {
    std::string name;
    bool loaded = false;
};

struct FogComponent {
    math::Vec3f color{0.5f, 0.5f, 0.5f};
    float density = 0.0f;
};

struct SceneLighting {
    math::Vec3f ambient{0.1f, 0.1f, 0.1f};
    math::Vec3f sun_direction{0.5f, 1.0f, 0.3f};
    math::Vec3f sun_color{1.0f, 1.0f, 1.0f};
};

} // namespace exd::render
