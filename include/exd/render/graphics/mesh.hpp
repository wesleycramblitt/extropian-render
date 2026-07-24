#pragma once

#include <exd/core/mesh_types.hpp>

namespace exd::render {

// Re-export core types under render namespace.
using Vertex    = exd::core::Vertex;
using Topology  = exd::core::PrimitiveTopology;
using Mesh      = exd::core::MeshData;
using Bounds    = exd::core::Bounds;

} // namespace exd::render
