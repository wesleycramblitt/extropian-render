#pragma once

#include <exd/math/vec3.hpp>
#include <exd/math/quat.hpp>

namespace exd::render {

struct Vertex {
    math::Vec3 position{};
    math::Vec3 normal{0.0f, 1.0f, 0.0f};
    math::Vec3 uv{0.0f, 0.0f, 0.0f};
    math::Quat tangent{1.0f, 0.0f, 0.0f, 1.0f};
    math::Quat color{0.8f, 0.8f, 0.8f, 1.0f};
};

enum class Topology { Triangles, Lines, Points };

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Topology topology = Topology::Triangles;
};

} // namespace exd::render
