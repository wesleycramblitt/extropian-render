#pragma once

#include <exd/render/mesh_manager.hpp>
#include <exd/render/texture_manager.hpp>
#include <exd/render/shader_manager.hpp>

namespace exd::render {

/// Aggregates all GPU resource managers. A single GraphicsContext is shared
/// by all render techniques and systems within an application.
struct GraphicsContext {
    MeshManager    mesh_manager;
    ShaderManager  shader_manager;
    TextureManager texture_manager;
};

} // namespace exd::render
