#pragma once

#include <exd/geometry/types.hpp>
#include <exd/render/graphics/mesh.hpp>

namespace exd::render {

/// Convert exd::geometry::MeshData → exd::render::Mesh.
/// The vertex layouts are bit-identical; this copies the data.
inline Mesh convert_geometry_mesh(const exd::geometry::MeshData& md) {
    Mesh m;
    m.vertices.reserve(md.vertices.size());
    for (auto& v : md.vertices) {
        m.vertices.push_back({v.position, v.normal, v.uv, v.tangent, v.color});
    }
    m.indices = md.indices;
    switch (md.topology) {
        case exd::geometry::PrimitiveTopology::Triangles: m.topology = Topology::Triangles; break;
        case exd::geometry::PrimitiveTopology::Lines:     m.topology = Topology::Lines;     break;
        case exd::geometry::PrimitiveTopology::Points:    m.topology = Topology::Points;    break;
        default: m.topology = Topology::Triangles; break;
    }
    return m;
}

} // namespace exd::render
