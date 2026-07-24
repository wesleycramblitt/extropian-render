#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <exd/math/vec3.hpp>

namespace exd::render {

/// Marks an entity as representing a loaded (or loading) named environment.
/// Populated by EnvironmentSystem.
struct EnvironmentComponent {
    std::string name;
    bool loaded = false;
};

/// Scene-wide fog — consumed by RenderSystem and fed to the lambertian pass.
/// Place on any entity; RenderSystem picks up the first one it finds.
struct FogComponent {
    math::Vec3f color{0.5f, 0.5f, 0.5f};
    float density = 0.0f;  // 0 = no fog
};

/// Scene-wide lighting override — replaces the hard-coded lambertian light.
/// Place on any entity; RenderSystem picks up the first one it finds.
struct SceneLighting {
    math::Vec3f ambient{0.1f, 0.1f, 0.1f};
    math::Vec3f sun_direction{0.5f, 1.0f, 0.3f};
    math::Vec3f sun_color{1.0f, 1.0f, 1.0f};
};

} // namespace exd::render
